/**
 * @file test_thread_safety.cpp
 * @brief Thread safety tests for uvector composite operations
 *
 * These tests verify that uvector's composite operations (reserve, push_back,
 * pop, etc.) are atomic with respect to the dalloc heap when multiple threads
 * share the same heap. The key scenario: one thread's uvector::reserve() does
 * dalloc+move+dfree+replace_pointers as separate calls; without heap_lock,
 * another thread's dfree can trigger defrag mid-operation, corrupting pointers.
 */

#include <gtest/gtest.h>
#include <pthread.h>
#include <atomic>
#include <cstring>

extern "C" {
#include "dalloc.h"
}
#include "uvector.h"

#ifdef USE_THREAD_SAFETY

/**
 * Type whose destructor triggers dfree on a separate heap allocation.
 * Simulates usmart_ptr behavior: each instance owns a heap allocation,
 * and its destructor frees it (potentially triggering defrag).
 */
struct HeapOwner {
    heap_t* heap;
    uint8_t* data;
    uint32_t value;

    HeapOwner() : heap(nullptr), data(nullptr), value(0) {}

    void alloc(heap_t* h, uint32_t val, uint32_t size = 32) {
        heap = h;
        value = val;
        dalloc(heap, size, reinterpret_cast<void**>(&data));
        if (data) {
            std::memset(data, static_cast<uint8_t>(val & 0xFF), size);
        }
    }

    HeapOwner(HeapOwner&& other) : heap(other.heap), data(nullptr), value(other.value) {
        if (other.data && other.heap) {
            replace_pointers(other.heap, reinterpret_cast<void**>(&other.data),
                           reinterpret_cast<void**>(&data));
        }
        other.heap = nullptr;
        other.value = 0;
    }

    HeapOwner& operator=(HeapOwner&& other) {
        if (this != &other) {
            // Free existing allocation
            if (data && heap) {
                dfree(heap, reinterpret_cast<void**>(&data), USING_PTR_ADDRESS);
            }
            heap = other.heap;
            value = other.value;
            data = nullptr;
            if (other.data && other.heap) {
                replace_pointers(other.heap, reinterpret_cast<void**>(&other.data),
                               reinterpret_cast<void**>(&data));
            }
            other.heap = nullptr;
            other.value = 0;
        }
        return *this;
    }

    ~HeapOwner() {
        if (data && heap) {
            dfree(heap, reinterpret_cast<void**>(&data), USING_PTR_VALUE);
            data = nullptr;
        }
    }

    // Prevent copy
    HeapOwner(const HeapOwner&) = delete;
    HeapOwner& operator=(const HeapOwner&) = delete;
};


class UvectorThreadTest : public ::testing::Test {
protected:
    static constexpr size_t HEAP_SIZE = 64 * 1024;
    uint8_t heap_buffer[HEAP_SIZE];
    heap_t heap;

    void SetUp() override {
        std::memset(heap_buffer, 0, HEAP_SIZE);
        heap_init(&heap, heap_buffer, HEAP_SIZE);
    }

    void TearDown() override {
        EXPECT_EQ(heap.alloc_info.allocations_num, 0u)
            << "Heap should be clean after test";
        heap_deinit(&heap);
    }
};

// ==================== Test 1: Concurrent push_back/pop on separate vectors ====================
// Two threads each own a uvector<HeapOwner> on the same heap.
// Each thread pushes and pops elements. Element destructors trigger dfree→defrag
// which can move the OTHER thread's vector buffer mid-operation.

struct VectorThreadArgs {
    heap_t* heap;
    int iterations;
    std::atomic<bool>* start_flag;
    std::atomic<int>* error_count;
};

void* vector_push_pop_thread(void* arg) {
    VectorThreadArgs* args = static_cast<VectorThreadArgs*>(arg);

    // Spin until all threads are ready
    while (!args->start_flag->load()) {}

    uvector<HeapOwner> vec(args->heap);

    for (int i = 0; i < args->iterations; i++) {
        // Push element (triggers reserve→dalloc on first push and when growing)
        HeapOwner item;
        item.alloc(args->heap, static_cast<uint32_t>(i), 16 + (i % 48));
        if (!item.data) continue;  // Heap full, skip

        if (!vec.push_back(static_cast<HeapOwner&&>(item))) {
            continue;  // Allocation failure
        }

        // Occasionally pop to trigger destructor→dfree→defrag
        if (i % 3 == 0 && vec.size() > 0) {
            vec.pop_back();
        }

        // Verify remaining elements are valid
        for (uint32_t j = 0; j < vec.size(); j++) {
            if (vec.at(j).data == nullptr && vec.at(j).heap != nullptr) {
                args->error_count->fetch_add(1);
                return nullptr;
            }
        }
    }

    // Clean up — pop all remaining elements
    while (vec.size() > 0) {
        vec.pop_back();
    }
    vec.shrink_to_fit();

    return nullptr;
}

TEST_F(UvectorThreadTest, ConcurrentVectors_PushPopIntegrity) {
    const int NUM_THREADS = 4;
    const int ITERATIONS = 50;

    pthread_t threads[NUM_THREADS];
    VectorThreadArgs args[NUM_THREADS];
    std::atomic<bool> start_flag(false);
    std::atomic<int> error_count(0);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].heap = &heap;
        args[i].iterations = ITERATIONS;
        args[i].start_flag = &start_flag;
        args[i].error_count = &error_count;

        int ret = pthread_create(&threads[i], nullptr, vector_push_pop_thread, &args[i]);
        ASSERT_EQ(ret, 0) << "Failed to create thread " << i;
    }

    // Release all threads simultaneously for maximum contention
    start_flag.store(true);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }

    EXPECT_EQ(error_count.load(), 0) << "Corrupted pointers detected in vector elements";
}

// ==================== Test 2: Vector operations vs independent dalloc/dfree ====================
// One thread does uvector operations, another thread does raw dalloc/dfree.
// The raw dfree→defrag can move the vector's buffer mid-reserve().

struct RawAllocThreadArgs {
    heap_t* heap;
    int iterations;
    std::atomic<bool>* start_flag;
    std::atomic<bool>* stop_flag;
};

void* raw_alloc_free_thread(void* arg) {
    RawAllocThreadArgs* args = static_cast<RawAllocThreadArgs*>(arg);

    while (!args->start_flag->load()) {}

    // Rapidly alloc+free to trigger defrag as often as possible
    for (int i = 0; i < args->iterations && !args->stop_flag->load(); i++) {
        uint8_t* ptr = nullptr;
        uint32_t size = 16 + (i % 128);
        dalloc(args->heap, size, reinterpret_cast<void**>(&ptr));

        if (ptr) {
            std::memset(ptr, 0xAA, size);
            dfree(args->heap, reinterpret_cast<void**>(&ptr), USING_PTR_ADDRESS);
        }
    }

    return nullptr;
}

TEST_F(UvectorThreadTest, VectorReserve_VsRawDefrag) {
    const int VECTOR_ITERATIONS = 40;
    const int RAW_ITERATIONS = 500;
    const int NUM_RAW_THREADS = 3;

    pthread_t vec_thread;
    pthread_t raw_threads[NUM_RAW_THREADS];
    VectorThreadArgs vec_args;
    RawAllocThreadArgs raw_args[NUM_RAW_THREADS];
    std::atomic<bool> start_flag(false);
    std::atomic<bool> stop_flag(false);
    std::atomic<int> error_count(0);

    vec_args.heap = &heap;
    vec_args.iterations = VECTOR_ITERATIONS;
    vec_args.start_flag = &start_flag;
    vec_args.error_count = &error_count;

    int ret = pthread_create(&vec_thread, nullptr, vector_push_pop_thread, &vec_args);
    ASSERT_EQ(ret, 0);

    for (int i = 0; i < NUM_RAW_THREADS; i++) {
        raw_args[i].heap = &heap;
        raw_args[i].iterations = RAW_ITERATIONS;
        raw_args[i].start_flag = &start_flag;
        raw_args[i].stop_flag = &stop_flag;

        ret = pthread_create(&raw_threads[i], nullptr, raw_alloc_free_thread, &raw_args[i]);
        ASSERT_EQ(ret, 0);
    }

    start_flag.store(true);

    pthread_join(vec_thread, nullptr);
    stop_flag.store(true);

    for (int i = 0; i < NUM_RAW_THREADS; i++) {
        pthread_join(raw_threads[i], nullptr);
    }

    EXPECT_EQ(error_count.load(), 0) << "Corrupted pointers during concurrent vector+raw alloc";
}

// ==================== Test 3: Multiple vectors growing simultaneously ====================
// Tests the specific race: Thread A's reserve() is between dalloc(new buffer)
// and dfree(old buffer); Thread B's reserve() triggers defrag that moves
// Thread A's new buffer.

TEST_F(UvectorThreadTest, ConcurrentGrowth_DataIntegrity) {
    const int NUM_THREADS = 4;
    const int ELEMENTS_PER_THREAD = 30;

    struct GrowthArgs {
        heap_t* heap;
        int thread_id;
        int num_elements;
        std::atomic<bool>* start_flag;
        std::atomic<int>* error_count;
    };

    auto growth_thread = [](void* arg) -> void* {
        GrowthArgs* args = static_cast<GrowthArgs*>(arg);
        while (!args->start_flag->load()) {}

        uvector<HeapOwner> vec(args->heap);

        // Grow vector one element at a time — each push triggers reserve()
        // when capacity is exceeded
        for (int i = 0; i < args->num_elements; i++) {
            HeapOwner item;
            item.alloc(args->heap, static_cast<uint32_t>(args->thread_id * 1000 + i), 24);
            if (!item.data) continue;

            vec.push_back(static_cast<HeapOwner&&>(item));

            // Verify ALL previously pushed elements are still valid
            for (uint32_t j = 0; j < vec.size(); j++) {
                HeapOwner& elem = vec.at(j);
                if (elem.data == nullptr) {
                    // Corrupted — defrag moved our buffer and pointer wasn't updated
                    args->error_count->fetch_add(1);
                    return nullptr;
                }
                // Verify data content is intact
                uint8_t expected = static_cast<uint8_t>(elem.value & 0xFF);
                if (elem.data[0] != expected) {
                    args->error_count->fetch_add(1);
                    return nullptr;
                }
            }
        }

        // Clean up
        while (vec.size() > 0) {
            vec.pop_back();
        }
        vec.shrink_to_fit();
        return nullptr;
    };

    pthread_t threads[NUM_THREADS];
    GrowthArgs args[NUM_THREADS];
    std::atomic<bool> start_flag(false);
    std::atomic<int> error_count(0);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].heap = &heap;
        args[i].thread_id = i;
        args[i].num_elements = ELEMENTS_PER_THREAD;
        args[i].start_flag = &start_flag;
        args[i].error_count = &error_count;

        int ret = pthread_create(&threads[i], nullptr, growth_thread, &args[i]);
        ASSERT_EQ(ret, 0);
    }

    start_flag.store(true);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }

    EXPECT_EQ(error_count.load(), 0)
        << "Data corruption detected during concurrent vector growth";
}

// ==================== Test 4: heap_lock/heap_unlock API ====================

TEST_F(UvectorThreadTest, HeapLock_PreventsInterleaving) {
    // Verify that heap_lock/heap_unlock correctly serializes access
    struct LockTestArgs {
        heap_t* heap;
        std::atomic<bool>* start_flag;
        std::atomic<int>* inside_count;
        std::atomic<int>* max_concurrent;
        int iterations;
    };

    auto lock_thread = [](void* arg) -> void* {
        LockTestArgs* args = static_cast<LockTestArgs*>(arg);
        while (!args->start_flag->load()) {}

        for (int i = 0; i < args->iterations; i++) {
            heap_lock(args->heap);

            int count = args->inside_count->fetch_add(1) + 1;
            // Track max concurrent threads inside locked region
            int cur_max = args->max_concurrent->load();
            while (count > cur_max) {
                args->max_concurrent->compare_exchange_weak(cur_max, count);
            }

            // Small delay to increase chance of overlap
            volatile int dummy = 0;
            for (int j = 0; j < 100; j++) dummy += j;
            (void)dummy;

            args->inside_count->fetch_sub(1);
            heap_unlock(args->heap);
        }
        return nullptr;
    };

    const int NUM_THREADS = 4;
    const int ITERATIONS = 200;

    pthread_t threads[NUM_THREADS];
    LockTestArgs args[NUM_THREADS];
    std::atomic<bool> start_flag(false);
    std::atomic<int> inside_count(0);
    std::atomic<int> max_concurrent(0);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].heap = &heap;
        args[i].start_flag = &start_flag;
        args[i].inside_count = &inside_count;
        args[i].max_concurrent = &max_concurrent;
        args[i].iterations = ITERATIONS;

        int ret = pthread_create(&threads[i], nullptr, lock_thread, &args[i]);
        ASSERT_EQ(ret, 0);
    }

    start_flag.store(true);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], nullptr);
    }

    // With proper mutual exclusion, max_concurrent should be exactly 1
    EXPECT_EQ(max_concurrent.load(), 1)
        << "heap_lock failed to provide mutual exclusion";
}

// ==================== Test 5: Recursive locking works ====================

TEST_F(UvectorThreadTest, RecursiveLock_NoDeadlock) {
    // heap_lock must be recursive: uvector::push_back locks the heap,
    // then calls reserve which also locks. This must not deadlock.
    heap_lock(&heap);
    heap_lock(&heap);  // Nested — would deadlock with non-recursive mutex
    heap_lock(&heap);  // Triple nested

    // Do some heap operations while locked
    uint8_t* ptr = nullptr;
    dalloc(&heap, 64, reinterpret_cast<void**>(&ptr));
    ASSERT_NE(ptr, nullptr);
    dfree(&heap, reinterpret_cast<void**>(&ptr), USING_PTR_ADDRESS);

    heap_unlock(&heap);
    heap_unlock(&heap);
    heap_unlock(&heap);

    // If we get here without deadlocking, the test passes
    EXPECT_EQ(heap.alloc_info.allocations_num, 0u);
}

#endif // USE_THREAD_SAFETY

/**
 * @file test_single_heap.cpp
 * @brief Tests for uvector in single-heap mode (USE_SINGLE_HEAP_MEMORY)
 *
 * In single-heap mode:
 * - uvector() constructor uses default_heap automatically
 * - uvector(uint32_t size) reserves capacity using default_heap
 * - No explicit heap pointer needed
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

extern "C" {
#include "dalloc.h"
}

// Undefine uvector_debug before including uvector.h to avoid redefinition warning
#ifdef uvector_debug
#undef uvector_debug
#endif

#include "uvector.h"

// Suppress debug output
#ifdef uvector_debug
#undef uvector_debug
#endif
#define uvector_debug(...)

// Test configuration
static constexpr size_t BUFFER_SIZE = 4096;
static constexpr uint8_t UNINIT_PATTERN = 0xAA;

// Simple POD type
struct SimplePOD {
    int x;
    float y;

    bool operator==(const SimplePOD& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief Fixture for single-heap mode tests
 */
class SingleHeapTest : public ::testing::Test {
protected:
    alignas(4) uint8_t buffer[BUFFER_SIZE];

    void SetUp() override {
        dalloc_unregister_heap(true);
        std::memset(buffer, UNINIT_PATTERN, BUFFER_SIZE);
        ASSERT_TRUE(dalloc_register_heap(buffer, BUFFER_SIZE));
    }

    void TearDown() override {
        dalloc_unregister_heap(true);
    }
};

// ==================== Default Constructor Tests ====================

TEST_F(SingleHeapTest, DefaultConstructor_CreatesEmptyVector) {
    uvector<int> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(SingleHeapTest, DefaultConstructor_UsesDefaultHeap) {
    uvector<int> vec;
    vec.push_back(42);

    // Should allocate from default heap
    EXPECT_NE(vec.data(), nullptr);
    EXPECT_EQ(vec.at(0), 42);
}

// ==================== Size Constructor Tests ====================

TEST_F(SingleHeapTest, SizeConstructor_ReservesCapacity) {
    uvector<int> vec(10);
    EXPECT_GE(vec.capacity(), 10u);
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(SingleHeapTest, SizeConstructor_CanPushBack) {
    uvector<int> vec(10);
    for (int i = 0; i < 10; i++) {
        EXPECT_TRUE(vec.push_back(i));
    }
    EXPECT_EQ(vec.size(), 10u);
}

// ==================== Basic Operations ====================

TEST_F(SingleHeapTest, PushBack_Works) {
    uvector<int> vec;
    EXPECT_TRUE(vec.push_back(1));
    EXPECT_TRUE(vec.push_back(2));
    EXPECT_TRUE(vec.push_back(3));

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
    EXPECT_EQ(vec.at(2), 3);
}

TEST_F(SingleHeapTest, PopBack_Works) {
    uvector<int> vec;
    vec.push_back(1);
    vec.push_back(2);

    EXPECT_TRUE(vec.pop_back());
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.back(), 1);
}

TEST_F(SingleHeapTest, Clear_Works) {
    uvector<int> vec;
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.clear();
    EXPECT_TRUE(vec.empty());
}

// ==================== Element Access ====================

TEST_F(SingleHeapTest, At_Works) {
    uvector<int> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
}

TEST_F(SingleHeapTest, FrontBack_Works) {
    uvector<int> vec;
    vec.push_back(100);
    vec.push_back(200);
    vec.push_back(300);

    EXPECT_EQ(vec.front(), 100);
    EXPECT_EQ(vec.back(), 300);
}

TEST_F(SingleHeapTest, SubscriptOperator_Works) {
    uvector<int> vec;
    vec.push_back(42);
    vec.push_back(84);

    EXPECT_EQ(vec[0], 42);
    EXPECT_EQ(vec[1], 84);

    vec[0] = 100;
    EXPECT_EQ(vec[0], 100);
}

// ==================== Capacity Operations ====================

TEST_F(SingleHeapTest, Reserve_Works) {
    uvector<int> vec;
    EXPECT_TRUE(vec.reserve(50));
    EXPECT_GE(vec.capacity(), 50u);
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(SingleHeapTest, Resize_Works) {
    uvector<int> vec;
    vec.resize(10);
    EXPECT_EQ(vec.size(), 10u);

    vec.resize(5);
    EXPECT_EQ(vec.size(), 5u);
}

TEST_F(SingleHeapTest, ShrinkToFit_Works) {
    uvector<int> vec;
    vec.reserve(100);
    vec.push_back(1);
    vec.push_back(2);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), vec.size());
}

// ==================== POD Type Tests ====================

TEST_F(SingleHeapTest, POD_BasicOperations) {
    uvector<SimplePOD> vec;
    vec.push_back({1, 1.5f});
    vec.push_back({2, 2.5f});

    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0).x, 1);
    EXPECT_FLOAT_EQ(vec.at(0).y, 1.5f);
}

// ==================== Multiple Vectors ====================

TEST_F(SingleHeapTest, MultipleVectors_ShareDefaultHeap) {
    uvector<int> v1;
    uvector<int> v2;
    uvector<int> v3;

    v1.push_back(1);
    v2.push_back(2);
    v3.push_back(3);

    EXPECT_EQ(v1.at(0), 1);
    EXPECT_EQ(v2.at(0), 2);
    EXPECT_EQ(v3.at(0), 3);

    // All use same heap, data pointers should be different
    EXPECT_NE(v1.data(), v2.data());
    EXPECT_NE(v2.data(), v3.data());
}

TEST_F(SingleHeapTest, MultipleVectors_Independent) {
    uvector<int> v1;
    uvector<int> v2;

    v1.push_back(100);
    v2.push_back(200);

    v1.at(0) = 111;
    EXPECT_EQ(v1.at(0), 111);
    EXPECT_EQ(v2.at(0), 200);  // Unchanged
}

// ==================== Copy Semantics ====================

TEST_F(SingleHeapTest, CopyConstructor_Works) {
    uvector<int> original;
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    uvector<int> copy(original);

    EXPECT_EQ(copy.size(), original.size());
    for (uint32_t i = 0; i < copy.size(); i++) {
        EXPECT_EQ(copy.at(i), original.at(i));
    }
}

TEST_F(SingleHeapTest, Assignment_Works) {
    uvector<int> original;
    original.push_back(10);
    original.push_back(20);

    uvector<int> copy;
    copy = original;

    EXPECT_EQ(copy.size(), 2u);
    EXPECT_EQ(copy.at(0), 10);
    EXPECT_EQ(copy.at(1), 20);
}

// ==================== Destructor ====================

TEST_F(SingleHeapTest, Destructor_FreesMemory) {
    heap_t* heap = dalloc_get_default_heap();
    ASSERT_NE(heap, nullptr);

    size_t allocs_before = heap->alloc_info.allocations_num;

    {
        uvector<int> vec;
        vec.push_back(1);
        vec.push_back(2);
        EXPECT_GT(heap->alloc_info.allocations_num, allocs_before);
    }

    EXPECT_EQ(heap->alloc_info.allocations_num, allocs_before);
}

// ==================== Stress Test ====================

TEST_F(SingleHeapTest, Stress_RapidOperations) {
    uvector<int> vec;

    for (int i = 0; i < 100; i++) {
        vec.push_back(i);
        if (i % 3 == 0) {
            vec.pop_back();
        }
    }

    // Should have ~67 elements
    EXPECT_GT(vec.size(), 60u);
    EXPECT_LT(vec.size(), 80u);
}

TEST_F(SingleHeapTest, Stress_MultipleVectorsGrowth) {
    uvector<int> v1;
    uvector<int> v2;

    for (int i = 0; i < 30; i++) {
        v1.push_back(i);
        v2.push_back(i * 2);
    }

    EXPECT_EQ(v1.size(), 30u);
    EXPECT_EQ(v2.size(), 30u);

    for (uint32_t i = 0; i < 30; i++) {
        EXPECT_EQ(v1.at(i), static_cast<int>(i));
        EXPECT_EQ(v2.at(i), static_cast<int>(i * 2));
    }
}

// ==================== Heap State ====================

TEST_F(SingleHeapTest, HeapState_Accessible) {
    heap_t* heap = dalloc_get_default_heap();
    ASSERT_NE(heap, nullptr);

    uvector<int> vec;
    vec.push_back(1);

    EXPECT_GT(heap->offset, 0u);
    EXPECT_GT(heap->alloc_info.allocations_num, 0u);
}

TEST_F(SingleHeapTest, HeapRegistration_Required) {
    // Unregister heap
    dalloc_unregister_heap(true);

    // Now uvector should fail to allocate
    uvector<int> vec;

    // Push should fail when no heap registered
    EXPECT_FALSE(vec.push_back(42));
    EXPECT_TRUE(vec.empty());
}

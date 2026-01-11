/**
 * @file uvector_test_config.h
 * @brief Test fixtures and helpers for uvector unit tests
 */

#ifndef UVECTOR_TEST_CONFIG_H
#define UVECTOR_TEST_CONFIG_H

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

extern "C" {
#include "dalloc.h"
}

// Undefine uvector_debug before including uvector.h to avoid redefinition warning
// (uvector_test_conf.h defines it, uvector.h tries to redefine as printf)
#ifdef uvector_debug
#undef uvector_debug
#endif

#include "uvector.h"

// Now suppress debug output by redefining to empty
#ifdef uvector_debug
#undef uvector_debug
#endif
#define uvector_debug(...)

// ==================== Test Configuration ====================

static constexpr size_t TEST_HEAP_SIZE = 4096;
static constexpr size_t TEST_LARGE_HEAP_SIZE = 16384;

// Canary configuration for memory corruption detection
static constexpr size_t CANARY_SIZE = 64;
static constexpr uint8_t CANARY_PATTERN = 0xCD;
static constexpr uint8_t UNINIT_PATTERN = 0xAA;

// ==================== Test Types ====================

/**
 * @brief Simple POD type for basic testing
 */
struct SimplePOD {
    int x;
    float y;

    bool operator==(const SimplePOD& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const SimplePOD& other) const {
        return !(*this == other);
    }
};

/**
 * @brief Type with constructor/destructor to test proper lifecycle
 */
class TrackedObject {
public:
    static int construction_count;
    static int destruction_count;
    static int copy_count;
    static int assignment_count;

    int value;

    TrackedObject() : value(0) { construction_count++; }
    TrackedObject(int v) : value(v) { construction_count++; }
    TrackedObject(const TrackedObject& other) : value(other.value) {
        construction_count++;
        copy_count++;
    }
    ~TrackedObject() { destruction_count++; }

    TrackedObject& operator=(const TrackedObject& other) {
        value = other.value;
        assignment_count++;
        return *this;
    }

    bool operator==(const TrackedObject& other) const {
        return value == other.value;
    }

    bool operator!=(const TrackedObject& other) const {
        return value != other.value;
    }

    static void reset_counters() {
        construction_count = 0;
        destruction_count = 0;
        copy_count = 0;
        assignment_count = 0;
    }
};

// Static member definitions (in header for simplicity)
inline int TrackedObject::construction_count = 0;
inline int TrackedObject::destruction_count = 0;
inline int TrackedObject::copy_count = 0;
inline int TrackedObject::assignment_count = 0;

// ==================== Advanced Integrity-Checking Type ====================

/**
 * @brief Advanced test type with integrity checking and lifecycle tracking
 *
 * This class simulates a resource-managing object (like a peripheral driver)
 * and provides comprehensive integrity checking:
 * - Magic numbers to detect memory corruption
 * - Double-destruction detection
 * - Use-after-free detection
 * - Unique instance IDs for tracking
 * - Simulated resource acquisition/release
 */
class IntegrityObject {
public:
    static constexpr uint32_t ALIVE_MAGIC = 0xCAFEBABE;
    static constexpr uint32_t DEAD_MAGIC = 0xDEADBEEF;
    static constexpr int MAX_INSTANCES = 256;

    // Static tracking
    static int construction_count;
    static int destruction_count;
    static int copy_construction_count;
    static int assignment_count;
    static int double_destruction_count;
    static int corrupted_destruction_count;
    static int next_id;
    static bool instance_alive[MAX_INSTANCES];

    // Instance data with integrity markers
    uint32_t magic_start;       // Should be ALIVE_MAGIC when valid
    int id;                     // Unique instance ID
    int value;                  // User data
    int resource_handle;        // Simulated resource (e.g., GPIO handle)
    uint32_t magic_end;         // Should match magic_start

    IntegrityObject() : magic_start(ALIVE_MAGIC), value(0), magic_end(ALIVE_MAGIC) {
        id = next_id++;
        resource_handle = acquire_resource();
        if (id < MAX_INSTANCES) instance_alive[id] = true;
        construction_count++;
    }

    IntegrityObject(int v) : magic_start(ALIVE_MAGIC), value(v), magic_end(ALIVE_MAGIC) {
        id = next_id++;
        resource_handle = acquire_resource();
        if (id < MAX_INSTANCES) instance_alive[id] = true;
        construction_count++;
    }

    IntegrityObject(const IntegrityObject& other)
        : magic_start(ALIVE_MAGIC), value(other.value), magic_end(ALIVE_MAGIC) {
        // Verify source is valid
        EXPECT_TRUE(other.is_valid()) << "Copy from corrupted/destroyed object, id=" << other.id;

        id = next_id++;
        resource_handle = acquire_resource();
        if (id < MAX_INSTANCES) instance_alive[id] = true;
        construction_count++;
        copy_construction_count++;
    }

    ~IntegrityObject() {
        if (magic_start != ALIVE_MAGIC || magic_end != ALIVE_MAGIC) {
            if (magic_start == DEAD_MAGIC) {
                // Double destruction detected!
                double_destruction_count++;
                ADD_FAILURE() << "Double destruction detected! id=" << id;
            } else {
                // Memory corruption detected
                corrupted_destruction_count++;
                ADD_FAILURE() << "Corrupted object destruction! id=" << id
                              << " magic_start=0x" << std::hex << magic_start
                              << " magic_end=0x" << magic_end << std::dec;
            }
            return;
        }

        // Release resource
        release_resource(resource_handle);
        resource_handle = -1;

        // Mark as dead
        magic_start = DEAD_MAGIC;
        magic_end = DEAD_MAGIC;
        if (id < MAX_INSTANCES) instance_alive[id] = false;
        destruction_count++;
    }

    IntegrityObject& operator=(const IntegrityObject& other) {
        if (this != &other) {
            // Verify both objects are valid
            EXPECT_TRUE(is_valid()) << "Assignment to corrupted/destroyed object, id=" << id;
            EXPECT_TRUE(other.is_valid()) << "Assignment from corrupted/destroyed object, id=" << other.id;

            // Release old resource, acquire new one (simulating proper RAII)
            release_resource(resource_handle);
            resource_handle = acquire_resource();
            value = other.value;
            assignment_count++;
        }
        return *this;
    }

    bool is_valid() const {
        return magic_start == ALIVE_MAGIC && magic_end == ALIVE_MAGIC;
    }

    bool operator==(const IntegrityObject& other) const {
        return value == other.value;
    }

    static void reset_counters() {
        construction_count = 0;
        destruction_count = 0;
        copy_construction_count = 0;
        assignment_count = 0;
        double_destruction_count = 0;
        corrupted_destruction_count = 0;
        next_id = 0;
        for (int i = 0; i < MAX_INSTANCES; i++) {
            instance_alive[i] = false;
        }
    }

    static bool all_destroyed() {
        for (int i = 0; i < next_id && i < MAX_INSTANCES; i++) {
            if (instance_alive[i]) return false;
        }
        return true;
    }

    static int alive_count() {
        int count = 0;
        for (int i = 0; i < next_id && i < MAX_INSTANCES; i++) {
            if (instance_alive[i]) count++;
        }
        return count;
    }

    static bool no_errors() {
        return double_destruction_count == 0 && corrupted_destruction_count == 0;
    }

private:
    // Simulated resource management
    static int next_resource_handle;

    static int acquire_resource() {
        return next_resource_handle++;
    }

    static void release_resource(int handle) {
        // In real code, this would release GPIO, close file, etc.
        (void)handle;
    }
};

// Static member definitions for IntegrityObject
inline int IntegrityObject::construction_count = 0;
inline int IntegrityObject::destruction_count = 0;
inline int IntegrityObject::copy_construction_count = 0;
inline int IntegrityObject::assignment_count = 0;
inline int IntegrityObject::double_destruction_count = 0;
inline int IntegrityObject::corrupted_destruction_count = 0;
inline int IntegrityObject::next_id = 0;
inline int IntegrityObject::next_resource_handle = 1000;
inline bool IntegrityObject::instance_alive[IntegrityObject::MAX_INSTANCES] = {false};

// ==================== Base Test Fixture ====================

/**
 * @brief Base fixture with canary-protected heap
 *
 * Memory layout:
 *   [CANARY_BEFORE][HEAP_MEMORY][CANARY_AFTER]
 *
 * If uvector/dalloc writes outside heap bounds, canary bytes will be
 * corrupted and TearDown will detect it.
 */
class UvectorTestFixture : public ::testing::Test {
protected:
    uint8_t canary_before[CANARY_SIZE];
    alignas(4) uint8_t heap_memory[TEST_HEAP_SIZE];
    uint8_t canary_after[CANARY_SIZE];
    heap_t heap;

    void SetUp() override {
        std::memset(canary_before, CANARY_PATTERN, CANARY_SIZE);
        std::memset(canary_after, CANARY_PATTERN, CANARY_SIZE);
        std::memset(heap_memory, UNINIT_PATTERN, TEST_HEAP_SIZE);
        heap_init(&heap, heap_memory, TEST_HEAP_SIZE);

        TrackedObject::reset_counters();
        IntegrityObject::reset_counters();
    }

    void TearDown() override {
        checkCanaryIntegrity();
    }

    void checkCanaryIntegrity() {
        for (size_t i = 0; i < CANARY_SIZE; i++) {
            EXPECT_EQ(canary_before[i], CANARY_PATTERN)
                << "Buffer UNDERFLOW at canary_before[" << i << "]";
            EXPECT_EQ(canary_after[i], CANARY_PATTERN)
                << "Buffer OVERFLOW at canary_after[" << i << "]";
        }
    }

    void assertCanaryIntegrity() {
        for (size_t i = 0; i < CANARY_SIZE; i++) {
            ASSERT_EQ(canary_before[i], CANARY_PATTERN)
                << "Buffer UNDERFLOW at canary_before[" << i << "]";
            ASSERT_EQ(canary_after[i], CANARY_PATTERN)
                << "Buffer OVERFLOW at canary_after[" << i << "]";
        }
    }
};

/**
 * @brief Large heap fixture for stress tests
 */
class UvectorLargeHeapFixture : public ::testing::Test {
protected:
    uint8_t canary_before[CANARY_SIZE];
    alignas(4) uint8_t heap_memory[TEST_LARGE_HEAP_SIZE];
    uint8_t canary_after[CANARY_SIZE];
    heap_t heap;

    void SetUp() override {
        std::memset(canary_before, CANARY_PATTERN, CANARY_SIZE);
        std::memset(canary_after, CANARY_PATTERN, CANARY_SIZE);
        std::memset(heap_memory, UNINIT_PATTERN, TEST_LARGE_HEAP_SIZE);
        heap_init(&heap, heap_memory, TEST_LARGE_HEAP_SIZE);
        TrackedObject::reset_counters();
        IntegrityObject::reset_counters();
    }

    void TearDown() override {
        for (size_t i = 0; i < CANARY_SIZE; i++) {
            EXPECT_EQ(canary_before[i], CANARY_PATTERN);
            EXPECT_EQ(canary_after[i], CANARY_PATTERN);
        }
    }

    void assertCanaryIntegrity() {
        for (size_t i = 0; i < CANARY_SIZE; i++) {
            ASSERT_EQ(canary_before[i], CANARY_PATTERN);
            ASSERT_EQ(canary_after[i], CANARY_PATTERN);
        }
    }
};

#ifdef USE_SINGLE_HEAP_MEMORY
/**
 * @brief Fixture for single heap mode tests
 */
class UvectorSingleHeapFixture : public ::testing::Test {
protected:
    static constexpr size_t BUFFER_SIZE = 4096;
    alignas(4) uint8_t buffer[BUFFER_SIZE];

    void SetUp() override {
        dalloc_unregister_heap(true);
        std::memset(buffer, UNINIT_PATTERN, BUFFER_SIZE);
        dalloc_register_heap(buffer, BUFFER_SIZE);
        TrackedObject::reset_counters();
    }

    void TearDown() override {
        dalloc_unregister_heap(true);
    }
};
#endif // USE_SINGLE_HEAP_MEMORY

#endif // UVECTOR_TEST_CONFIG_H

/**
 * @file test_edge_cases.cpp
 * @brief Edge case and boundary condition tests for uvector
 */

#include "uvector_test_config.h"

class EdgeCaseTest : public UvectorTestFixture {};

// ==================== Empty Vector Operations ====================

TEST_F(EdgeCaseTest, EmptyVector_PopBack_ReturnsFalse) {
    uvector<int> vec(&heap);
    EXPECT_FALSE(vec.pop_back());
}

TEST_F(EdgeCaseTest, EmptyVector_Pop_ReturnsFalse) {
    uvector<int> vec(&heap);
    EXPECT_FALSE(vec.pop(0));
}

TEST_F(EdgeCaseTest, EmptyVector_Data_ReturnsNull) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.data(), nullptr);
}

TEST_F(EdgeCaseTest, EmptyVector_SizeIsZero) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(EdgeCaseTest, EmptyVector_CapacityIsZero) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.capacity(), 0u);
}

TEST_F(EdgeCaseTest, EmptyVector_Clear_NoOp) {
    uvector<int> vec(&heap);
    vec.clear();
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, EmptyVector_At_ReturnsErrorValue) {
    uvector<int> vec(&heap);
    // at() on empty vector should return error value (won't crash)
    // The return is a reference to internal err_retval
    int& result = vec.at(0);
    (void)result;  // Just verify it doesn't crash
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, At_InvalidIndex_ReturnsErrorValue) {
    uvector<int> vec(&heap);
    vec.push_back(10);
    vec.push_back(20);

    // Valid indices are 0 and 1, index 2 is out of bounds
    int& result = vec.at(2);
    (void)result;  // Just verify it doesn't crash

    // Original elements should be unchanged
    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
}

TEST_F(EdgeCaseTest, EmptyVector_ShrinkToFit_NoOp) {
    uvector<int> vec(&heap);
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0u);
}

// ==================== Single Element Tests ====================

TEST_F(EdgeCaseTest, SingleElement_FrontEqualsBack) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_EQ(vec.front(), vec.back());
    EXPECT_EQ(vec.front(), 42);
}

TEST_F(EdgeCaseTest, SingleElement_PopBack_LeavesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    vec.pop_back();
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, SingleElement_Pop0_LeavesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_TRUE(vec.pop(0));
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, SingleElement_SizeIsOne) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_EQ(vec.size(), 1u);
}

// ==================== Boundary Index Tests ====================

TEST_F(EdgeCaseTest, Pop_IndexEqualsSize_ReturnsFalse) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);

    EXPECT_FALSE(vec.pop(2));  // size is 2, valid indices are 0, 1
}

TEST_F(EdgeCaseTest, Pop_IndexGreaterThanSize_ReturnsFalse) {
    uvector<int> vec(&heap);
    vec.push_back(1);

    EXPECT_FALSE(vec.pop(10));
    EXPECT_FALSE(vec.pop(1000));
}

// ==================== Null/Invalid Heap Tests ====================

TEST_F(EdgeCaseTest, NullHeap_Constructor) {
    uvector<int> vec(nullptr);
    EXPECT_EQ(vec.get_mem_pointer(), nullptr);
}

// ==================== Reserve Edge Cases ====================

TEST_F(EdgeCaseTest, Reserve_Zero_NoOp) {
    uvector<int> vec(&heap);
    vec.reserve(0);
    EXPECT_EQ(vec.capacity(), 0u);
}

TEST_F(EdgeCaseTest, Reserve_SameAsCapacity_NoOp) {
    uvector<int> vec(&heap);
    vec.reserve(10);
    uint32_t cap = vec.capacity();

    vec.reserve(cap);
    EXPECT_EQ(vec.capacity(), cap);
}

// ==================== Resize Edge Cases ====================

TEST_F(EdgeCaseTest, Resize_ToZero_MakesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);

    vec.resize(0);
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, Resize_SameSize_NoChange) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);

    vec.resize(2);
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
}

// ==================== Double Operations ====================

TEST_F(EdgeCaseTest, DoubleClear_NoOp) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.clear();
    vec.clear();  // Second clear should be no-op
    EXPECT_TRUE(vec.empty());
}

TEST_F(EdgeCaseTest, MultipleShrinkToFit_NoOp) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.shrink_to_fit();
    uint32_t cap = vec.capacity();

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), cap);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), cap);
}

// ==================== Heap Exhaustion Tests ====================

TEST_F(EdgeCaseTest, HeapExhaustion_PushBackFails) {
    // Use a very small heap
    alignas(4) uint8_t small_heap_mem[128];
    heap_t small_heap;
    heap_init(&small_heap, small_heap_mem, 128);

    uvector<int> vec(&small_heap);

    // Try to allocate more than the heap can hold
    int count = 0;
    for (int i = 0; i < 1000; i++) {
        if (!vec.push_back(i)) {
            break;
        }
        count++;
    }

    // Should have failed at some point
    EXPECT_LT(count, 1000);
    EXPECT_GT(count, 0);  // Should have succeeded at least once
}

TEST_F(EdgeCaseTest, HeapExhaustion_VectorStillUsable) {
    alignas(4) uint8_t small_heap_mem[128];
    heap_t small_heap;
    heap_init(&small_heap, small_heap_mem, 128);

    uvector<int> vec(&small_heap);

    // Fill until exhaustion
    while (vec.push_back(42)) {}

    uint32_t final_size = vec.size();
    EXPECT_GT(final_size, 0u);

    // Vector should still be usable
    EXPECT_EQ(vec.at(0), 42);
    EXPECT_EQ(vec.back(), 42);

    // Pop should work
    EXPECT_TRUE(vec.pop_back());
    EXPECT_EQ(vec.size(), final_size - 1);
}

// ==================== Assign Mem Pointer ====================

TEST_F(EdgeCaseTest, AssignMemPointer_ChangesHeap) {
    uvector<int> vec(&heap);

    alignas(4) uint8_t other_heap_mem[1024];
    heap_t other_heap;
    heap_init(&other_heap, other_heap_mem, 1024);

    vec.assign_mem_pointer(&other_heap);
    EXPECT_EQ(vec.get_mem_pointer(), &other_heap);

    // Verify vector works with new heap
    EXPECT_TRUE(vec.push_back(42));
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.at(0), 42);
}

// ==================== POD Edge Cases ====================

TEST_F(EdgeCaseTest, POD_ZeroInitialized) {
    uvector<SimplePOD> vec(&heap);
    SimplePOD pod = {0, 0.0f};
    vec.push_back(pod);

    EXPECT_EQ(vec.at(0).x, 0);
    EXPECT_FLOAT_EQ(vec.at(0).y, 0.0f);
}

TEST_F(EdgeCaseTest, POD_NegativeValues) {
    uvector<SimplePOD> vec(&heap);
    vec.push_back({-100, -99.9f});

    EXPECT_EQ(vec.at(0).x, -100);
    EXPECT_FLOAT_EQ(vec.at(0).y, -99.9f);
}

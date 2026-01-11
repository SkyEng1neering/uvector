/**
 * @file test_basic.cpp
 * @brief Basic operation tests for uvector
 */

#include "uvector_test_config.h"

class BasicTest : public UvectorTestFixture {};

// ==================== Constructor Tests ====================

TEST_F(BasicTest, HeapConstructor_SetsHeapPointer) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.get_mem_pointer(), &heap);
}

TEST_F(BasicTest, SizeHeapConstructor_ReservesCapacity) {
    uvector<int> vec(10, &heap);
    EXPECT_GE(vec.capacity(), 10u);
}

TEST_F(BasicTest, SizeHeapConstructor_SizeRemainsZero) {
    uvector<int> vec(10, &heap);
    EXPECT_EQ(vec.size(), 0u);
}

// ==================== Initial State Tests ====================

TEST_F(BasicTest, NewVector_IsEmpty) {
    uvector<int> vec(&heap);
    EXPECT_TRUE(vec.empty());
}

TEST_F(BasicTest, NewVector_SizeIsZero) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(BasicTest, NewVector_CapacityIsZero) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.capacity(), 0u);
}

TEST_F(BasicTest, NewVector_DataIsNull) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.data(), nullptr);
}

// ==================== Basic Operation Tests ====================

TEST_F(BasicTest, PushBack_IncreasesSize) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.size(), 0u);

    vec.push_back(42);
    EXPECT_EQ(vec.size(), 1u);

    vec.push_back(43);
    EXPECT_EQ(vec.size(), 2u);
}

TEST_F(BasicTest, PushBack_StoresValue) {
    uvector<int> vec(&heap);
    vec.push_back(42);
    vec.push_back(100);
    vec.push_back(-5);

    EXPECT_EQ(vec.at(0), 42);
    EXPECT_EQ(vec.at(1), 100);
    EXPECT_EQ(vec.at(2), -5);
}

TEST_F(BasicTest, PushBack_ReturnsTrue) {
    uvector<int> vec(&heap);
    EXPECT_TRUE(vec.push_back(42));
}

TEST_F(BasicTest, PopBack_DecreasesSize) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    EXPECT_EQ(vec.size(), 3u);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 2u);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 1u);
}

TEST_F(BasicTest, Clear_ResetsSizeToZero) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    EXPECT_EQ(vec.size(), 3u);

    vec.clear();
    EXPECT_EQ(vec.size(), 0u);
    EXPECT_TRUE(vec.empty());
}

// ==================== Destructor Tests ====================

TEST_F(BasicTest, Destructor_FreesMemory) {
    size_t allocs_before = heap.alloc_info.allocations_num;

    {
        uvector<int> vec(&heap);
        vec.push_back(1);
        vec.push_back(2);
        EXPECT_GT(heap.alloc_info.allocations_num, allocs_before);
    }

    // After destructor, allocations should be freed
    EXPECT_EQ(heap.alloc_info.allocations_num, allocs_before);
}

TEST_F(BasicTest, Destructor_EmptyVector_NoOp) {
    size_t allocs_before = heap.alloc_info.allocations_num;

    {
        uvector<int> vec(&heap);
        // Don't add anything
    }

    EXPECT_EQ(heap.alloc_info.allocations_num, allocs_before);
}

// ==================== Memory Allocation Tests ====================

TEST_F(BasicTest, PushBack_AllocatesMemory) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.data(), nullptr);

    vec.push_back(42);
    EXPECT_NE(vec.data(), nullptr);
}

TEST_F(BasicTest, MultipleVectors_IndependentAllocations) {
    uvector<int> vec1(&heap);
    uvector<int> vec2(&heap);

    vec1.push_back(1);
    vec2.push_back(2);

    EXPECT_EQ(vec1.at(0), 1);
    EXPECT_EQ(vec2.at(0), 2);

    // Data pointers should be different
    EXPECT_NE(vec1.data(), vec2.data());
}

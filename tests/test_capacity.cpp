/**
 * @file test_capacity.cpp
 * @brief Capacity operation tests for uvector (size, capacity, reserve, resize, shrink_to_fit)
 */

#include "uvector_test_config.h"

class CapacityTest : public UvectorTestFixture {};

// ==================== empty() Tests ====================

TEST_F(CapacityTest, Empty_NewVector_ReturnsTrue) {
    uvector<int> vec(&heap);
    EXPECT_TRUE(vec.empty());
}

TEST_F(CapacityTest, Empty_AfterPushBack_ReturnsFalse) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    EXPECT_FALSE(vec.empty());
}

TEST_F(CapacityTest, Empty_AfterClear_ReturnsTrue) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.clear();
    EXPECT_TRUE(vec.empty());
}

TEST_F(CapacityTest, Empty_AfterPopBackAll_ReturnsTrue) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.pop_back();
    EXPECT_TRUE(vec.empty());
}

// ==================== size() Tests ====================

TEST_F(CapacityTest, Size_MatchesPushBackCount) {
    uvector<int> vec(&heap);

    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(vec.size(), static_cast<uint32_t>(i));
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 10u);
}

TEST_F(CapacityTest, Size_DecreasesOnPopBack) {
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

TEST_F(CapacityTest, Size_UnchangedByReserve) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);

    EXPECT_EQ(vec.size(), 2u);
    vec.reserve(100);
    EXPECT_EQ(vec.size(), 2u);
}

// ==================== capacity() Tests ====================

TEST_F(CapacityTest, Capacity_InitiallyZero) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.capacity(), 0u);
}

TEST_F(CapacityTest, Capacity_IncreasesAfterReserve) {
    uvector<int> vec(&heap);
    vec.reserve(50);
    EXPECT_GE(vec.capacity(), 50u);
}

TEST_F(CapacityTest, Capacity_GreaterThanOrEqualSize) {
    uvector<int> vec(&heap);
    for (int i = 0; i < 20; i++) {
        vec.push_back(i);
        EXPECT_GE(vec.capacity(), vec.size());
    }
}

// ==================== reserve() Tests ====================

TEST_F(CapacityTest, Reserve_IncreasesCapacity) {
    uvector<int> vec(&heap);
    EXPECT_EQ(vec.capacity(), 0u);

    vec.reserve(10);
    EXPECT_GE(vec.capacity(), 10u);
}

TEST_F(CapacityTest, Reserve_LessThanCurrent_NoOp) {
    uvector<int> vec(&heap);
    vec.reserve(100);
    uint32_t cap = vec.capacity();

    vec.reserve(50);  // Less than current
    EXPECT_EQ(vec.capacity(), cap);
}

TEST_F(CapacityTest, Reserve_PreservesExistingData) {
    uvector<int> vec(&heap);
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    vec.reserve(100);

    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
}

TEST_F(CapacityTest, Reserve_ReturnsTrue_OnSuccess) {
    uvector<int> vec(&heap);
    EXPECT_TRUE(vec.reserve(10));
}

// ==================== resize() Tests ====================

TEST_F(CapacityTest, Resize_Grow_IncreasesSize) {
    uvector<int> vec(&heap);
    vec.resize(10);
    EXPECT_EQ(vec.size(), 10u);
}

TEST_F(CapacityTest, Resize_Shrink_DecreasesSize) {
    uvector<int> vec(&heap);
    vec.resize(10);
    vec.resize(5);
    EXPECT_EQ(vec.size(), 5u);
}

TEST_F(CapacityTest, Resize_SameSize_NoOp) {
    uvector<int> vec(&heap);
    vec.resize(10);
    vec.resize(10);
    EXPECT_EQ(vec.size(), 10u);
}

TEST_F(CapacityTest, Resize_PreservesExistingData) {
    uvector<int> vec(&heap);
    vec.push_back(100);
    vec.push_back(200);

    vec.resize(10);

    EXPECT_EQ(vec.at(0), 100);
    EXPECT_EQ(vec.at(1), 200);
}

TEST_F(CapacityTest, Resize_ToZero_MakesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);

    vec.resize(0);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(CapacityTest, ResizeWithValue_InitializesNewElements) {
    uvector<int> vec(&heap);
    vec.resize(5, 42);

    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i), 42);
    }
}

TEST_F(CapacityTest, ResizeWithValue_PreservesExisting) {
    uvector<int> vec(&heap);
    vec.push_back(100);
    vec.push_back(200);

    vec.resize(5, 0);

    EXPECT_EQ(vec.at(0), 100);
    EXPECT_EQ(vec.at(1), 200);
    // New elements should be initialized with value 0
    EXPECT_EQ(vec.at(2), 0);
    EXPECT_EQ(vec.at(3), 0);
    EXPECT_EQ(vec.at(4), 0);
}

// ==================== shrink_to_fit() Tests ====================

TEST_F(CapacityTest, ShrinkToFit_ReducesCapacityToSize) {
    uvector<int> vec(&heap);
    vec.reserve(100);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_GT(vec.capacity(), vec.size());

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), vec.size());
}

TEST_F(CapacityTest, ShrinkToFit_PreservesData) {
    uvector<int> vec(&heap);
    vec.reserve(100);
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    vec.shrink_to_fit();

    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
}

TEST_F(CapacityTest, ShrinkToFit_EmptyVector) {
    uvector<int> vec(&heap);
    vec.reserve(100);
    EXPECT_GT(vec.capacity(), 0u);

    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), 0u);
}

TEST_F(CapacityTest, ShrinkToFit_AlreadyFit_NoOp) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.shrink_to_fit();

    uint32_t cap = vec.capacity();
    vec.shrink_to_fit();
    EXPECT_EQ(vec.capacity(), cap);
}

// ==================== Capacity Growth Pattern ====================

TEST_F(CapacityTest, PushBack_CapacityGrowsAutomatically) {
    uvector<int> vec(&heap);
    uint32_t prev_cap = 0;
    int growth_count = 0;

    for (int i = 0; i < 50; i++) {
        vec.push_back(i);
        if (vec.capacity() > prev_cap) {
            growth_count++;
            prev_cap = vec.capacity();
        }
    }

    // Should have grown multiple times, not 50 times
    EXPECT_LT(growth_count, 50);
    EXPECT_GT(growth_count, 0);
}

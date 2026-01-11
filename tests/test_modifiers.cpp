/**
 * @file test_modifiers.cpp
 * @brief Modifier operation tests for uvector (push_back, pop_back, pop, clear)
 */

#include "uvector_test_config.h"

class ModifierTest : public UvectorTestFixture {};

// ==================== push_back() Tests ====================

TEST_F(ModifierTest, PushBack_AddsToEnd) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    EXPECT_EQ(vec.back(), 3);
    EXPECT_EQ(vec.at(2), 3);
}

TEST_F(ModifierTest, PushBack_MultipleTimes_Sequential) {
    uvector<int> vec(&heap);

    for (int i = 0; i < 20; i++) {
        EXPECT_TRUE(vec.push_back(i));
        EXPECT_EQ(vec.back(), i);
    }

    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i), static_cast<int>(i));
    }
}

TEST_F(ModifierTest, PushBack_CopiesValue) {
    uvector<SimplePOD> vec(&heap);
    SimplePOD pod = {42, 3.14f};

    vec.push_back(pod);

    // Modify original - should not affect vector
    pod.x = 100;

    EXPECT_EQ(vec.at(0).x, 42);
}

// ==================== pop_back() Tests ====================

TEST_F(ModifierTest, PopBack_RemovesLast) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.back(), 2);

    vec.pop_back();
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.back(), 1);
}

TEST_F(ModifierTest, PopBack_ReturnsTrue_OnSuccess) {
    uvector<int> vec(&heap);
    vec.push_back(1);

    EXPECT_TRUE(vec.pop_back());
}

TEST_F(ModifierTest, PopBack_ReturnsFalse_EmptyVector) {
    uvector<int> vec(&heap);
    EXPECT_FALSE(vec.pop_back());
}

TEST_F(ModifierTest, PopBack_AllElements_LeavesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.pop_back();
    vec.pop_back();
    vec.pop_back();

    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.size(), 0u);
}

// ==================== pop(i) Tests ====================

TEST_F(ModifierTest, Pop_LastElement_Works) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    // Pop last element (index 2)
    EXPECT_TRUE(vec.pop(2));
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
}

TEST_F(ModifierTest, Pop_InvalidIndex_ReturnsFalse) {
    uvector<int> vec(&heap);
    vec.push_back(1);

    EXPECT_FALSE(vec.pop(1));  // Index out of range
    EXPECT_FALSE(vec.pop(100));
}

TEST_F(ModifierTest, Pop_EmptyVector_ReturnsFalse) {
    uvector<int> vec(&heap);
    EXPECT_FALSE(vec.pop(0));
}

TEST_F(ModifierTest, Pop_SingleElement_LeavesEmpty) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_TRUE(vec.pop(0));
    EXPECT_TRUE(vec.empty());
}

TEST_F(ModifierTest, Pop_FirstElement_ShiftsLeft) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);

    // Pop element at index 0 (value 1)
    EXPECT_TRUE(vec.pop(0));
    EXPECT_EQ(vec.size(), 3u);

    // vec should be: [2, 3, 4]
    EXPECT_EQ(vec.at(0), 2);
    EXPECT_EQ(vec.at(1), 3);
    EXPECT_EQ(vec.at(2), 4);
}

/**
 * @brief Test that pop(i) correctly shifts elements left
 */
TEST_F(ModifierTest, Pop_MiddleElement_ShiftsLeft) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);
    vec.push_back(4);
    vec.push_back(5);

    // Pop element at index 2 (value 3)
    EXPECT_TRUE(vec.pop(2));
    EXPECT_EQ(vec.size(), 4u);

    // vec should be: [1, 2, 4, 5]
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
    EXPECT_EQ(vec.at(2), 4);  // Shifted from index 3
    EXPECT_EQ(vec.at(3), 5);  // Shifted from index 4
}

// ==================== clear() Tests ====================

TEST_F(ModifierTest, Clear_ResetsSizeToZero) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec.clear();
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(ModifierTest, Clear_KeepsCapacity) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    uint32_t cap_before = vec.capacity();
    vec.clear();

    EXPECT_EQ(vec.capacity(), cap_before);
}

TEST_F(ModifierTest, Clear_EmptyVector_NoOp) {
    uvector<int> vec(&heap);
    vec.clear();  // Should not crash
    EXPECT_TRUE(vec.empty());
}

TEST_F(ModifierTest, Clear_ThenPushBack_Works) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.clear();

    vec.push_back(100);
    EXPECT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.at(0), 100);
}

// ==================== Combined Operations ====================

TEST_F(ModifierTest, PushPopCycle) {
    uvector<int> vec(&heap);

    for (int cycle = 0; cycle < 10; cycle++) {
        vec.push_back(cycle);
        vec.push_back(cycle + 100);
        vec.pop_back();
    }

    EXPECT_EQ(vec.size(), 10u);
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i), static_cast<int>(i));
    }
}

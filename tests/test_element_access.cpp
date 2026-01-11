/**
 * @file test_element_access.cpp
 * @brief Element access tests for uvector (at, [], front, back, data)
 */

#include "uvector_test_config.h"

class ElementAccessTest : public UvectorTestFixture {};

// ==================== at() Tests ====================

TEST_F(ElementAccessTest, At_ValidIndex_ReturnsCorrectElement) {
    uvector<int> vec(&heap);
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    EXPECT_EQ(vec.at(0), 10);
    EXPECT_EQ(vec.at(1), 20);
    EXPECT_EQ(vec.at(2), 30);
}

TEST_F(ElementAccessTest, At_ModifyValue_PersistsChange) {
    uvector<int> vec(&heap);
    vec.push_back(10);

    vec.at(0) = 99;
    EXPECT_EQ(vec.at(0), 99);
}

TEST_F(ElementAccessTest, At_SequentialAccess) {
    uvector<int> vec(&heap);
    for (int i = 0; i < 10; i++) {
        vec.push_back(i * 10);
    }

    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i), static_cast<int>(i * 10));
    }
}

// ==================== operator[] Tests ====================

TEST_F(ElementAccessTest, SubscriptOperator_ValidIndex) {
    uvector<int> vec(&heap);
    vec.push_back(100);
    vec.push_back(200);

    EXPECT_EQ(vec[0], 100);
    EXPECT_EQ(vec[1], 200);
}

TEST_F(ElementAccessTest, SubscriptOperator_ModifyValue) {
    uvector<int> vec(&heap);
    vec.push_back(100);

    vec[0] = 555;
    EXPECT_EQ(vec[0], 555);
}

// ==================== front() Tests ====================

TEST_F(ElementAccessTest, Front_ReturnsFirstElement) {
    uvector<int> vec(&heap);
    vec.push_back(111);
    vec.push_back(222);
    vec.push_back(333);

    EXPECT_EQ(vec.front(), 111);
}

TEST_F(ElementAccessTest, Front_ModifyValue) {
    uvector<int> vec(&heap);
    vec.push_back(111);

    vec.front() = 999;
    EXPECT_EQ(vec.front(), 999);
    EXPECT_EQ(vec.at(0), 999);
}

TEST_F(ElementAccessTest, Front_SingleElement) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_EQ(vec.front(), 42);
}

// ==================== back() Tests ====================

TEST_F(ElementAccessTest, Back_ReturnsLastElement) {
    uvector<int> vec(&heap);
    vec.push_back(111);
    vec.push_back(222);
    vec.push_back(333);

    EXPECT_EQ(vec.back(), 333);
}

TEST_F(ElementAccessTest, Back_ModifyValue) {
    uvector<int> vec(&heap);
    vec.push_back(111);
    vec.push_back(222);

    vec.back() = 888;
    EXPECT_EQ(vec.back(), 888);
    EXPECT_EQ(vec.at(1), 888);
}

TEST_F(ElementAccessTest, Back_SingleElement_EqualsFront) {
    uvector<int> vec(&heap);
    vec.push_back(42);

    EXPECT_EQ(vec.front(), vec.back());
}

TEST_F(ElementAccessTest, Back_UpdatesAfterPushBack) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    EXPECT_EQ(vec.back(), 1);

    vec.push_back(2);
    EXPECT_EQ(vec.back(), 2);

    vec.push_back(3);
    EXPECT_EQ(vec.back(), 3);
}

// ==================== data() Tests ====================

TEST_F(ElementAccessTest, Data_ReturnsContiguousMemory) {
    uvector<int> vec(&heap);
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);

    int* ptr = vec.data();
    ASSERT_NE(ptr, nullptr);

    EXPECT_EQ(ptr[0], 10);
    EXPECT_EQ(ptr[1], 20);
    EXPECT_EQ(ptr[2], 30);
}

TEST_F(ElementAccessTest, Data_ModifyViaPointer) {
    uvector<int> vec(&heap);
    vec.push_back(10);
    vec.push_back(20);

    int* ptr = vec.data();
    ptr[0] = 100;
    ptr[1] = 200;

    EXPECT_EQ(vec.at(0), 100);
    EXPECT_EQ(vec.at(1), 200);
}

TEST_F(ElementAccessTest, Data_PointerArithmetic) {
    uvector<int> vec(&heap);
    for (int i = 0; i < 5; i++) {
        vec.push_back(i);
    }

    int* ptr = vec.data();
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(*(ptr + i), static_cast<int>(i));
    }
}

// ==================== POD Type Tests ====================

TEST_F(ElementAccessTest, SimplePOD_AccessWorks) {
    uvector<SimplePOD> vec(&heap);
    vec.push_back({1, 1.5f});
    vec.push_back({2, 2.5f});

    EXPECT_EQ(vec.at(0).x, 1);
    EXPECT_FLOAT_EQ(vec.at(0).y, 1.5f);
    EXPECT_EQ(vec.at(1).x, 2);
    EXPECT_FLOAT_EQ(vec.at(1).y, 2.5f);
}

TEST_F(ElementAccessTest, SimplePOD_ModifyFields) {
    uvector<SimplePOD> vec(&heap);
    vec.push_back({1, 1.0f});

    vec.at(0).x = 100;
    vec.at(0).y = 99.9f;

    EXPECT_EQ(vec.at(0).x, 100);
    EXPECT_FLOAT_EQ(vec.at(0).y, 99.9f);
}

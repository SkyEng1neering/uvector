/**
 * @file test_copy_semantics.cpp
 * @brief Copy constructor and assignment operator tests for uvector
 */

#include "uvector_test_config.h"

class CopySemanticsTest : public UvectorTestFixture {};

// ==================== Copy Constructor Tests ====================

TEST_F(CopySemanticsTest, CopyConstructor_CopiesAllElements) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    uvector<int> copy(original);

    EXPECT_EQ(copy.size(), original.size());
    for (uint32_t i = 0; i < copy.size(); i++) {
        EXPECT_EQ(copy.at(i), original.at(i));
    }
}

TEST_F(CopySemanticsTest, CopyConstructor_IndependentOfOriginal) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);

    uvector<int> copy(original);

    // Modify original
    original.at(0) = 100;
    original.push_back(3);

    // Copy should be unaffected
    EXPECT_EQ(copy.at(0), 1);
    EXPECT_EQ(copy.size(), 2u);
}

TEST_F(CopySemanticsTest, CopyConstructor_EmptyVector) {
    uvector<int> original(&heap);
    uvector<int> copy(original);

    EXPECT_TRUE(copy.empty());
    EXPECT_EQ(copy.size(), 0u);
}

TEST_F(CopySemanticsTest, CopyConstructor_InheritsHeapPointer) {
    uvector<int> original(&heap);
    original.push_back(1);

    uvector<int> copy(original);

    EXPECT_EQ(copy.get_mem_pointer(), original.get_mem_pointer());
}

TEST_F(CopySemanticsTest, CopyConstructor_PODType) {
    uvector<SimplePOD> original(&heap);
    original.push_back({1, 1.0f});
    original.push_back({2, 2.0f});

    uvector<SimplePOD> copy(original);

    EXPECT_EQ(copy.size(), 2u);
    EXPECT_EQ(copy.at(0).x, 1);
    EXPECT_FLOAT_EQ(copy.at(0).y, 1.0f);
    EXPECT_EQ(copy.at(1).x, 2);
    EXPECT_FLOAT_EQ(copy.at(1).y, 2.0f);
}

// ==================== Assignment Operator Tests ====================

TEST_F(CopySemanticsTest, Assignment_CopiesAllElements) {
    uvector<int> original(&heap);
    original.push_back(10);
    original.push_back(20);
    original.push_back(30);

    uvector<int> copy(&heap);
    copy = original;

    EXPECT_EQ(copy.size(), original.size());
    for (uint32_t i = 0; i < copy.size(); i++) {
        EXPECT_EQ(copy.at(i), original.at(i));
    }
}

TEST_F(CopySemanticsTest, Assignment_ClearsExisting) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);

    uvector<int> copy(&heap);
    copy.push_back(100);
    copy.push_back(200);
    copy.push_back(300);

    copy = original;

    EXPECT_EQ(copy.size(), 2u);
    EXPECT_EQ(copy.at(0), 1);
    EXPECT_EQ(copy.at(1), 2);
}

TEST_F(CopySemanticsTest, Assignment_ToEmptyVector) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);

    uvector<int> copy(&heap);
    copy = original;

    EXPECT_EQ(copy.size(), 2u);
}

TEST_F(CopySemanticsTest, Assignment_FromEmptyVector) {
    uvector<int> original(&heap);

    uvector<int> copy(&heap);
    copy.push_back(1);
    copy.push_back(2);

    copy = original;

    EXPECT_TRUE(copy.empty());
}

TEST_F(CopySemanticsTest, Assignment_DifferentSizes) {
    uvector<int> small_vec(&heap);
    small_vec.push_back(1);

    uvector<int> large_vec(&heap);
    for (int i = 0; i < 10; i++) {
        large_vec.push_back(i);
    }

    // Large to small
    small_vec = large_vec;
    EXPECT_EQ(small_vec.size(), 10u);

    // Small to large (reusing small_vec which now has 10)
    uvector<int> tiny(&heap);
    tiny.push_back(999);
    large_vec = tiny;
    EXPECT_EQ(large_vec.size(), 1u);
    EXPECT_EQ(large_vec.at(0), 999);
}

// ==================== Deep Copy Verification ====================

TEST_F(CopySemanticsTest, DeepCopy_ModifyOriginal_NoCopyChange) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    uvector<int> copy(original);

    // Modify original
    original.at(0) = 999;
    original.at(1) = 888;
    original.push_back(4);

    // Verify copy is unchanged
    EXPECT_EQ(copy.at(0), 1);
    EXPECT_EQ(copy.at(1), 2);
    EXPECT_EQ(copy.at(2), 3);
    EXPECT_EQ(copy.size(), 3u);
}

TEST_F(CopySemanticsTest, DeepCopy_ModifyCopy_NoOriginalChange) {
    uvector<int> original(&heap);
    original.push_back(1);
    original.push_back(2);
    original.push_back(3);

    uvector<int> copy(original);

    // Modify copy
    copy.at(0) = 999;
    copy.push_back(4);
    copy.pop_back();

    // Verify original is unchanged
    EXPECT_EQ(original.at(0), 1);
    EXPECT_EQ(original.at(1), 2);
    EXPECT_EQ(original.at(2), 3);
    EXPECT_EQ(original.size(), 3u);
}

TEST_F(CopySemanticsTest, DeepCopy_DataPointersDifferent) {
    uvector<int> original(&heap);
    original.push_back(1);

    uvector<int> copy(original);

    // Data pointers should be different (deep copy)
    EXPECT_NE(original.data(), copy.data());
}

// ==================== Self-Assignment ====================

TEST_F(CopySemanticsTest, SelfAssignment_NoChange) {
    uvector<int> vec(&heap);
    vec.push_back(1);
    vec.push_back(2);
    vec.push_back(3);

    vec = vec;  // Self-assignment

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 1);
    EXPECT_EQ(vec.at(1), 2);
    EXPECT_EQ(vec.at(2), 3);
}

// ==================== Chain Operations ====================

TEST_F(CopySemanticsTest, ChainedAssignment) {
    uvector<int> v1(&heap);
    v1.push_back(1);

    uvector<int> v2(&heap);
    uvector<int> v3(&heap);

    v3 = v2 = v1;

    EXPECT_EQ(v2.size(), 1u);
    EXPECT_EQ(v3.size(), 1u);
    EXPECT_EQ(v2.at(0), 1);
    EXPECT_EQ(v3.at(0), 1);
}

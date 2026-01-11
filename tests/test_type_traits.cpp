/**
 * @file test_type_traits.cpp
 * @brief Type instantiation tests for uvector with different types
 */

#include "uvector_test_config.h"

class TypeTraitsTest : public UvectorTestFixture {};

// ==================== Primitive Type Tests ====================

TEST_F(TypeTraitsTest, Int_BasicOperations) {
    uvector<int> vec(&heap);
    vec.push_back(42);
    vec.push_back(-100);
    vec.push_back(0);

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 42);
    EXPECT_EQ(vec.at(1), -100);
    EXPECT_EQ(vec.at(2), 0);
}

TEST_F(TypeTraitsTest, Float_BasicOperations) {
    uvector<float> vec(&heap);
    vec.push_back(3.14f);
    vec.push_back(-2.5f);
    vec.push_back(0.0f);

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_FLOAT_EQ(vec.at(0), 3.14f);
    EXPECT_FLOAT_EQ(vec.at(1), -2.5f);
    EXPECT_FLOAT_EQ(vec.at(2), 0.0f);
}

TEST_F(TypeTraitsTest, Double_BasicOperations) {
    uvector<double> vec(&heap);
    vec.push_back(3.14159265359);
    vec.push_back(-2.71828);

    EXPECT_EQ(vec.size(), 2u);
    EXPECT_DOUBLE_EQ(vec.at(0), 3.14159265359);
    EXPECT_DOUBLE_EQ(vec.at(1), -2.71828);
}

TEST_F(TypeTraitsTest, Char_BasicOperations) {
    uvector<char> vec(&heap);
    vec.push_back('H');
    vec.push_back('i');
    vec.push_back('!');

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 'H');
    EXPECT_EQ(vec.at(1), 'i');
    EXPECT_EQ(vec.at(2), '!');
}

TEST_F(TypeTraitsTest, UInt8_BasicOperations) {
    uvector<uint8_t> vec(&heap);
    vec.push_back(0);
    vec.push_back(127);
    vec.push_back(255);

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 0u);
    EXPECT_EQ(vec.at(1), 127u);
    EXPECT_EQ(vec.at(2), 255u);
}

TEST_F(TypeTraitsTest, UInt32_BasicOperations) {
    uvector<uint32_t> vec(&heap);
    vec.push_back(0);
    vec.push_back(1000000);
    vec.push_back(0xFFFFFFFF);

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0), 0u);
    EXPECT_EQ(vec.at(1), 1000000u);
    EXPECT_EQ(vec.at(2), 0xFFFFFFFFu);
}

TEST_F(TypeTraitsTest, Pointer_BasicOperations) {
    int a = 1, b = 2, c = 3;
    uvector<int*> vec(&heap);
    vec.push_back(&a);
    vec.push_back(&b);
    vec.push_back(&c);

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(*vec.at(0), 1);
    EXPECT_EQ(*vec.at(1), 2);
    EXPECT_EQ(*vec.at(2), 3);

    // Modify via pointer
    *vec.at(0) = 100;
    EXPECT_EQ(a, 100);
}

// ==================== POD Struct Tests ====================

TEST_F(TypeTraitsTest, SimplePOD_BasicOperations) {
    uvector<SimplePOD> vec(&heap);
    vec.push_back({1, 1.5f});
    vec.push_back({2, 2.5f});
    vec.push_back({3, 3.5f});

    EXPECT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0).x, 1);
    EXPECT_FLOAT_EQ(vec.at(0).y, 1.5f);
}

TEST_F(TypeTraitsTest, SimplePOD_PreservesAllFields) {
    uvector<SimplePOD> vec(&heap);

    for (int i = 0; i < 10; i++) {
        vec.push_back({i, static_cast<float>(i) * 0.5f});
    }

    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i).x, static_cast<int>(i));
        EXPECT_FLOAT_EQ(vec.at(i).y, static_cast<float>(i) * 0.5f);
    }
}

// ==================== TrackedObject Lifecycle Tests ====================

TEST_F(TypeTraitsTest, TrackedObject_ConstructorCalled) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);
        vec.push_back(TrackedObject(1));
        vec.push_back(TrackedObject(2));
    }

    // Constructors should have been called
    EXPECT_GT(TrackedObject::construction_count, 0);
}

TEST_F(TypeTraitsTest, TrackedObject_DestructorCalled) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);
        vec.push_back(TrackedObject(1));
        vec.push_back(TrackedObject(2));
        // Vector destructor should call element destructors
    }

    // All constructed objects must be destroyed (no leaks)
    EXPECT_EQ(TrackedObject::construction_count, TrackedObject::destruction_count);
}

TEST_F(TypeTraitsTest, TrackedObject_CopyConstructorUsed) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);
        TrackedObject obj(42);
        vec.push_back(obj);
    }

    // Copy should have been made when pushing
    EXPECT_GT(TrackedObject::copy_count, 0);
}

TEST_F(TypeTraitsTest, TrackedObject_ValuesPreserved) {
    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(10));
    vec.push_back(TrackedObject(20));
    vec.push_back(TrackedObject(30));

    EXPECT_EQ(vec.at(0).value, 10);
    EXPECT_EQ(vec.at(1).value, 20);
    EXPECT_EQ(vec.at(2).value, 30);
}

// ==================== Large Struct Tests ====================

struct LargeStruct {
    int data[32];  // 128 bytes

    LargeStruct() {
        for (int i = 0; i < 32; i++) data[i] = 0;
    }

    LargeStruct(int fill) {
        for (int i = 0; i < 32; i++) data[i] = fill;
    }

    bool operator==(const LargeStruct& other) const {
        for (int i = 0; i < 32; i++) {
            if (data[i] != other.data[i]) return false;
        }
        return true;
    }
};

TEST_F(TypeTraitsTest, LargeStruct_BasicOperations) {
    uvector<LargeStruct> vec(&heap);
    vec.push_back(LargeStruct(1));
    vec.push_back(LargeStruct(2));

    EXPECT_EQ(vec.size(), 2u);

    for (int i = 0; i < 32; i++) {
        EXPECT_EQ(vec.at(0).data[i], 1);
        EXPECT_EQ(vec.at(1).data[i], 2);
    }
}

TEST_F(TypeTraitsTest, LargeStruct_DataPreserved) {
    uvector<LargeStruct> vec(&heap);

    LargeStruct s;
    for (int i = 0; i < 32; i++) {
        s.data[i] = i * 10;
    }
    vec.push_back(s);

    for (int i = 0; i < 32; i++) {
        EXPECT_EQ(vec.at(0).data[i], i * 10);
    }
}

// ==================== ShrinkToFit Lifecycle Tests ====================

TEST_F(TypeTraitsTest, TrackedObject_ShrinkToFit_EmptyVector_DestructorsCalled) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);

        // uvector has err_retval member that is constructed, so 1 construction here
        int constructions_after_vec_create = TrackedObject::construction_count;
        EXPECT_EQ(constructions_after_vec_create, 1);  // err_retval

        vec.reserve(10);

        // 10 additional objects should have been constructed by reserve()
        EXPECT_EQ(TrackedObject::construction_count, constructions_after_vec_create + 10);

        // Now shrink_to_fit when size is 0
        vec.shrink_to_fit();

        // All 10 reserved objects should have been destroyed
        EXPECT_EQ(TrackedObject::destruction_count, 10);
        EXPECT_EQ(vec.capacity(), 0u);
    }

    // After vector destruction (err_retval also destroyed), balance should be maintained
    EXPECT_EQ(TrackedObject::construction_count, TrackedObject::destruction_count);
}

TEST_F(TypeTraitsTest, TrackedObject_ClearThenShrinkToFit_DestructorsCalled) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);
        vec.push_back(TrackedObject(1));
        vec.push_back(TrackedObject(2));
        vec.push_back(TrackedObject(3));

        int destructions_before_clear = TrackedObject::destruction_count;

        vec.clear();
        EXPECT_EQ(vec.size(), 0u);

        // clear() now calls destructors on removed elements
        EXPECT_GT(TrackedObject::destruction_count, destructions_before_clear);

        vec.shrink_to_fit();
        EXPECT_EQ(vec.capacity(), 0u);
    }

    // After vector destruction, all objects should be destroyed
    EXPECT_EQ(TrackedObject::construction_count, TrackedObject::destruction_count);
}

// ==================== Destructor Calls on Remove Operations ====================

TEST_F(TypeTraitsTest, TrackedObject_Clear_CallsDestructors) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(1));
    vec.push_back(TrackedObject(2));
    vec.push_back(TrackedObject(3));

    int destructions_before = TrackedObject::destruction_count;

    vec.clear();

    // 3 elements were cleared, so 3 additional destructor calls
    EXPECT_EQ(TrackedObject::destruction_count, destructions_before + 3);
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(TypeTraitsTest, TrackedObject_PopBack_CallsDestructor) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(1));
    vec.push_back(TrackedObject(2));

    int destructions_before = TrackedObject::destruction_count;

    vec.pop_back();

    // 1 element was popped, so 1 additional destructor call
    EXPECT_EQ(TrackedObject::destruction_count, destructions_before + 1);
    EXPECT_EQ(vec.size(), 1u);
}

TEST_F(TypeTraitsTest, TrackedObject_Pop_CallsDestructor) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(1));
    vec.push_back(TrackedObject(2));
    vec.push_back(TrackedObject(3));

    int destructions_before = TrackedObject::destruction_count;

    vec.pop(1);  // Remove middle element

    // Element at index 1 was removed, plus last element after shift
    // So at least 2 destructor calls (removed element + duplicate cleanup)
    EXPECT_GE(TrackedObject::destruction_count, destructions_before + 2);
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0).value, 1);
    EXPECT_EQ(vec.at(1).value, 3);
}

TEST_F(TypeTraitsTest, TrackedObject_ResizeShrink_CallsDestructors) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(1));
    vec.push_back(TrackedObject(2));
    vec.push_back(TrackedObject(3));
    vec.push_back(TrackedObject(4));
    vec.push_back(TrackedObject(5));

    int destructions_before = TrackedObject::destruction_count;

    vec.resize(2);  // Remove 3 elements

    // 3 elements were removed, so 3 additional destructor calls
    EXPECT_EQ(TrackedObject::destruction_count, destructions_before + 3);
    EXPECT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0).value, 1);
    EXPECT_EQ(vec.at(1).value, 2);
}

TEST_F(TypeTraitsTest, TrackedObject_PopLastElement_SingleDestructorCall) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(1));
    vec.push_back(TrackedObject(2));
    vec.push_back(TrackedObject(3));

    int destructions_before = TrackedObject::destruction_count;

    // Pop the last element using pop(index)
    vec.pop(2);

    // Only 1 destructor should be called (not 2!)
    EXPECT_EQ(TrackedObject::destruction_count, destructions_before + 1);
    EXPECT_EQ(vec.size(), 2u);
}

TEST_F(TypeTraitsTest, TrackedObject_PopSingleElement_SingleDestructorCall) {
    TrackedObject::reset_counters();

    uvector<TrackedObject> vec(&heap);
    vec.push_back(TrackedObject(42));

    int destructions_before = TrackedObject::destruction_count;

    // Pop the only element
    vec.pop(0);

    // Only 1 destructor should be called
    EXPECT_EQ(TrackedObject::destruction_count, destructions_before + 1);
    EXPECT_TRUE(vec.empty());
}

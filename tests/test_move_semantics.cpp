/**
 * @file test_move_semantics.cpp
 * @brief Move-only type tests for uvector
 *
 * Verifies that uvector correctly handles types with deleted copy
 * constructor/assignment (e.g. usmart_ptr, unique_ptr, file handles).
 * The internal migrations in reserve(), push_back(), pop(), and
 * shrink_to_fit() must use move semantics, not copy.
 */

#include "uvector_test_config.h"
#include <utility>

// ==================== Move-Only Test Type ====================

/**
 * @brief Move-only wrapper around an int value.
 *
 * Copy constructor and copy assignment are explicitly deleted.
 * Tracks construction, move, and destruction counts so tests can
 * verify that no unexpected copies occur.
 */
class MoveOnly {
public:
    /* construct_count tracks every object ever alive (default, int, and move ctor).
     * destruct_count tracks every destruction.
     * Balance: construct_count == destruct_count after all objects are destroyed. */
    static int construct_count;
    static int destruct_count;

    int value;
    bool valid;  /* false after the object has been moved-from */

    explicit MoveOnly(int v = 0) : value(v), valid(true) {
        construct_count++;
    }

    /* Copy is deleted — uvector must never attempt a copy */
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    /* Move constructor creates a new live object — counted as a construction */
    MoveOnly(MoveOnly&& other) : value(other.value), valid(true) {
        other.valid = false;
        construct_count++;
    }

    MoveOnly& operator=(MoveOnly&& other) {
        if (this != &other) {
            value = other.value;
            valid = true;
            other.valid = false;
        }
        return *this;
    }

    ~MoveOnly() {
        destruct_count++;
    }

    static void reset_counters() {
        construct_count = 0;
        destruct_count = 0;
    }
};

inline int MoveOnly::construct_count = 0;
inline int MoveOnly::destruct_count = 0;

// ==================== Fixture ====================

class MoveOnlyTest : public UvectorTestFixture {
protected:
    void SetUp() override {
        UvectorTestFixture::SetUp();
        MoveOnly::reset_counters();
    }
};

// ==================== push_back Tests ====================

TEST_F(MoveOnlyTest, PushBack_AcceptsRvalue) {
    uvector<MoveOnly> vec(&heap);
    EXPECT_TRUE(vec.push_back(MoveOnly(42)));
    ASSERT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.at(0).value, 42);
    EXPECT_TRUE(vec.at(0).valid);
}

TEST_F(MoveOnlyTest, PushBack_MultipleElements) {
    uvector<MoveOnly> vec(&heap);
    for (int i = 0; i < 5; i++) {
        EXPECT_TRUE(vec.push_back(MoveOnly(i * 10)));
    }
    ASSERT_EQ(vec.size(), 5u);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(vec.at((uint32_t)i).value, i * 10);
        EXPECT_TRUE(vec.at((uint32_t)i).valid);
    }
}

TEST_F(MoveOnlyTest, PushBack_MovesFromNamedVariable) {
    uvector<MoveOnly> vec(&heap);
    MoveOnly obj(99);
    EXPECT_TRUE(vec.push_back(static_cast<MoveOnly&&>(obj)));
    EXPECT_FALSE(obj.valid);  /* source was moved-from */
    EXPECT_EQ(vec.at(0).value, 99);
    EXPECT_TRUE(vec.at(0).valid);
}

TEST_F(MoveOnlyTest, PushBack_NoCopyOccurs) {
    uvector<MoveOnly> vec(&heap);
    /* The fact that this compiles proves no copy constructor/assignment is called.
     * MoveOnly has copy constructor/assignment = delete — any copy attempt is a
     * compile error, not a runtime error. */
    EXPECT_TRUE(vec.push_back(MoveOnly(1)));
    EXPECT_TRUE(vec.push_back(MoveOnly(2)));
    EXPECT_TRUE(vec.push_back(MoveOnly(3)));
    ASSERT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0).value, 1);
    EXPECT_EQ(vec.at(1).value, 2);
    EXPECT_EQ(vec.at(2).value, 3);
}

// ==================== pop Tests ====================

TEST_F(MoveOnlyTest, Pop_RemovesElement) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(10));
    vec.push_back(MoveOnly(20));
    vec.push_back(MoveOnly(30));

    MoveOnly::reset_counters();
    EXPECT_TRUE(vec.pop(1));  /* remove middle element */

    ASSERT_EQ(vec.size(), 2u);
    EXPECT_EQ(vec.at(0).value, 10);
    EXPECT_EQ(vec.at(1).value, 30);
}

TEST_F(MoveOnlyTest, Pop_ShiftsElementsWithMove) {
    uvector<MoveOnly> vec(&heap);
    for (int i = 0; i < 4; i++) vec.push_back(MoveOnly(i));

    MoveOnly::reset_counters();
    vec.pop(0);  /* remove first — triggers shifting of 3 elements */

    /* All surviving elements must still be valid */
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_TRUE(vec.at(i).valid) << "element " << i << " invalid after pop";
    }
}

TEST_F(MoveOnlyTest, Pop_LastElement) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(55));
    EXPECT_TRUE(vec.pop(0));
    EXPECT_EQ(vec.size(), 0u);
}

TEST_F(MoveOnlyTest, Pop_OutOfRange_ReturnsFalse) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(1));
    EXPECT_FALSE(vec.pop(5));
    EXPECT_EQ(vec.size(), 1u);
}

// ==================== reserve / reallocation Tests ====================

TEST_F(MoveOnlyTest, Reserve_TriggersReallocationWithMove) {
    uvector<MoveOnly> vec(&heap);

    /* Push elements that exactly fill initial capacity, forcing reallocation */
    for (int i = 0; i < 8; i++) {
        ASSERT_TRUE(vec.push_back(MoveOnly(i * 7)));
    }

    /* Now force an explicit large reserve to guarantee reallocation */
    ASSERT_TRUE(vec.reserve(64));

    /* All elements must survive reallocation intact */
    ASSERT_EQ(vec.size(), 8u);
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(vec.at((uint32_t)i).value, i * 7)
            << "element " << i << " corrupted after reserve";
        EXPECT_TRUE(vec.at((uint32_t)i).valid)
            << "element " << i << " invalid after reserve";
    }
}

TEST_F(MoveOnlyTest, Reserve_ElementsValidAfterMultipleGrowths) {
    uvector<MoveOnly> vec(&heap);

    /* Trigger several reallocations by pushing many elements */
    for (int i = 0; i < 20; i++) {
        ASSERT_TRUE(vec.push_back(MoveOnly(i + 100)));
    }

    ASSERT_EQ(vec.size(), 20u);
    for (int i = 0; i < 20; i++) {
        EXPECT_EQ(vec.at((uint32_t)i).value, i + 100);
        EXPECT_TRUE(vec.at((uint32_t)i).valid);
    }
}

// ==================== clear Tests ====================

TEST_F(MoveOnlyTest, Clear_DestroysAllElements) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(1));
    vec.push_back(MoveOnly(2));
    vec.push_back(MoveOnly(3));

    MoveOnly::reset_counters();
    vec.clear();

    EXPECT_EQ(vec.size(), 0u);
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(MoveOnly::destruct_count, 3);
}

TEST_F(MoveOnlyTest, Clear_AllowsReuseAfterClear) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(10));
    vec.clear();

    ASSERT_TRUE(vec.push_back(MoveOnly(99)));
    ASSERT_EQ(vec.size(), 1u);
    EXPECT_EQ(vec.at(0).value, 99);
}

// ==================== shrink_to_fit Tests ====================

TEST_F(MoveOnlyTest, ShrinkToFit_PreservesElements) {
    uvector<MoveOnly> vec(16, &heap);  /* pre-reserve 16 slots */
    vec.push_back(MoveOnly(11));
    vec.push_back(MoveOnly(22));
    vec.push_back(MoveOnly(33));

    EXPECT_GE(vec.capacity(), 16u);
    ASSERT_TRUE(vec.shrink_to_fit());

    ASSERT_EQ(vec.size(), 3u);
    EXPECT_EQ(vec.at(0).value, 11);
    EXPECT_EQ(vec.at(1).value, 22);
    EXPECT_EQ(vec.at(2).value, 33);
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_TRUE(vec.at(i).valid) << "element " << i << " invalid after shrink_to_fit";
    }
}

TEST_F(MoveOnlyTest, ShrinkToFit_ReducesCapacity) {
    uvector<MoveOnly> vec(32, &heap);
    vec.push_back(MoveOnly(5));
    vec.push_back(MoveOnly(6));

    size_t cap_before = vec.capacity();
    ASSERT_TRUE(vec.shrink_to_fit());
    EXPECT_LT(vec.capacity(), cap_before);
}

// ==================== Destructor / lifetime Tests ====================

TEST_F(MoveOnlyTest, Destructor_BalancedLifecycle) {
    /* Track every construction and destruction over the full scope.
     * uvector<T> holds a T err_retval member (error return for out-of-bounds at()),
     * so creating an uvector<MoveOnly> default-constructs one extra MoveOnly. */
    MoveOnly::reset_counters();
    {
        uvector<MoveOnly> vec(&heap);
        vec.push_back(MoveOnly(1));
        vec.push_back(MoveOnly(2));
        vec.push_back(MoveOnly(3));
        ASSERT_EQ(vec.size(), 3u);
    }
    /* Every constructed object must be destroyed exactly once — no leaks, no double-frees */
    EXPECT_EQ(MoveOnly::construct_count, MoveOnly::destruct_count);
    EXPECT_GE(MoveOnly::destruct_count, 3);  /* at least the 3 logical elements */
}

TEST_F(MoveOnlyTest, ElementAt_ReturnsReference) {
    uvector<MoveOnly> vec(&heap);
    vec.push_back(MoveOnly(77));
    MoveOnly& ref = vec.at(0);
    ref.value = 88;
    EXPECT_EQ(vec.at(0).value, 88);
}

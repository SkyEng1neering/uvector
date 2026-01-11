/**
 * @file test_integrity.cpp
 * @brief Comprehensive integrity tests for uvector using IntegrityObject
 *
 * These tests verify correct constructor/destructor behavior, detect
 * double destruction, memory corruption, and ensure proper RAII semantics.
 */

#include "uvector_test_config.h"

class IntegrityTest : public UvectorTestFixture {};
class IntegrityStressTest : public UvectorLargeHeapFixture {};

// ==================== Basic Lifecycle Tests ====================

TEST_F(IntegrityTest, PushBack_SingleElement_ProperLifecycle) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(42));

        EXPECT_EQ(vec.size(), 1u);
        EXPECT_EQ(vec.at(0).value, 42);
        EXPECT_TRUE(vec.at(0).is_valid());
    }

    // After vector destruction
    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, PushBack_MultipleElements_ProperLifecycle) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        EXPECT_EQ(vec.size(), 3u);
        for (uint32_t i = 0; i < vec.size(); i++) {
            EXPECT_TRUE(vec.at(i).is_valid());
            EXPECT_EQ(vec.at(i).value, static_cast<int>(i + 1));
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== Pop Operations Tests ====================

TEST_F(IntegrityTest, PopBack_SingleElement_ProperDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(42));

        int destructions_before = IntegrityObject::destruction_count;
        vec.pop_back();

        EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 1);
        EXPECT_TRUE(vec.empty());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
}

TEST_F(IntegrityTest, PopBack_MultipleElements_ProperDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        // Pop all elements one by one
        for (int i = 0; i < 3; i++) {
            int destructions_before = IntegrityObject::destruction_count;
            vec.pop_back();
            EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 1);
        }

        EXPECT_TRUE(vec.empty());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, Pop_FirstElement_ProperDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        int destructions_before = IntegrityObject::destruction_count;
        vec.pop(0);

        // Should have destructor calls for removed element and duplicate cleanup
        EXPECT_GE(IntegrityObject::destruction_count, destructions_before + 1);
        EXPECT_EQ(vec.size(), 2u);
        EXPECT_EQ(vec.at(0).value, 2);
        EXPECT_EQ(vec.at(1).value, 3);
        EXPECT_TRUE(vec.at(0).is_valid());
        EXPECT_TRUE(vec.at(1).is_valid());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
}

TEST_F(IntegrityTest, Pop_MiddleElement_ProperDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));
        vec.push_back(IntegrityObject(4));
        vec.push_back(IntegrityObject(5));

        vec.pop(2);  // Remove element with value 3

        EXPECT_EQ(vec.size(), 4u);
        EXPECT_EQ(vec.at(0).value, 1);
        EXPECT_EQ(vec.at(1).value, 2);
        EXPECT_EQ(vec.at(2).value, 4);
        EXPECT_EQ(vec.at(3).value, 5);

        // All remaining elements must be valid
        for (uint32_t i = 0; i < vec.size(); i++) {
            EXPECT_TRUE(vec.at(i).is_valid()) << "Element at index " << i << " is invalid";
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, Pop_LastElement_SingleDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        int destructions_before = IntegrityObject::destruction_count;
        vec.pop(2);  // Remove last element

        // Only ONE destructor should be called (not double destruction)
        EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 1);
        EXPECT_EQ(vec.size(), 2u);
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::double_destruction_count, 0);
}

TEST_F(IntegrityTest, Pop_OnlyElement_SingleDestruction) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(42));

        int destructions_before = IntegrityObject::destruction_count;
        vec.pop(0);

        EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 1);
        EXPECT_TRUE(vec.empty());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::double_destruction_count, 0);
}

// ==================== Clear Tests ====================

TEST_F(IntegrityTest, Clear_AllElementsDestroyed) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        int destructions_before = IntegrityObject::destruction_count;
        vec.clear();

        // All 3 elements should be destroyed
        EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 3);
        EXPECT_TRUE(vec.empty());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
}

TEST_F(IntegrityTest, Clear_ThenPushBack_Works) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.clear();

        // Push new elements
        vec.push_back(IntegrityObject(10));
        vec.push_back(IntegrityObject(20));

        EXPECT_EQ(vec.size(), 2u);
        EXPECT_EQ(vec.at(0).value, 10);
        EXPECT_EQ(vec.at(1).value, 20);
        EXPECT_TRUE(vec.at(0).is_valid());
        EXPECT_TRUE(vec.at(1).is_valid());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== Resize Tests ====================

TEST_F(IntegrityTest, ResizeShrink_ElementsDestroyed) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));
        vec.push_back(IntegrityObject(4));
        vec.push_back(IntegrityObject(5));

        int destructions_before = IntegrityObject::destruction_count;
        vec.resize(2);

        // 3 elements should be destroyed
        EXPECT_EQ(IntegrityObject::destruction_count, destructions_before + 3);
        EXPECT_EQ(vec.size(), 2u);
        EXPECT_EQ(vec.at(0).value, 1);
        EXPECT_EQ(vec.at(1).value, 2);
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
}

TEST_F(IntegrityTest, ResizeGrow_ElementsValid) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));

        vec.resize(5);

        EXPECT_EQ(vec.size(), 5u);
        // Original elements preserved
        EXPECT_EQ(vec.at(0).value, 1);
        EXPECT_EQ(vec.at(1).value, 2);
        // All elements valid
        for (uint32_t i = 0; i < vec.size(); i++) {
            EXPECT_TRUE(vec.at(i).is_valid()) << "Element at index " << i << " is invalid";
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== Copy Semantics Tests ====================

TEST_F(IntegrityTest, CopyConstructor_AllElementsValid) {
    {
        uvector<IntegrityObject> original(&heap);
        original.push_back(IntegrityObject(1));
        original.push_back(IntegrityObject(2));
        original.push_back(IntegrityObject(3));

        uvector<IntegrityObject> copy(original);

        EXPECT_EQ(copy.size(), original.size());
        for (uint32_t i = 0; i < copy.size(); i++) {
            EXPECT_TRUE(copy.at(i).is_valid());
            EXPECT_EQ(copy.at(i).value, original.at(i).value);
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, Assignment_AllElementsValid) {
    {
        uvector<IntegrityObject> original(&heap);
        original.push_back(IntegrityObject(1));
        original.push_back(IntegrityObject(2));

        uvector<IntegrityObject> copy(&heap);
        copy.push_back(IntegrityObject(100));

        copy = original;

        EXPECT_EQ(copy.size(), 2u);
        EXPECT_EQ(copy.at(0).value, 1);
        EXPECT_EQ(copy.at(1).value, 2);
        EXPECT_TRUE(copy.at(0).is_valid());
        EXPECT_TRUE(copy.at(1).is_valid());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== ShrinkToFit Tests ====================

TEST_F(IntegrityTest, ShrinkToFit_EmptyVector_ElementsDestroyed) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.reserve(10);

        vec.shrink_to_fit();

        // All reserved elements should be destroyed
        EXPECT_EQ(vec.capacity(), 0u);
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, ShrinkToFit_WithElements_ElementsPreserved) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.reserve(20);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        vec.shrink_to_fit();

        EXPECT_EQ(vec.capacity(), vec.size());
        EXPECT_EQ(vec.size(), 3u);
        for (uint32_t i = 0; i < vec.size(); i++) {
            EXPECT_TRUE(vec.at(i).is_valid());
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== Stress Tests ====================

TEST_F(IntegrityStressTest, RapidPushPop_NoCorruption) {
    {
        uvector<IntegrityObject> vec(&heap);

        for (int cycle = 0; cycle < 100; cycle++) {
            // Push several elements
            for (int i = 0; i < 5; i++) {
                vec.push_back(IntegrityObject(cycle * 10 + i));
            }

            // Pop some
            vec.pop_back();
            vec.pop_back();

            // Verify remaining are valid
            for (uint32_t i = 0; i < vec.size(); i++) {
                ASSERT_TRUE(vec.at(i).is_valid())
                    << "Corruption at cycle " << cycle << ", index " << i;
            }
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityStressTest, PopFromAllPositions_NoDoubleDestruction) {
    for (int pos = 0; pos < 10; pos++) {
        IntegrityObject::reset_counters();

        {
            uvector<IntegrityObject> vec(&heap);
            for (int i = 0; i < 10; i++) {
                vec.push_back(IntegrityObject(i));
            }

            vec.pop(pos);

            EXPECT_EQ(vec.size(), 9u);
            for (uint32_t i = 0; i < vec.size(); i++) {
                EXPECT_TRUE(vec.at(i).is_valid())
                    << "Invalid element at index " << i << " after pop(" << pos << ")";
            }
        }

        EXPECT_TRUE(IntegrityObject::no_errors())
            << "Error detected after pop(" << pos << ")";
        EXPECT_EQ(IntegrityObject::double_destruction_count, 0)
            << "Double destruction after pop(" << pos << ")";
    }
}

TEST_F(IntegrityStressTest, ClearAndRefill_NoCorruption) {
    {
        uvector<IntegrityObject> vec(&heap);

        for (int cycle = 0; cycle < 20; cycle++) {
            // Fill vector
            for (int i = 0; i < 10; i++) {
                vec.push_back(IntegrityObject(cycle * 100 + i));
            }

            // Clear
            vec.clear();
            EXPECT_TRUE(vec.empty());

            // Refill
            for (int i = 0; i < 5; i++) {
                vec.push_back(IntegrityObject(cycle * 100 + 50 + i));
            }

            // Verify
            for (uint32_t i = 0; i < vec.size(); i++) {
                ASSERT_TRUE(vec.at(i).is_valid())
                    << "Corruption at cycle " << cycle << ", index " << i;
            }

            vec.clear();
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityStressTest, ResizeUpDown_NoCorruption) {
    {
        uvector<IntegrityObject> vec(&heap);

        // Initial fill
        for (int i = 0; i < 5; i++) {
            vec.push_back(IntegrityObject(i));
        }

        // Resize up and down multiple times
        for (int cycle = 0; cycle < 10; cycle++) {
            vec.resize(20);
            for (uint32_t i = 0; i < vec.size(); i++) {
                ASSERT_TRUE(vec.at(i).is_valid())
                    << "Invalid after resize up, cycle " << cycle << ", index " << i;
            }

            vec.resize(3);
            for (uint32_t i = 0; i < vec.size(); i++) {
                ASSERT_TRUE(vec.at(i).is_valid())
                    << "Invalid after resize down, cycle " << cycle << ", index " << i;
            }
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityStressTest, MultipleVectors_IndependentLifecycles) {
    {
        uvector<IntegrityObject> v1(&heap);
        uvector<IntegrityObject> v2(&heap);
        uvector<IntegrityObject> v3(&heap);

        // Interleaved operations
        for (int i = 0; i < 10; i++) {
            v1.push_back(IntegrityObject(i));
            v2.push_back(IntegrityObject(i * 2));
            v3.push_back(IntegrityObject(i * 3));

            if (i % 2 == 0) {
                v1.pop_back();
            }
            if (i % 3 == 0 && !v2.empty()) {
                v2.pop(0);
            }
        }

        // Verify all vectors have valid elements
        for (uint32_t i = 0; i < v1.size(); i++) {
            EXPECT_TRUE(v1.at(i).is_valid());
        }
        for (uint32_t i = 0; i < v2.size(); i++) {
            EXPECT_TRUE(v2.at(i).is_valid());
        }
        for (uint32_t i = 0; i < v3.size(); i++) {
            EXPECT_TRUE(v3.at(i).is_valid());
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityStressTest, CopyAndModify_NoCorruption) {
    {
        uvector<IntegrityObject> original(&heap);
        for (int i = 0; i < 10; i++) {
            original.push_back(IntegrityObject(i));
        }

        // Multiple copies
        for (int cycle = 0; cycle < 5; cycle++) {
            uvector<IntegrityObject> copy(original);

            // Modify copy
            copy.pop_back();
            copy.pop(0);
            copy.push_back(IntegrityObject(100 + cycle));

            // Verify original unchanged and valid
            EXPECT_EQ(original.size(), 10u);
            for (uint32_t i = 0; i < original.size(); i++) {
                EXPECT_TRUE(original.at(i).is_valid());
                EXPECT_EQ(original.at(i).value, static_cast<int>(i));
            }

            // Verify copy valid
            for (uint32_t i = 0; i < copy.size(); i++) {
                EXPECT_TRUE(copy.at(i).is_valid());
            }
        }
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

// ==================== Edge Case Tests ====================

TEST_F(IntegrityTest, ReserveAfterOperations_NoLeaks) {
    {
        uvector<IntegrityObject> vec(&heap);
        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));

        vec.reserve(50);

        EXPECT_GE(vec.capacity(), 50u);
        EXPECT_EQ(vec.size(), 2u);
        EXPECT_TRUE(vec.at(0).is_valid());
        EXPECT_TRUE(vec.at(1).is_valid());
    }

    EXPECT_TRUE(IntegrityObject::no_errors());
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
}

TEST_F(IntegrityTest, AliveCountTracking) {
    IntegrityObject::reset_counters();

    {
        uvector<IntegrityObject> vec(&heap);

        // err_retval is constructed, so we start with some alive objects
        int initial_alive = IntegrityObject::alive_count();

        vec.push_back(IntegrityObject(1));
        vec.push_back(IntegrityObject(2));
        vec.push_back(IntegrityObject(3));

        // We should have more alive objects now
        EXPECT_GT(IntegrityObject::alive_count(), initial_alive);

        vec.pop_back();
        vec.pop_back();
        vec.pop_back();

        // Vector is empty but err_retval and reserved capacity may still exist
    }

    // After destruction, construction == destruction
    EXPECT_EQ(IntegrityObject::construction_count, IntegrityObject::destruction_count);
    EXPECT_TRUE(IntegrityObject::no_errors());
}

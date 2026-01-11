/**
 * @file test_stress.cpp
 * @brief Stress tests for uvector
 */

#include "uvector_test_config.h"
#include <random>

class StressTest : public UvectorLargeHeapFixture {};

// ==================== Rapid Operations ====================

TEST_F(StressTest, RapidPushPop_1000Cycles) {
    uvector<int> vec(&heap);

    for (int cycle = 0; cycle < 1000; cycle++) {
        vec.push_back(cycle);
        if (cycle % 2 == 0) {
            vec.pop_back();
        }

        if (cycle % 100 == 0) {
            assertCanaryIntegrity();
        }
    }

    EXPECT_EQ(vec.size(), 500u);  // Half were popped
}

TEST_F(StressTest, RapidReserveResize) {
    uvector<int> vec(&heap);

    for (int i = 0; i < 100; i++) {
        vec.reserve(i * 2);
        vec.resize(i);
        vec.shrink_to_fit();

        if (i % 20 == 0) {
            assertCanaryIntegrity();
        }
    }
}

// ==================== Large Vectors ====================

TEST_F(StressTest, LargeVector_100Elements) {
    uvector<int> vec(&heap);

    for (int i = 0; i < 100; i++) {
        EXPECT_TRUE(vec.push_back(i));
    }

    EXPECT_EQ(vec.size(), 100u);

    // Verify all values
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i), static_cast<int>(i));
    }
}

TEST_F(StressTest, LargeVector_RepeatedAccess) {
    uvector<int> vec(&heap);

    for (int i = 0; i < 50; i++) {
        vec.push_back(i);
    }

    // Access each element many times
    for (int round = 0; round < 100; round++) {
        for (uint32_t i = 0; i < vec.size(); i++) {
            EXPECT_EQ(vec.at(i), static_cast<int>(i));
        }
    }

    assertCanaryIntegrity();
}

// ==================== Random Operations ====================

TEST_F(StressTest, RandomOperations_500Iterations) {
    std::mt19937 rng(42);  // Fixed seed for reproducibility
    std::uniform_int_distribution<int> op_dist(0, 3);
    std::uniform_int_distribution<int> val_dist(0, 1000);

    uvector<int> vec(&heap);

    for (int iter = 0; iter < 500; iter++) {
        int op = op_dist(rng);

        switch (op) {
            case 0:  // push_back
                vec.push_back(val_dist(rng));
                break;
            case 1:  // pop_back
                vec.pop_back();
                break;
            case 2:  // clear (occasionally)
                if (vec.size() > 20) {
                    vec.clear();
                }
                break;
            case 3:  // access
                if (!vec.empty()) {
                    std::uniform_int_distribution<uint32_t> idx_dist(0, vec.size() - 1);
                    (void)vec.at(idx_dist(rng));
                }
                break;
        }

        if (iter % 50 == 0) {
            assertCanaryIntegrity();
        }
    }
}

TEST_F(StressTest, RandomPushPopPattern) {
    std::mt19937 rng(777);
    std::uniform_int_distribution<int> dist(0, 100);

    uvector<int> vec(&heap);

    for (int i = 0; i < 200; i++) {
        int push_count = dist(rng) % 10 + 1;
        int pop_count = dist(rng) % 5;

        for (int j = 0; j < push_count; j++) {
            vec.push_back(dist(rng));
        }

        for (int j = 0; j < pop_count && !vec.empty(); j++) {
            vec.pop_back();
        }
    }

    assertCanaryIntegrity();
}

// ==================== Memory Patterns ====================

TEST_F(StressTest, AllocFreeAlloc_Cycles) {
    for (int cycle = 0; cycle < 50; cycle++) {
        uvector<int> vec(&heap);

        for (int i = 0; i < 20; i++) {
            vec.push_back(i);
        }

        // Vector destroyed at end of scope
    }

    assertCanaryIntegrity();

    // Heap should be reusable
    uvector<int> final_vec(&heap);
    for (int i = 0; i < 50; i++) {
        EXPECT_TRUE(final_vec.push_back(i));
    }
}

TEST_F(StressTest, GrowShrinkGrow_Pattern) {
    uvector<int> vec(&heap);

    // Grow
    for (int i = 0; i < 50; i++) {
        vec.push_back(i);
    }
    EXPECT_EQ(vec.size(), 50u);

    // Shrink
    for (int i = 0; i < 30; i++) {
        vec.pop_back();
    }
    EXPECT_EQ(vec.size(), 20u);

    // Grow again
    for (int i = 0; i < 40; i++) {
        vec.push_back(100 + i);
    }
    EXPECT_EQ(vec.size(), 60u);

    // Verify first 20 are original
    for (int i = 0; i < 20; i++) {
        EXPECT_EQ(vec.at(i), i);
    }

    assertCanaryIntegrity();
}

// ==================== Multiple Vectors ====================

TEST_F(StressTest, MultipleVectors_Interleaved) {
    uvector<int> v1(&heap);
    uvector<int> v2(&heap);
    uvector<int> v3(&heap);

    for (int i = 0; i < 30; i++) {
        v1.push_back(i);
        v2.push_back(i * 2);
        v3.push_back(i * 3);
    }

    EXPECT_EQ(v1.size(), 30u);
    EXPECT_EQ(v2.size(), 30u);
    EXPECT_EQ(v3.size(), 30u);

    // Verify independence
    for (uint32_t i = 0; i < 30; i++) {
        EXPECT_EQ(v1.at(i), static_cast<int>(i));
        EXPECT_EQ(v2.at(i), static_cast<int>(i * 2));
        EXPECT_EQ(v3.at(i), static_cast<int>(i * 3));
    }

    assertCanaryIntegrity();
}

TEST_F(StressTest, MultipleVectors_InterleavedGrowShrink) {
    uvector<int> v1(&heap);
    uvector<int> v2(&heap);

    for (int i = 0; i < 50; i++) {
        v1.push_back(i);
        v2.push_back(i);

        if (i % 3 == 0 && !v1.empty()) {
            v1.pop_back();
        }
        if (i % 5 == 0 && !v2.empty()) {
            v2.pop_back();
        }
    }

    assertCanaryIntegrity();
}

// ==================== Object Lifecycle Stress ====================

TEST_F(StressTest, TrackedObjects_ManyAllocations) {
    TrackedObject::reset_counters();

    {
        uvector<TrackedObject> vec(&heap);

        for (int i = 0; i < 30; i++) {
            vec.push_back(TrackedObject(i));
        }

        // Pop some
        for (int i = 0; i < 10; i++) {
            vec.pop_back();
        }

        // Push more
        for (int i = 0; i < 5; i++) {
            vec.push_back(TrackedObject(100 + i));
        }

        EXPECT_EQ(vec.size(), 25u);
    }

    // All constructed objects must be destroyed (no leaks)
    EXPECT_EQ(TrackedObject::construction_count, TrackedObject::destruction_count);
    assertCanaryIntegrity();
}

TEST_F(StressTest, POD_ManyOperations) {
    uvector<SimplePOD> vec(&heap);

    for (int i = 0; i < 100; i++) {
        vec.push_back({i, static_cast<float>(i) * 0.5f});
    }

    // Modify all
    for (uint32_t i = 0; i < vec.size(); i++) {
        vec.at(i).x *= 2;
        vec.at(i).y += 1.0f;
    }

    // Verify
    for (uint32_t i = 0; i < vec.size(); i++) {
        EXPECT_EQ(vec.at(i).x, static_cast<int>(i * 2));
        EXPECT_FLOAT_EQ(vec.at(i).y, static_cast<float>(i) * 0.5f + 1.0f);
    }

    assertCanaryIntegrity();
}

// ==================== Copy Stress ====================

TEST_F(StressTest, RepeatedCopies) {
    uvector<int> original(&heap);
    for (int i = 0; i < 30; i++) {
        original.push_back(i);
    }

    for (int cycle = 0; cycle < 50; cycle++) {
        uvector<int> copy(original);
        EXPECT_EQ(copy.size(), original.size());

        // Modify copy
        if (!copy.empty()) {
            copy.at(0) = 999;
        }

        // Original should be unchanged
        EXPECT_EQ(original.at(0), 0);
    }

    assertCanaryIntegrity();
}

TEST_F(StressTest, ChainedCopyAssign) {
    uvector<int> v1(&heap);
    v1.push_back(1);
    v1.push_back(2);

    uvector<int> v2(&heap);
    uvector<int> v3(&heap);
    uvector<int> v4(&heap);

    for (int i = 0; i < 20; i++) {
        v2 = v1;
        v3 = v2;
        v4 = v3;

        v1.push_back(i);
    }

    EXPECT_GT(v1.size(), v4.size());
    assertCanaryIntegrity();
}

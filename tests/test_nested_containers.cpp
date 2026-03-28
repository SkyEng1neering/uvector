/**
 * @file test_nested_containers.cpp
 * @brief Tests for nested uvector containers
 *
 * Exercises dalloc defragmentation correctness under nested uvector growth:
 *   uvector<uvector<T>> and uvector<uvector<uvector<T>>>
 *
 * Each outer realloc (reserve) does:
 *   1. Alloc new outer buffer
 *   2. Copy-assign inner vectors to new positions  (each copy allocates a new
 *      inner buffer, each inner dfree triggers defrag)
 *   3. Destroy old inner vectors  (dfree inner bufs → defrag)
 *   4. dfree old outer buffer  → defrag
 *
 * This chain exercises both dalloc Bug #1 (tracking variable address inside a
 * later heap region not updated) and Bug #2 (pointer value before freed region
 * incorrectly decremented) fix paths.
 */

#include "uvector_test_config.h"

// ==================== Fixture ====================

class NestedContainerTest : public UvectorLargeHeapFixture {};

// ==================== Two-level nested uvector ====================

// Push 10 inner vectors — triggers multiple outer reallocations.
// After each outer realloc, inner vector contents must be intact.
TEST_F(NestedContainerTest, TwoLevel_GrowthAndContentIntegrity) {
    uvector<uvector<int>> outer(&heap);

    for (int i = 0; i < 10; i++) {
        uvector<int> inner(&heap);
        for (int j = 0; j < 4; j++) {
            ASSERT_TRUE(inner.push_back(i * 10 + j));
        }
        ASSERT_TRUE(outer.push_back(inner));
    }

    ASSERT_EQ(outer.size(), 10u);
    for (int i = 0; i < 10; i++) {
        uvector<int>& inner = outer.at((uint32_t)i);
        ASSERT_EQ(inner.size(), 4u) << "inner[" << i << "] size";
        for (int j = 0; j < 4; j++) {
            EXPECT_EQ(inner.at((uint32_t)j), i * 10 + j)
                << "inner[" << i << "][" << j << "]";
        }
    }
}

// Build outer with small inner vecs, then grow each inner vector independently.
// Growing an inner vec (that is stored inside another vec's buffer) triggers
// defrag inside the inner allocation — outer tracking must survive.
TEST_F(NestedContainerTest, TwoLevel_GrowInnerAfterOuterBuilt) {
    uvector<uvector<int>> outer(&heap);

    for (int i = 0; i < 6; i++) {
        uvector<int> inner(&heap);
        inner.push_back(i * 100);
        ASSERT_TRUE(outer.push_back(inner));
    }

    for (uint32_t i = 0; i < outer.size(); i++) {
        for (int j = 1; j < 8; j++) {
            ASSERT_TRUE(outer.at(i).push_back((int)i * 100 + j))
                << "outer[" << i << "].push_back(" << j << ")";
        }
    }

    ASSERT_EQ(outer.size(), 6u);
    for (uint32_t i = 0; i < 6u; i++) {
        ASSERT_EQ(outer.at(i).size(), 8u) << "inner[" << i << "] size";
        for (int j = 0; j < 8; j++) {
            EXPECT_EQ(outer.at(i).at((uint32_t)j), (int)i * 100 + j)
                << "inner[" << i << "][" << j << "]";
        }
    }
}

// Pop the middle element — shifts right elements via copy-assign, each of which
// reallocates inner buffers and triggers defrag during the old-inner dfree.
TEST_F(NestedContainerTest, TwoLevel_PopMiddleVerifyRemaining) {
    uvector<uvector<int>> outer(&heap);

    for (int i = 0; i < 5; i++) {
        uvector<int> inner(&heap);
        for (int j = 0; j < 3; j++) {
            inner.push_back(i * 10 + j);
        }
        ASSERT_TRUE(outer.push_back(inner));
    }

    ASSERT_TRUE(outer.pop(2));
    ASSERT_EQ(outer.size(), 4u);

    // Remaining: [0,1,3,4]
    int ids[] = {0, 1, 3, 4};
    for (int k = 0; k < 4; k++) {
        int i = ids[k];
        ASSERT_EQ(outer.at((uint32_t)k).size(), 3u) << "vec[" << k << "] size";
        for (int j = 0; j < 3; j++) {
            EXPECT_EQ(outer.at((uint32_t)k).at((uint32_t)j), i * 10 + j)
                << "outer[" << k << "][" << j << "]";
        }
    }
}

// Pre-reserve excess capacity at both levels, push few elements, shrink at both.
// shrink_to_fit reallocates to exact size → two separate defrag events per level.
TEST_F(NestedContainerTest, TwoLevel_ShrinkBothLevels) {
    uvector<uvector<int>> outer(8, &heap);

    for (int i = 0; i < 4; i++) {
        uvector<int> inner(12, &heap);
        for (int j = 0; j < 3; j++) {
            inner.push_back(i * 10 + j);
        }
        ASSERT_TRUE(outer.push_back(inner));
    }

    for (uint32_t i = 0; i < outer.size(); i++) {
        ASSERT_TRUE(outer.at(i).shrink_to_fit()) << "shrink inner " << i;
    }
    ASSERT_TRUE(outer.shrink_to_fit());

    ASSERT_EQ(outer.size(), 4u);
    for (int i = 0; i < 4; i++) {
        ASSERT_EQ(outer.at((uint32_t)i).size(), 3u);
        for (int j = 0; j < 3; j++) {
            EXPECT_EQ(outer.at((uint32_t)i).at((uint32_t)j), i * 10 + j);
        }
    }
}

// Pop, then push more to trigger realloc with modified element layout.
TEST_F(NestedContainerTest, TwoLevel_PopThenGrowOuter) {
    uvector<uvector<int>> outer(&heap);

    for (int i = 0; i < 7; i++) {
        uvector<int> inner(&heap);
        for (int j = 0; j < 3; j++) {
            inner.push_back(i * 10 + j);
        }
        ASSERT_TRUE(outer.push_back(inner));
    }

    ASSERT_TRUE(outer.pop(3));
    ASSERT_EQ(outer.size(), 6u);

    // Push 4 more to force realloc after pop
    for (int i = 7; i < 11; i++) {
        uvector<int> inner(&heap);
        for (int j = 0; j < 3; j++) {
            inner.push_back(i * 10 + j);
        }
        ASSERT_TRUE(outer.push_back(inner));
    }

    ASSERT_EQ(outer.size(), 10u);

    // Check all non-removed inner vecs: original [0,1,2,4,5,6], then [7,8,9,10]
    int ids[] = {0, 1, 2, 4, 5, 6, 7, 8, 9, 10};
    for (int k = 0; k < 10; k++) {
        int i = ids[k];
        ASSERT_EQ(outer.at((uint32_t)k).size(), 3u) << "vec[" << k << "] size";
        for (int j = 0; j < 3; j++) {
            EXPECT_EQ(outer.at((uint32_t)k).at((uint32_t)j), i * 10 + j)
                << "outer[" << k << "][" << j << "]";
        }
    }
}

// ==================== Three-level nested uvector ====================

// top[i][j][k] = i*100 + j*10 + k
// Building this structure causes outer/mid/inner reallocations in cascade,
// each with its own defrag event.
TEST_F(NestedContainerTest, ThreeLevel_BuildAndVerifyContent) {
    uvector<uvector<uvector<int>>> top(&heap);

    for (int i = 0; i < 3; i++) {
        uvector<uvector<int>> mid(&heap);
        for (int j = 0; j < 3; j++) {
            uvector<int> inner(&heap);
            for (int k = 0; k < 4; k++) {
                ASSERT_TRUE(inner.push_back(i * 100 + j * 10 + k));
            }
            ASSERT_TRUE(mid.push_back(inner));
        }
        ASSERT_TRUE(top.push_back(mid));
    }

    ASSERT_EQ(top.size(), 3u);
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(top.at((uint32_t)i).size(), 3u) << "top[" << i << "] size";
        for (int j = 0; j < 3; j++) {
            ASSERT_EQ(top.at((uint32_t)i).at((uint32_t)j).size(), 4u)
                << "top[" << i << "][" << j << "] size";
            for (int k = 0; k < 4; k++) {
                EXPECT_EQ(top.at((uint32_t)i).at((uint32_t)j).at((uint32_t)k),
                          i * 100 + j * 10 + k)
                    << "top[" << i << "][" << j << "][" << k << "]";
            }
        }
    }
}

// After building the 3-level structure, grow each level-3 vector.
// This fires additional defrag events inside the fully-built structure and
// tests that all tracking remains correct under mixed-growth patterns.
TEST_F(NestedContainerTest, ThreeLevel_GrowInnermostAfterBuild) {
    uvector<uvector<uvector<int>>> top(&heap);

    for (int i = 0; i < 3; i++) {
        uvector<uvector<int>> mid(&heap);
        for (int j = 0; j < 3; j++) {
            uvector<int> inner(&heap);
            inner.push_back(i * 100 + j * 10);
            ASSERT_TRUE(mid.push_back(inner));
        }
        ASSERT_TRUE(top.push_back(mid));
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            for (int k = 1; k < 6; k++) {
                ASSERT_TRUE(top.at((uint32_t)i).at((uint32_t)j)
                                .push_back(i * 100 + j * 10 + k))
                    << "top[" << i << "][" << j << "].push(" << k << ")";
            }
        }
    }

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            ASSERT_EQ(top.at((uint32_t)i).at((uint32_t)j).size(), 6u)
                << "top[" << i << "][" << j << "] size";
            for (int k = 0; k < 6; k++) {
                EXPECT_EQ(top.at((uint32_t)i).at((uint32_t)j).at((uint32_t)k),
                          i * 100 + j * 10 + k);
            }
        }
    }
}

// Pop from mid level, then grow the modified mid-level vec.
TEST_F(NestedContainerTest, ThreeLevel_PopMidAndGrow) {
    uvector<uvector<uvector<int>>> top(&heap);

    for (int i = 0; i < 3; i++) {
        uvector<uvector<int>> mid(&heap);
        for (int j = 0; j < 4; j++) {
            uvector<int> inner(&heap);
            for (int k = 0; k < 3; k++) {
                inner.push_back(i * 100 + j * 10 + k);
            }
            ASSERT_TRUE(mid.push_back(inner));
        }
        ASSERT_TRUE(top.push_back(mid));
    }

    // Pop mid element from top[1]
    ASSERT_TRUE(top.at(1).pop(1));
    ASSERT_EQ(top.at(1).size(), 3u);

    // Grow top[1] back to 4 mid elements
    uvector<int> extra(&heap);
    for (int k = 0; k < 3; k++) {
        extra.push_back(199 + k);
    }
    ASSERT_TRUE(top.at(1).push_back(extra));
    ASSERT_EQ(top.at(1).size(), 4u);

    // Verify top[0] and top[2] are untouched
    for (int i : {0, 2}) {
        ASSERT_EQ(top.at((uint32_t)i).size(), 4u) << "top[" << i << "] size";
        for (int j = 0; j < 4; j++) {
            for (int k = 0; k < 3; k++) {
                EXPECT_EQ(top.at((uint32_t)i).at((uint32_t)j).at((uint32_t)k),
                          i * 100 + j * 10 + k);
            }
        }
    }

    // Verify surviving top[1] elements: [0,2,3] (index 1 removed)
    int top1_ids[] = {0, 2, 3};
    for (int k = 0; k < 3; k++) {
        int j = top1_ids[k];
        for (int m = 0; m < 3; m++) {
            EXPECT_EQ(top.at(1).at((uint32_t)k).at((uint32_t)m), 1 * 100 + j * 10 + m)
                << "top[1][" << k << "][" << m << "]";
        }
    }
}

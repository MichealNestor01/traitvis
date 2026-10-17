#include "uniqueHandle.hpp"

#include <gtest/gtest.h>
#include <utility>

namespace {

struct CountingDeleter {
    static inline int calls = 0;
    void operator()(unsigned int) const noexcept { ++calls; }
};

using Handle = UniqueHandle<CountingDeleter>;

struct TaggedDeleter {
    int tag = 0;
    static inline int last = 0;
    void operator()(unsigned int) const noexcept { last = tag; }
};

}

TEST(UniqueHandleTests, DeletesOnceOnDestruction) {
    CountingDeleter::calls = 0;
    {
        Handle handle(7);
    }
    EXPECT_EQ(CountingDeleter::calls, 1);
}

TEST(UniqueHandleTests, DefaultHandleDeletesNothing) {
    CountingDeleter::calls = 0;
    {
        Handle handle;
    }
    EXPECT_EQ(CountingDeleter::calls, 0);
}

TEST(UniqueHandleTests, MoveTransfersOwnershipWithoutDeleting) {
    CountingDeleter::calls = 0;
    {
        Handle source(7);
        Handle target(std::move(source));
        EXPECT_FALSE(source);
        EXPECT_EQ(target.get(), 7u);
        EXPECT_EQ(CountingDeleter::calls, 0);
    }
    EXPECT_EQ(CountingDeleter::calls, 1);
}

TEST(UniqueHandleTests, MoveAssignmentReleasesPreviousHandle) {
    CountingDeleter::calls = 0;
    Handle a(1);
    Handle b(2);
    a = std::move(b);
    EXPECT_EQ(CountingDeleter::calls, 1);
    EXPECT_EQ(a.get(), 2u);
    EXPECT_FALSE(b);
}

TEST(UniqueHandleTests, MoveKeepsTheDeleterWithTheHandle) {
    TaggedDeleter::last = 0;
    UniqueHandle<TaggedDeleter> source(4, TaggedDeleter{9});
    UniqueHandle<TaggedDeleter> target(std::move(source));
    target.reset();
    EXPECT_EQ(TaggedDeleter::last, 9);
    EXPECT_FALSE(source);
}

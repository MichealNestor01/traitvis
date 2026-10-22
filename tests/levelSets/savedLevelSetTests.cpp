#include "savedLevelSet.hpp"
#include "sessionState.hpp"

#include <gtest/gtest.h>
#include <sstream>
#include <thread>
#include <vector>

namespace {

FeatureLevelSet keptLevelSet() {
    FeatureLevelSet levelSet({}, {}, glm::vec3{0.2f, 0.4f, 0.6f}, "high wind", {}, false);
    levelSet.transparency = 0.35f;
    levelSet.isoValue = 0.5f;
    levelSet.distanceField = ScalarField{
        .layout = {2, 2, 2},
        .values = {0.f, 1.f, 0.4f, 0.f, 0.f, 0.f, 1.f, 0.f},
    };
    return levelSet;
}

std::vector<unsigned char> bytesOf(const FeatureLevelSet& levelSet) {
    std::stringstream out;
    EXPECT_TRUE(writeSavedLevelSet(levelSet, out));
    const std::string text = out.str();
    return {text.begin(), text.end()};
}

void finishOpen(SessionState& session) {
    while (session.activeLevelSet && !session.activeLevelSet->finished()) {
        if (!session.activeLevelSet->runsOnWorker()) session.activeLevelSet->stepOnMainThread(8);
        else std::this_thread::yield();
    }
    session.pollLevelSet();
}

}

TEST(SavedLevelSetTests, RoundTripKeepsTheFieldAndTheAppearance) {
    const FeatureLevelSet levelSet = keptLevelSet();
    const std::vector<unsigned char> bytes = bytesOf(levelSet);
    const SavedLevelSet saved = readSavedLevelSet(bytes);

    ASSERT_TRUE(saved.ok());
    EXPECT_EQ(saved.id, "high wind");
    EXPECT_FLOAT_EQ(saved.colour.x, 0.2f);
    EXPECT_FLOAT_EQ(saved.colour.y, 0.4f);
    EXPECT_FLOAT_EQ(saved.colour.z, 0.6f);
    EXPECT_FLOAT_EQ(saved.transparency, 0.35f);
    EXPECT_FLOAT_EQ(saved.isoValue, 0.5f);
    EXPECT_FALSE(saved.closeAtMissingData);
    EXPECT_EQ(saved.field.layout.x, 2);
    EXPECT_EQ(saved.field.layout.y, 2);
    EXPECT_EQ(saved.field.layout.z, 2);
    EXPECT_EQ(saved.field.values, levelSet.distanceField->values);
}

TEST(SavedLevelSetTests, ALevelSetWithoutAFieldIsNotWritten) {
    FeatureLevelSet levelSet({}, {}, glm::vec3{1.f, 0.f, 0.f}, "mesh", {});
    std::stringstream out;
    EXPECT_FALSE(writeSavedLevelSet(levelSet, out));
    EXPECT_TRUE(out.str().empty());
}

TEST(SavedLevelSetTests, AFileThatIsNotALevelSetIsRefused) {
    const unsigned char text[] = {'h', 'e', 'l', 'l', 'o'};
    const SavedLevelSet saved = readSavedLevelSet(text);
    EXPECT_FALSE(saved.ok());
    EXPECT_EQ(saved.error, "That file is not a TraitVis level set.");
}

TEST(SavedLevelSetTests, ATruncatedFileIsRefused) {
    std::vector<unsigned char> bytes = bytesOf(keptLevelSet());
    bytes.pop_back();
    const SavedLevelSet saved = readSavedLevelSet(bytes);
    EXPECT_FALSE(saved.ok());
    EXPECT_EQ(saved.error, "That level set file is not valid.");
}

TEST(SavedLevelSetTests, OpeningExtractsASurfaceThatCanBeResliced) {
    SessionState session;
    ASSERT_TRUE(session.openSavedLevelSet(bytesOf(keptLevelSet())));
    finishOpen(session);

    ASSERT_EQ(session.levelSets.size(), 1u);
    FeatureLevelSet& levelSet = session.levelSets[0];
    EXPECT_EQ(levelSet.id, "high wind");
    EXPECT_FLOAT_EQ(levelSet.transparency, 0.35f);
    ASSERT_TRUE(levelSet.distanceField);
    EXPECT_FALSE(levelSet.surface.vertices.empty());

    const std::vector<glm::vec3> first = levelSet.surface.vertices;
    ASSERT_TRUE(levelSet.reslice(0.2f));
    EXPECT_NE(levelSet.surface.vertices, first);
    EXPECT_FLOAT_EQ(levelSet.isoValue, 0.2f);
}

TEST(SavedLevelSetTests, ASecondCopyOfTheSameNameGetsASuffix) {
    SessionState session;
    ASSERT_TRUE(session.openSavedLevelSet(bytesOf(keptLevelSet())));
    finishOpen(session);
    ASSERT_TRUE(session.openSavedLevelSet(bytesOf(keptLevelSet())));
    finishOpen(session);

    ASSERT_EQ(session.levelSets.size(), 2u);
    EXPECT_EQ(session.levelSets[0].id, "high wind");
    EXPECT_EQ(session.levelSets[1].id, "high wind 2");
}

TEST(SavedLevelSetTests, OpeningIsRefusedWhileALevelSetIsAlreadyBuilding) {
    SessionState session;
    ASSERT_TRUE(session.openSavedLevelSet(bytesOf(keptLevelSet())));
    EXPECT_FALSE(session.openSavedLevelSet(bytesOf(keptLevelSet())));
    EXPECT_EQ(session.loadError, "A level set is already being built.");
    finishOpen(session);
    EXPECT_EQ(session.levelSets.size(), 1u);
}

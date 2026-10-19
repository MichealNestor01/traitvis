#include "sessionState.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

std::shared_ptr<MultiField> loadTestDataset() {
    return std::make_shared<MultiField>(readDataset("tests/parsingData/testDataset/config.txt"));
}

struct SessionView {
    struct Point {
        const Attribute* attribute = nullptr;
        float value = 0.f;
        bool operator==(const Point&) const = default;
    };
    struct Set {
        std::string id;
        float r = 0.f;
        float g = 0.f;
        float b = 0.f;
        float transparency = 0.f;
        bool operator==(const Set&) const = default;
    };
    std::vector<Point> points;
    std::vector<Set> levelSets;
    bool operator==(const SessionView&) const = default;
};

SessionView viewOf(const SessionState& session) {
    SessionView view;
    for (const TraitPoint& point : session.attributeSpacePointsBuffer)
        for (const TraitPointComponent& component : point.values)
            view.points.push_back({component.attribute, component.value});
    for (const FeatureLevelSet& levelSet : session.levelSets)
        view.levelSets.push_back({levelSet.id, levelSet.colour.x, levelSet.colour.y, levelSet.colour.z, levelSet.transparency});
    return view;
}

template <typename Edit>
void expectRoundTrip(SessionState& session, Edit edit) {
    const SessionView before = viewOf(session);
    edit();
    const SessionView after = viewOf(session);
    EXPECT_NE(before, after);
    session.undo();
    EXPECT_EQ(viewOf(session), before);
    session.redo();
    EXPECT_EQ(viewOf(session), after);
}

void addPoint(SessionState& session, float value) {
    TraitPoint point;
    point.values.push_back({&session.activeDataset()->attributeDomain[0], value});
    session.addAttributeSpacePointToBuffer(std::move(point));
}

void addShell(SessionState& session, std::string id, glm::vec3 colour = {1.f, 0.f, 0.f}) {
    session.addLevelSet(FeatureLevelSet({}, Surface{}, colour, std::move(id), {}));
}

void useTestDataset(SessionState& session) {
    session.addDataset(loadTestDataset());
    ASSERT_TRUE(session.activeDataset());
    ASSERT_TRUE(session.activeDataset()->ok());
}

}

TEST(UndoTests, EmptyHistoryIgnoresUndoAndRedo) {
    SessionState session;
    session.undo();
    session.redo();
    EXPECT_FALSE(session.canUndo());
    EXPECT_FALSE(session.canRedo());
    EXPECT_TRUE(session.attributeSpacePointsBuffer.empty());
    EXPECT_TRUE(session.levelSets.empty());
}

TEST(UndoTests, AddingATraitPointRoundTrips) {
    SessionState session;
    useTestDataset(session);
    expectRoundTrip(session, [&] { addPoint(session, 0.25f); });
}

TEST(UndoTests, RemovingATraitPointRestoresItAtTheSameIndex) {
    SessionState session;
    useTestDataset(session);
    addPoint(session, 0.25f);
    addPoint(session, 0.75f);
    expectRoundTrip(session, [&] { session.removeAttributeSpacePoint(0); });
}

TEST(UndoTests, AddingALevelSetRoundTrips) {
    SessionState session;
    expectRoundTrip(session, [&] { addShell(session, "a"); });
}

TEST(UndoTests, RemovingALevelSetRestoresItAtTheSameIndex) {
    SessionState session;
    addShell(session, "a");
    addShell(session, "b");
    expectRoundTrip(session, [&] { session.removeLevelSet(0); });
}

TEST(UndoTests, ColourChangeRoundTrips) {
    SessionState session;
    addShell(session, "a", {1.f, 0.f, 0.f});
    expectRoundTrip(session, [&] { session.setLevelSetColour(0, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f}); });
}

TEST(UndoTests, TransparencyChangeRoundTrips) {
    SessionState session;
    addShell(session, "a");
    expectRoundTrip(session, [&] { session.setLevelSetTransparency(0, 1.f, 0.25f); });
}

TEST(UndoTests, UnchangedColourAndTransparencyAreNotRecorded) {
    SessionState session;
    addShell(session, "a", {1.f, 0.f, 0.f});
    session.setLevelSetColour(0, {1.f, 0.f, 0.f}, {1.f, 0.f, 0.f});
    session.setLevelSetTransparency(0, 1.f, 1.f);
    session.undo();
    EXPECT_TRUE(session.levelSets.empty());
    EXPECT_FALSE(session.canUndo());
}

TEST(UndoTests, ANewCommandClearsTheRedoStack) {
    SessionState session;
    useTestDataset(session);
    addPoint(session, 0.25f);
    session.undo();
    ASSERT_TRUE(session.canRedo());

    addPoint(session, 0.75f);
    EXPECT_FALSE(session.canRedo());

    session.undo();
    ASSERT_TRUE(session.attributeSpacePointsBuffer.empty());
    session.redo();
    ASSERT_EQ(session.attributeSpacePointsBuffer.size(), 1u);
    EXPECT_FLOAT_EQ(session.attributeSpacePointsBuffer[0].values[0].value, 0.75f);
}

TEST(UndoTests, UndoRestoresARemovedLevelSetBeforeRevertingItsColour) {
    SessionState session;
    addShell(session, "a");
    addShell(session, "b");
    session.setLevelSetColour(1, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f});
    session.removeLevelSet(0);

    session.undo();
    ASSERT_EQ(session.levelSets.size(), 2u);
    EXPECT_EQ(session.levelSets[0].id, "a");
    EXPECT_EQ(session.levelSets[1].id, "b");
    EXPECT_FLOAT_EQ(session.levelSets[1].colour.z, 1.f);

    session.undo();
    EXPECT_FLOAT_EQ(session.levelSets[1].colour.x, 1.f);
    EXPECT_FLOAT_EQ(session.levelSets[1].colour.z, 0.f);
}

TEST(UndoTests, SwitchingDatasetDropsTraitCommandsAndKeepsLevelSetCommands) {
    SessionState session;
    const std::shared_ptr<MultiField> first = loadTestDataset();
    const std::shared_ptr<MultiField> second = loadTestDataset();
    ASSERT_TRUE(first->ok());
    session.addDataset(first);
    session.addDataset(second);
    ASSERT_TRUE(session.setActiveDataset(0));

    addPoint(session, 0.25f);
    addShell(session, "shell");
    session.setLevelSetColour(0, {1.f, 0.f, 0.f}, {0.f, 0.f, 1.f});
    ASSERT_TRUE(session.setActiveDataset(1));
    EXPECT_TRUE(session.attributeSpacePointsBuffer.empty());

    session.undo();
    EXPECT_FLOAT_EQ(session.levelSets[0].colour.x, 1.f);
    EXPECT_TRUE(session.attributeSpacePointsBuffer.empty());

    session.undo();
    EXPECT_TRUE(session.levelSets.empty());
    EXPECT_FALSE(session.canUndo());
}

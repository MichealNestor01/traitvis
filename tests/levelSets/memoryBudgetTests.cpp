#include "sessionState.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace {

std::shared_ptr<MultiField> fourSamples() {
    auto dataset = std::make_shared<MultiField>();
    dataset->attributeDomain.push_back(Attribute{"a", {0.f, 1.f}, std::vector<float>(4)});
    return dataset;
}

}

TEST(MemoryBudgetTests, FourteenIsabelAttributesFitAndASecondCopyDoesNot) {
    const SessionState session;
    constexpr std::size_t oneAttribute = 25000000ull * sizeof(float);
    EXPECT_TRUE(session.loadFits(14 * oneAttribute));
    EXPECT_FALSE(session.loadFits(14 * oneAttribute * 2));
}

TEST(MemoryBudgetTests, AFourteenAttributeFolderOverTheCapIsRefused) {
    const std::filesystem::path directory = std::filesystem::temp_directory_path() / "traitvis-memory-cap";
    std::filesystem::create_directories(directory);
    const std::filesystem::path configPath = directory / "config.txt";
    {
        std::ofstream config(configPath);
        config << "NAME:over\nFILEPATH:./\nSPATIALDOMAIN:1:1:1\nDIMENSIONORDER:WIDTH_DEPTH_HEIGHT\n"
                  "INDEXSCHEME:ROWMAJOR\nDATASETSTRUCTURE:ATTRIBUTEPERFILE:40000000\n";
        for (int index = 0; index < 14; ++index)
            config << "ATTRIBUTE:attr" << index << ".bin:Attr " << index << ":0:1\n";
    }

    SessionState session;
    EXPECT_FALSE(session.beginDatasetLoad(configPath.string()));
    EXPECT_FALSE(session.activeLoad);
    EXPECT_EQ(session.loadError, "That selection is over the 2 GB limit.");
    std::filesystem::remove_all(directory);
}

TEST(MemoryBudgetTests, ResidentBytesFollowsDatasetsLevelSetsAndUndo) {
    SessionState session;
    const std::shared_ptr<MultiField> dataset = fourSamples();
    session.addDataset(dataset);
    EXPECT_EQ(session.residentBytes(), 4u * sizeof(float));

    FeatureLevelSet levelSet({}, Surface{}, {1.f, 0.f, 0.f}, "shell", dataset);
    levelSet.distanceField = ScalarField{GridLayout{1, 1, 2}, std::vector<float>(2)};
    session.addLevelSet(std::move(levelSet));
    EXPECT_EQ(session.residentBytes(), 6u * sizeof(float));

    session.removeDataset(0);
    EXPECT_EQ(session.datasets.size(), 0u);
    EXPECT_EQ(session.residentBytes(), 6u * sizeof(float));

    session.removeLevelSet(0);
    EXPECT_TRUE(session.levelSets.empty());
    EXPECT_EQ(session.residentBytes(), 6u * sizeof(float));

    session.clearHistory();
    EXPECT_EQ(session.residentBytes(), 0u);
}

TEST(MemoryBudgetTests, ASmallDatasetStillLoads) {
    SessionState session;
    ASSERT_TRUE(session.beginDatasetLoad("tests/parsingData/testDataset/config.txt"));
    for (;;) {
        if (session.pollDatasetLoad()) break;
        ASSERT_TRUE(session.activeLoad);
        if (session.activeLoad->runsOnWorker()) std::this_thread::yield();
    }
    ASSERT_TRUE(session.activeDataset());
    EXPECT_TRUE(session.activeDataset()->ok());
    EXPECT_GT(session.residentBytes(), 0u);
    EXPECT_TRUE(session.loadError.empty());
}

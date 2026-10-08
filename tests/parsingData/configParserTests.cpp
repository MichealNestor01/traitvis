#include "configParser.hpp"

#include <gtest/gtest.h>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

struct BadConfigCase {
    const char* name;
    const char* filepath;
    const char* expectedError;
};

TEST(ConfigParserTests, RelativeFilepathIsResolvedAgainstConfigDirectory) {
    DatasetDirConfig config = parseConfig("tests/parsingData/testDataset/config.txt");
    ASSERT_FALSE(config.parseError);
    EXPECT_EQ(std::filesystem::weakly_canonical(config.filePath),
              std::filesystem::weakly_canonical("tests/parsingData/testDataset/"));
}

class ConfigParserBadFileTests : public testing::TestWithParam<BadConfigCase> {};

TEST_P(ConfigParserBadFileTests, ReturnsParseErrorWithMessage) {
    const auto& [name, filepath, expectedError] = GetParam();

    std::streambuf* originalCerrBuffer = std::cerr.rdbuf();
    std::ostringstream redirectedCerr;
    std::cerr.rdbuf(redirectedCerr.rdbuf());

    DatasetDirConfig result = parseConfig(filepath);

    std::cerr.rdbuf(originalCerrBuffer);

    EXPECT_TRUE(result.parseError);
    EXPECT_EQ(redirectedCerr.str(), expectedError);
}

INSTANTIATE_TEST_SUITE_P(
    BadConfigFiles,
    ConfigParserBadFileTests,
    testing::Values(
        BadConfigCase{"MissingName",             "tests/parsingData/badConfigFiles/missingName.txt",            "Failed to find NAME in \"tests/parsingData/badConfigFiles/missingName.txt\"\n"},
        BadConfigCase{"MissingFilepath",         "tests/parsingData/badConfigFiles/missingFilepath.txt",        "Failed to find FILEPATH in \"tests/parsingData/badConfigFiles/missingFilepath.txt\"\n"},
        BadConfigCase{"MissingSpatialdomain",    "tests/parsingData/badConfigFiles/missingSpatialdomain.txt",   "Failed to find SPATIALDOMAIN in \"tests/parsingData/badConfigFiles/missingSpatialdomain.txt\"\n"},
        BadConfigCase{"MissingDimensionorder",   "tests/parsingData/badConfigFiles/missingDimensionorder.txt",  "Failed to find DIMENSIONORDER in \"tests/parsingData/badConfigFiles/missingDimensionorder.txt\"\n"},
        BadConfigCase{"MissingIndexscheme",      "tests/parsingData/badConfigFiles/missingIndexscheme.txt",     "Failed to find INDEXSCHEME in \"tests/parsingData/badConfigFiles/missingIndexscheme.txt\"\n"},
        BadConfigCase{"MissingDatasetstructure", "tests/parsingData/badConfigFiles/missingDatasetstructure.txt","Failed to find DATASETSTRUCTURE in \"tests/parsingData/badConfigFiles/missingDatasetstructure.txt\"\n"},
        BadConfigCase{"EmptyFile",               "tests/parsingData/badConfigFiles/emptyFile.txt",              "Failed to find NAME in \"tests/parsingData/badConfigFiles/emptyFile.txt\"\n"},
        BadConfigCase{"MissingAttribute",        "tests/parsingData/badConfigFiles/missingAttribute.txt",       "Failed to find ATTRIBUTE where expected in \"tests/parsingData/badConfigFiles/missingAttribute.txt\"\n"},
        BadConfigCase{"AttributeMisformatted",  "tests/parsingData/badConfigFiles/attributeMisformatted.txt", "Attribute misformatted in \"tests/parsingData/badConfigFiles/attributeMisformatted.txt\": ATTRIBUTE:\n"},
        BadConfigCase{"BadSpatialdomain",        "tests/parsingData/badConfigFiles/badSpatialdomain.txt",       "Failed to parse SPATIALDOMAIN values from \"tests/parsingData/badConfigFiles/badSpatialdomain.txt\": Invalid Integer\n"},
        BadConfigCase{"TruncatedSpatialdomain",  "tests/parsingData/badConfigFiles/truncatedSpatialdomain.txt", "Spatial domain formatting error in \"tests/parsingData/badConfigFiles/truncatedSpatialdomain.txt\"\n"},
        BadConfigCase{"BadNodata",              "tests/parsingData/badConfigFiles/badNodata.txt",             "Failed to parse NODATA value from \"tests/parsingData/badConfigFiles/badNodata.txt\": Invalid float\n"}
    ),
    [](const testing::TestParamInfo<BadConfigCase>& info) {
        return info.param.name;
    }
);
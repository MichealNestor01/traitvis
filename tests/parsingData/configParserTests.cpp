#include "configParser.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <string>

struct BadConfigCase {
    const char* name;
    const char* filepath;
    const char* expectedError;
};

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
        BadConfigCase{"MissingSpacialdomain",    "tests/parsingData/badConfigFiles/missingSpacialdomain.txt",   "Failed to find SPACIALDOMAIN in \"tests/parsingData/badConfigFiles/missingSpacialdomain.txt\"\n"},
        BadConfigCase{"MissingDimensionorder",   "tests/parsingData/badConfigFiles/missingDimensionorder.txt",  "Failed to find DIMENSIONORDER in \"tests/parsingData/badConfigFiles/missingDimensionorder.txt\"\n"},
        BadConfigCase{"MissingIndexscheme",      "tests/parsingData/badConfigFiles/missingIndexscheme.txt",     "Failed to find INDEXSCHEME in \"tests/parsingData/badConfigFiles/missingIndexscheme.txt\"\n"},
        BadConfigCase{"MissingDatasetstructure", "tests/parsingData/badConfigFiles/missingDatasetstructure.txt","Failed to find DATASETSTRUCTURE in \"tests/parsingData/badConfigFiles/missingDatasetstructure.txt\"\n"},
        BadConfigCase{"EmptyFile",               "tests/parsingData/badConfigFiles/emptyFile.txt",              "Failed to find NAME in \"tests/parsingData/badConfigFiles/emptyFile.txt\"\n"},
        BadConfigCase{"MissingAttribute",        "tests/parsingData/badConfigFiles/missingAttribute.txt",       "Failed to find ATTRIBUTE where expected in \"tests/parsingData/badConfigFiles/missingAttribute.txt\"\n"},
        BadConfigCase{"AttributeMissformatted",  "tests/parsingData/badConfigFiles/attributeMissformatted.txt", "Attribute miss formatted in \"tests/parsingData/badConfigFiles/attributeMissformatted.txt\": ATTRIBUTE:\n"},
        BadConfigCase{"BadSpacialdomain",        "tests/parsingData/badConfigFiles/badSpacialdomain.txt",       "Failed to parse SPACIALDOMAIN values from \"tests/parsingData/badConfigFiles/badSpacialdomain.txt\": Invalid Integer\n"},
        BadConfigCase{"TruncatedSpacialdomain",  "tests/parsingData/badConfigFiles/truncatedSpacialdomain.txt", "Spacial domain formatting error in \"tests/parsingData/badConfigFiles/truncatedSpacialdomain.txt\"\n"}
    ),
    [](const testing::TestParamInfo<BadConfigCase>& info) {
        return info.param.name;
    }
);
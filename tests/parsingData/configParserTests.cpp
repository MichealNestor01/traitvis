#include "configParser.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

TEST(ConfigParserTests, TestBadConfigFiles) {
    std::vector<std::vector<std::string>> badConfigFiles = {
        {"tests/parsingData/badConfigFiles/missingName.txt", "Failed to find NAME in \"tests/parsingData/badConfigFiles/missingName.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingFilepath.txt", "Failed to find FILEPATH in \"tests/parsingData/badConfigFiles/missingFilepath.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingSpacialdomain.txt", "Failed to find SPACIALDOMAIN in \"tests/parsingData/badConfigFiles/missingSpacialdomain.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingDimensionorder.txt", "Failed to find DIMENSIONORDER in \"tests/parsingData/badConfigFiles/missingDimensionorder.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingIndexscheme.txt", "Failed to find INDEXSCHEME in \"tests/parsingData/badConfigFiles/missingIndexscheme.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingDatasetstructure.txt", "Failed to find DATASETSTRUCTURE in \"tests/parsingData/badConfigFiles/missingDatasetstructure.txt\"\n"},
        {"tests/parsingData/badConfigFiles/emptyFile.txt", "Failed to find NAME in \"tests/parsingData/badConfigFiles/emptyFile.txt\"\n"},
        {"tests/parsingData/badConfigFiles/missingAttribute.txt", "Failed to find ATTRIBUTE where expected in \"tests/parsingData/badConfigFiles/missingAttribute.txt\"\n"},
        {"tests/parsingData/badConfigFiles/attributeMissformatted.txt", "Attribute miss formatted in \"tests/parsingData/badConfigFiles/attributeMissformatted.txt\": ATTRIBUTE:\n"},
    };

    for (auto& badFile : badConfigFiles) {
        // Redirect std::cerr so it can be analysed
        std::streambuf* originalCerrBuffer = std::cerr.rdbuf();
        std::ostringstream redirectedCerr;
        std::cerr.rdbuf(redirectedCerr.rdbuf());

        // call the parse config
        DatasetDirConfig result = parseConfig(badFile[0]);

        // Restore std::cerr to its original buffer
        std::cerr.rdbuf(originalCerrBuffer);
        
        // Assertions
        EXPECT_EQ(result.parseError, true); // Expect parse error
        EXPECT_EQ(redirectedCerr.str(), badFile[1]); // Expect correct message
    }
}
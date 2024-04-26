#include "configParser.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

TEST(ConfigParserTests, TestBadConfigFiles) {
    std::vector<std::vector<std::string>> badConfigFiles = {
        {"tests/dataParsing/badConfigFiles/missingName.txt", "Failed to find NAME in \"tests/dataParsing/badConfigFiles/missingName.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingFilepath.txt", "Failed to find FILEPATH in \"tests/dataParsing/badConfigFiles/missingFilepath.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingSpacialdomain.txt", "Failed to find SPACIALDOMAIN in \"tests/dataParsing/badConfigFiles/missingSpacialdomain.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingDimensionorder.txt", "Failed to find DIMENSIONORDER in \"tests/dataParsing/badConfigFiles/missingDimensionorder.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingIndexscheme.txt", "Failed to find INDEXSCHEME in \"tests/dataParsing/badConfigFiles/missingIndexscheme.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingDatasetstructure.txt", "Failed to find DATASETSTRUCTURE in \"tests/dataParsing/badConfigFiles/missingDatasetstructure.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/emptyFile.txt", "Failed to find NAME in \"tests/dataParsing/badConfigFiles/emptyFile.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/missingAttribute.txt", "Failed to find ATTRIBUTE where expected in \"tests/dataParsing/badConfigFiles/missingAttribute.txt\"\n"},
        {"tests/dataParsing/badConfigFiles/attributeMissformatted.txt", "Attribute miss formatted in \"tests/dataParsing/badConfigFiles/attributeMissformatted.txt\": ATTRIBUTE:\n"},
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
#include "poTestApp.h"
#include "poDominatorTest.h"
#include "poSyntaxTests.h"
#include "poSSATests.h"
#include "poLiveTests.h"
#include "poRegLinearTests.h"
#include "poNLFTests.h"
#include "poIntegrationTest.h"
#include "poOptMemoryToRegTests.h"
#include "poOptDCETests.h"
#include "poRegGraphTests.h"
#include "poSCCTests.h"

#include <iostream>
#include <cstring>

using namespace po;

int main(int numArgs, const char** const args)
{
    std::cout << "Running tests..." << std::endl;
    syntaxTest();
    runDominatorTests();
    runSsaTests();
    runLiveTests();
    runRegLinearTests();
    runNestedLoopForestsTests();
    runOptMemoryToRegTests();
    runOptDCETests();
    runRegGraphTests();
    runSSCTests();

    if (numArgs >= 4)
    {
        bool interactive = false;
        std::string optimizationLevel = "/O2";
        for (int i = 4; i < numArgs; i++)
        {
            if (std::strcmp(args[i], "-i") == 0)
            {
                interactive = true;
            }
            else if (std::strcmp(args[i], "-O0") == 0)
            {
                optimizationLevel = "/O0";
            }
            else if (std::strcmp(args[i], "-O1") == 0)
            {
                optimizationLevel = "/O1";
            }
            else if (std::strcmp(args[i], "-O2") == 0)
            {
                optimizationLevel = "/O2";
            }
        }

        runIntegrationTests(args[1], args[2], args[3], interactive, optimizationLevel);
    }

    return 0;
}

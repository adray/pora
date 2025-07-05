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

#include <iostream>

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

    if (numArgs >= 3)
    {
        runIntegrationTests(args[1], args[2]);
    }

    return 0;
}

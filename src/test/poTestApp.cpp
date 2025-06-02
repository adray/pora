#include "poTestApp.h"
#include "poDominatorTest.h"
#include "poSyntaxTests.h"
#include "poSSATests.h"
#include "poLiveTests.h"
#include "poRegLinearTests.h"
#include "poNLFTests.h"

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

    return 0;
}

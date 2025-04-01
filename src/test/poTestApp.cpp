#include "poTestApp.h"
#include "poDominatorTest.h"
#include "poSyntaxTests.h"
#include "poSSATests.h"

#include <iostream>

using namespace po;

int main(int numArgs, const char* const args)
{
    std::cout << "Running tests..." << std::endl;
    syntaxTest();
    runDominatorTests();
    runSsaTests();

    return 0;
}

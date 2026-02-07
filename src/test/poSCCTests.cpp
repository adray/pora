#include "poSCCTests.h"
#include "poSCC.h"
#include <iostream>

using namespace po;

static void SSCTest1()
{
    std::cout << "SSCTest #1 ";

    poSCC scc;
    scc.init(4);
    scc.addEdge(0, 1);
    scc.addEdge(1, 2);
    scc.addEdge(2, 0);
    scc.addEdge(1, 3);
    scc.addEdge(3, 2);
    scc.compute();
    
    if (scc.header(1) == scc.header(0) &&
        scc.header(2) == scc.header(0) &&
        scc.header(3) == scc.header(0))
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void SSCTest2()
{
    std::cout << "SSCTest #2 ";

    poSCC scc;
    scc.init(3);
    scc.addEdge(0, 1);
    scc.addEdge(1, 2);
    scc.addEdge(2, 0);
    scc.compute();

    if (scc.header(1) == scc.header(0) &&
        scc.header(2) == scc.header(0))
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void SSCTest3()
{
    std::cout << "SSCTest #3 ";

    poSCC scc;
    scc.init(5);
    scc.addEdge(0, 1);
    scc.addEdge(1, 2);
    scc.addEdge(2, 0);
    scc.addEdge(3, 4);
    scc.addEdge(4, 3);
    scc.compute();

    if (scc.header(1) == scc.header(0) &&
        scc.header(2) == scc.header(0) &&
        scc.header(3) == scc.header(4))
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runSSCTests()
{
    SSCTest1();
    SSCTest2();
    SSCTest3();
}

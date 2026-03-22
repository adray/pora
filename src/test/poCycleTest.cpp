#include "poCycleTest.h"
#include "poCycle.h"

#include <iostream>

using namespace po;

static void cycleTest1() {
    std::cout << "Cycle Test #1 ";

    poCycle c;
    c.init(3);
    c.addEdge(0, 1);
    c.addEdge(1, 2);
    c.addEdge(2, 0);
    c.compute();

    if (c.hasCycle()) {
        std::cout << "OK" << std::endl;
    }
    else {
        std::cout << "FAILED" << std::endl;
    }
}

static void cycleTest2() {
    std::cout << "Cycle Test #2 ";

    poCycle c;
    c.init(5);
    c.addEdge(0, 1);
    c.addEdge(2, 0);
    c.addEdge(2, 3);
    c.addEdge(3, 4);
    c.compute();

    if (!c.hasCycle()) {
        std::cout << "OK" << std::endl;
    }
    else {
        std::cout << "FAILED" << std::endl;
    }
}

static void cycleTest3() {
    std::cout << "Cycle Test #3 ";

    poCycle c;
    c.init(5);
    c.addEdge(4, 0);
    c.addEdge(0, 1);
    c.addEdge(1, 2);
    c.addEdge(2, 0);
    c.compute();

    if (c.hasCycle()) {
        std::cout << "OK" << std::endl;
    }
    else {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runCycleTests() {
    cycleTest1();
    cycleTest2();
    cycleTest3();
}


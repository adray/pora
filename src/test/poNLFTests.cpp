#include "poNLFTests.h"
#include "poNLF.h"
#include "poDom.h"
#include "poCFG.h"
#include <iostream>

using namespace po;

static void nestedLoopForestTest1()
{
    // Tests a Cfg with 2 basic blocks
    // where the second bb loops back to the first.

    std::cout << "Nested Loop Forest #1 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();

    bb2->setBranch(bb1, false);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::Reducible &&
        nlf.getType(1) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest2()
{
    // Tests a Cfg without any loops

    std::cout << "Nested Loop Forest #2 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok = 
        nlf.getType(0) == poNLFType::NonHeader &&
        nlf.getType(1) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest3()
{
    std::cout << "Nested Loop Forest #3 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->setBranch(bb4, false);
    bb2->setBranch(bb4, false);
    bb3->setBranch(bb2, true);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::NonHeader &&
        nlf.getType(1) == poNLFType::Reducible &&
        nlf.getType(2) == poNLFType::NonHeader &&
        nlf.getType(3) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest4()
{
    std::cout << "Nested Loop Forest #4 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();
    poBasicBlock* bb5 = new poBasicBlock();

    bb1->setBranch(bb5, false);
    bb2->setBranch(bb5, false);
    bb3->setBranch(bb3, false);
    bb4->setBranch(bb2, true);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    cfg.addBasicBlock(bb5);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::NonHeader &&
        nlf.getType(1) == poNLFType::Reducible &&
        nlf.getType(2) == poNLFType::Self &&
        nlf.getType(3) == poNLFType::NonHeader &&
        nlf.getType(4) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest5()
{
    std::cout << "Nested Loop Forest #5 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();
    poBasicBlock* bb5 = new poBasicBlock();
    poBasicBlock* bb6 = new poBasicBlock();

    bb1->setBranch(bb6, false);
    bb2->setBranch(bb6, false);
    bb3->setBranch(bb2, false);
    bb4->setBranch(bb3, false);
    bb5->setBranch(bb2, true);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    cfg.addBasicBlock(bb5);
    cfg.addBasicBlock(bb6);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::NonHeader &&
        nlf.getType(1) == poNLFType::Reducible &&
        nlf.getType(2) == poNLFType::Reducible &&
        nlf.getType(3) == poNLFType::NonHeader &&
        nlf.getType(4) == poNLFType::NonHeader &&
        nlf.getType(5) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest6()
{
    std::cout << "Nested Loop Forest #6 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->setBranch(bb3, false);
    bb4->setBranch(bb2, false);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::NonHeader &&
        nlf.getType(1) == poNLFType::NonReducible &&
        nlf.getType(2) == poNLFType::NonHeader &&
        nlf.getType(3) == poNLFType::NonHeader &&
        nlf.isIrreducible();

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void nestedLoopForestTest7()
{
    std::cout << "Nested Loop Forest #7 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();
    poBasicBlock* bb5 = new poBasicBlock();

    bb1->setBranch(bb3, false);
    bb2->setBranch(bb4, true);
    bb3->setBranch(bb1, true);
    bb4->setBranch(bb2, false);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    cfg.addBasicBlock(bb5);

    poDom dom;
    dom.compute(cfg);

    poNLF nlf;
    nlf.compute(dom);

    const bool ok =
        nlf.getType(0) == poNLFType::Reducible &&
        nlf.getType(1) == poNLFType::Reducible &&
        nlf.getType(2) == poNLFType::NonHeader &&
        nlf.getType(3) == poNLFType::NonHeader &&
        nlf.getType(4) == poNLFType::NonHeader;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runNestedLoopForestsTests()
{
    nestedLoopForestTest1();
    nestedLoopForestTest2();
    nestedLoopForestTest3();
    nestedLoopForestTest4();
    nestedLoopForestTest5();
    nestedLoopForestTest6();
    nestedLoopForestTest7();
}

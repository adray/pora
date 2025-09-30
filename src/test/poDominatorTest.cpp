#include "poDominatorTest.h"
#include "poDom.h"
#include "poCFG.h"
#include <iostream>

using namespace po;

static bool checkDominanceFrontier(const poDomNode& node, const std::vector<int>& expected)
{
    const std::vector<int>& df = node.dominanceFrontier();
    if (df.size() != expected.size())
    {
        return false;
    }

    for (size_t i = 0; i < df.size(); i++)
    {
        if (df[i] != expected[i])
        {
            return false;
        }
    }

    return true;
}

static bool checkImmediateDominators(const poDomNode& node, const std::vector<int>& expected)
{
    const std::vector<int>& dominators = node.immediateDominatedBy();
    if (dominators.size() != expected.size())
    {
        return false;
    }

    for (size_t i = 0; i < dominators.size(); i++)
    {
        if (dominators[i] != expected[i])
        {
            return false;
        }
    }

    return true;
}

static bool checkDominators(const poDomNode& node, const std::vector<int>& expected)
{
    const std::vector<int>& dominators = node.dominators();
    if (dominators.size() != expected.size())
    {
        return false;
    }

    for (size_t i = 0; i < dominators.size(); i++)
    {
        if (dominators[i] != expected[i])
        {
            return false;
        }
    }

    return true;
}

static void dominatorTest1()
{
    std::cout << "Dominator Test #1";

    poBasicBlock* bb_a = new poBasicBlock();
    poBasicBlock* bb_b = new poBasicBlock();
    poBasicBlock* bb_c = new poBasicBlock();
    poBasicBlock* bb_d = new poBasicBlock();

    bb_a->setBranch(bb_c, false);
    bb_b->setBranch(bb_d, true);

    poFlowGraph cfg;
    cfg.addBasicBlock(bb_a);
    cfg.addBasicBlock(bb_b);
    cfg.addBasicBlock(bb_c);
    cfg.addBasicBlock(bb_d);

    std::vector<int> expected_d = { 0, 3 };

    poDom dom;
    dom.compute(cfg);

    bool success = true;
    const auto& node_d = dom.get(3);
    if (node_d.getBasicBlock() == bb_d && !checkDominators(node_d, expected_d))
    {
        success = false;
    }

    if (success)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED" << std::endl;
    }
}

static void dominatorTest2()
{
    std::cout << "Dominator Test #2";

    poBasicBlock* bb_a = new poBasicBlock();
    poBasicBlock* bb_b = new poBasicBlock();
    poBasicBlock* bb_c = new poBasicBlock();
    poBasicBlock* bb_d = new poBasicBlock();

    bb_a->setBranch(bb_c, true);
    bb_b->setBranch(bb_d, false);
    bb_c->setBranch(bb_b, false);

    poFlowGraph cfg;
    cfg.addBasicBlock(bb_a);
    cfg.addBasicBlock(bb_b);
    cfg.addBasicBlock(bb_c);
    cfg.addBasicBlock(bb_d);

    std::vector<int> expected_a = { 0 };
    std::vector<int> expected_b = { 0, 1, 2 };
    std::vector<int> expected_c = { 0, 2 };
    std::vector<int> expected_d = { 0, 2, 3 };

    poDom dom;
    dom.compute(cfg);

    bool success = true;
    const auto& node_b = dom.get(1);
    if (node_b.getBasicBlock() == bb_b && !checkDominators(node_b, expected_b))
    {
        success = false;
    }
    const auto& node_a = dom.get(0);
    if (node_a.getBasicBlock() == bb_a && !checkDominators(node_a, expected_a))
    {
        success = false;
    }
    const auto& node_c = dom.get(2);
    if (node_c.getBasicBlock() == bb_c && !checkDominators(node_c, expected_c))
    {
        success = false;
    }
    const auto& node_d = dom.get(3);
    if (node_d.getBasicBlock() == bb_d && !checkDominators(node_d, expected_d))
    {
        success = false;
    }

    if (success)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED" << std::endl;
    }
}

static void dominatorTest3()
{
    // Test the immediate dominators are correct

    std::cout << "Dominator Test #3";

    poBasicBlock* bb_a = new poBasicBlock();
    poBasicBlock* bb_b = new poBasicBlock();
    poBasicBlock* bb_c = new poBasicBlock();
    poBasicBlock* bb_d = new poBasicBlock();
    poBasicBlock* bb_e = new poBasicBlock();

    bb_b->setBranch(bb_d, false);
    bb_c->setBranch(bb_e, true);

    poFlowGraph cfg;
    cfg.addBasicBlock(bb_a);
    cfg.addBasicBlock(bb_b);
    cfg.addBasicBlock(bb_c);
    cfg.addBasicBlock(bb_d);
    cfg.addBasicBlock(bb_e);

    poDom dom;
    dom.compute(cfg);

    const bool success =
        checkImmediateDominators(dom.get(0), { 1 }) &&
        checkImmediateDominators(dom.get(1), { 2, 3, 4 }) &&
        checkImmediateDominators(dom.get(2), { }) &&
    
        dom.get(1).immediateDominator() == 0 &&
        dom.get(2).immediateDominator() == 1 &&
        dom.get(3).immediateDominator() == 1 &&
        dom.get(4).immediateDominator() == 1;

    if (success)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED " << std::endl;
    }
}

static void dominatorTest4()
{
    // Test the dominance frontier is correct

    std::cout << "Dominator Test #4";

    poBasicBlock* bb_a = new poBasicBlock();
    poBasicBlock* bb_b = new poBasicBlock();
    poBasicBlock* bb_c = new poBasicBlock();
    poBasicBlock* bb_d = new poBasicBlock();

    bb_a->setBranch(bb_c, true);
    bb_b->setBranch(bb_d, false);
    bb_c->setBranch(bb_b, false);

    poFlowGraph cfg;
    cfg.addBasicBlock(bb_a);
    cfg.addBasicBlock(bb_b);
    cfg.addBasicBlock(bb_c);
    cfg.addBasicBlock(bb_d);

    poDom dom;
    dom.compute(cfg);

    const bool success =
        checkDominanceFrontier(dom.get(0), { }) &&
        checkDominanceFrontier(dom.get(1), { 2, 3 }) &&
        checkDominanceFrontier(dom.get(2), { 2 }) &&
        checkDominanceFrontier(dom.get(3), { });

    if (success)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED " << std::endl;
    }
}

static void dominatorTest5()
{
    // Test the iterated dominance frontier is correct
    std::cout << "Dominator Test #5";
    
    poBasicBlock* bb_a = new poBasicBlock();
    poBasicBlock* bb_b = new poBasicBlock();
    poBasicBlock* bb_c = new poBasicBlock();
    poBasicBlock* bb_d = new poBasicBlock();

    bb_a->setBranch(bb_c, true);
    bb_b->setBranch(bb_d, false);
    bb_c->setBranch(bb_b, false);

    poFlowGraph cfg;
    cfg.addBasicBlock(bb_a);
    cfg.addBasicBlock(bb_b);
    cfg.addBasicBlock(bb_c);
    cfg.addBasicBlock(bb_d);

    poDom dom;
    dom.compute(cfg);
    
    std::unordered_set<int> idf;
    dom.iteratedDominanceFrontier({ 1 }, idf);
    
    const bool success =
        idf.size() == 2 &&
        idf.find(2) != idf.end() &&
        idf.find(3) != idf.end();

    if (success)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED " << std::endl;
    }
}

void po::runDominatorTests()
{
    dominatorTest1();
    dominatorTest2();
    dominatorTest3();
    dominatorTest4();
    dominatorTest5();
}

#include "poRegGraphTests.h"
#include "poRegGraph.h"
#include "poModule.h"
#include "poPhiWeb.h"

#include <iostream>

using namespace po;

static void regGraphTest1()
{
    // Test inserting nodes into the interference graph

    std::cout << "Reg Graph Test #1 ";

    poInterferenceGraph graph;
    graph.insert(poInterferenceGraph_Node(1000, false, 0, 5));
    graph.insert(poInterferenceGraph_Node(1001, false, 1, 3));
    graph.insert(poInterferenceGraph_Node(1002, false, 4, 6));
    graph.insert(poInterferenceGraph_Node(1003, false, 5, 7));

    const auto& nodes = graph.nodes();
    const auto& n1 = nodes[0].getNeighbours();
    const auto& n2 = nodes[1].getNeighbours();
    const auto& n3 = nodes[2].getNeighbours();
    const auto& n4 = nodes[3].getNeighbours();
    if (n1.size() == 3 &&
        n1[0] == 1 &&
        n2.size() == 1 &&
        n2[0] == 0 &&
        n3.size() == 2 &&
        n4.size() == 2)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest2()
{
    // Test coloring the interference graph

    std::cout << "Reg Graph Test #2 ";

    poInterferenceGraph graph;
    graph.insert(poInterferenceGraph_Node(1000, false, 0, 5));
    graph.insert(poInterferenceGraph_Node(1001, false, 1, 3));
    graph.insert(poInterferenceGraph_Node(1002, false, 4, 6));
    graph.insert(poInterferenceGraph_Node(1003, false, 5, 7));

    graph.colorGraph(4, 2);

    const auto& nodes = graph.nodes();
    if (nodes[0].color() == 0 &&
        nodes[1].color() == 1 &&
        nodes[2].color() == 1 &&
        nodes[3].color() == 2)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest3()
{
    std::cout << "Reg Graph Test #3 ";

    poModule module;
    poFlowGraph cfg;
    poConstantPool& pool = module.constants();
    pool.addConstant((int64_t)100);
    pool.addConstant((int64_t)200);
    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(1000, TYPE_I64, -1, -1, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, TYPE_I64, -1, -1, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1002, TYPE_I64, 1000, 1001, IR_ADD));
    bb1->addInstruction(poInstruction(1003, TYPE_I64, 1001, 1002, IR_ADD));
    bb1->addInstruction(poInstruction(1004, TYPE_I64, 1003, 1002, IR_ADD));
    cfg.addBasicBlock(bb1);

    poRegGraph graph(module);
    graph.setNumRegisters(6);
    for (int i = 0; i < 6; i++)
    {
        graph.setVolatile(i, false);
        graph.setType(i, poRegType::General);
    }
    graph.allocateRegisters(cfg);

    const int r0 = graph.getRegisterByVariable(1000, 0);
    const int r1 = graph.getRegisterByVariable(1001, 1);
    const int r2 = graph.getRegisterByVariable(1002, 2);
    const int r3 = graph.getRegisterByVariable(1003, 3);
    const int r4 = graph.getRegisterByVariable(1004, 4);
    if (r0 == 0 &&
        r1 == 1 &&
        r2 == 2 &&
        r3 == 0 &&
        r4 == 1)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest4()
{
    // Test spilling the interference graph

    std::cout << "Reg Graph Test #4 ";

    poInterferenceGraph graph;
    graph.insert(poInterferenceGraph_Node(1000, false, 0, 15));
    graph.insert(poInterferenceGraph_Node(1001, false, 1, 15));
    graph.insert(poInterferenceGraph_Node(1002, false, 2, 15));
    graph.insert(poInterferenceGraph_Node(1003, false, 3, 15));
    graph.insert(poInterferenceGraph_Node(1004, false, 4, 15));
    graph.insert(poInterferenceGraph_Node(1005, false, 5, 15));
    graph.insert(poInterferenceGraph_Node(1006, false, 6, 15));
    graph.insert(poInterferenceGraph_Node(1007, false, 7, 15));

    graph.colorGraph(6, 2);

    const auto& nodes = graph.nodes();
    if (nodes[4].color() == 0 &&
        nodes[5].color() == 1 &&
        nodes[6].color() == 2 &&
        nodes[7].color() == 3)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest5()
{
    // Test affinities

    std::cout << "Reg Graph Test #5 ";

    poFlowGraph cfg;
    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    bb->addInstruction(poInstruction(1000, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1001, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1002, TYPE_I64, 1000, 1001, IR_PHI));

    poPhiWeb web;
    web.findPhiWebs(cfg);

    poInterferenceGraph graph;
    graph.insert(poInterferenceGraph_Node(1000, false, 0, 2));
    graph.insert(poInterferenceGraph_Node(1001, false, 1, 2));
    graph.insert(poInterferenceGraph_Node(1002, true, 2, 2));
    graph.calculateAffinity(web);
    graph.colorGraph(8, 2);

    if (graph.nodes()[0].getMerged().size() == 2 &&
        graph.nodes()[0].getNeighbours().size() == 2 &&
        graph.nodes()[0].color() == 0 &&
        graph.nodes()[1].color() == 0 &&
        graph.nodes()[2].color() == 0)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest6()
{
    // Test affinites (loop)

    std::cout << "Reg Graph Test #6 ";

    poFlowGraph cfg;
    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    bb->addInstruction(poInstruction(1000, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1001, TYPE_I64, 1000, 1002, IR_PHI));
    bb->addInstruction(poInstruction(1002, TYPE_I64, 0, IR_CONSTANT));

    poPhiWeb web;
    web.findPhiWebs(cfg);

    poInterferenceGraph graph;
    graph.insert(poInterferenceGraph_Node(1000, false, 0, 2));
    graph.insert(poInterferenceGraph_Node(1001, false, 1, 2));
    graph.insert(poInterferenceGraph_Node(1002, true, 2, 2));
    graph.calculateAffinity(web);
    graph.colorGraph(8, 2);

    if (graph.nodes()[0].getMerged().size() == 2 &&
        graph.nodes()[0].getNeighbours().size() == 2 &&
        graph.nodes()[0].color() == 0 &&
        graph.nodes()[1].color() == 0 &&
        graph.nodes()[2].color() == 0)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regGraphTest7()
{
    std::cout << "Reg Graph Test #7 ";

    poFlowGraph cfg;
    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    bb->addInstruction(poInstruction(1000, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1001, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1002, TYPE_I64, 1000, 1001, IR_ADD));
    bb->addInstruction(poInstruction(1003, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1004, TYPE_I64, 0, IR_CONSTANT));
    bb->addInstruction(poInstruction(1005, TYPE_I64, 1003, 1004, IR_ADD));
    bb->addInstruction(poInstruction(1006, TYPE_I64, 1005, 1002, IR_ADD));
    bb->addInstruction(poInstruction(1007, TYPE_I64, 1006, 1000, IR_ADD));
    bb->addInstruction(poInstruction(1008, TYPE_I64, 1007, 1001, IR_ADD));

    poModule mod;
    poRegGraph graph(mod);
    graph.setNumRegisters(8);
    for (int i = 0; i < 8; i++)
    {
        graph.setType(i, poRegType::General);
    }
    graph.allocateRegisters(cfg);

    // Check spills and restores

    bool ok = true;
    poRegSpill spill;
    ok &= graph.spillAt(0, 0, &spill);
    ok &= spill.spillVariable() == 1000;
    ok &= graph.spillAt(1, 0, &spill);
    ok &= spill.spillVariable() == 1001;

    poRegRestore restore;
    ok &= graph.restoreAt(2, 0, &restore);
    ok &= restore.restoreVariable() == 1000;
    ok &= graph.restoreAt(2, 1, &restore);
    ok &= restore.restoreVariable() == 1001;
    ok &= graph.restoreAt(7, 0, &restore);
    ok &= restore.restoreVariable() == 1000;
    ok &= graph.restoreAt(8, 0, &restore);
    ok &= restore.restoreVariable() == 1001;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}


static void regGraphTest8() {
    /*

    Basic Block 3:
    42 IR_CMP 1 39 4
    43 IR_BR 1 5 --> Basic Block 5
    Basic Block 4:
    44 IR_COPY 1 39
    45 IR_BR 1 0 --> Basic Block 6
    Basic Block 5:
    46 IR_COPY 1 4
    Basic Block 6:
    47 IR_PHI 1 44 46
    49 IR_CONSTANT 1 7
    50 IR_COPY 1 49
    Basic Block 7:
    51 IR_PHI 1 50 52
    53 IR_CMP 1 51 47
    54 IR_BR 1 5 --> Basic Block 10

    */

    std::cout << "Reg Graph Test #8 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();
    poBasicBlock* bb5 = new poBasicBlock();

    bb1->setBranch(bb3, false);
    bb2->setBranch(bb4, true);
    bb3->addIncoming(bb1);
    bb4->addIncoming(bb2);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    cfg.addBasicBlock(bb5);

    bb1->addInstruction(poInstruction(4, 1, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(39, 1, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(42, 1, 39, 4, IR_CMP));
    bb1->addInstruction(poInstruction(43, 1, 5, IR_BR));

    bb2->addInstruction(poInstruction(44, 1, 39, -1, IR_COPY));
    bb2->addInstruction(poInstruction(45, 1, 0, -1, IR_BR));

    bb3->addInstruction(poInstruction(46, 1, 4, -1, IR_COPY));

    bb4->addInstruction(poInstruction(47, 1, 44, 46, IR_PHI));
    bb4->addInstruction(poInstruction(49, 1, 2, IR_CONSTANT));
    bb4->addInstruction(poInstruction(50, 1, 49, -1, IR_COPY));

    bb5->addInstruction(poInstruction(53, 1, 50, 47, IR_CMP));

    poModule mod;
    poRegGraph graph(mod);
    graph.setNumRegisters(8);
    for (int i = 0; i < 8; i++)
    {
        graph.setType(i, poRegType::General);
    }
    graph.allocateRegisters(cfg);

    bool ok =
        graph.getRegisterByVariable(47, 7) !=
        graph.getRegisterByVariable(50, 9);

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runRegGraphTests()
{
    regGraphTest1();
    regGraphTest2();
    regGraphTest3();
    regGraphTest4();
    regGraphTest5();
    regGraphTest6();
    regGraphTest7();
    regGraphTest8();
}


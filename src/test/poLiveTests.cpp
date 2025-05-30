#include "poLiveTests.h"
#include "poLive.h"
#include "poCFG.h"
#include "poDom.h"

#include <iostream>

using namespace po;

static void liveTest1()
{
    std::cout << "Live Test #1 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(1000, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1002, 0, 1000, 1001, IR_ADD));

    cfg.addBasicBlock(bb1);

    poLiveRange live;
    live.compute(cfg);

    if (live.getLiveRange(0) == 2 &&
        live.getLiveRange(1) == 1)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void liveTest2()
{
    std::cout << "Live Test #2 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(1000, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1002, 0, 1000, 1001, IR_ADD));

    poBasicBlock* bb2 = new poBasicBlock();
    bb2->addInstruction(poInstruction(1003, 0, 0, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1004, 0, 1003, 1002, IR_ADD));

    poBasicBlock* bb3 = new poBasicBlock();
    bb3->addInstruction(poInstruction(1005, 0, 0, IR_CONSTANT));
    bb3->addInstruction(poInstruction(1004, 0, 1002, 1005, IR_ADD));

    poBasicBlock* bb4 = new poBasicBlock();
    bb4->addInstruction(poInstruction(1006, 0, 0, IR_CONSTANT));
    bb4->addInstruction(poInstruction(1004, 0, 1004, 1006, IR_ADD));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poLiveRange live;
    live.compute(cfg);

    if (live.getLiveRange(0) == 2 &&
        live.getLiveRange(1) == 1 &&
        live.getLiveRange(2) == 4 &&
        live.getLiveRange(3) == 1 &&
        live.getLiveRange(4) == 4 &&
        live.getLiveRange(5) == 1 &&
        live.getLiveRange(6) == 2 &&
        live.getLiveRange(7) == 1)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void liveTest3()
{
    std::cout << "Live Test #3 ";

    // Construct a CFG with a loop and verify
    // the live range of the variables is as expected.
    //

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->addInstruction(poInstruction(1000, 0, 0, IR_CONSTANT));
    
    bb2->addInstruction(poInstruction(1001, 0, 0, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1002, 0, 1000, 1001, IR_CMP));
    bb2->addInstruction(poInstruction(1003, 0, -1, -1, IR_BR));
    bb2->setBranch(bb4, false);

    bb3->addInstruction(poInstruction(1004, 0, 0, IR_CONSTANT));
    bb3->addInstruction(poInstruction(1000, 0, 1000, 1004, IR_ADD));
    bb3->addInstruction(poInstruction(1005, 0, -1, -1, IR_BR));
    bb2->setBranch(bb2, true);

    bb4->addInstruction(poInstruction(1006, 0, 0, IR_RETURN));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poLiveRange live;
    live.compute(cfg);

    if (live.getLiveRange(0) == 5 &&
        live.getLiveRange(1) == 1 &&
        live.getLiveRange(2) == 0 &&
        live.getLiveRange(3) == 0 &&
        live.getLiveRange(4) == 1 &&
        live.getLiveRange(5) == 0 &&
        live.getLiveRange(6) == 0 &&
        live.getLiveRange(7) == 0)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void liveTest4()
{
    std::cout << "Live Test #4 ";

    // Create a CFG in SSA form

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->addInstruction(poInstruction(1000, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, 0, 1, IR_PARAM));
    bb1->addInstruction(poInstruction(1002, 0, 1000, 1001, IR_CMP));
    bb1->addInstruction(poInstruction(1003, 0, 0, IR_BR));
    bb1->setBranch(bb3, false);

    bb2->addInstruction(poInstruction(1004, 0, 0, IR_CALL));
    bb2->addInstruction(poInstruction(1005, 0, 1001, -1, IR_ARG));
    bb2->setBranch(bb4, true);

    bb3->addInstruction(poInstruction(1006, 0, 0, IR_CALL));
    bb3->addInstruction(poInstruction(1007, 0, 1001, -1, IR_ARG));

    bb4->addInstruction(poInstruction(1008, 0, 1004, 1006, IR_PHI));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poPhi phi(1008, 0);
    phi.addValue(1004, bb2);
    phi.addValue(1006, bb3);
    bb4->addPhi(phi);

    poDom dom;
    dom.compute(cfg);

    poLive live;
    live.compute(cfg, dom);

    const std::vector<poLiveNode>& liveNodes = live.nodes();
    const poLiveNode& l1 = liveNodes.at(0);
    bool ok = l1.liveOut().contains(1001) &&
        !l1.liveOut().contains(1000) &&
        l1.liveIn().size() == 0;
    const poLiveNode& l2 = liveNodes.at(1);
    ok &= l2.liveIn().size() == 1 &&
        l2.liveIn().contains(1001) &&
        l2.liveOut().size() == 1 &&
        l2.liveOut().contains(1004);
    const poLiveNode& l3 = liveNodes.at(2);
    ok &= l3.liveIn().size() == 1 &&
        l3.liveIn().contains(1001);
    const poLiveNode& l4 = liveNodes.at(3);
    ok &= l4.liveOut().size() == 0 &&
        l4.liveIn().size() == 1 &&
        l4.liveIn().contains(1008);

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void liveTest5()
{
    std::cout << "Live Test #5 ";

    // Create a CFG in SSA form
    //
    //             bb1    
    //            /   \     
    //          bb2   bb5   
    //        /    \   |   
    //       bb3  bb4  |   
    //         \   |  /    
    //           bb6       

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();
    poBasicBlock* bb5 = new poBasicBlock();
    poBasicBlock* bb6 = new poBasicBlock();

    bb1->addInstruction(poInstruction(1000, 0, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, 0, 1, IR_PARAM));
    bb1->addInstruction(poInstruction(1002, 0, 1000, 1001, IR_CMP));
    bb1->addInstruction(poInstruction(1003, 0, 0, IR_BR));
    bb1->setBranch(bb5, false);

    bb2->addInstruction(poInstruction(1004, 0, 0, IR_CALL));
    bb2->addInstruction(poInstruction(1005, 0, 1001, -1, IR_ARG));
    bb2->setBranch(bb4, false);

    bb3->setBranch(bb6, true);
    bb4->setBranch(bb6, true);
    
    bb5->addInstruction(poInstruction(1006, 0, 0, IR_CALL));
    bb5->addInstruction(poInstruction(1007, 0, 1001, -1, IR_ARG));

    bb6->addInstruction(poInstruction(1008, 0, 1004, 1006, IR_PHI));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    cfg.addBasicBlock(bb5);
    cfg.addBasicBlock(bb6);

    poPhi phi(1008, 0);
    phi.addValue(1004, bb2);
    phi.addValue(1006, bb5);
    bb6->addPhi(phi);

    poDom dom;
    dom.compute(cfg);

    poLive live;
    live.compute(cfg, dom);

    const std::vector<poLiveNode>& liveNodes = live.nodes();
    const poLiveNode& l1 = liveNodes.at(0);
    bool ok = l1.liveOut().contains(1001) &&
        !l1.liveOut().contains(1000) &&
        l1.liveIn().size() == 0;
    const poLiveNode& l2 = liveNodes.at(1);
    ok &= l2.liveIn().size() == 1 &&
        l2.liveIn().contains(1001) &&
        l2.liveOut().size() == 1 &&
        l2.liveOut().contains(1004);
    const poLiveNode& l5 = liveNodes.at(4);
    ok &= l5.liveIn().size() == 1 &&
        l5.liveIn().contains(1001);
    const poLiveNode& l6 = liveNodes.at(5);
    ok &= l6.liveOut().size() == 0 &&
        l6.liveIn().size() == 1 &&
        l6.liveIn().contains(1008);

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runLiveTests()
{
    //liveTest1();
    //liveTest2();
    //liveTest3();
    liveTest4();
    liveTest5();
}

#include "poLiveTests.h"
#include "poLive.h"
#include "poCFG.h"

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

void po::runLiveTests()
{
    liveTest1();
    liveTest2();
    liveTest3();
}

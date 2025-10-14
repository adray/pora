#include "poOptDCETests.h"
#include "poOptDCE.h"
#include "poModule.h"

#include <iostream>

using namespace po;

static void runDCETest1()
{
    std::cout << "DCE Test #1 ";

    poModule module;
    poNamespace ns("Example");

    poFunction func("testFunc", "Example::testFunc", 0, poAttributes::PUBLIC, poCallConvention::X86_64);
    poFlowGraph& cfg = func.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(0, TYPE_I64, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1, TYPE_I64, 2, IR_CONSTANT));
    bb1->addInstruction(poInstruction(2, TYPE_I64, 1, 0, IR_ADD));
    bb1->addInstruction(poInstruction(3, TYPE_VOID, -1, -1, IR_RETURN));

    cfg.addBasicBlock(bb1);

    ns.addFunction(0);
    module.addFunction(func);
    module.addNamespace(ns);

    poOptDCE dce;
    dce.optimize(module);

    if (bb1->numInstructions() == 1)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void runDCETest2()
{
    std::cout << "DCE Test #2 ";

    poModule module;
    poNamespace ns("Example");

    poFunction func("testFunc", "Example::testFunc", 0, poAttributes::PUBLIC, poCallConvention::X86_64);
    poFlowGraph& cfg = func.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(0, TYPE_I64, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1, TYPE_I64, 2, IR_CONSTANT));
    bb1->addInstruction(poInstruction(2, TYPE_I64, 1, 0, IR_ADD));
    bb1->addInstruction(poInstruction(3, TYPE_I64, 2, -1, IR_RETURN));

    cfg.addBasicBlock(bb1);

    ns.addFunction(0);
    module.addFunction(func);
    module.addNamespace(ns);

    poOptDCE dce;
    dce.optimize(module);

    if (bb1->numInstructions() == 4)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void runDCETest3()
{
    std::cout << "DCE Test #3 ";

    poModule module;
    poNamespace ns("Example");

    poFunction func("testFunc", "Example::testFunc", 0, poAttributes::PUBLIC, poCallConvention::X86_64);
    poFlowGraph& cfg = func.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, TYPE_I64, 0, -1, IR_PARAM));
    bb1->addInstruction(poInstruction(1, TYPE_I64, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(2, TYPE_I64, 0, 1, IR_CMP));
    bb1->addInstruction(poInstruction(3, TYPE_I64, IR_JUMP_EQUALS, -1, IR_BR));
    bb1->setBranch(bb3, false);
    bb3->addIncoming(bb1);

    bb2->addInstruction(poInstruction(4, TYPE_I64, 0, 1, IR_ADD));

    bb3->addInstruction(poInstruction(5, TYPE_I64, 4, 0, IR_PHI));
    bb3->addInstruction(poInstruction(6, TYPE_I64, 5, -1, IR_RETURN));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);

    poPhi phi(5, TYPE_I64);
    phi.addValue(4, bb2);
    phi.addValue(0, bb1);
    bb3->addPhi(phi);

    ns.addFunction(0);
    module.addFunction(func);
    module.addNamespace(ns);

    poOptDCE dce;
    dce.optimize(module);

    if (bb1->numInstructions() == 4 &&
        bb2->numInstructions() == 1 &&
        bb3->numInstructions() == 2)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void runDCETest4()
{
    std::cout << "DCE Test #4 ";

    poModule module;
    poNamespace ns("Example");

    poFunction func("testFunc", "Example::testFunc", 0, poAttributes::PUBLIC, poCallConvention::X86_64);
    poFlowGraph& cfg = func.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, TYPE_I64, 0, -1, IR_PARAM));
    bb1->addInstruction(poInstruction(1, TYPE_I64, 1, IR_CONSTANT));
    bb1->addInstruction(poInstruction(2, TYPE_I64, 0, 1, IR_CMP));
    bb1->addInstruction(poInstruction(3, TYPE_I64, IR_JUMP_EQUALS, -1, IR_BR));
    bb1->setBranch(bb3, false);
    bb3->addIncoming(bb1);

    bb2->addInstruction(poInstruction(4, TYPE_I64, 0, 1, IR_ADD));
    bb2->addInstruction(poInstruction(5, TYPE_I64, 0, -1, IR_CONSTANT));

    bb3->addInstruction(poInstruction(6, TYPE_I64, 4, 0, IR_PHI));
    bb3->addInstruction(poInstruction(7, TYPE_I64, 6, -1, IR_RETURN));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);

    poPhi phi(6, TYPE_I64);
    phi.addValue(4, bb2);
    phi.addValue(0, bb1);
    bb3->addPhi(phi);

    ns.addFunction(0);
    module.addFunction(func);
    module.addNamespace(ns);

    poOptDCE dce;
    dce.optimize(module);

    if (bb1->numInstructions() == 4 &&
        bb2->numInstructions() == 1 &&
        bb3->numInstructions() == 2)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void runDCETest5()
{
    std::cout << "DCE Test #5 ";

    poModule module;
    poNamespace ns("Example");

    poFunction func("testFunc", "Example::testFunc", 0, poAttributes::PUBLIC, poCallConvention::X86_64);
    poFlowGraph& cfg = func.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, TYPE_I64, 0, -1, IR_PARAM));
    bb1->addInstruction(poInstruction(1, TYPE_I64, 1, IR_CONSTANT));

    bb2->addInstruction(poInstruction(2, TYPE_I64, 6, 0, IR_PHI));
    bb2->addInstruction(poInstruction(3, TYPE_I64, 2, 1, IR_CMP));
    bb2->addInstruction(poInstruction(4, TYPE_I64, IR_JUMP_EQUALS, -1, IR_BR));

    bb2->setBranch(bb4, false);
    bb4->addIncoming(bb2);

    bb3->addInstruction(poInstruction(5, TYPE_I64, 0, -1, IR_CONSTANT));
    bb3->addInstruction(poInstruction(6, TYPE_I64, 5, 2, IR_ADD));
    bb3->addInstruction(poInstruction(7, TYPE_I64, IR_JUMP_UNCONDITIONAL, -1, IR_BR));

    bb3->setBranch(bb2, true);
    bb2->addIncoming(bb3);

    bb4->addInstruction(poInstruction(8, TYPE_I64, 2, -1, IR_RETURN));

    poPhi phi(2, TYPE_I64);
    phi.addValue(6, bb3);
    phi.addValue(0, bb1);
    bb2->addPhi(phi);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    ns.addFunction(0);
    module.addFunction(func);
    module.addNamespace(ns);

    poOptDCE dce;
    dce.optimize(module);

    if (bb1->numInstructions() == 2 &&
        bb2->numInstructions() == 3 &&
        bb3->numInstructions() == 3 &&
        bb4->numInstructions() == 1)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runOptDCETests()
{
    runDCETest1();
    runDCETest2();
    runDCETest3();
    runDCETest4();
    runDCETest5();
}


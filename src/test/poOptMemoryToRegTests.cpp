#include "poOptMemoryToRegTests.h"
#include "poOptMemToReg.h"
#include "poModule.h"

#include <iostream>

using namespace po;

static void memToRegTest1()
{
    // Tests:
    // 0 IR_ALLOCA I64*
    // 1 IR_PTR I64* 0
    // 2 IR_CONSTANT I64 100
    // 3 IR_STORE I64 1 2
    // 4 IR_RETURN
    //
    // Is changed to:
    // 0 IR_CONSTANT I64 0
    // 1 IR_CONSTANT I64 100
    // 2 IR_COPY I64 1
    // 3 IR_RETURN

    std::cout << "MemToReg #1 ";

    poModule mod;
    const int intPtrType = int(mod.types().size());
    poType type(intPtrType, TYPE_I64, "I64*");
    type.setPointer(true);
    mod.addType(type);
    const int constant = mod.constants().addConstant(100);

    poFlowGraph fg;

    poBasicBlock* bb1 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA));
    bb1->addInstruction(poInstruction(1, intPtrType, 0, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(2, TYPE_I64, constant, IR_CONSTANT));
    bb1->addInstruction(poInstruction(3, TYPE_I64, 1, 2, IR_STORE));
    bb1->addInstruction(poInstruction(4, TYPE_VOID, -1, -1, IR_RETURN));

    fg.addBasicBlock(bb1);

    poOptMemToReg reg;
    reg.optimize(mod, fg);

    bool ok = true;
    auto& ins0 = bb1->getInstruction(0);
    ok &= ins0.code() == IR_CONSTANT &&
        ins0.left() == -1 &&
        ins0.right() == -1;
    auto& ins1 = bb1->getInstruction(1);
    ok &= ins1.code() == IR_CONSTANT;
    auto& ins2 = bb1->getInstruction(2);
    ok &= ins2.code() == IR_COPY;
    auto& ins3 = bb1->getInstruction(3);
    ok &= ins3.code() == IR_RETURN;
    ok &= int(bb1->numInstructions()) == 4;

    ok &= ins2.left() == ins1.name();

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void memToRegTest2()
{
    std::cout << "MemToReg #2 ";

    poModule mod;
    const int intPtrType = int(mod.types().size());
    poType type(intPtrType, TYPE_I64, "I64*");
    type.setPointer(true);
    mod.addType(type);
    const int constant = mod.constants().addConstant(100);
    const int constant2 = mod.constants().addConstant(10);

    poFlowGraph fg;

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA));
    bb1->addInstruction(poInstruction(1, intPtrType, 0, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(2, TYPE_I64, constant, IR_CONSTANT));
    bb1->addInstruction(poInstruction(3, TYPE_I64, 1, 2, IR_STORE));

    bb1->addInstruction(poInstruction(4, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA));
    bb1->addInstruction(poInstruction(5, intPtrType, 4, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(6, TYPE_I64, constant, IR_CONSTANT));
    bb1->addInstruction(poInstruction(7, TYPE_I64, 5, 6, IR_STORE));

    bb1->addInstruction(poInstruction(8, TYPE_I64, 1, -1, IR_LOAD));
    bb1->addInstruction(poInstruction(9, TYPE_I64, 5, -1, IR_LOAD));
    bb1->addInstruction(poInstruction(10, TYPE_VOID, 8, 9, IR_CMP));
    bb1->addInstruction(poInstruction(11, TYPE_VOID, 8, 9, IR_BR));
    bb1->setBranch(bb3, false);
    bb3->addIncoming(bb1);

    bb2->addInstruction(poInstruction(12, TYPE_I64, 1, -1, IR_LOAD));
    bb2->addInstruction(poInstruction(13, TYPE_I64, 5, -1, IR_LOAD));
    bb2->addInstruction(poInstruction(14, TYPE_I64, 12, 13, IR_ADD));
    bb2->addInstruction(poInstruction(15, TYPE_I64, 1, 14, IR_STORE));

    bb3->addInstruction(poInstruction(4, TYPE_VOID, -1, -1, IR_RETURN));

    fg.addBasicBlock(bb1);
    fg.addBasicBlock(bb2);
    fg.addBasicBlock(bb3);

    poOptMemToReg reg;
    reg.optimize(mod, fg);

    bool ok = true;
    ok &= bb1->numInstructions() == 10 &&
        bb2->numInstructions() == 4 &&
        bb3->numInstructions() == 1;

    auto& ins0 = bb1->getInstruction(0);
    ok &= ins0.code() == IR_CONSTANT &&
        ins0.left() == -1 &&
        ins0.right() == -1;
    auto& ins1 = bb1->getInstruction(1);
    ok &= ins1.code() == IR_CONSTANT;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void memToRegTest3()
{
    bool ok = true;
    std::cout << "MemToReg #3 ";

    // Test with a loop and phi nodes
    poModule mod;
    const int intPtrType = int(mod.types().size());
    poType type(intPtrType, TYPE_I64, "I64*");
    type.setPointer(true);
    mod.addType(type);
    const int constant = mod.constants().addConstant(1);
    const int constant2 = mod.constants().addConstant(10);
    const int constant3 = mod.constants().addConstant(0);

    poFlowGraph fg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->addInstruction(poInstruction(0, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA)); // loop stride
    bb1->addInstruction(poInstruction(1, intPtrType, 0, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(2, TYPE_I64, constant, IR_CONSTANT));
    bb1->addInstruction(poInstruction(3, TYPE_I64, 1, 2, IR_STORE));

    bb1->addInstruction(poInstruction(4, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA)); // loop counter
    bb1->addInstruction(poInstruction(5, intPtrType, 4, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(6, TYPE_I64, constant2, IR_CONSTANT));
    bb1->addInstruction(poInstruction(7, TYPE_I64, 5, 6, IR_STORE));

    bb1->addInstruction(poInstruction(8, intPtrType, 1 /*num elements*/, -1, IR_ALLOCA)); // loop sum
    bb1->addInstruction(poInstruction(9, intPtrType, 8, -1, 0, IR_PTR));
    bb1->addInstruction(poInstruction(10, TYPE_I64, constant3, IR_CONSTANT));
    bb1->addInstruction(poInstruction(11, TYPE_I64, 9, 10, IR_STORE));

    bb2->addInstruction(poInstruction(12, TYPE_I64, 9, -1, IR_LOAD));
    bb2->addInstruction(poInstruction(13, TYPE_I64, 5, -1, IR_LOAD));
    bb2->addInstruction(poInstruction(14, TYPE_I64, 12, 13, IR_CMP));
    bb2->addInstruction(poInstruction(15, TYPE_VOID, -1, -1, IR_BR));
    bb2->setBranch(bb4, false);

    bb3->addInstruction(poInstruction(16, TYPE_I64, 9, -1, IR_LOAD));
    bb3->addInstruction(poInstruction(17, TYPE_I64, 1, -1, IR_LOAD));
    bb3->addInstruction(poInstruction(18, TYPE_I64, 16, 17, IR_ADD));
    bb3->addInstruction(poInstruction(19, TYPE_I64, 9, 18, IR_STORE));
    bb3->addInstruction(poInstruction(20, TYPE_VOID, -1, -1, IR_BR));
    bb3->setBranch(bb2, true);

    bb2->addIncoming(bb3);
    bb4->addIncoming(bb2);

    bb4->addInstruction(poInstruction(21, TYPE_VOID, -1, -1, IR_RETURN));

    fg.addBasicBlock(bb1);
    fg.addBasicBlock(bb2);
    fg.addBasicBlock(bb3);
    fg.addBasicBlock(bb4);

    poOptMemToReg reg;
    reg.optimize(mod, fg);

    // Check the basic blocks and instructions
    ok &= bb1->numInstructions() == 9 &&
        bb2->numInstructions() == 5 && /* Phi node inserted */
        bb3->numInstructions() == 5 &&
        bb4->numInstructions() == 1;

    if (ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::runOptMemoryToRegTests()
{
    memToRegTest1();
    memToRegTest2();
    memToRegTest3();
}


#include "poRegLinearTests.h"
#include "poCFG.h"
#include "poRegLinear.h"
#include "poType.h"
#include "poModule.h"

#include <iostream>

using namespace po;

static void regLinearTest1()
{
    std::cout << "Reg Linear Test #1";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    bb1->addInstruction(poInstruction(1000, TYPE_I64, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, TYPE_I64, 0, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1002, TYPE_I64, 1000, 1001, IR_ADD));

    poBasicBlock* bb2 = new poBasicBlock();
    bb2->addInstruction(poInstruction(1003, TYPE_I64, 0, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1004, TYPE_I64, 1003, 1002, IR_ADD));

    poBasicBlock* bb3 = new poBasicBlock();
    bb3->addInstruction(poInstruction(1005, TYPE_I64, 0, IR_CONSTANT));
    bb3->addInstruction(poInstruction(1004, TYPE_I64, 1002, 1005, IR_ADD));

    poBasicBlock* bb4 = new poBasicBlock();
    bb4->addInstruction(poInstruction(1006, TYPE_I64, 0, IR_CONSTANT));
    bb4->addInstruction(poInstruction(1004, TYPE_I64, 1004, 1006, IR_ADD));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poModule mod;
    poRegLinear linear(mod);
    linear.setNumRegisters(16);
    linear.allocateRegisters(cfg);

    const int i4 = linear.getRegister(4);
    const int i6 = linear.getRegister(6);
    const int i8 = linear.getRegister(8);

    // Check the register allocated to instructions 1004
    // are all the same register and no other instruction
    // which overlaps with its live range is allocated the
    // same register.

    bool ok = true;
    for (int i = 0; i < 9; i++)
    {
        if (i != 4 &&
            i != 6 &&
            i != 8 && 
            i > 4)
        {
            if (linear.getRegister(i) == i4)
            {
                ok = false;
                break;
            }
        }
    }

    if (i4 == i6 &&
        i4 == i8 &&
        ok)
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED" << std::endl;
    }
}

static void regLinearTest2()
{
    std::cout << "Reg Linear Test #2 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->addInstruction(poInstruction(1000, TYPE_I64, 0, IR_CONSTANT));

    bb2->addInstruction(poInstruction(1001, TYPE_I64, 0, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1002, TYPE_I64, 1000, 1001, IR_CMP));
    bb2->addInstruction(poInstruction(1003, 0, -1, -1, IR_BR));
    bb2->setBranch(bb4, false);

    bb3->addInstruction(poInstruction(1004, TYPE_I64, 0, IR_CONSTANT));
    bb3->addInstruction(poInstruction(1000, TYPE_I64, 1000, 1004, IR_ADD));
    bb3->addInstruction(poInstruction(1005, 0, -1, -1, IR_BR));
    bb3->setBranch(bb2, true);

    bb4->addInstruction(poInstruction(1006, 0, 0, IR_RETURN));

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    poModule mod;
    poRegLinear linear(mod);
    linear.setNumRegisters(16);
    linear.allocateRegisters(cfg);

    const int i0 = linear.getRegister(0);
    const int i5 = linear.getRegister(5);
 
    bool ok = true;
    for (int i = 0; i < 8; i++)
    {
        // Check the register for an instruction doesn't interfere with
        // the register used for '1004'. Ignore those which are '1004'
        // or those which don't need a register e.g. BR, CMP, RETURN.
        if (i != 0 &&
            i != 2 &&
            i != 5 &&
            i != 3 &&
            i != 6 &&
            i != 7)
        {
            if (linear.getRegister(i) == i0)
            {
                ok = false;
                break;
            }
        }
    }

    if (i0 == i5 &&
        ok)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

static void regLinearTest3()
{
    std::cout << "Reg Linear Test #3 ";

    poFlowGraph cfg;
    poBasicBlock* bb1 = new poBasicBlock();

    bb1->addInstruction(poInstruction(1000, TYPE_I64, 100, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1001, TYPE_I64, 200, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1002, TYPE_I64, 300, IR_CONSTANT));
    bb1->addInstruction(poInstruction(1003, TYPE_I64, 1002, 1001, IR_ADD));
    bb1->addInstruction(poInstruction(1004, TYPE_I64, 1003, 1000, IR_ADD));

    cfg.addBasicBlock(bb1);

    poModule mod;
    poRegLinear linear(mod);
    linear.setNumRegisters(2);
    linear.allocateRegisters(cfg);

    // TODO: check that the register has been spilled

    std::cout << "OK" << std::endl;
    //std::cout << "FAILED" << std::endl;
}

void po::runRegLinearTests()
{
    regLinearTest1();
    regLinearTest2();
    regLinearTest3();
}


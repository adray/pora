#include "poSSATests.h"
#include "poSSA.h"
#include "poModule.h"
#include "poCFG.h"
#include "poTypeChecker.h"

#include <iostream>
#include <unordered_set>

using namespace po;

static bool checkPhi(const poFlowGraph& cfg, const poInstruction& phi, const poInstruction& expectedLeft, const poInstruction& expectedRight)
{
    if (phi.code() != IR_PHI)
    {
        return false;
    }

    if (phi.left() != expectedLeft.name())
    {
        return false;
    }

    if (phi.right() != expectedRight.name())
    {
        return false;
    }

    return true;
}

static bool checkSSA(const poFlowGraph& cfg)
{
    std::unordered_set<int> set;
    for (int i = 0; i < cfg.numBlocks(); i++)
    {
        poBasicBlock* bb = cfg.getBasicBlock(i);
        for (size_t i = 0; i < bb->numInstructions(); i++)
        {
            auto& ins = bb->getInstruction(int(i));
            if (set.contains(ins.name()))
            {
                return false;
            }
            set.insert(ins.name());
        }
    }

    return true;
}

static void ssaTest1()
{
    std::cout << "SSA Test #1";

    poModule module;
    poNamespace ns("Test");
    poFunction function("MyFunc", 0, poAttributes::PUBLIC);
    function.addVariable(0);
    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->setBranch(bb3, false);
    bb2->setBranch(bb4, true);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);
    
    bb1->addInstruction(poInstruction(0, TYPE_I64, 10, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1, TYPE_I64, 5, IR_CONSTANT));
    bb2->addInstruction(poInstruction(0, TYPE_I64, 0, 1, IR_ADD));
    bb3->addInstruction(poInstruction(2, TYPE_I64, 7, IR_CONSTANT));
    bb3->addInstruction(poInstruction(0, TYPE_I64, 0, 2, IR_ADD));
    bb4->addInstruction(poInstruction(3, TYPE_I64, 2, IR_CONSTANT));
    bb4->addInstruction(poInstruction(0, TYPE_I64, 0, 3, IR_ADD));

    ns.addFunction(function);
    module.addNamespace(ns);

    poSSA ssa;
    ssa.construct(module);

    if (checkSSA(cfg) &&
        checkPhi(cfg, bb4->getInstruction(0), bb2->getInstruction(1), bb3->getInstruction(1)))
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED" << std::endl;
    }
}

static void ssaTest2()
{
    std::cout << "SSA Test #2";

    poModule module;
    poNamespace ns("Test");
    poFunction function("MyFunc", 0, poAttributes::PUBLIC);
    function.addVariable(0);
    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb1 = new poBasicBlock();
    poBasicBlock* bb2 = new poBasicBlock();
    poBasicBlock* bb3 = new poBasicBlock();
    poBasicBlock* bb4 = new poBasicBlock();

    bb1->setBranch(bb4, false);
    bb2->setBranch(bb4, false);

    cfg.addBasicBlock(bb1);
    cfg.addBasicBlock(bb2);
    cfg.addBasicBlock(bb3);
    cfg.addBasicBlock(bb4);

    bb1->addInstruction(poInstruction(0, TYPE_I64, 10, IR_CONSTANT));
    bb2->addInstruction(poInstruction(1, TYPE_I64, 5, IR_CONSTANT));
    bb2->addInstruction(poInstruction(0, TYPE_I64, 0, 1, IR_ADD));
    bb3->addInstruction(poInstruction(2, TYPE_I64, 7, IR_CONSTANT));
    bb3->addInstruction(poInstruction(0, TYPE_I64, 0, 2, IR_ADD));
    bb4->addInstruction(poInstruction(3, TYPE_I64, 2, IR_CONSTANT));
    bb4->addInstruction(poInstruction(0, TYPE_I64, 0, 3, IR_ADD));

    ns.addFunction(function);
    module.addNamespace(ns);

    poSSA ssa;
    ssa.construct(module);

    if (checkSSA(cfg) &&
        checkPhi(cfg, bb4->getInstruction(0), bb1->getInstruction(0), bb2->getInstruction(1)) &&
        checkPhi(cfg, bb4->getInstruction(1), bb4->getInstruction(0), bb3->getInstruction(1)))
    {
        std::cout << " OK" << std::endl;
    }
    else
    {
        std::cout << " FAILED" << std::endl;
    }
}

void po::runSsaTests()
{
    ssaTest1();
    ssaTest2();
}

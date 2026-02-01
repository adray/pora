#include "poOptCopy.h"
#include "poModule.h"
#include "poUses.h"

using namespace po;

void poOptCopy::optimize(poModule& module)
{
    for (size_t j = 0; j < module.functions().size(); j++)
    {
        poFunction& func = module.functions()[j];
        poFlowGraph& cfg = func.cfg();
        optimize(module, cfg);
    }
}

void poOptCopy::copyPropagation(poInstruction& instr, poUses& uses, const int name, const int source) 
{
    const std::vector<poInstructionRef>& refs = uses.getUses(name);

    for (size_t i = 0; i < refs.size(); ++i)
    {
        const poInstructionRef& ref = refs[i];
        poBasicBlock* bb = ref.getBasicBlock();
        poInstruction& instr = bb->getInstruction(ref.getAdjustedRef());
        if (instr.isSpecialInstruction() ||
                instr.code() == IR_PHI)
        {
            return;
        }
    }

    for (size_t i = 0; i < refs.size(); ++i)
    {
        const poInstructionRef& ref = refs[i];
        poBasicBlock* bb = ref.getBasicBlock();
        poInstruction& instr = bb->getInstruction(ref.getAdjustedRef());

        if (instr.left() == name)
        {
            instr.setLeft(source);
        }
        if (instr.right() == name)
        {
            instr.setRight(source);
        }

        /*if (instr.code() == IR_PHI)
        {
            for (poPhi phi : bb->phis())
            {
                if (phi.name() != instr.name())
                {
                    continue;
                }

                for (size_t k = 0; k < phi.values().size(); ++k)
                {
                    if (phi.values()[k] == name)
                    {
                        phi.setValue(int(k), source);
                    }
                }
            }
        }*/
    }

    instr.setName(-1); // Mark instruction as removed
}

void poOptCopy::optimize(poModule& module, poFlowGraph& cfg)
{
    // Find instances of copy instructions

    poUses uses;
    uses.analyze(cfg);

    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (size_t i = 0; i < bb->numInstructions(); ++i)
        {
             poInstruction& instr = bb->getInstruction(int(i));
             if (instr.code() == IR_COPY)
             {
                 // Perform copy propagation

                 const int name = instr.name();
                 const int source = instr.left();

                 if (uses.hasUses(name))
                 {
                     copyPropagation(instr, uses, name, source);
                 }
             }
        }

        bb = bb->getNext();
    }

    // Remove marked instructions
    bb = cfg.getFirst();
    while (bb)
    {
        int pos = 0;
        while (pos < int(bb->instructions().size()))
        {
            auto& ins = bb->getInstruction(pos);
            if (ins.name() == -1)
            {
                bb->removeInstruction(pos);
            }
            else
            {
                pos++;
            }
        }

        bb = bb->getNext();
    }
}

#include "poUses.h"
#include "poCFG.h"

#include <algorithm>

using namespace po;

void poUses::addUse(const int variable, const int pos, const int basePos, poBasicBlock* bb)
{
    const auto& it = _uses.find(variable);
    if (it != _uses.end())
    {
        auto& myUses = it->second;
        auto lower = std::lower_bound(myUses.begin(), myUses.end(), pos, [](const poInstructionRef& ref, const int pos) -> bool
            {
                return ref.getRef() < pos;
            });
        it->second.insert(lower, poInstructionRef(bb, pos, basePos));
    }
    else
    {
        std::vector<poInstructionRef> uses;
        uses.push_back(poInstructionRef(bb, pos, basePos));
        _uses.insert(std::pair<int, std::vector<poInstructionRef>>(variable, uses));
    }
}

void poUses::analyze(poFlowGraph& cfg)
{
    _uses.clear();

    int pos = 0;
    int basePos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        auto& instructions = bb->instructions();
        for (auto& ins : instructions)
        {
            if (ins.isSpecialInstruction())
            {
                // Special instructions do not have uses
                pos++;
                continue;
            }

            if (ins.left() != -1)
            {
                addUse(ins.left(), pos, basePos, bb);
            }
            if (ins.right() != -1)
            {
                addUse(ins.right(), pos, basePos, bb);
            }

            pos++;
        }

        bb = bb->getNext();
        basePos = pos;
    }
}

const int poUses::findNextUse(const int variable, const int pos) const
{
    const auto& it = _uses.find(variable);

    if (it != _uses.end())
    {
        const auto& lowerBound = std::lower_bound(it->second.begin(), it->second.end(), pos, [](const poInstructionRef& ref, const int pos) -> bool
            {
                return ref.getRef() < pos;
            });
        if (lowerBound != it->second.end())
        {
            const int value = lowerBound->getRef();
            return value;
        }
    }

    return -1;
}

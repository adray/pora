#include "poUses.h"
#include "poCFG.h"

#include <algorithm>

using namespace po;

void poUses::addUse(const int variable, const int pos)
{
    const auto& it = _uses.find(variable);
    if (it != _uses.end())
    {
        auto& myUses = it->second;
        const auto& lower = std::lower_bound(myUses.begin(), myUses.end(), pos);
        it->second.insert(lower, pos);
    }
    else
    {
        std::vector<int> uses;
        uses.push_back(pos);
        _uses.insert(std::pair<int, std::vector<int>>(variable, uses));
    }
}

void poUses::analyze(poFlowGraph& cfg)
{
    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        auto& instructions = bb->instructions();
        for (auto& ins : instructions)
        {
            switch (ins.code())
            {
            case IR_BR:
            case IR_PARAM:
            case IR_CONSTANT:
            case IR_CALL:
                break;
            default:
                if (ins.left() != -1)
                {
                    addUse(ins.left(), pos);
                }
                if (ins.right() != -1)
                {
                    addUse(ins.right(), pos);
                }
                break;
            }

            pos++;
        }

        bb = bb->getNext();
    }
}

const int poUses::findNextUse(const int variable, const int pos) const
{
    const auto& it = _uses.find(variable);

    if (it != _uses.end())
    {
        const auto& lowerBound = std::lower_bound(it->second.begin(), it->second.end(), pos);
        const int value = (*lowerBound);
        return value;
    }

    return -1;
}

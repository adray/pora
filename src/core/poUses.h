#pragma once
#include "poCFG.h"

#include <unordered_map>
#include <vector>

namespace po
{
    class poUses
    {
    public:
        void analyze(poFlowGraph& cfg);
        const int findNextUse(const int variable, const int pos) const;
        const bool hasUses(const int variable) const { return _uses.find(variable) != _uses.end(); }
        const std::vector<poInstructionRef>& getUses(const int variable) const { return _uses.at(variable); }

    private:
        void addUse(const int variable, const int pos, const int basePos, poBasicBlock* bb);

        std::unordered_map<int, std::vector<poInstructionRef>> _uses;
    };
}

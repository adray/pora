#pragma once
#include <unordered_map>
#include <vector>

namespace po
{
    class poFlowGraph;
    class poBasicBlock;

    class poUses
    {
    public:
        void analyze(poFlowGraph& cfg);
        const int findNextUse(const int variable, const int pos) const;

    private:
        void addUse(const int variable, const int pos);

        std::unordered_map<int, std::vector<int>> _uses;
    };
}

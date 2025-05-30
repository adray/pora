#pragma once
#include "poPhiWeb.h"

#include <unordered_set>
#include <unordered_map>

namespace po
{
    class poModule;
    class poNamespace;
    class poFlowGraph;
    class poBasicBlock;
    class poInstruction;
    class poDom;

    class poSSABasicBlock
    {
    public:
        poSSABasicBlock();
        inline const int maxUse()const { return _maxUse; }
        inline const int maxDef()const { return _maxDef; }
        inline void setMaxUse(const int use) { _maxUse = use; }
        inline void setMaxDef(const int def) { _maxDef = def; }

    private:
        int _maxUse;
        int _maxDef;
    };

    class poSSA
    {
    public:
        void construct(poModule& module);
    private:
        void constructNamespaces(poNamespace& ns);
        void constructFunction(const std::vector<int>& variables, poFlowGraph& cfg);
        void insertPhiNodes(const std::vector<int>& variables, poDom& dom);
        void ssaRename(const std::vector<int>& variables, poDom& dom);
        void ssaRename(const std::vector<int>& variables, poDom& dom, int bb_id);
        int genName(const int variable);
        int getTopStack(const int variable);
        void popStack(const int variable);

        std::vector<poSSABasicBlock> _ssa;
        std::unordered_map<int, std::vector<int>> _renamingStack;
        std::unordered_map<int, int> _renameMap;
        std::unordered_set<poBasicBlock*> _visited;
        int _variableNames;
    };

    class poSSA_Destruction
    {
    public:
        void destruct(poFlowGraph& cfg);
    private:
        poPhiWeb _web;
    };
}

#pragma once
#include "poPhiWeb.h"
#include "poCFG.h"

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
    class poDomNode;

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
        void ssaRename(poDom& dom, int bb_id);
        void rebaseNames(const int baseName, poDom& dom);
        int genName(const int variable);
        int getTopStack(const int variable);
        void popStack(const int variable);

        std::vector<poSSABasicBlock> _ssa;
        std::unordered_map<int, std::vector<int>> _renamingStack;
        std::unordered_map<int, int> _renameMap;
        std::unordered_set<poBasicBlock*> _visited;
        int _variableNames;
    };

    class poSSA_Phi
    {
    public:
        poSSA_Phi(poBasicBlock* bb, const int name);

        inline const int name() const { return _name; }
        inline poBasicBlock* getBasicBlock() const { return _bb; }

    private:
        poBasicBlock* _bb;
        int _name;
    };

    class poSSA_Reconstruct
    {
    public:
        poSSA_Reconstruct();
        void reconstruct(poFlowGraph& cfg, const std::vector<int>& variables);

    private:
        void reconstructUse(poDom& dom, const int node, poInstruction& inst, const int var, const int ref);
        int genName();
        int findDefFromTop(poDom& dom, poDomNode& node, const std::vector<poInstructionRef>& defs, const int var, const int type, poBasicBlock** outBB);
        int findDefFromBottom(poDom& dom, poDomNode& node, const std::vector<poInstructionRef>& defs, const int var, const int type, poBasicBlock** outBB);
        void rebuildPhiInstructions(poDom& dom);
        void rebaseNames(poFlowGraph& cfg);

        int _variableNames;
        std::unordered_map<int, std::vector<poInstructionRef>> _defs;
        std::unordered_map<int, std::vector<poSSA_Phi>> _phis;
    };

    class poSSA_Destruction
    {
    public:
        void destruct(poFlowGraph& cfg);
    private:
        poPhiWeb _web;
    };
}

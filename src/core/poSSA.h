#pragma once

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

    class poSSAPhi
    {
    public:
        poSSAPhi(const int name);
        void addValue(const int value);
        inline const std::vector<int>& values() const { return _rhs; }
        inline void setName(const int name) { _name = name; }
        inline const int name() const { return _name; }
        inline const int initalName() const { return _initialName; }

    private:
        int _initialName;
        int _name;
        std::vector<int> _rhs;
    };

    class poSSABasicBlock
    {
    public:
        poSSABasicBlock();
        void addPhi(const int name);
        inline std::vector<poSSAPhi>& phis() {
            return _phis;
        }
        inline const int maxUse()const { return _maxUse; }
        inline const int maxDef()const { return _maxDef; }
        inline void setMaxUse(const int use) { _maxUse = use; }
        inline void setMaxDef(const int def) { _maxDef = def; }

    private:
        std::vector<poSSAPhi> _phis;
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
}

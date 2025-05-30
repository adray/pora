#pragma once
#include <vector>

/*
* A file to calculate dominators, dominance frontiers etc.
*/

namespace po
{
    class poBasicBlock;
    class poFlowGraph;

    class poDomNode
    {
    public:
        poDomNode(poBasicBlock* bb);
        inline void addPredecessor(const int id) { _predecessors.push_back(id); }
        inline void addDominator(const int id) { _dominators.push_back(id); }
        inline void addDominatedBy(const int id) { _dominatedBy.push_back(id); }
        inline void addSuccessor(const int id) { _successors.push_back(id); }
        inline void addDominanceFrontier(const int id) { _dominanceFrontier.push_back(id); }
        inline void addImmediateDominatedBy(const int id) { _immediateDominatedBy.push_back(id); }

        inline void clearPredecessors() { _predecessors.clear(); }
        inline void clearDominators() { _dominators.clear(); }

        inline poBasicBlock* getBasicBlock() const { return _bb; }
        inline const std::vector<int>& dominators() const { return _dominators; }
        inline const std::vector<int>& predecessors() const { return _predecessors; }
        inline const std::vector<int>& successors() const { return _successors; }
        inline const std::vector<int>& dominatedBy() const { return _dominatedBy; }
        inline const std::vector<int>& dominanceFrontier() const { return _dominanceFrontier; }
        inline const std::vector<int>& immediateDominatedBy() const { return _immediateDominatedBy; }

    private:
        poBasicBlock* _bb;
        std::vector<int> _predecessors; /* the immediate predecessors of this node */
        std::vector<int> _successors; /* the immediate successors of this node */
        std::vector<int> _dominators; /* nodes which dominate this node */
        std::vector<int> _dominatedBy; /* nodes which this node dominates */
        std::vector<int> _immediateDominatedBy; /* nodes which this node immediately dominates */
        std::vector<int> _dominanceFrontier;
    };

    class poDom
    {
    public:
        void compute(poFlowGraph& cfg);
        inline int start() const { return 0; }
        inline poDomNode& get(const int index) { return _nodes[index]; }
        inline const poDomNode& get(const int index) const { return _nodes[index]; }
        inline int num() const { return int(_nodes.size()); }

    private:
        void computePredecessors();
        void computeDominators();
        void computeDominanceFrontier();
        void computeImmediateDominators();

        std::vector<poDomNode> _nodes;
    };
}

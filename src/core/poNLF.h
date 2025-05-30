#pragma once
#include <vector>

//
// File for computing a Nested Loop Forest (NLF)
// for a given CFG. This is based on the algorithms by Havlak and Tarjan
// described in the paper "Nesting of Reducible and Irreducible Loops".
//
//
// Briefly, this works by initially performing a pre-pass to build a
// DFST (depth first search tree) in pre-order. Then it performs the main pass
// which computes the nested loop forest:
// 1) The first loop finds the forward and backward edges, then
// records them in a data structure.
// 2) The following loop runs through the DFST in reverse pre-order.
// The goal of this is to find the nodes in the loops in the flow graph.
// This works by finding the nodes inbetween a loop header X and node Y which
// is an incoming backward node to X. This is done by simply following the
// incoming nodes forward nodes into X and then the incoming forward nodes 
// into those until the node X is reached.
//
// If node Y, or any nodes which are visited are themselves part of a loop
// this loop will be merged into the outer loop.
//

namespace po
{
    class poDom;
    class poBasicBlock;

    enum class poNLFType
    {
        NonHeader, /* not a loop header */
        Reducible, /* a loop with a single loop header */
        NonReducible, /* multiple loop headers for a given loop */
        Self /* a node which has an edge connecting directly to itself */
    };

    class poNLFSet
    {
    public:
        void initialize(const int size);
        void set_union(const int x, const int y);
        const int set_find(const int x, std::vector<int>* set) const;

    private:
        std::vector<int> _set;
    };

    class poNLF
    {
    public:
        poNLF();
        void compute(const poDom& dom);
        inline const int getHeader(const int node) const { return _header[node]; }
        inline const poNLFType getType(const int node) const { return _type[node]; }
        const bool isIrreducible() const;

    private:
        bool isAncestor(const int w, const int v);
        void dfs(const int pos, const poDom& dom);
        void analyzeLoops(const poDom& dom);

        int _current;
        std::vector<int> _nodes;    // records order of nodes visited e.g. order -> id
        std::vector<int> _number;   // records the order nodes were visited in (inverse of the "nodes") e.g. id -> order
        std::vector<int> _last;     // records the last ancestor of a node
        std::vector<int> _header;     // records the header of a node
        std::vector<std::vector<int>> _backPreds;     // records backwards predecessors
        std::vector<std::vector<int>> _nonBackPreds;     // records the forwards predecessors
        std::vector<poNLFType> _type;
        poNLFSet _set;
    };
}

#include "poNLF.h"
#include "poCFG.h"
#include "poDom.h"

#include <algorithm>

using namespace po;

//===============
// poNLFSet
//===============


void poNLFSet::initialize(const int size)
{
    _set.clear();
    _set.resize(size);

    for (int i = 0; i < size; i++)
    {
        _set[i] = i;
    }
}

void poNLFSet::set_union(const int x, const int y)
{
    for (int i = 0; i < int(_set.size()); i++)
    {
        if (_set[i] == x)
        {
            _set[i] = y;
        }
    }
}

const int poNLFSet::set_find(const int x, std::vector<int>* set) const
{
    if (set)
    {
        for (int i = 0; i < int(_set.size()); i++)
        {
            if (_set[i] == x)
            {
                set->push_back(i);
            }
        }
    }

    if (x < 0 || x >= int(_set.size()))
    {
        return -1;
    }

    return _set[x];
}

//==============
// poNLF
//==============

poNLF::poNLF()
    :
    _current(0)
{
}

const bool poNLF::isIrreducible() const
{
    // It is considered irreducible if there any loops which are irreducible

    bool ok = false;

    for (const poNLFType type : _type)
    {
        if (type == poNLFType::NonReducible)
        {
            ok = true;
            break;
        }
    }

    return ok;
}

void poNLF::compute(const poDom& dom)
{
    const size_t numNodes = dom.num();
    _nodes.resize(numNodes);
    _number.resize(numNodes, -1); // initialize all to -1
    _last.resize(numNodes);
    _type.resize(numNodes);
    _header.resize(numNodes);
    _backPreds.resize(numNodes);
    _nonBackPreds.resize(numNodes);
    _set.initialize(int(numNodes));

    if (numNodes > 0)
    {
        analyzeLoops(dom);
    }
}

// Check if W is an ancestor of V. The range of nodes which W is an ancestor for is: [W, last(w)].
// This is the case if we passed through node W on the DFS towards node V
bool poNLF::isAncestor(const int w, const int v)
{
    return _number[w] <= _number[v] && _number[v] <= _last[_number[w]];
}

void poNLF::dfs(const int pos, const poDom& dom)
{
    _nodes[_current] = pos;
    _number[pos] = _current++;

    auto& successors = dom.get(pos).successors();
    for (const int node : successors)
    {
        if (_number[node] == -1) /*unvisited*/
        {
            dfs(node, dom);
        }
    }
    
    _last[_number[pos]] = _current - 1;
}

void poNLF::analyzeLoops(const poDom& dom)
{
    dfs(dom.start(), dom);

    for (int i = dom.start(); i < int(dom.num()); i++)
    {
        const int w = _nodes[i];
        _type[w] = poNLFType::NonHeader;
        _header[w] = 0;

        const poDomNode& node = dom.get(w);
        for (const int predecessor : node.predecessors())
        {
            if (isAncestor(w, predecessor))
            {
                // backward edge - the definition for a backward edge
                // here is related to the DFS graph rather than the CFG,
                // i.e. it is looping back on its ancestor in the DFS graph.
                // 
                std::vector<int>& backwards = _backPreds[w];
                backwards.push_back(predecessor);
            }
            else
            {
                // forward node
                std::vector<int>& forwards = _nonBackPreds[w];
                forwards.push_back(predecessor);
            }
        }
    }

    _header[0] = -1; // start node is the header of the tree
    for (int i = int(dom.num()) - 1; i >= 0; i--) // iterate in reverse DFST preorder
    {
        const int w = _nodes[i];

        std::vector<int> P;
        for (const int v : _backPreds[w])
        {
            if (v != w)
            {
                // add FIND(v) to P
                _set.set_find(v, &P);
            }
            else
            {
                /* If the node loops back on itself it is a SELF node */
                _type[w] = poNLFType::Self;
            }
        }

        std::vector<int> worklist = P;
        if (P.size() != 0) { _type[w] = poNLFType::Reducible; }
        while (worklist.size() > 0)
        {
            const int x = worklist[worklist.size() - 1];
            worklist.erase(worklist.begin() + worklist.size() - 1);
            for (const int y : _nonBackPreds[x])
            {
                const int y2 = _set.set_find(y, nullptr); // FIND(y)
                if (!isAncestor(w, y2))
                {
                    _type[w] = poNLFType::NonReducible;
                    auto& list = _nonBackPreds[w];
                    list.push_back(y2);
                }
                else if (std::find(P.begin(), P.end(), y2) == P.end() && y2 != w)
                {
                    P.push_back(y2);
                    worklist.push_back(y2);
                }
            }
        }

        for (const int x : P)
        {
            _header[x] = w;
            _set.set_union(x, w); // UNION(x, w)
        }
    }
}

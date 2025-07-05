#include "poDom.h"
#include "poCFG.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace po;


//==============
// Dom Node
//==============

poDomNode::poDomNode(poBasicBlock* bb)
    :
    _bb(bb),
    _immediateDominator(-1)
{
}

//=========================
// Dominators
//=========================

void poDom::computePredecessors()
{
    std::unordered_map<poBasicBlock*, int> map;
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& node = _nodes[i];
        node.clearPredecessors();
        map.insert(std::pair<poBasicBlock*, int>(node.getBasicBlock(), int(i)));
    }

    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& node = _nodes[i];
        poBasicBlock* bb = node.getBasicBlock();

        if (bb->getNext() && !bb->unconditionalBranch())
        {
            _nodes[map[bb->getNext()]].addPredecessor(int(i));
            _nodes[i].addSuccessor(map[bb->getNext()]);
        }
        if (bb->getBranch())
        {
            _nodes[map[bb->getBranch()]].addPredecessor(int(i));
            _nodes[i].addSuccessor(map[bb->getBranch()]);
        }
    }
}

static void intersect(const std::vector<int>& v1, const std::vector<int>& v2, std::vector<int>& o)
{
    o.clear();

    int x = 0;
    int y = 0;
    while (x < v1.size() && y < v2.size())
    {
        if (v1[x] == v2[y])
        {
            o.push_back(v1[x]);
            x++;
            y++;
        }
        else if (v1[x] < v2[y])
        {
            x++;
        }
        else if (v2[y] < v1[x])
        {
            y++;
        }
    }
}

void poDom::computeDominators()
{
    const int start = 0;
    _nodes[start].addDominator(start);

    for (size_t i = 0; i < _nodes.size(); i++)
    {
        if (i != start)
        {
            for (size_t j = 0; j < _nodes.size(); j++)
            {
                _nodes[i].addDominator(int(j));
            }
        }
    }

    bool changes = true;
    while (changes)
    {
        changes = false;
        for (size_t i = 0; i < _nodes.size(); i++)
        {
            if (i != start)
            {
                // New dominators = {node} union (intersection of Dom(p) where p in Predecessor(node))
                std::vector<int> newDominators = { int(i) };
                std::vector<int> dominators = _nodes[i].dominators();
                std::vector<int> working = dominators;
                const size_t numNodes = dominators.size();
                for (int pred : _nodes[i].predecessors())
                {
                    auto& domPred = _nodes[pred].dominators();
                    intersect(domPred, dominators, working);
                    dominators = working;
                }
                
                if (std::find(dominators.begin(), dominators.end(), int(i)) == dominators.end())
                {
                    working.resize(newDominators.size() + dominators.size());
                    std::set_union(newDominators.begin(), newDominators.end(), dominators.begin(), dominators.end(), working.begin());
                }

                if (numNodes != working.size())
                {
                    changes = true;
                    _nodes[i].clearDominators();
                    for (size_t j = 0; j < working.size(); j++)
                    {
                        _nodes[i].addDominator(int(working[j]));
                    }
                }
            }
        }
    }

    // Calculate the reverse i.e. which nodes X dominates
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& dominators = _nodes[i].dominators();
        for (size_t j = 0; j < dominators.size(); j++)
        {
            _nodes[dominators[j]].addDominatedBy(int(i));
        }
    }
}

void poDom::computeDominanceFrontier()
{
    std::unordered_set<int> dominanceFrontier;
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& nodes = _nodes[i];

        // 1) For each node X find the nodes it dominates
        const auto& dominatedBy = nodes.dominatedBy();

        // 2) Find the successors of these nodes
        for (const int dominator : dominatedBy)
        {
            auto& dominatedNode = _nodes[dominator];
            for (const int successor : dominatedNode.successors())
            {
                dominanceFrontier.insert(successor);
            }
        }

        // 3) Remove any which X strictly dominates (i.e. except for X)
        for (const int dominator : dominatedBy)
        {
            if (dominator != i)
            {
                dominanceFrontier.erase(dominator);
            }
        }

        for (const int df : dominanceFrontier)
        {
            nodes.addDominanceFrontier(df);
        }

        dominanceFrontier.clear();
    }
}

void poDom::computeImmediateDominators()
{
    // This algorithm is not the most efficient solution.
    // But it is quite easy to understand, simply take a node X and look at the nodes
    // it dominates. Then try to find a node which is 'between' them. If such a node
    // exists it can't be the immediate dominator. 'Between' them is if a theoretical node Z
    // exists which X strictly dominates and node Z strictly dominates Y then
    // Z would be 'between' them in the dominator tree.

    for (size_t i = 0 ; i < _nodes.size(); i++)
    {
        const std::vector<int>& dominatedBy = _nodes[i].dominatedBy(); /*nodes which are dominated by this node*/
        for (size_t k = 0; k < dominatedBy.size(); k++)
        {
            if (i != dominatedBy[k])
            {
                bool ok = true;
                auto& dominators = _nodes[dominatedBy[k]].dominators();
                for (size_t j = 0; j < dominators.size(); j++)
                {
                    if (i != dominators[j] && dominators[j] != dominatedBy[k])
                    {
                        auto& dominator = _nodes[dominators[j]];

                        // nodes[i] does not immediately dominate nodes[k] if
                        // nodes[j] strictly dominates nodes[k] (which it does) and
                        // nodes[i] strictly dominates nodes[j]
                        for (size_t l = 0; l < dominator.dominators().size(); l++)
                        {
                            if (dominator.dominators()[l] == i)
                            {
                                ok = false;
                                break;
                            }
                        }
                    }

                    if (!ok)
                    {
                        break;
                    }
                }

                if (ok)
                {
                    _nodes[dominatedBy[k]].setImmediateDominator(int(i));
                    _nodes[i].addImmediateDominatedBy(dominatedBy[k]);
                }
            }
        }
    }
}

void poDom::compute(poFlowGraph& cfg)
{
    _nodes.clear();

    for (size_t i = 0; i < cfg.numBlocks(); i++)
    {
        poBasicBlock* bb = cfg.getBasicBlock(int(i));
        _nodes.push_back(poDomNode(bb));
    }

    computePredecessors();
    computeDominators();
    computeDominanceFrontier();
    computeImmediateDominators();
}

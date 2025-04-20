#include "poLive.h"
#include "poDom.h"
#include "poCFG.h"
#include <vector>
#include <unordered_set>
#include <iostream>

using namespace po;

//=====================
// poLiveNode
//=====================

void poLiveNode::addEdge(const int node, const bool isBackEdge)
{
    if (isBackEdge)
    {
        _backEdges.push_back(node);
    }
    else
    {
        _forwardEdges.push_back(node);
    }
}

void poLiveNode::addLiveIn(const int variable)
{
    _liveIn.insert(variable);
}

void poLiveNode::addLiveOut(const int variable)
{
    _liveOut.insert(variable);
}

void poLiveNode::removeLiveIn(const int variable)
{
    _liveIn.erase(variable);
}

void poLiveNode::removeLiveOut(const int variable)
{
    _liveOut.erase(variable);
}

//===================
// poLive
//===================

bool poLive::isReducible(poFlowGraph& cfg, poDom& dom)
{
    // The cfg is reducible if when removing the backedges (A->B is a backedge if B dominates A)
    // the resulting cfg is a DAG (directed acyclic graph) where each node can be reached from the initial node.

    for (int i = 0; i < dom.num(); i++)
    {
        auto& live = _nodes.emplace_back();
        auto& node = dom.get(i);
        auto& succ = node.successors();
        auto& dominators = node.dominators();
        bool isBackEdge = false;
        for (int j = 0; j < succ.size(); j++)
        {
            for (int k = 0; k < dominators.size(); k++)
            {
                if (succ[j] == dominators[k])
                {
                    isBackEdge = true;
                    break;
                }
            }

            live.addEdge(succ[j], isBackEdge);
        }
    }

    if (_nodes.size() > 0)
    {
        // Check if all nodes can be visited by traversing the forward edges.

        std::unordered_set<int> visited;
        std::vector<int> worklist = { 0 };

        while (worklist.size() > 0)
        {
            const int id = worklist[0];
            worklist.erase(worklist.begin());
            visited.insert(id);

            auto& live = _nodes[id];
            auto& edges = live.forwardEdges();
            for (size_t i = 0; i < edges.size(); i++)
            {
                if (!visited.contains(edges[i]))
                {
                    worklist.push_back(edges[i]);
                }
            }
        }

        if (_nodes.size() != visited.size())
        {
            return false;
        }
    }

    return true;
}

void poLive::dagDfs(poDom& dom, const int id)
{
    auto& live = _nodes[id];
    const auto& edges = live.forwardEdges(); /* I think we can assume all of these are also successors */
    for (size_t i = 0; i < edges.size(); i++)
    {
        if (_visited.contains(edges[int(i)]))
        {
            continue;
        }
        dagDfs(dom, edges[int(i)]);
    }

    poBasicBlock* bb = dom.get(id).getBasicBlock();
    auto& ins = bb->instructions();
    //for (size_t i = 0; i < ins.size(); i++)
    //{
    //    if (ins[i].code() == IR_PHI)
    //    {
    //        live.addLiveOut(ins[i].left());
    //        live.addLiveOut(ins[i].right());
    //    }
    //}
    for (size_t i = 0; i < edges.size(); i++)
    {
        poBasicBlock* bb = dom.get(edges[i]).getBasicBlock();
        auto& ins = bb->instructions();

    }

    for (size_t i = 0; i < edges.size(); i++)
    {
        auto& liveIn = _nodes[edges[i]].liveIn();
        for (const int item : liveIn)
        {
            live.addLiveOut(item);
        }

        // We need to remove the PHI from the set of liveOut, only if
        // the PHI def is not already present in the liveIn.

        poBasicBlock* bb = dom.get(edges[i]).getBasicBlock();
        auto& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            const int name = ins[i].name();
            if (ins[i].code() == IR_PHI &&
                !liveIn.contains(name))
            {
                live.removeLiveOut(name);
            }
        }
    }

    for (const int item : live.liveOut())
    {
        live.addLiveIn(item);
    }

    for (int i = int(ins.size()) - 1; i >= 0; i--)
    {
        const auto& instruction = ins[i];
        if (instruction.code() != IR_PHI)
        {
            live.removeLiveIn(instruction.name());
            if (instruction.code() != IR_BR &&
                instruction.code() != IR_CONSTANT &&
                instruction.code() != IR_CALL &&
                instruction.code() != IR_PARAM)
            {
                if (instruction.left() != -1)
                {
                    live.addLiveIn(instruction.left());
                }
                if (instruction.right() != -1)
                {
                    live.addLiveIn(instruction.right());
                }
            }
        }
    }

    for (size_t i = 0; i < ins.size(); i++)
    {
        if (ins[i].code() == IR_PHI)
        {
            live.addLiveIn(ins[i].name());
        }
    }

    _visited.insert(id);
}

void poLive::compute(poFlowGraph& cfg, poDom& dom)
{
    // First check to see if is reducible.
    // If it isn't reducible will need to use a different algorithm

    if (isReducible(cfg, dom))
    {
        dagDfs(dom, 0);
    }
    else
    {
        // .. not reducible
    }
}

//===============
// poLiveRange
//===============

void poLiveRange::extendLiveRange(const int variable, const int pos)
{
    if (_variableMapping.find(variable) != _variableMapping.end())
    {
        const std::vector<int> index = _variableMapping[variable];
        for (int i : index)
        {
            _range[i] = pos - i;
        }
    }
}

void poLiveRange::setLiveRange(const int variable, const int range)
{
    if (_variableMapping.find(variable) != _variableMapping.end())
    {
        const std::vector<int> index = _variableMapping[variable];
        for (int i : index)
        {
            _range[i] = range;
        }
    }
}

void poLiveRange::addLiveRange(const int variable)
{
    const int size = int(_range.size());
    if (_variableMapping.find(variable) == _variableMapping.end())
    {
        _range.push_back(0);
        _variableMapping.insert(std::pair<int, std::vector<int>>(variable, std::vector<int> { size }));
    }
    else
    {
        _variableMapping[variable].push_back(size);
        _range.push_back(0);
    }
}

//int poLiveRange::getLiveRange(const int variable)
//{
//    if (_variableMapping.find(variable) != _variableMapping.end())
//    {
//        const int index = _variableMapping[variable];
//        return _range[index];
//    }
//    else
//    {
//        return 0;
//    }
//}

int poLiveRange::getLiveRange(const int index)
{
    return _range[index];
}

void poLiveRange::compute(poFlowGraph& cfg)
{
    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        const auto& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            auto& instruction = ins[i];

            if (instruction.code() != IR_CALL &&
                instruction.code() != IR_CONSTANT &&
                instruction.code() != IR_BR &&
                instruction.code() != IR_PARAM)
            {
                if (instruction.left() != -1)
                {
                    extendLiveRange(instruction.left(), int(pos));
                }
                if (instruction.right() != -1)
                {
                    extendLiveRange(instruction.right(), int(pos));
                }
            }

            addLiveRange(instruction.name());
            pos++;
        }

        bb = bb->getNext();
    }
}


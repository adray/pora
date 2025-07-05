#include "poLive.h"
#include "poDom.h"
#include "poCFG.h"
#include "poNLF.h"
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

poLive::poLive()
    :
    _error(false)
{
}

void poLive::phiUses(poDom& dom, const int id)
{
    auto& live = _nodes[id];
    const auto& succ = dom.get(id).successors();
    const auto& dominators = dom.get(id).dominators();
    poBasicBlock* entryBB = dom.get(id).getBasicBlock();

    for (size_t i = 0; i < succ.size(); i++)
    {
        poBasicBlock* bb = dom.get(succ[i]).getBasicBlock(); // successor basic block

        for (auto& phi : bb->phis())
        {
            auto& values = phi.values();
            auto& basicBlocks = phi.getBasicBlock();
            for (size_t j = 0; j < values.size(); j++)
            {
                const int name = values[j];
                const poBasicBlock* sourceBB = basicBlocks[j];

                bool ok = sourceBB == entryBB;
                if (!ok)
                {
                    /*  loop over the nodes which dominate this node - hopefully it has to be case the variable
                        will be live in our basic block, if was declared in one of our dominators.
                    */
                    for (const int dominator : dominators)
                    {
                        if (dom.get(dominator).getBasicBlock() == sourceBB)
                        {
                            ok = true;
                            break;
                        }
                    }
                }

                if (ok)
                {
                    live.addLiveOut(name);
                }
            }
        }
    }
}

void poLive::dagDfs(poDom& dom, const int id)
{
    auto& live = _nodes[id];
    const auto& edges = live.forwardEdges();
    for (size_t i = 0; i < edges.size(); i++)
    {
        if (_visited.contains(edges[int(i)]))
        {
            continue;
        }
        dagDfs(dom, edges[int(i)]);
    }

    phiUses(dom, id);

    poBasicBlock* bb = dom.get(id).getBasicBlock();
    auto& ins = bb->instructions();

    for (size_t i = 0; i < edges.size(); i++)
    {
        // Take a copy as we need to remove the PHI from the set of live
        std::unordered_set<int> liveIn = _nodes[edges[i]].liveIn();

        poBasicBlock* bb = dom.get(edges[i]).getBasicBlock();
        auto& ins = bb->instructions();
        for (size_t j = 0; j < ins.size(); j++)
        {
            const int name = ins[j].name();
            if (ins[j].code() == IR_PHI)
            {
                liveIn.erase(name);
            }
        }

        for (const int item : liveIn)
        {
            live.addLiveOut(item);
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
            if (!instruction.isSpecialInstruction())
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

    poNLF nlf;
    nlf.compute(dom);

    if (nlf.isIrreducible())
    {
        // .. not reducible
        _error = true;
        _errorText = "Control flow graph is irreducible, liveness information cannot be computed (yet).";
    }
    else
    {
        initializeLive(dom);
        dagDfs(dom, 0);

        for (int i = dom.start(); i < dom.num(); i++)
        {
            loopTreeDfs(dom, nlf, i);
        }
    }
}

void poLive::loopTreeDfs(poDom& dom, poNLF& nlf, const int id)
{
    if (nlf.getType(id) == poNLFType::Reducible)
    {
        poBasicBlock *bb = dom.get(id).getBasicBlock();
        const auto& node = _nodes[id];
        // Live loop = LiveIn - PhiDefs
        std::unordered_set<int> liveLoop = node.liveIn();
        for (const auto& phi : bb->phis())
        {
            liveLoop.erase(phi.name());
        }

        // Loop over the loop tree children and propagate the liveness
        for (int i = dom.start(); i < dom.num(); i++)
        {
            if (i != id && nlf.getHeader(i) == id) // if it is a child of this loop
            {
                auto& child = _nodes[i];
                for (const int live : liveLoop)
                {
                    child.addLiveIn(live);
                    child.addLiveOut(live);
                }

                loopTreeDfs(dom, nlf, i);
            }
        }
    }
}

void poLive::initializeLive(poDom& dom)
{
    for (int i = 0; i < dom.num(); i++)
    {
        auto& live = _nodes.emplace_back();
        auto& node = dom.get(i);
        auto& succ = node.successors();
        auto& dominators = node.dominators();
        bool isBackEdge = false;
        for (int j = 0; j < int(succ.size()); j++)
        {
            for (int k = 0; k < int(dominators.size()); k++)
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
}

//===============
// poLiveRange
//===============

void poLiveRange::extendLiveRange(const int variable, const int pos)
{
    if (_variableMapping.find(variable) != _variableMapping.end())
    {
        const std::vector<int>& index = _variableMapping[variable];
        for (const int i : index)
        {
            _range[i] = pos - i;
        }
    }
}

void poLiveRange::setLiveRange(const int variable, const int range)
{
    if (_variableMapping.find(variable) != _variableMapping.end())
    {
        const std::vector<int>& index = _variableMapping[variable];
        for (const int i : index)
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

int poLiveRange::getLiveRange(const int index)
{
    return _range[index];
}

void poLiveRange::compute(poFlowGraph& cfg)
{
    poDom dom;
    dom.compute(cfg);

    poLive live;
    live.compute(cfg, dom);

    int id = 0;
    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        const auto& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            auto& instruction = ins[i];

            if (!instruction.isSpecialInstruction())
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

        const std::vector<poLiveNode>& nodes = live.nodes();
        const std::unordered_set<int> liveOut = nodes[id].liveOut();
        for (const int live : liveOut)
        {
            extendLiveRange(live, int(pos));
        }

        bb = bb->getNext();
        id++;
    }
}


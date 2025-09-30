#include "poRegGraph.h"
#include "poModule.h"
#include "poCFG.h"
#include "poLive.h"
#include "poSSA.h"
#include "poUses.h"
#include "poPhiWeb.h"

#include <algorithm>
#include <assert.h>

using namespace po;

//
// poInterferenceGraph
//

void poInterferenceGraph::insert(const poInterferenceGraph_Node& node) {
    _nodes.push_back(node);

    // Remove any nodes which are no longer live
    std::vector<int> toRemove;
    for (const int id : _liveNodes)
    {
        const poInterferenceGraph_Node& liveNode = _nodes[id];
        if (liveNode.liveEnd() < node.liveStart())
        {
            toRemove.push_back(id);
        }
    }

    for (const int id : toRemove)
    {
        const auto& it = std::lower_bound(_liveNodes.begin(), _liveNodes.end(), id); // Nodes always in order
        if (it != _liveNodes.end())
        {
            _liveNodes.erase(it);
        }
    }

    // Check for interference with other live variables
    std::vector<int>& neighbours = _nodes.back().getNeighbours();
    const int curId = int(_nodes.size()) - 1;
    for (const int id : _liveNodes)
    {
        neighbours.push_back(id);
        poInterferenceGraph_Node& node = _nodes[id];
        node.getNeighbours().push_back(curId);
    }

    // Add current node to live nodes
    const auto& curIt = std::lower_bound(_liveNodes.begin(), _liveNodes.end(), curId);
    _liveNodes.insert(curIt, curId); // Invariant: nodes always in order
}

void poInterferenceGraph::calculateAffinity(const poPhiWeb& web)
{
    // Calculate affinity between nodes based on phi webs
    
    std::unordered_map<int, int> webVariables;
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        poInterferenceGraph_Node& node = _nodes[i];
        const int webId = web.find(node.name());
        if (webId == -1)
        {
            continue;
        }

        const auto& var = webVariables.find(webId);
        if (var == webVariables.end())
        {
            webVariables.insert(std::pair<int, int>(webId, node.name()));
            continue;
        }

        const int affinityName = var->second;
        for (size_t j = 0; j < _nodes.size(); j++)
        {
            if (i == j)
            {
                continue;
            }

            poInterferenceGraph_Node& otherNode = _nodes[j];
            if (otherNode.name() != affinityName)
            {
                continue;
            }

            std::vector<int>& affinities = node.getAffinities();
            affinities.push_back(int(j));

            std::vector<int>& otherAffinities = otherNode.getAffinities();
            otherAffinities.push_back(int(i));
        }
    }
}

void poInterferenceGraph::colorGraph(const int numColors, const int numSpills)
{
    // Aggressive coalescing based on affinities

    for (int i = 0; i < int(_nodes.size()); i++)
    {
        auto& node = _nodes[i];
        for (const int affinityId : node.getAffinities())
        {
            auto& affinityNode = _nodes[affinityId];
            // Check if already merged
            if (std::find(node.getMerged().begin(), node.getMerged().end(), affinityId) != node.getMerged().end())
            {
                continue;
            }

            // Merge affinity nodes
            std::vector<int>& neighbours = node.getNeighbours();
            for (const int neighbourId : affinityNode.getNeighbours())
            {
                if (neighbourId == i)
                {
                    continue;
                }
                
                // Add neighbour to current node
                if (std::find(neighbours.begin(), neighbours.end(), neighbourId) == neighbours.end())
                {
                    neighbours.push_back(neighbourId);
                }
            }
            for (const int neighbourId : neighbours)
            {
                if (neighbourId == affinityId)
                {
                    continue;
                }

                // Add neighbour to current node
                auto& affinityNeighbours = affinityNode.getNeighbours();
                if (std::find(affinityNeighbours.begin(), affinityNeighbours.end(), neighbourId) == affinityNeighbours.end())
                {
                    affinityNeighbours.push_back(neighbourId);
                }
            }

            // Mark as merged
            node.getMerged().push_back(affinityId);
            affinityNode.getMerged().push_back(i);
        }
    }


    // Spill if there are not enough colors (this is often called simplification)

    int numSpilled = 0;
    int colorsToUse = numColors;
    bool changes = true;
    while (changes)
    {
        changes = false;
        for (int i = 0; i < int(_nodes.size()); i++)
        {
            auto& node = _nodes[i];
            if (node.getNeighbours().size() >= colorsToUse)
            {
                const int numNeighbours = int(node.getNeighbours().size());
                for (int j = 0; j < numNeighbours; j++)
                {
                    const int neighbourId = node.getNeighbours()[j];
                    auto& neighbour = _nodes[neighbourId];
                    neighbour.removeNeighbour(i);
                }

                node.setSpilled(true);
                node.clearNeightbours();
                changes = true;

                if (numSpilled == 0)
                {
                    colorsToUse -= numSpills;
                }
                numSpilled++;
            }
        }
    }

    // Greedy coloring algorithm

    for (poInterferenceGraph_Node& node : _nodes)
    {
        if (node.spilled())
        {
            continue;
        }

        if (node.color() != -1)
        {
            continue;
        }

        std::vector<bool> usedColors(colorsToUse, false);
        for (const int neighbourId : node.getNeighbours())
        {
            const int color = _nodes[neighbourId].color();
            if (color >= 0 && color < colorsToUse)
            {
                usedColors[color] = true;
            }
        }
        for (int c = 0; c < colorsToUse; c++)
        {
            if (!usedColors[c])
            {
                node.setColor(c);

                // Color merged nodes the same
                for (const int mergedId : node.getMerged())
                {
                    _nodes[mergedId].setColor(c);
                }

                break;
            }
        }
    }
}

const int poInterferenceGraph::findNode(const int variable, const int pos) const
{
    // Naive linear search

    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& node = _nodes[i];
        if (node.name() != variable)
        {
            continue;
        }

        if (pos >= node.liveStart() &&
            node.liveEnd() >= pos)
        {
            return int(i);
        }

        for (const int merger : node.getMerged())
        {
            auto& mergeNode = _nodes[merger];
            if (pos >= mergeNode.liveStart() &&
                mergeNode.liveEnd() >= pos)
            {
                return int(i);
            }
        }
    }

    return -1;
}

//
// poRegGraph
//

poRegGraph::poRegGraph(poModule& module)
    :
    _module(module),
    _numRegisters(0)
{
}

void poRegGraph::setNumRegisters(const int numRegisters)
{
    _numRegisters = numRegisters;
    _volatile.resize(_numRegisters);
    _type.resize(_numRegisters);
}

void poRegGraph::setVolatile(const int reg, const bool isVolatile)
{
    if (reg < 0 || reg >= _numRegisters)
    {
        return;
    }

    _volatile[reg] = isVolatile;
}

void poRegGraph::setType(const int reg, const poRegType type)
{
    if (reg < 0 || reg >= _numRegisters)
    {
        return;
    }

    _type[reg] = type;
}

void poRegGraph::allocateRegisters(poFlowGraph& cfg)
{
    // Setup
    _maxRegistersUsedByType.resize(int(poRegType::MAX));
    _registersSet.resize(_numRegisters, false);
    _numRegistersPerType.resize(int(poRegType::MAX));

    for (int i = 0; i < _numRegisters; i++)
    {
        _numRegistersPerType[int(_type[i])]++;

        if (_volatile[i])
        {
            continue;
        }

        _maxRegistersUsedByType[int(_type[i])]++;
        _colorToRegister.push_back(i);
    }

    poLiveRange liveRange;
    liveRange.compute(cfg);

    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (const poInstruction& ins : bb->instructions())
        {
            switch (ins.code())
            {
            case IR_BR:
                pos++;
                continue;
            case IR_ARG:
            case IR_CMP:
                pos++;
                continue;
            case IR_RETURN:
                pos++;
                continue;
            default:
                break;
            }

            if (ins.code() == IR_ALLOCA)
            {
                pos++;

                const poType& type = _module.types()[ins.type()];
                assert(type.isPointer() || type.isArray());

                const poType& baseType = _module.types()[type.baseType()];

                const int elements = ins.left();
                const int size = _module.types()[baseType.id()].size();
                _stackAllocator.allocateSlot(ins.name(), size * elements);
                continue;
            }

            const int live = liveRange.getLiveRange(pos);
            switch (ins.type())
            {
            case TYPE_F32:
            case TYPE_F64:
                _sse.insert(poInterferenceGraph_Node(ins.name(),
                    ins.code() == IR_PHI,
                    pos,
                    pos + live));
                break;
            case TYPE_BOOLEAN:
            case TYPE_I64:
            case TYPE_I32:
            case TYPE_I16:
            case TYPE_I8:
            case TYPE_U64:
            case TYPE_U32:
            case TYPE_U16:
            case TYPE_U8:
                _general.insert(poInterferenceGraph_Node(ins.name(),
                    ins.code() == IR_PHI,
                    pos,
                    pos + live));
                break;
            default:
                if (!_module.types()[ins.type()].isPointer())
                {
                    pos++;
                    continue;
                }
                _general.insert(poInterferenceGraph_Node(ins.name(),
                    ins.code() == IR_PHI,
                    pos,
                    pos + live));
                break;
            }
            
            pos++;
        }

        bb = bb->getNext();
    }

    // Calculate phi webs
    poPhiWeb web;
    web.findPhiWebs(cfg);
    _general.calculateAffinity(web);
    _sse.calculateAffinity(web);

    // Color the interference graphs
    _general.colorGraph(_maxRegistersUsedByType[int(poRegType::General)], 2);
    _sse.colorGraph(_maxRegistersUsedByType[int(poRegType::SSE)], 2);

    // SSA destruction
    poSSA_Destruction ssaDestruction;
    ssaDestruction.destruct(cfg);

    // Analyze uses
    poUses uses;
    uses.analyze(cfg);

    // Generate spills and restores
    generateSpillsAndRestores(_general, uses, poRegType::General);
    generateSpillsAndRestores(_sse, uses, poRegType::SSE);

    // Calculate which registers we have used
    gatherUsedRegisters(_general, poRegType::General);
    gatherUsedRegisters(_sse, poRegType::SSE);

    // TODO: We may need to insert copy instructions where there were PHI nodes
}

void poRegGraph::gatherUsedRegisters(const poInterferenceGraph& graph, const poRegType type)
{
    for (const poInterferenceGraph_Node& node : graph.nodes())
    {
        if (node.color() == -1) { continue; }

        int color = node.color();
        if (type == poRegType::SSE)
        {
            color += _maxRegistersUsedByType[(int)poRegType::General];
        }

        const int reg = _colorToRegister[color];
        _registersSet[reg] = true;
    }
}

void poRegGraph::generateSpillsAndRestores(const poInterferenceGraph& graph, const poUses& uses, const poRegType& type)
{
    int registerOffset = 0;
    if (type == poRegType::SSE)
    {
        registerOffset = _maxRegistersUsedByType[(int)poRegType::General];
    }

    const int reg1 = _colorToRegister[registerOffset + _maxRegistersUsedByType[int(type)] - 1];
    const int reg2 = _colorToRegister[registerOffset + _maxRegistersUsedByType[int(type)] - 2];

    for (size_t i = 0; i < graph.nodes().size(); i++)
    {
        const poInterferenceGraph_Node& node = graph.nodes()[i];
        if (node.color() != -1)
        {
            continue;
        }

        int slot = -1;

        // Merged nodes should share the same slot
        //
        for (const int mergerId : node.getMerged())
        {
            const poInterferenceGraph_Node& mergerNode = graph.nodes()[mergerId];
            const int mergerSlot = _stackAllocator.findSlot(mergerNode.name());
            if (mergerSlot != -1)
            {
                slot = mergerSlot;
                break;
            }
        }

        if (slot == -1)
        {
            slot = _stackAllocator.allocateSlot(node.name(), 8); // TODO: size based on type
        }

        if (uses.hasUses(node.name()))
        {
            const auto& variableUses = uses.getUses(node.name());
            for (const poInstructionRef& use : variableUses)
            {
                if (use.getInstruction().code() == IR_PHI)
                {
                    continue; // PHI nodes are handled separately
                }

                const int usePos = use.getRef();
                const auto& it = _restores.find(usePos);
                if (it != _restores.end())
                {
                    const int reg = reg1;
                    it->second.push_back(poRegRestore(reg, node.name(), slot));
                }
                else
                {
                    const int reg = reg2;
                    _restores[usePos] = std::vector<poRegRestore>{ poRegRestore(reg, node.name(), slot) };
                }
            }
        }

        if (node.isPhi())
        {
            continue;
        }

        const auto& it = _spills.find(node.liveStart());
        if (it == _spills.end())
        {
            const int reg = reg1;
            _spills[node.liveStart()] = std::vector<poRegSpill>{ poRegSpill(reg, node.name(), slot) };
        }
    }
}

const bool poRegGraph::spillAt(const int index, const int element, poRegSpill* spill) const
{
    const auto& it = _spills.find(index);
    if (it != _spills.end())
    {
        if (element < 0 || element >= it->second.size())
        {
            return false;
        }
        if (spill)
        {
            *spill = it->second[element];
        }
        return true;
    }
    return false;
}

const bool poRegGraph::restoreAt(const int index, const int element, poRegRestore* spill) const
{
    const auto& it = _restores.find(index);
    if (it != _restores.end())
    {
        if (element < 0 || element >= it->second.size())
        {
            return false;
        }
        if (spill)
        {
            *spill = it->second[element];
        }
        return true;
    }
    return false;
}

int poRegGraph::getStackSlotByVariable(const int variable) const
{
    return _stackAllocator.findSlot(variable);
}

int poRegGraph::getRegisterByVariable(const int variable) const
{
    return getRegisterByVariable(variable, _iterator.position());
}

int poRegGraph::getRegisterByVariable(const int variable, const int pos) const {
    auto& nodes = _general.nodes();
    const int generalPos = _general.findNode(variable, pos);
    if (generalPos != -1)
    {
        auto& node = _general.nodes()[generalPos];
        if (node.color() != -1)
        {
            return _colorToRegister[node.color()];
        }
    }

    const int ssePos = _sse.findNode(variable, pos);
    if (ssePos != -1)
    {
        auto& node = _sse.nodes()[ssePos];
        if (node.color() != -1)
        {
            return _colorToRegister[node.color() + _maxRegistersUsedByType[int(poRegType::General)]];
        }
    }

    const auto& restore = _restores.find(pos);
    if (restore != _restores.end())
    {
        for (const poRegRestore& r : restore->second)
        {
            if (r.restoreVariable() == variable)
            {
                return r.restoreRegister();
            }
        }

        if (generalPos != -1)
        {
            auto& node = _general.nodes()[generalPos];
            for (const int merge : node.getMerged())
            {
                auto& mergeNode = _general.nodes()[merge];
                for (const poRegRestore& r : restore->second)
                {
                    if (r.restoreVariable() == mergeNode.name())
                    {
                        return r.restoreRegister();
                    }
                }
            }
        }

        if (ssePos != -1)
        {
            auto& node = _sse.nodes()[ssePos];
            for (const int merge : node.getMerged())
            {
                auto& mergeNode = _sse.nodes()[merge];
                for (const poRegRestore& r : restore->second)
                {
                    if (r.restoreVariable() == mergeNode.name())
                    {
                        return r.restoreRegister();
                    }
                }
            }
        }
    }

    const auto& spill = _spills.find(pos);
    if (spill != _spills.end())
    {
        for (const poRegSpill& r : spill->second)
        {
            if (r.spillVariable() == variable)
            {
                return r.spillRegister();
            }
        }

        if (generalPos != -1)
        {
            auto& node = _general.nodes()[generalPos];
            for (const int merge : node.getMerged())
            {
                auto& mergeNode = _general.nodes()[merge];
                for (const poRegSpill& r : spill->second)
                {
                    if (r.spillVariable() == mergeNode.name())
                    {
                        return r.spillRegister();
                    }
                }
            }
        }

        if (ssePos != -1)
        {
            auto& node = _sse.nodes()[ssePos];
            for (const int merge : node.getMerged())
            {
                auto& mergeNode = _sse.nodes()[merge];
                for (const poRegSpill& r : spill->second)
                {
                    if (r.spillVariable() == mergeNode.name())
                    {
                        return r.spillRegister();
                    }
                }
            }
        }
    }

    return -1;
}

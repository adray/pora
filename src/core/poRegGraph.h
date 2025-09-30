#pragma once
#include <vector>
#include "poRegLinear.h"

namespace po
{
    class poModule;
    class poFlowGraph;
    class poPhiWeb;

    class poInterferenceGraph_Node
    {
    public:
        poInterferenceGraph_Node(const int name, const bool isPhi, const int liveStart, const int liveEnd)
            : _name(name), _isPhi(isPhi), _liveStart(liveStart), _liveEnd(liveEnd), _color(-1), _spilled(false) {
        }

        inline void setSpilled(const bool spilled) { _spilled = spilled; }
        inline void setColor(const int color) { _color = color; }

        inline const bool isPhi() const { return _isPhi; }
        inline const bool spilled() const { return _spilled; }
        inline const int color() const { return _color; }
        inline const int name() const { return _name; }
        inline const int liveStart() const { return _liveStart; }
        inline const int liveEnd() const { return _liveEnd; }
        inline std::vector<int>& getNeighbours() { return _neighbours; }
        inline const std::vector<int>& getNeighbours() const { return _neighbours; }
        inline std::vector<int>& getAffinities() { return _affinities; }
        inline std::vector<int>& getMerged() { return _merged; }
        inline const std::vector<int>& getMerged() const { return _merged; }

        inline void clearNeightbours() { _neighbours.clear(); }
        void removeNeighbour(const int id) {
            const auto& it = std::find(_neighbours.begin(), _neighbours.end(), id);
            if (it != _neighbours.end()) {
                _neighbours.erase(it);
            }
        }

    private:
        bool _spilled;
        bool _isPhi;
        int _color;
        int _name;
        int _liveStart;
        int _liveEnd;
        std::vector<int> _neighbours;
        std::vector<int> _affinities;
        std::vector<int> _merged;
    };

    class poInterferenceGraph
    {
    public:
        void insert(const poInterferenceGraph_Node& node);
        void calculateAffinity(const poPhiWeb& web);
        void colorGraph(const int numColors, const int numSpills);
        inline const std::vector<poInterferenceGraph_Node>& nodes() const { return _nodes; }
        const int findNode(const int variable, const int pos) const;

    private:
        std::vector<poInterferenceGraph_Node> _nodes;
        std::vector<int> _liveNodes;
    };

    class poRegGraph
    {
    public:
        poRegGraph(poModule& module);
        void setNumRegisters(const int numRegisters);
        void setVolatile(const int reg, const bool isVolatile);
        void setType(const int reg, const poRegType type);
        void allocateRegisters(poFlowGraph& cfg);
        int getRegisterByVariable(const int variable, const int pos) const;
        int getRegisterByVariable(const int variable) const;

        int getStackSlotByVariable(const int variable) const;

        inline int stackSize() const { return _stackAllocator.numSlots(); }
        inline const bool isRegisterSet(const int index) const { return _registersSet[index]; }
        inline const bool isVolatile(const int index) const { return _volatile[index]; }

        const bool spillAt(const int index, const int element, poRegSpill* spill) const;
        const bool restoreAt(const int index, const int element, poRegRestore* spill) const;

        inline poRegLinearIterator& iterator() { return _iterator; }

    private:
        void generateSpillsAndRestores(const poInterferenceGraph& graph, const poUses& uses, const poRegType& type);
        void gatherUsedRegisters(const poInterferenceGraph& graph, const poRegType type);

        int _numRegisters;
        std::vector<bool> _volatile;
        std::vector<poRegType> _type;
        std::vector<int> _maxRegistersUsedByType;
        std::vector<int> _colorToRegister;
        std::vector<int> _numRegistersPerType; /* the number of registers per type (including volatile) */
        std::vector<bool> _registersSet; /* the registers which have been used at any point */
        std::unordered_map<int, std::vector<poRegSpill>> _spills; /* a mapping from instruction index -> spill */
        std::unordered_map<int, std::vector<poRegRestore>> _restores; /* a mapping from instruction index -> restore */

        poModule& _module;
        poStackAllocator _stackAllocator;
        poInterferenceGraph _general;
        poInterferenceGraph _sse;
        poRegLinearIterator _iterator;
    };
}

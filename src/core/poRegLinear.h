#pragma once
#include <vector>
#include <unordered_map>

namespace po
{
    //
    // Linear register allocator.
    // 
    // A linear register allocator is fairly quick due to treating the flow graph
    // as a "hyperblock" and therefore able to allocate the registers using only few
    // simple loops. However, the does not produce optimal register allocation
    // or spilling.
    //

    class poLive;
    class poFlowGraph;
    class poBasicBlock;
    class poUses;
    class poInstruction;
    class poModule;
    class poRegLinear;

    enum class poRegType
    {
        General,
        SSE,
        MAX // number of register types
    };

    class poStackAllocator
    {
    public:
        poStackAllocator();
        inline const int numSlots() const { return _numSlots; }
        inline const std::vector<bool>& slots() const { return _slots; }
        const int allocateSlot(const int variable, const int size);
        const void freeSlot(const int variable);
        const int findSlot(const int variable) const;

    private:
        int _numSlots;
        std::vector<bool> _slots;
        std::unordered_map<int, int> _occupancy;
    };

    class poRegSpill
    {
    public:
        poRegSpill();
        poRegSpill(const int spillRegister, const int spillVariable, const int spillStackSlot);

        const int spillRegister() const { return _spillRegister; }
        const int spillVariable() const { return _spillVariable; }
        const int spillStackSlot() const { return _spillStackSlot; }

    private:
        int _spillRegister;
        int _spillVariable;
        int _spillStackSlot;
    };

    class poRegRestore
    {
    public:
        poRegRestore();
        poRegRestore(const int restoreRegister, const int restoreVariable, const int restoreStackSlot);

        const int restoreRegister() const { return _restoreRegister; }
        const int restoreVariable() const { return _restoreVariable; }
        const int restoreStackSlot() const { return _restoreStackSlot; }

    private:
        int _restoreRegister;
        int _restoreVariable;
        int _restoreStackSlot;
    };

    class poStackSlot
    {
    public:
        poStackSlot() :
            _type(poRegType::General), _live(0) {
        }
        poStackSlot(const poRegType type, const int live)
            : _type(type), _live(live) {
        }

        inline const poRegType type() const { return _type; }
        inline const int live() const { return _live; }

    private:
        poRegType _type;
        int _live;
    };

    class poRegLinearExit
    {
    public:
        poRegLinearExit(const std::unordered_map<int, poStackSlot>& stackSlots) : _stackSlots(stackSlots) {}
        void addStackSlot(const int slot, const poStackSlot& stackSlot) { _stackSlots.insert(std::pair<int, poStackSlot>(slot, stackSlot)); }
        inline const std::unordered_map<int, poStackSlot>& stackSlots() const { return _stackSlots; }

    private:
        std::unordered_map<int, poStackSlot> _stackSlots; /* slot -> stack slot */
    };

    class poRegLinearEntry
    {
    public:
        poRegLinearEntry(const std::vector<int>& variables,
            const std::vector<bool>& registerUsed,
            const std::vector<int>& registersUsedByType,
            const int pos);
        inline const int getRegister(const int index) const { return _variables[index]; }
        inline const int getRegisterType(const int index) const { return _registersUsedByType[index]; }
        inline const int isUsed(const int index) const { return _registerUsed[index]; }
        inline const int position() const { return _pos; }

    private:
        std::vector<int> _variables; /* a list indicating which register is used by which variable (register -> variable) */
        std::vector<int> _registersUsedByType;
        std::vector<bool> _registerUsed;
        int _pos;
    };

    class poRegLinearIterator
    {
    public:
        poRegLinearIterator() : _pos(0) {}
        inline const void next() { _pos++; }
        inline const void advance(const int n) { _pos += n; }
        inline const int position() const { return _pos; }
        inline void reset() { _pos = 0; }

    private:
        int _pos;
    };

    class poRegLinearAssignment
    {
    public:
        poRegLinearAssignment(const int pos, const int live, const int variable, const int reg) :
            _pos(pos),
            _live(live),
            _variable(variable),
            _reg(reg) {
        }
        inline const int variable() const { return _variable; }
        inline const int reg() const { return _reg; }
        inline const int pos() const { return _pos; }
        inline const int live() const { return _live; }

    private:
        int _pos;
        int _variable;
        int _reg;
        int _live;
    };

    class poRegLinear
    {
    public:
        poRegLinear(poModule& module);
        void setNumRegisters(const int numRegisters);
        void setVolatile(const int reg, const bool isVolatile);
        void setType(const int reg, const poRegType type);
        void allocateRegisters(poFlowGraph& cfg);
        int getRegisterByVariable(const int variable, const int pos) const;
        int getRegisterByVariable(const int variable) const;
        const bool spillAt(const int index, const int element, poRegSpill* spill) const;
        const bool restoreAt(const int index, const int element, poRegRestore* spill) const;
        const int getStackSlotByVariable(const int pos) const;

        //inline const int getRegister(const int index) const { return _registers[index].reg(); }
        inline const int numRegisters() const { return _numRegisters; }
        inline const int numInstructions() const { return int(_registers.size()); }
        inline const bool isRegisterSet(const int index) const { return _registersSet[index]; }
        inline const bool isVolatile(const int index) const { return _volatile[index]; }
        inline const int stackSize() const { return _stackAlloc.numSlots(); }

        inline poRegLinearIterator& iterator() { return _iterator; }

    private:
        void allocateRegister(const int pos, const poRegType type, const int live, const int variable);
        void freeRegisters(const int pos);
        void spillRegisters(const int pos, const poRegType type, const poInstruction& ins, poUses& uses);
        void balanceRegisters(const int pos, poBasicBlock* bb);
        void repairSpills(poBasicBlock* bb);
        void spill(const int pos, const poRegSpill& spill);
        void restore(const int pos, const poRegRestore& restore);
        void mapRegister(const poRegLinearAssignment& assignment);
        void freeStackSlots(const int pos);

        poStackAllocator _stackAlloc;
        poModule& _module;
        poRegLinearIterator _iterator;

        int _numRegisters;
        std::vector<bool> _volatile;
        std::vector<poRegType> _type;
        std::vector<int> _variables; /* a list indicating which register is used by which variable (register -> variable) */
        std::vector<bool> _registerUsed;
        std::vector<int> _registerExpiry;
        std::vector<int> _registersUsedByType;
        std::vector<int> _maxRegistersUsedByType;
        std::vector<bool> _registersSet; /* the registers which have been used at any point */
        std::unordered_map<int, std::vector<poRegLinearAssignment>> _registers; /* a mapping from variable -> (last) instruction index */
        std::unordered_map<poBasicBlock*, poRegLinearEntry> _entries; /* a list of entries associated with basic blocks */
        std::unordered_map<poBasicBlock*, poRegLinearExit> _exits; /* a list of entries associated with basic blocks */
        std::unordered_map<int, std::vector<poRegSpill>> _spills; /* a mapping from instruction index -> spill */
        std::unordered_map<int, std::vector<poRegRestore>> _restores; /* a mapping from instruction index -> restore */
        std::unordered_map<int, int> _registerMap; /* mapping from variable -> register  */
        std::unordered_map<int, poStackSlot> _stackSlots; /* slot -> stack slot */
    };
}

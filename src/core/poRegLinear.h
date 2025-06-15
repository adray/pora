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
        inline const int allocateSlot(const int variable, const int size);
        inline const void freeSlot(const int variable);
        inline const int findSlot(const int variable) const;

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

    //class poRegRestore
    //{
    //public:

    //private:

    //};

    class poRegLinear
    {
    public:
        poRegLinear(poModule& module);
        void setNumRegisters(const int numRegisters);
        void setVolatile(const int reg, const bool isVolatile);
        void setType(const int reg, const poRegType type);
        void allocateRegisters(poFlowGraph& cfg);
        int getRegisterByVariable(const int variable) const;
        const bool spillAt(const int index, poRegSpill* spill) const;
        const int getStackSlot(const int pos) const;
        const int getStackSlotByVariable(const int pos) const;

        inline const int getRegister(const int index) const { return _registers[index]; }
        inline const int numRegisters() const { return _numRegisters; }
        inline const int numInstructions() const { return int(_registers.size()); }
        inline const bool isRegisterSet(const int index) const { return _registersSet[index]; }
        inline const bool isVolatile(const int index) const { return _volatile[index]; }
        inline const int stackSize() const { return _stackAlloc.numSlots(); }
        //inline const poStackAllocator& getStackAllocator() const { return _stackAlloc; }

    private:
        void allocateRegister(const int pos, const poRegType type, const int live, const int variable);
        void freeRegisters(const int pos);
        void spillRegisters(const int pos, const poRegType type, const poInstruction& ins, poUses& uses);

        poStackAllocator _stackAlloc;
        poModule& _module;

        int _numRegisters;
        std::vector<bool> _volatile;
        std::vector<poRegType> _type;
        std::vector<int> _variables; /* a list indicating which register is used by which variable (register -> variable) */
        std::vector<int> _registers; /* a mapping from register -> (last) instruction index */
        std::vector<bool> _registerUsed;
        std::vector<int> _registerExpiry;
        std::vector<int> _registersUsedByType;
        std::vector<int> _maxRegistersUsedByType;
        std::vector<int> _registersSet; /* the registers which have been used at any point */
        std::unordered_map<int, poRegSpill> _spills; /* a mapping from instruction index -> spill */
        std::unordered_map<int, int> _registerMap; /* mapping from variable -> register  */
        std::unordered_map<int, int> _stackSlots; /* instruction -> stack slot */
    };
}

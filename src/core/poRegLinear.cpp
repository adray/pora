#include "poRegLinear.h"
#include "poCFG.h"
#include "poLive.h"
#include "poUses.h"
#include "poSSA.h"
#include "poType.h"
#include "poModule.h"

#include <assert.h>

using namespace po;


//====================
// poAsmStackAllocator
//====================

poStackAllocator::poStackAllocator()
    :
    _numSlots(0)
{
}

inline const int poStackAllocator::allocateSlot(const int variable, const int size)
{
    const int numSlots = size / 8;

    // Sliding window algorithm: we start with a window the size of the num slots AAABBAAA
    // where A=free slot B=used slot
    // Then we keep a count of the number used or free. If doesn't match what we need, we remove the first time
    // and add the next item to the end. If there are no more new items we need to insert a new set at the end.
    // We can further optimize it, by keeping track of the last 'used' slot, and then we can insert less items.
    //

    int start = 0;
    int end = numSlots;
    int numUsed = 0;
    int slot = -1;

    if (numSlots <= _numSlots)
    {
        for (int i = start; i < end; i++)
        {
            if (_slots[i])
            {
                numUsed++;
            }
        }

        while (end < _numSlots)
        {
            if (numUsed == 0)
            {
                break;
            }

            if (_slots[start])
            {
                numUsed--;
            }

            start++;
            end++;

            if (_slots[start])
            {
                numUsed++;
            }
        }

        if (numUsed == 0)
        {
            slot = start;
            for (int i = start; i < end; i++)
            {
                _slots[slot] = true;
            }
            _occupancy.insert(std::pair<int, int>(variable, slot));
        }
    }

    if (slot == -1)
    {
        slot = int(_slots.size());
        for (int i = 0; i < numSlots; i++)
        {
            _slots.push_back(true);
        }
        _occupancy.insert(std::pair<int, int>(variable, slot));
        _numSlots += numSlots;
    }
    return slot;
}
inline const void poStackAllocator::freeSlot(const int variable)
{
    const auto& it = _occupancy.find(variable);
    if (it != _occupancy.end())
    {
        _slots[it->second] = false;
        _occupancy.erase(it);
    }
}
inline const int poStackAllocator::findSlot(const int variable) const
{
    int index = -1;
    const auto& it = _occupancy.find(variable);
    if (it != _occupancy.end())
    {
        index = it->second;
    }
    return index;
}

//
// poRegSpill
//

poRegSpill::poRegSpill()
    :
    _spillRegister(0),
    _spillVariable(0),
    _spillStackSlot(0)
{
}

poRegSpill::poRegSpill(const int spillRegister, const int spillVariable, const int spillStackSlot)
    :
    _spillRegister(spillRegister),
    _spillVariable(spillVariable),
    _spillStackSlot(spillStackSlot)
{
}


//
// poRegLinear
//


poRegLinear::poRegLinear(poModule& module)
    :
    _numRegisters(0),
    _module(module)
{
}

void poRegLinear::setNumRegisters(const int numRegisters)
{
    _numRegisters = numRegisters;
    _volatile.resize(_numRegisters);
    _type.resize(_numRegisters);
    _registerUsed.resize(_numRegisters);
    _registerExpiry.resize(_numRegisters);
    _registersSet.resize(_numRegisters);
    _variables.resize(_numRegisters);
}

void poRegLinear::setVolatile(const int reg, const bool isVolatile)
{
    if (reg < 0 || reg >= _numRegisters)
    {
        return;
    }

    _volatile[reg] = isVolatile;
}

void poRegLinear::setType(const int reg, const poRegType type)
{
    if (reg < 0 || reg >= _numRegisters)
    {
        return;
    }

    _type[reg] = type;
}

void poRegLinear::spillRegisters(const int pos, const poRegType type, const poInstruction& ins, poUses& uses)
{
    std::vector<int> toSpill;

    if (ins.left() != -1)
    {
        const int slot = _stackAlloc.findSlot(ins.left());
        if (slot != -1)
        {
            toSpill.push_back(ins.left());
        }
    }

    if (ins.right() != -1)
    {
        const int slot = _stackAlloc.findSlot(ins.right());
        if (slot != -1)
        {
            toSpill.push_back(ins.right());
        }
    }

    const int variable = ins.name();
    const auto& foundRegister = _registerMap.find(variable);
    const int usedVariable = foundRegister == _registerMap.end() ? -1 : _variables[_registerMap[variable]];

    if (usedVariable != variable && /* we only need to spill if the variable used is not already in a register */
        _registersUsedByType[int(type)] == _maxRegistersUsedByType[int(type)])
    {
        toSpill.push_back(variable);
    }

    for (int variable : toSpill)
    {
        // We need to evict a register; ideally the one which is next used furthest away

        int variableToSpill = -1;
        int registerToSpill = -1;
        int longestDistance = -1;

        for (int i = 0; i < _numRegisters; i++)
        {
            if (_registerUsed[i] && _type[i] == type)
            {
                // Search for the next use

                const int distance = uses.findNextUse(_variables[i], pos) - pos;

                if (distance > longestDistance)
                {
                    longestDistance = distance;
                    registerToSpill = i;
                    variableToSpill = _variables[i];
                }
            }
        }

        if (registerToSpill != -1)
        {
            _registerUsed[registerToSpill] = false;
            _registersUsedByType[int(type)]--;

            const int stackSlot = _stackAlloc.allocateSlot(variable, 8);
            _spills.insert(std::pair<int, poRegSpill>(pos, poRegSpill(registerToSpill, variableToSpill, stackSlot)));
            _stackSlots.insert(std::pair<int, int>(pos, stackSlot));
        }
    }
}

void poRegLinear::freeRegisters(const int pos)
{
    // Free any registers which are no longer live
    
    for (int i = 0; i < _numRegisters; i++)
    {
        // Skip volatile registers.
        if (_volatile[i])
        {
            continue;
        }

        // Check if the live range has expired
        if (_registerUsed[i] && _registerExpiry[i] < pos)
        {
            _registerUsed[i] = false;
            _registersUsedByType[(int)_type[i]]--;
            // TODO: can possibly free up a stack slot if used? not sure if it is safe if a pointer is taken to it for instance, or worthwhile?
        }
    }
}

void poRegLinear::allocateRegister(const int pos, const poRegType type, const int live, const int variable)
{
    // If the variable already exists, extend the live range
    const auto& foundRegister = _registerMap.find(variable);
    if (foundRegister != _registerMap.end())
    {
        _registerUsed[foundRegister->second] = true;
        _registerExpiry[foundRegister->second] = std::max(
            _registerExpiry[foundRegister->second],
            pos + live);
        _registers.push_back(foundRegister->second);
        return;
    }

    for (int i = 0; i < _numRegisters; i++)
    {
        // Do not attempt to use volatile registers.
        if (_volatile[i])
        {
            continue;
        }

        // Check for the correct register type.
        if (_type[i] != type)
        {
            continue;
        }

        // Use the register if it is available.
        if (!_registerUsed[i])
        {
            _registersUsedByType[(int)type]++;
            _registerUsed[i] = true;
            _registersSet[i] = true;
            _registerExpiry[i] = pos + live;
            _variables[i] = variable;
            _registers.push_back(i);
            _registerMap.insert(std::pair<int, int>(variable, i));
            break;
        }
    }
}

void poRegLinear::allocateRegisters(poFlowGraph& cfg)
{
    // Compute live range.
    poLiveRange liveRange;
    liveRange.compute(cfg);

    // Perform SSA destruction
    poSSA_Destruction ssa;
    ssa.destruct(cfg);

    // Compute uses.
    poUses uses;
    uses.analyze(cfg);

    // Setup
    _registersUsedByType.resize(int(poRegType::MAX));
    _maxRegistersUsedByType.resize(int(poRegType::MAX));

    for (int i = 0; i < _numRegisters; i++)
    {
        if (_volatile[i])
        {
            continue;
        }

        _maxRegistersUsedByType[int(_type[i])]++;
    }

    // Allocate the registers
    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        auto& instructions = bb->instructions();
        for (int i = 0; i < int(instructions.size()); i++)
        {
            const poInstruction& ins = instructions[i];
            const int live = liveRange.getLiveRange(pos);

            freeRegisters(pos);

            switch (ins.code())
            {
            case IR_RETURN:
            case IR_ARG:
            case IR_BR:
            case IR_CMP:
                _registers.push_back(0);
                pos++;
                continue;
            default:
                break;
            }

            if (ins.code() == IR_ALLOCA)
            {
                _registers.push_back(0);
                pos++;

                const poType& type = _module.types()[ins.type()];
                assert(type.isPointer() || type.isArray());

                const poType& baseType = _module.types()[type.baseType()];

                const int elements = ins.left();
                const int size = _module.types()[baseType.id()].size();
                _stackSlots.insert(std::pair<int, int>(pos, _stackAlloc.allocateSlot(ins.name(), size * elements)));
                continue;
            }

            poRegType type;
            switch (ins.type())
            {
            case TYPE_BOOLEAN:
            case TYPE_I64:
            case TYPE_I32:
            case TYPE_I8:
            case TYPE_U64:
            case TYPE_U32:
            case TYPE_U8:
                type = poRegType::General;
                break;
            case TYPE_F64:
            case TYPE_F32:
                type = poRegType::SSE;
                break;
            default:
                if (!_module.types()[ins.type()].isPointer())
                {
                    _registers.push_back(0);
                    pos++;
                    continue;
                }
                type = poRegType::General;
                break;
            }

            //spillRegisters(pos, type, ins, uses);
            allocateRegister(pos, type, live, ins.name());

            pos++;
        }

        bb = bb->getNext();
    }
}

int poRegLinear::getRegisterByVariable(const int variable) const
{
    const auto& it = _registerMap.find(variable);
    if (it != _registerMap.end())
    {
        return it->second;
    }
    return -1;
}

const bool poRegLinear::spillAt(const int index, poRegSpill* spill) const
{
    const auto& it = _spills.find(index);
    if (it != _spills.end())
    {
        *spill = it->second;
        return true;
    }
    return false;
}

const int poRegLinear::getStackSlotByVariable(const int variable) const
{
    return _stackAlloc.findSlot(variable);
}

const int poRegLinear::getStackSlot(const int pos) const
{
    const auto& it = _stackSlots.find(pos);
    if (it != _stackSlots.end())
    {
        return it->second;
    }

    return -1;
}


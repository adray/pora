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

const int poStackAllocator::allocateSlot(const int variable, const int size)
{
    const int remainder = size % 8;
    const int numSlots = (size / 8) + (remainder > 0 ? 1 : 0);

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
const void poStackAllocator::freeSlot(const int variable)
{
    const auto& it = _occupancy.find(variable);
    if (it != _occupancy.end())
    {
        _slots[it->second] = false;
        _occupancy.erase(it);
    }
}
const int poStackAllocator::findSlot(const int variable) const
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
// poRegRestore
//

poRegRestore::poRegRestore()
    :
    _restoreRegister(0),
    _restoreVariable(0),
    _restoreStackSlot(0)
{
}

poRegRestore::poRegRestore(const int restoreRegister, const int restoreVariable, const int restoreStackSlot)
    :
    _restoreRegister(restoreRegister),
    _restoreVariable(restoreVariable),
    _restoreStackSlot(restoreStackSlot)
{
}

//
// poRegLinearEntry
//

poRegLinearEntry::poRegLinearEntry(const std::vector<int>& variables,
    const std::vector<bool>& registerUsed,
    const std::vector<int>& registersUsedByType,
    const int pos)
    :
    _variables(variables),
    _registerUsed(registerUsed),
    _registersUsedByType(registersUsedByType),
    _pos(pos)
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
    // This function looks at the left and right operands of the instruction and spills registers to make room to restore the registers (if they are required).
    // If the instruction is a variable assignment, it will need to spill a register to make space for the new variable.
    //

    std::vector<int> spillTypes(int(poRegType::MAX), 0);

    if (ins.left() != -1 && !ins.isSpecialInstruction())
    {
        const int slot = _stackAlloc.findSlot(ins.left());
        if (slot != -1 && _stackSlots.find(slot) != _stackSlots.end())
        {
            spillTypes[int(_stackSlots[slot].type())]++;
        }
    }

    if (ins.right() != -1 &&
        ins.left() != ins.right() &&
        !ins.isSpecialInstruction())
    {
        const int slot = _stackAlloc.findSlot(ins.right());
        if (slot != -1 && _stackSlots.find(slot) != _stackSlots.end())
        {
            spillTypes[int(_stackSlots[slot].type())]++;
        }
    }

    if (ins.code() != IR_ARG &&
        ins.code() != IR_CMP)
    {
        const int variable = ins.name();
        const auto& foundRegister = _registerMap.find(variable);
        const int usedVariable = foundRegister == _registerMap.end() ? -1 : _variables[_registerMap[variable]];

        if (usedVariable != variable)  /* we only need to spill if the variable used is not already in a register */
        {
            spillTypes[int(type)]++;
        }
    }

    for (size_t j = 0; j < spillTypes.size(); j++)
    {
        const poRegType type = (poRegType)j;
        const int numAvailable = _maxRegistersUsedByType[j] - _registersUsedByType[j];
        const int numToSpill = spillTypes[j] - numAvailable;

        for (int k = 0; k < numToSpill; k++)
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

                    const int nextUse = uses.findNextUse(_variables[i], pos);
                    if (nextUse == -1 || nextUse == pos)
                    {
                        // If it is -1 then it MAY be live during a loop, so we opt to not spill it.
                        // If it is pos, then it is used in this instruction, so we can't spill it.
                        continue;
                    }

                    const int distance = nextUse - pos;

                    if (distance > longestDistance)
                    {
                        longestDistance = distance;
                        registerToSpill = i;
                        variableToSpill = _variables[i];
                    }
                }
            }

            if (registerToSpill == -1)
            {
                // Fallback: spill any register of the correct type
                for (int i = 0; i < _numRegisters; i++)
                {
                    if (_registerUsed[i] && _type[i] == type)
                    {
                        // We can't spill a register we are about to restore
                        const int nextUse = uses.findNextUse(_variables[i], pos);
                        if (nextUse == pos)
                        {
                            continue;
                        }

                        registerToSpill = i;
                        variableToSpill = _variables[i];
                        break;
                    }
                }
            }

            assert(registerToSpill != -1);
            const poRegType spillType = _type[registerToSpill];
            _registerUsed[registerToSpill] = false;
            _registersUsedByType[int(spillType)]--;

            int stackSlot = _stackAlloc.findSlot(variableToSpill);
            if (stackSlot == -1)
            {
                stackSlot = _stackAlloc.allocateSlot(variableToSpill, 8); // FIXME: should be size of type
            }

            spill(pos, poRegSpill(registerToSpill, variableToSpill, stackSlot));
            _stackSlots.insert(std::pair<int, poStackSlot>(stackSlot, poStackSlot(spillType, _registerExpiry[registerToSpill])));
        }
    }

    if (ins.left() != -1 &&
        !ins.isSpecialInstruction())
    {
        const int slot = _stackAlloc.findSlot(ins.left());
        if (slot != -1 && _stackSlots.find(slot) != _stackSlots.end())
        {
            const auto& stackSlot = _stackSlots[slot];

            allocateRegister(pos, stackSlot.type(), stackSlot.live() - pos, ins.left());
            restore(pos, poRegRestore(
                _registerMap[ins.left()],
                ins.left(),
                slot));
            _stackSlots.erase(slot);
        }
    }

    if (ins.right() != -1 &&
        ins.left() != ins.right() &&
        !ins.isSpecialInstruction())
    {
        const int slot = _stackAlloc.findSlot(ins.right());
        if (slot != -1 && _stackSlots.find(slot) != _stackSlots.end())
        {
            const auto& stackSlot = _stackSlots[slot];

            allocateRegister(pos, stackSlot.type(), stackSlot.live() - pos, ins.right());
            restore(pos, poRegRestore(
                _registerMap[ins.right()],
                ins.right(),
                slot));
            _stackSlots.erase(slot);
        }
    }
}

void poRegLinear::balanceRegisters(const int pos, poBasicBlock* bb)
{
    // We need to look at our basic block successors and see if we have visited them already,
    // then we need to make sure that the variable stored in registers and stack slots
    // match what is expected in the successor basic block.
    //
    // Example:
    // Current basic block: [R1=A, R2=B, R3=C] [S1=D]
    // Successor basic block: [R1=D, R2=B, R3=C] [S1=A]
    // In this case we need to spill A to a stack slot and restore D from the stack slot.

    if (bb->getBranch())
    {
        const auto& it = _entries.find(bb->getBranch());
        if (it == _entries.end())
        {
            return;
        }

        const poRegLinearEntry& entry = it->second;

        // First we need to spill any registers which are not the same as the successor basic block
        for (int i = 0; i < _numRegisters; i++)
        {
            // Skip volatile registers.
            if (_volatile[i])
            {
                continue;
            }

            if (!_registerUsed[i])
            {
                continue;
            }

            if (entry.getRegister(i) == _variables[i])
            {
                continue;
            }

            // The register is not the same as the one we have in the successor basic block,
            // so we need to spill it.

            const int variable = _variables[i];
            int slot = _stackAlloc.findSlot(variable);
            if (slot == -1)
            {
                slot = _stackAlloc.allocateSlot(variable, 8);
            }
            spill(pos, poRegSpill(i, variable, slot));
            _stackSlots.insert(std::pair<int, poStackSlot>(slot, poStackSlot(_type[i], _registerExpiry[i])));
            _registerUsed[i] = false;
            _registersUsedByType[int(_type[i])]--;
        }

        // Now we need to restore any registers which are not the same as the successor basic block
        for (int i = 0; i < _numRegisters; i++)
        {
            // Skip volatile registers.
            if (_volatile[i])
            {
                continue;
            }

            if (!entry.isUsed(i))
            {
                continue;
            }

            if (entry.getRegister(i) == _variables[i])
            {
                continue;
            }

            // Restore the register if it is not already set

            const int variable = entry.getRegister(i);
            const int slot = getStackSlotByVariable(variable);

            if (slot != -1)
            {
                const poStackSlot& stackSlot = _stackSlots[slot];

                assert(!_registerUsed[i]);

                _registersUsedByType[(int)stackSlot.type()]++;
                _registerUsed[i] = true;
                _registersSet[i] = true;
                _registerExpiry[i] = stackSlot.live();
                _variables[i] = variable;
                mapRegister(poRegLinearAssignment(pos, stackSlot.live(), variable, i));
                _registerMap.insert(std::pair<int, int>(variable, i));

                restore(pos, poRegRestore(
                    _registerMap[variable],
                    variable,
                    slot));
                _stackSlots.erase(slot);
            }
        }
    }
}

void poRegLinear::repairSpills(poBasicBlock* bb)
{
    // Examine the next basic block for its predecessors (only check what has already been visited)
    // and make sure that the registers and stack slots match.

    poBasicBlock* next = bb->getNext();
    if (!next)
    {
        return;
    }

    std::vector<poBasicBlock*> preds = next->getIncoming();
    preds.push_back(bb);

    // First we need to gather all the stack slots which are live in the predecessor basic blocks

    for (poBasicBlock* bb : next->getIncoming())
    {
        const auto& it = _exits.find(bb);
        if (it == _exits.end())
        {
            continue;
        }

        for (const auto& slot : it->second.stackSlots())
        {
            if (_stackSlots.find(slot.first) != _stackSlots.end())
            {
                continue;
            }
            _stackSlots.insert(slot);
        }
    }

    for (poBasicBlock* pred : preds)
    {
        poBasicBlock* next = pred->getNext();

        const auto& it = _entries.find(next);
        if (it == _entries.end())
        {
            continue;
        }

        const auto& it2 = _exits.find(pred);
        if (it2 == _exits.end())
        {
            continue;
        }

        const poRegLinearEntry& entry = it->second;
        poRegLinearExit& exit = it2->second;
        for (int i = 0; i < _numRegisters; i++)
        {
            // Skip volatile registers.
            if (_volatile[i])
            {
                continue;
            }
            if (!entry.isUsed(i))
            {
                continue;
            }

            const int variable = entry.getRegister(i);
            int slot = getStackSlotByVariable(variable);
            if (slot == -1)
            {
                slot = _stackAlloc.allocateSlot(variable, 8);
            }

            if (exit.stackSlots().find(slot) == exit.stackSlots().end() &&
                _stackSlots.find(slot) != _stackSlots.end())
            {
                spill(entry.position() - 1, poRegSpill(i, variable, slot));
                exit.addStackSlot(slot, poStackSlot(_type[i], _registerExpiry[i]));

                // TODO: we may need to remove the register assignment
            }
        }
    }
}

void poRegLinear::spill(const int pos, const poRegSpill& spill)
{
    const auto& it = _spills.find(pos);
    if (it != _spills.end())
    {
        _spills[pos].push_back(spill);
    }
    else
    {
        _spills.insert(std::pair<int, std::vector<poRegSpill>>(pos, std::vector<poRegSpill>(1, spill)));
    }
}

void poRegLinear::restore(const int pos, const poRegRestore& restore)
{
    const auto& it = _restores.find(pos);
    if (it != _restores.end())
    {
        _restores[pos].push_back(restore);
    }
    else
    {
        _restores.insert(std::pair<int, std::vector<poRegRestore>>(pos, std::vector<poRegRestore>(1, restore)));
    }
}

void poRegLinear::mapRegister(const poRegLinearAssignment& assignment)
{
    const auto& it = _registers.find(assignment.variable());
    if (it != _registers.end())
    {
        it->second.push_back(assignment);
    }
    else
    {
        _registers.insert(std::pair<int, std::vector<poRegLinearAssignment>>(assignment.variable(), std::vector<poRegLinearAssignment>(1, assignment)));
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

void poRegLinear::freeStackSlots(const int pos)
{
    // Free any stack slots which are no longer live

    std::vector<int> toFree;
    for (const auto& slot : _stackSlots)
    {
        if (slot.second.live() < pos)
        {
            toFree.push_back(slot.first);
        }
    }

    for (const int slot : toFree)
    {
        _stackSlots.erase(slot);
    }
}

void poRegLinear::allocateRegister(const int pos, const poRegType type, const int live, const int variable)
{
    // If the variable already exists, extend the live range
    const auto& foundRegister = _registerMap.find(variable);
    if (foundRegister != _registerMap.end() &&
        (!_registerUsed[foundRegister->second] || _variables[foundRegister->second] == variable))
    {
        if (!_registerUsed[foundRegister->second])
        {
            assert(_registersSet[foundRegister->second]);
            _registerUsed[foundRegister->second] = true;
            _variables[foundRegister->second] = variable;
            _registersUsedByType[(int)type]++;
        }
        _registerExpiry[foundRegister->second] = std::max(
            _registerExpiry[foundRegister->second],
            pos + live);
        mapRegister(poRegLinearAssignment(pos, pos + live, variable, foundRegister->second));
        return;
    }

    bool allocated = false;
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
            mapRegister(poRegLinearAssignment(pos, pos + live, variable, i));

            if (foundRegister != _registerMap.end())
            {
                _registerMap.erase(foundRegister);
            }
            _registerMap.insert(std::pair<int, int>(variable, i));
            allocated = true;

            const int slot = _stackAlloc.findSlot(variable);
            if (slot != -1)
            {
                _stackSlots.erase(slot);
            }

            break;
        }
    }

    if (!allocated)
    {
        abort();
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
        _entries.insert(std::pair<poBasicBlock*, poRegLinearEntry>(
            bb,
            poRegLinearEntry(_variables, _registerUsed, _registersUsedByType, pos)));

        auto& instructions = bb->instructions();
        for (int i = 0; i < int(instructions.size()); i++)
        {
            const poInstruction& ins = instructions[i];
            const int live = liveRange.getLiveRange(pos);

            freeRegisters(pos);
            freeStackSlots(pos);

            switch (ins.code())
            {
            case IR_BR:
                balanceRegisters(pos, bb);
                pos++;
                continue;
            case IR_ARG:
            case IR_CMP:
                spillRegisters(pos, poRegType::General, ins, uses);
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
                _stackAlloc.allocateSlot(ins.name(), size * elements);
                continue;
            }

            const int type_ = ins.type();
            poRegType type;
            switch (type_)
            {
            case TYPE_BOOLEAN:
            case TYPE_I64:
            case TYPE_I32:
            case TYPE_I16:
            case TYPE_I8:
            case TYPE_U64:
            case TYPE_U32:
            case TYPE_U16:
            case TYPE_U8:
                type = poRegType::General;
                break;
            case TYPE_F64:
            case TYPE_F32:
                type = poRegType::SSE;
                break;
            default:
                if (!_module.types()[type_].isPointer())
                {
                    pos++;
                    continue;
                }
                type = poRegType::General;
                break;
            }

            spillRegisters(pos, type, ins, uses);
            allocateRegister(pos, type, live, ins.name());

            pos++;
        }

        _exits.insert(std::pair<poBasicBlock*, poRegLinearExit>(
            bb,
            poRegLinearExit(_stackSlots)));

        repairSpills(bb);
        bb = bb->getNext();
    }
}

int poRegLinear::getRegisterByVariable(const int variable, const int pos) const
{
    const auto& it = _registers.find(variable);

    if (it != _registers.end())
    {
        for (int i = int(it->second.size()) - 1; i >= 0; i--)
        {
            const poRegLinearAssignment& assign = it->second[i];
            if (pos >= assign.pos() && pos <= assign.live())
            {
                return assign.reg();
            }
        }

        abort();
    }

    return -1;
}

int poRegLinear::getRegisterByVariable(const int variable) const
{
    return getRegisterByVariable(variable, _iterator.position());
}

const bool poRegLinear::spillAt(const int index, const int element, poRegSpill* spill) const
{
    const auto& it = _spills.find(index);
    if (it != _spills.end() &&
        it->second.size() > element)
    {
        *spill = it->second[element];
        return true;
    }
    return false;
}

const bool poRegLinear::restoreAt(const int index, const int element, poRegRestore* spill) const
{
    const auto& it = _restores.find(index);
    if (it != _restores.end() &&
        it->second.size() > element)
    {
        *spill = it->second[element];
        return true;
    }
    return false;
}

const int poRegLinear::getStackSlotByVariable(const int variable) const
{
    return _stackAlloc.findSlot(variable);
}




#include "poRegLinear.h"
#include "poCFG.h"
#include "poLive.h"
#include "poTypeChecker.h"

using namespace po;

void poRegLinear::setNumRegisters(const int numRegisters)
{
    _numRegisters = numRegisters;
    _volatile.resize(_numRegisters);
    _type.resize(_numRegisters);
    _registerUsed.resize(_numRegisters);
    _registerExpiry.resize(_numRegisters);
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

void poRegLinear::allocateRegister(const int pos, const poRegType type, const int live, const int variable)
{
    // If the variable already exists, extend the leve range
    const auto& foundRegister = _registerMap.find(variable);
    if (foundRegister != _registerMap.end())
    {
        _registerUsed[foundRegister->second] = true;
        _registerExpiry[foundRegister->second] = pos + live;
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
            _registerUsed[i] = true;
            _registerExpiry[i] = pos + live;
            _registers.push_back(i);
            _registerMap.insert(std::pair<int, int>(variable, i));
            break;
        }

        // Otherwise check if its live range has expired.
        if (_registerExpiry[i] < pos)
        {
            _registerUsed[i] = true;
            _registerExpiry[i] = pos + live;
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
                _registers.push_back(0);
                pos++;
                continue;
            }

            allocateRegister(pos, type, live, ins.name());

            pos++;
        }

        bb = bb->getNext();
    }
}

int poRegLinear::getRegisterByVariable(const int variable)
{
    const auto& it = _registerMap.find(variable);
    if (it != _registerMap.end())
    {
        return it->second;
    }
    return -1;
}

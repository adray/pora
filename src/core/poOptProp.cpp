#include "poOptProp.h"
#include "poModule.h"
#include "poDom.h"

#include <assert.h>

using namespace po;

void poOptProp:: optimize(poModule& module)
{
    for (auto& function : module.functions())
    {
        if (function.hasAttribute(poAttributes::EXTERN) ||
            function.hasAttribute(poAttributes::GENERIC))
        {
            continue;
        }

        optimize(module, function);
    }
}

void poOptProp:: optimize(poModule& module, poFunction& function)
{
    _constants.clear();
    _blocksToRemove.clear();

    poDom dom;
    dom.compute(function.cfg());

    int instructionId = 0;
    for (poBasicBlock* bb = function.cfg().getFirst(); bb != nullptr; bb = bb->getNext())
    {
        for (int i = 0; i < int(bb->instructions().size()); i++)
        {
            poInstruction& ins = bb->getInstruction(i);
            if (ins.code() == IR_CONSTANT)
            {
                _constants.insert(std::pair<int, poInstructionRef>(ins.name(), poInstructionRef(bb, i + instructionId, instructionId)));
            }
        }
        instructionId += int(bb->instructions().size());
    }

    for (int  i = dom.start(); i < dom.num(); i++)
    {
        poBasicBlock* bb = dom.get(i).getBasicBlock();
        for (int j = 0; j < int(bb->instructions().size()); j++)
        {
            poInstruction& ins = bb->getInstruction(j);
            switch (ins.code())
            {
                case IR_ADD:
                case IR_SUB:
                case IR_MUL:
                case IR_DIV:
                    propagate(module, ins);
                    break;
                case IR_CMP:
                    eliminateBranch(module, ins, i, dom);
                    break;
            }
        }
    }

    // Remove the marked blocks.
    for (poBasicBlock* bb : _blocksToRemove)
    {
        function.cfg().removeBasicBlock(bb);
    }

    // Remove the marked instructions.
    for (poBasicBlock* bb = function.cfg().getFirst(); bb != nullptr; bb = bb->getNext())
    {
        for (int i = 0; i < int(bb->instructions().size()); i++)
        {
            poInstruction& ins = bb->getInstruction(i);
            if (ins.name() == -1)
            {
                bb->removeInstruction(i);
                i--;
            }
        }
    }

    function.cfg().optimize();
}

void poOptProp::eliminateBranch(poModule& module, poInstruction& ins, int nodeId, poDom& dom)
{
    // If the compare can be evaluated statically we can eliminate the branch.

    const auto& leftIt = _constants.find(ins.left());
    const auto& rightIt = _constants.find(ins.right());

    if (leftIt == _constants.end() || rightIt == _constants.end())
    {
        return;
    }

    poConstantPool& constants = module.constants();

    auto& leftIns = leftIt->second.getInstruction();
    auto& rightIns = rightIt->second.getInstruction();

    int64_t result = 0;
    switch (ins.type())
    {
    case TYPE_I64:
        result = constants.getI64(leftIns.constant()) - constants.getI64(rightIns.constant());
        break;
    case TYPE_I32:
        result = constants.getI32(leftIns.constant()) - constants.getI32(rightIns.constant());
        break;
    default:
        return;
    }

    poBasicBlock* bb = dom.get(nodeId).getBasicBlock();

    poInstruction& br = bb->getInstruction(int(bb->numInstructions()) - 1);
    assert(br.code() == IR_BR);

    poBasicBlock* blockToRemove = nullptr;

    switch (br.left())
    {
        case IR_JUMP_EQUALS:
            if (result == 0)
            {
                blockToRemove = bb->getBranch();
            }
            else
            {
                blockToRemove = bb->getNext();
            }
            break;
        case IR_JUMP_GREATER:
            if (result <= 0)
            {
                blockToRemove = bb->getBranch();
            }
            else
            {
                blockToRemove = bb->getNext();
            }
            break;
        case IR_JUMP_GREATER_EQUALS:
            if (result < 0)
            {
                blockToRemove = bb->getBranch();
            }
            else
            {
                blockToRemove = bb->getNext();
            }
            break;
        case IR_JUMP_LESS:
            if (result > 0)
            {
                blockToRemove = bb->getBranch();
            }
            else
            {
                blockToRemove = bb->getNext();
            }
            break;
        case IR_JUMP_LESS_EQUALS:
            if (result >= 0)
            {
                blockToRemove = bb->getBranch();
            }
            else
            {
                blockToRemove = bb->getNext();
            }
            break;
    }

    if (blockToRemove == nullptr)
    {
        return;
    }

    if (blockToRemove == bb->getBranch())
    {
        // Mark the instructions for deletion.
        br.setName(-1);
        ins.setName(-1);
        bb->setBranch(nullptr, false);
        blockToRemove->removeIncoming(bb);
    }
    else
    {
        // Branch to the alternate
        br.setLeft(IR_JUMP_UNCONDITIONAL);
        bb->setBranch(bb->getBranch(), true);
    }

    // Don't remove the block if it has other ways to be reached.
    if (blockToRemove->getIncoming().size() >= 1)
    {
        return;
    }

    // Examine PHI nodes in the iterated dominance frontier. This is where
    // PHIs could be (as opposed to trying every basic block).

    const std::vector<int>& succs = dom.get(nodeId).successors();

    int succ_id = -1;
    for (const int i : succs) {
        if (dom.get(i).getBasicBlock() == blockToRemove)
        {
            succ_id = i;
            break;
        }
    }

    std::unordered_set<int> iteratedDF;
    dom.iteratedDominanceFrontier(std::vector<int>({ succ_id }), iteratedDF);

    for (const int i : iteratedDF)
    {
        poBasicBlock* successor = dom.get(i).getBasicBlock();
        if (successor->phis().size() > size_t(0)) {
            fixUpPhis(blockToRemove, successor);
        }
    }

    _blocksToRemove.push_back(blockToRemove);
}

void poOptProp:: fixUpPhis(poBasicBlock* bb, poBasicBlock* successor)
{
    std::vector<poInstruction> copies;
    std::vector<poPhi> phiCopy = successor->phis();
    successor->phis().clear();
    for (const poPhi& phi : phiCopy)
    {
        poPhi newPhi(phi.name(), phi.getType());
        for (int j = 0; j < int(phi.values().size()); j++)
        {
            if (phi.getBasicBlock()[j] != bb)
            {
                newPhi.addValue(phi.values()[j], phi.getBasicBlock()[j]);
            }
        }

        if (newPhi.values().size() > size_t(1)) {
            successor->phis().push_back(newPhi);
        }
        else {
            copies.push_back(poInstruction(newPhi.name(), newPhi.getType(), newPhi.values()[0], -1, IR_COPY));
        }
    }

    // Remove PHI instructions
    std::vector<int> freeNames;
    bool cont = true;
    while (cont)
    {
        poInstruction& ins = successor->getInstruction(0);
        if (ins.code() == IR_PHI)
        {
            freeNames.push_back(ins.name());
            successor->removeInstruction(0);
            cont = successor->numInstructions() > size_t(0);
        }
        else
        {
            cont = false;
        }
    }

    // Regenerate PHI nodes for the successor block.
    int numPhis = 0;
    int namePos = 0;
    for (poPhi& phi : successor->phis())
    {
        const int numValues = int(phi.values().size());

        if (numValues == 2)
        {
            poInstruction phiIns(phi.name(), phi.getType(), phi.values()[0], phi.values()[1], IR_PHI);
            successor->insertInstruction(phiIns, numPhis++);
        }
        else
        {
            // TODO: this usage of the free list needs more work

            int name = phi.values()[0];
            for (int i = 1; i < numValues - 1; i++)
            {
                const int newName = freeNames[namePos++];
                poInstruction phiIns(newName, phi.getType(), phi.values()[i], name, IR_PHI);
                successor->insertInstruction(phiIns, numPhis++);
                name = newName;
            }

            poInstruction phiIns(phi.name(), phi.getType(), name, phi.values()[numValues - 1], IR_PHI);
            successor->insertInstruction(phiIns, numPhis++);
        }
    }

    for (poInstruction& ins : copies) {
        successor->insertInstruction(ins, numPhis);
    }
}

template<typename T>
T fold(T left, T right, int code, bool& ok)
{
    T result = 0;
    switch (code)
    {
    case IR_ADD:
        result = left + right;
        ok = true;
        break;
    case IR_MUL:
        result = left * right;
        ok = true;
        break;
    case IR_SUB:
        result = left - right;
        ok = true;
        break;
    case IR_DIV:
        if (right == 0)
        {
            ok = false;
        }
        else
        {
            result = left / right;
            ok = true;
        }
        break;
    }
    return result;
}

void poOptProp::foldI64(poConstantPool& pool, poInstruction& ins, poInstruction& left, poInstruction& right)
{
    const int16_t leftValue = left.constant();
    const int16_t rightValue = right.constant();

    const int64_t leftConstant = pool.getI64(leftValue);
    const int64_t rightConstant = pool.getI64(rightValue);

    bool ok = false;
    const int64_t result = fold(leftConstant, rightConstant, ins.code(), ok);

    if (ok)
    {
        int id = pool.getConstant(result);
        if (id == -1)
        {
            id = pool.addConstant(result);
        }

        ins.setCode(IR_CONSTANT);
        ins.setConstant(int16_t(id));
    }
}

void poOptProp::foldI32(poConstantPool& pool, poInstruction& ins, poInstruction& left, poInstruction& right)
{
    const int16_t leftValue = left.constant();
    const int16_t rightValue = right.constant();

    const int32_t leftConstant = pool.getI32(leftValue);
    const int32_t rightConstant = pool.getI32(rightValue);

    bool ok = false;
    const int32_t result = fold(leftConstant, rightConstant, ins.code(), ok);

    if (ok)
    {
        int id = pool.getConstant(result);
        if (id == -1)
        {
            id = pool.addConstant(result);
        }

        ins.setCode(IR_CONSTANT);
        ins.setConstant(int16_t(id));
    }
}

void poOptProp::propagate(poModule& module, poInstruction& ins)
{
    const auto& leftIt = _constants.find(ins.left());
    const auto& rightIt = _constants.find(ins.right());

    if (leftIt == _constants.end() || rightIt == _constants.end())
    {
        return;
    }

    poConstantPool& constants = module.constants();

    auto& leftIns = leftIt->second.getInstruction();
    auto& rightIns = rightIt->second.getInstruction();

    switch (ins.type())
    {
    case TYPE_I64:
        foldI64(constants, ins, leftIns, rightIns);
        break;
    case TYPE_I32:
        foldI32(constants, ins, leftIns, rightIns);
        break;
    }
}


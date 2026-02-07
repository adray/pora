#include "poOptInline.h"
#include "poModule.h"
#include "poSSA.h"

#include <assert.h>
#include <iostream>

using namespace po;

constexpr int INLINE_THRESHOLD = 20;

void poOptInline::optimize(poModule& module)
{
    // We need to build a call graph and perform inlining in a bottom up manner.

    _graph.analyze(module);

    for (poFunction& func : module.functions())
    {
        if (!func.hasAttribute(poAttributes::EXTERN))
        {
            func.setCanInline(shouldInline(module, func));
        }
    }

    bool changed = true;
    std::vector<bool> visited(_graph.nodes().size(), false);

    while (changed)
    {
        changed = false;

        for (poCallGraphNode* node : _graph.nodes())
        {
            if (visited[node->id()])
            {
                continue;
            }

            if (node->isLeafNode())
            {
                poFunction& func = module.functions()[node->id()];
                visited[node->id()] = true;
                changed = true;
            }
            else
            {
                // If all the children of this node have been visited, we can process this node.

                bool allChildrenVisited = true;
                for (poCallGraphNode* child : node->children())
                {
                    if (!visited[child->id()] &&
                        child->sccId() != node->sccId())
                    {
                        allChildrenVisited = false;
                        break;
                    }
                }

                if (allChildrenVisited)
                {
                    poFunction& func = module.functions()[node->id()];
                    if (!func.hasAttribute(poAttributes::EXTERN))
                    {
                        optimize(module, func.cfg());
                    }
                    visited[node->id()] = true;
                    changed = true;
                }
            }
        }
    }
}

void poOptInline:: optimize(poModule& module, poFlowGraph& cfg)
{
    bool inlined = true;
    while (inlined)
    {
        inlined = false;

        for (poBasicBlock* bb = cfg.getFirst(); bb != nullptr; bb = bb->getNext())
        {
            for (int i = 0; i < int(bb->numInstructions()); i++)
            {
                poInstruction& ins = bb->getInstruction(i);

                if (ins.code() == IR_CALL && canInline(module, ins))
                {
                    const int numArguments = ins.left();
                    splitBasicBlock(bb, i + numArguments + 1, cfg);
                    inlineFunctionCall(ins, bb, module, cfg);

                    inlined = true;
                    break;
                }
            }

            if (inlined)
            {
                break;
            }
        }
    }
}

poBasicBlock* poOptInline::splitBasicBlock(poBasicBlock* bb, const int instructionIndex, poFlowGraph& cfg)
{
    poBasicBlock* newBB = new poBasicBlock();
    
    // Move instructions after instructionIndex to newBB
    for (int i = instructionIndex; i < int(bb->numInstructions()); i++)
    {
        newBB->addInstruction(bb->instructions()[i]);
    }
    
    // Remove moved instructions from original bb
    for (int i = int(bb->numInstructions()) - 1; i >= instructionIndex; i--)
    {
        bb->removeInstruction(i);
    }

    // Update control flow
    cfg.insertBasicBlock(bb, newBB);

    // Update branches
    if (bb->getBranch())
    {
        newBB->setBranch(bb->getBranch(), bb->unconditionalBranch());
        bb->setBranch(nullptr, false);
        newBB->getBranch()->addIncoming(newBB);
        newBB->getBranch()->removeIncoming(bb);
    }
    return newBB;
}

bool poOptInline::shouldInline(poModule& module, poFunction& function)
{
    // Heuristics are determined here (e.g., function size, etc.)

    int numInstructions = 0;
    for (const poBasicBlock* bb = function.cfg().getFirst(); bb != nullptr; bb = bb->getNext())
    {
        numInstructions += int(bb->numInstructions());
    }

    if (numInstructions > INLINE_THRESHOLD)
    {
        return false;
    }

    for (const poBasicBlock* bb = function.cfg().getFirst(); bb != nullptr; bb = bb->getNext())
    {
        for (int i = 0; i < int(bb->numInstructions()); i++)
        {
            const poInstruction& fIns = bb->instructions()[i];
            if (fIns.code() == IR_CALL)
            {
                const int symbolId = fIns.right();
                std::string functionName;
                if (!module.getSymbol(symbolId, functionName))
                {
                    return false;
                }

                if (functionName == function.fullname())
                {
                    return false; // Recursive call
                }
            }
        }
    }

    return true;
}

bool poOptInline::canInline(poModule& module, const poInstruction& ins)
{
    const int symbolId = ins.right();
    std::string functionName;
    if (!module.getSymbol(symbolId, functionName))
    {
        return false;
    }

    poCallGraphNode* node = _graph.findNodeByName(functionName);
    if (node == nullptr)
    {
        return false;
    }

    const poFunction& func = module.functions()[node->id()];
    if (func.hasAttribute(poAttributes::EXTERN))
    {
        return false;
    }

    return func.canInline();
}

void poOptInline::inlineFunctionCall(poInstruction& ins, poBasicBlock* bb, poModule& module, poFlowGraph& cfg)
{
    const int symbolId = ins.right();
    std::string functionName;
    if (!module.getSymbol(symbolId, functionName))
    {
        return;
    }

    poCallGraphNode* node = _graph.findNodeByName(functionName);
    if (node == nullptr)
    {
        return;
    }

    const poFunction& func = module.functions()[node->id()];

    // Map parameters to arguments
    poBasicBlock* callee = func.cfg().getFirst();
    std::vector<int> paramValues;
    for (int i = 0; i < int(callee->numInstructions()); i++)
    {
        const poInstruction& paramIns = callee->instructions()[i];
        if (paramIns.code() == IR_PARAM)
        {
            paramValues.push_back(paramIns.name());
        }
    }

    std::unordered_map<int, int> paramToArg;
    const int numArguments = ins.left();
    for (int i = 0; i < numArguments; i++)
    {
        const poInstruction& argIns = bb->instructions()[int(bb->numInstructions()) - numArguments + i];
        assert(argIns.code() == IR_ARG);
        paramToArg.insert(std::pair<int, int>(paramValues[i], argIns.left()));
    }

    const int REBASE_OFFSET = 10000;

    // Copy the basic blocks and instructions from the function into the caller
    std::vector<int> returnValues;
    std::vector<poBasicBlock*> returnBlocks;
    poBasicBlock* lastInsertedBB = bb->getNext();
    int maxName = 0;
    for (poBasicBlock* funcBB = callee; funcBB != nullptr; funcBB = funcBB->getNext())
    {
        poBasicBlock* newBB = new poBasicBlock();
        for (int i = 0; i < int(funcBB->numInstructions()); i++)
        {
            poInstruction funcIns = funcBB->instructions()[i];
            // Replace parameter references with argument values
            if (funcIns.code() == IR_PARAM)
            {
                continue; // Skip parameter instructions
            }

            if (!funcIns.isSpecialInstruction())
            {
                if (paramToArg.find(funcIns.left()) != paramToArg.end())
                {
                    funcIns.setLeft(paramToArg[funcIns.left()]);
                }
                else if (funcIns.left() != -1)
                {
                    funcIns.setLeft(funcIns.left() + REBASE_OFFSET);
                }
                if (paramToArg.find(funcIns.right()) != paramToArg.end())
                {
                    funcIns.setRight(paramToArg[funcIns.right()]);
                }
                else if (funcIns.right() != -1)
                {
                    funcIns.setRight(funcIns.right() + REBASE_OFFSET);
                }
            }

            // Rebase instruction names to avoid conflicts
            funcIns.setName(funcIns.name() + REBASE_OFFSET);
            maxName = std::max(maxName, funcIns.name());

            // Handle return instructions
            if (funcIns.code() == IR_RETURN)
            {
                if (funcIns.type() != TYPE_VOID)
                {
                    returnValues.push_back(funcIns.left());
                    returnBlocks.push_back(newBB);
                }

                // We need to branch to the block after the call.
                // If it is the last block, we can just not add a branch.

                if (!funcBB->getNext())
                {
                    continue;
                }
                
                funcIns.setCode(IR_BR);
                funcIns.setLeft(IR_JUMP_UNCONDITIONAL);
                funcIns.setRight(-1);
                newBB->setBranch(lastInsertedBB, true);
                lastInsertedBB->addIncoming(newBB);
            }

            newBB->addInstruction(funcIns);
        }

        // Handle phis
        for (poPhi& phi : funcBB->phis())
        {
            poPhi newPhi = phi;
            // Rebase phi names
            newPhi.setName(newPhi.name() + REBASE_OFFSET);
            for (int i = 0; i < int(newPhi.values().size()); i++)
            {
                newPhi.setValue(i, newPhi.values()[i] + REBASE_OFFSET);
            }
            newBB->addPhi(newPhi);
        }

        cfg.insertBasicBlock(lastInsertedBB->getPrev(), newBB);
    }

    // We need to update the basic blocks to have the correct branch targets.
    const int numBB = int(func.cfg().numBlocks());
    std::unordered_map<poBasicBlock*, int> blocks;
    for (int i = 0; i < numBB; i++)
    {
        blocks[func.cfg().getBasicBlock(i)] = i;
    }

    int inlineStartIndex = 1; // We start at 1 because the first block of the caller is not inlined
    for (poBasicBlock* funcBB = cfg.getFirst(); funcBB != bb; funcBB = funcBB->getNext())
    {
        inlineStartIndex++;
    }

    for (int i = 0; i < numBB; i++)
    {
        poBasicBlock* originalBB = func.cfg().getBasicBlock(i);

        if (originalBB->getBranch())
        {
            const int targetIndex = blocks[originalBB->getBranch()];

            poBasicBlock* newBB = cfg.getBasicBlock(i + inlineStartIndex);
            poBasicBlock* mappedBB = cfg.getBasicBlock(targetIndex + inlineStartIndex);
            newBB->setBranch(mappedBB, originalBB->unconditionalBranch());
            mappedBB->addIncoming(newBB);
        }
    }

    // After inlining, we need to handle the return values
    if (returnValues.size() >= 2)
    {
        poPhi phi(ins.name(), ins.type());
        for (int i = 0; i < int(returnValues.size()); i++)
        {
            phi.addValue(returnValues[i], returnBlocks[i]);
        }
        lastInsertedBB->addPhi(phi);

        int name = ins.name();
        poInstruction phiIns(name, ins.type(), returnValues[0], returnValues[1], IR_PHI);
        lastInsertedBB->insertInstruction(phiIns, 0);

        for (int i = 2; i < int(returnValues.size()); i++)
        {
            const int newName = maxName + 1;
            poInstruction phiIns(newName, ins.type(), returnValues[i], name, IR_PHI);
            lastInsertedBB->insertInstruction(phiIns, i - 1);
            name = newName;
        }
    }
    else if (returnValues.size() == 1)
    {
        lastInsertedBB->getPrev()->addInstruction(poInstruction(ins.name(), ins.type(), returnValues[0], -1, IR_COPY));
    }

    // Finally, remove the call instruction and argument instructions
    for (int i = 0; i < numArguments + 1; i++)
    {
        bb->removeInstruction(int(bb->numInstructions()) - 1);
    }

    // We run the SSA reconstruct mostly to handle renaming the inlined variables.
    poSSA_Reconstruct ssa;
    ssa.reconstruct(cfg, std::vector<int>());

}

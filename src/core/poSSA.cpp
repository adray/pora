#include "poSSA.h"
#include "poModule.h"
#include "poDom.h"
#include "poLive.h"

using namespace po;

static bool isSpecialInstruction(const int code)
{
    switch (code)
    {
    case (IR_CONSTANT):
    case (IR_CALL):
    case (IR_BR):
    case (IR_PARAM):
    case (IR_ALLOCA):
    case (IR_MALLOC):
        return true;
    }
    return false;
}

//=================
// poSSABasicBlock
//=================

poSSABasicBlock::poSSABasicBlock()
    :
    _maxDef(-1),
    _maxUse(-1)
{
}

//=============
// poSSA
//============

void poSSA::construct(poModule& module)
{
    for (poNamespace& ns : module.namespaces())
    {
        constructNamespaces(ns);
    }
}

void poSSA::constructNamespaces(poNamespace& ns)
{
    for (poFunction& func : ns.functions())
    {
        constructFunction(func.variables(), func.cfg());
    }
}

void poSSA::insertPhiNodes(const std::vector<int>& variables, poDom& dom)
{
    for (int i = 0; i < dom.num(); i++)
    {
        _ssa.push_back(poSSABasicBlock());
    }

    // Standard phi node insertion algorithm

    for (const int var : variables)
    {
        std::unordered_set<int> blocksInsertedAt;
        std::unordered_set<int> defs; /* blocks containing a defintion of var */
        std::vector<int> worklist;

        int type = 0;

        // 1) Add all nodes which contain assignments to the variable.

        for (int i = dom.start(); i < dom.num(); i++)
        {
            const auto& node = dom.get(i);
            poBasicBlock* bb = node.getBasicBlock();
            for (int j = 0; j < bb->numInstructions(); j++)
            {
                const auto& ins = bb->getInstruction(j);
                if (ins.name() == var)
                {
                    worklist.push_back(i);
                    defs.insert(i);
                    type = ins.type();
                    break;
                }
            }
        }

        // 2) Insert phi nodes at the dominance frontiers

        while (worklist.size() > 0)
        {
            const int id = worklist[0];
            worklist.erase(worklist.begin());
            poDomNode& node = dom.get(id);
            auto& df = node.dominanceFrontier();
            for (int i = 0; i < df.size(); i++)
            {
                const int blk = df[i];
                if (blocksInsertedAt.find(blk) == blocksInsertedAt.end())
                {
                    // Insert phi node at the entry of BLK
                    poBasicBlock* bb = dom.get(blk).getBasicBlock();
                    bb->addPhi(poPhi(var, type));

                    blocksInsertedAt.insert(blk);
                    if (defs.find(blk) == defs.end())
                    {
                        worklist.push_back(blk);
                    }
                }
            }
        }
    }
}

void poSSA::ssaRename(const std::vector<int>& variables, poDom& dom)
{
    // 1) Initialize the reaching definitions.

    for (size_t i = 0; i < variables.size(); i++)
    {
        _renamingStack.insert(std::pair<int, std::vector<int>>(variables[i], std::vector<int>()));
    }

    // 2) Perform the renaming

    _variableNames = 1000; /* just pick a high number.. */
    ssaRename(variables, dom, dom.start());

    // 3) Convert the phi nodes into instructions

    for (int i = dom.start(); i < dom.num(); i++)
    {
        poBasicBlock* bb = dom.get(i).getBasicBlock();
        for (poPhi& phi : bb->phis())
        {
            auto& phis = phi.values();
            if (phis.size() >= 2)
            {
                int name = phis[0];
                for (int i = 0; i < phis.size() - 2; i++)
                {
                    const int newName = _variableNames++;
                    if (i == 0)
                    {
                        bb->insertInstruction(poInstruction(newName, phi.getType(), phis[i], phis[i + 1], IR_PHI), i);
                    }
                    else
                    {
                        bb->insertInstruction(poInstruction(newName, phi.getType(), name, phis[i + 1], IR_PHI), i);
                    }
                    name = newName;
                }
                bb->insertInstruction(poInstruction(phi.name(), phi.getType(), name, phis[phis.size() - 1], IR_PHI), int(phis.size()) - 2);
            }
        }
    }
}

void poSSA::ssaRename(const std::vector<int>& variables, poDom& dom, int bb_id)
{
    poBasicBlock* bb = dom.get(bb_id).getBasicBlock();
    if (_visited.contains(bb))
    {
        return;
    }

    _visited.insert(bb);

    poSSABasicBlock& ssa = _ssa[bb_id];

    for (poPhi& phi : bb->phis())
    {
        phi.setName(genName(phi.name()));
    }

    for (size_t i = 0; i < bb->numInstructions(); i++)
    {
        auto& ins = bb->getInstruction(int(i));
        if (ins.code() != IR_PHI)
        {
            if (ins.left() != -1 && !isSpecialInstruction(ins.code()))
            {
                ins.setLeft(getTopStack(ins.left()));
            }
            if (ins.right() != -1 && !isSpecialInstruction(ins.code()))
            {
                ins.setRight(getTopStack(ins.right()));
            }

            ins.setName(genName(ins.name()));
        }
    }

    auto& succ = dom.get(bb_id).successors();
    for (size_t i = 0; i < succ.size(); i++)
    {
        // Rename max of one per variable per successor basic block

        auto& succ_node = dom.get(succ[i]);
        poBasicBlock* succ_bb = succ_node.getBasicBlock();

        poSSABasicBlock& succ_ssa = _ssa[succ[i]];
        for (poPhi& phi : succ_bb->phis())
        {
            const int name = getTopStack(phi.initalName());
            if (name != -1)
            {
                phi.addValue(name, bb);
            }
        }
    }

    for (size_t i = 0; i < succ.size(); i++)
    {
        ssaRename(variables, dom, succ[i]);
    }

    for (size_t i = 0; i < bb->numInstructions(); i++)
    {
        auto& ins = bb->getInstruction(int(i));
        popStack(ins.name());
    }

    for (poPhi& phi : bb->phis())
    {
        popStack(phi.name());
    }
}

int poSSA::genName(const int variable)
{
    const int newName = _variableNames++;
    _renamingStack[variable].push_back(newName);
    _renameMap.insert(std::pair<int, int>(newName, variable));
    return newName;
}

int poSSA::getTopStack(const int variable)
{
    auto& stack = _renamingStack[variable];
    if (stack.size() == 0)
    {
        return -1;
    }

    return stack[stack.size() - 1];
}

void poSSA::popStack(const int variable)
{
    const int originalVariable = _renameMap[variable];
    if (originalVariable != variable)
    {
        auto& stack = _renamingStack[originalVariable];
        stack.erase(stack.begin() + stack.size() - 1);
    }
}

void poSSA::constructFunction(const std::vector<int>& variables, poFlowGraph& cfg)
{
    // Reset the state
    _ssa.clear();
    _renameMap.clear();
    _renamingStack.clear();
    _visited.clear();

    // First calculate dominator tree, dominator frontiers
    poDom dom;
    dom.compute(cfg);

    // Insert PHI nodes
    insertPhiNodes(variables, dom);

    // SSA renaming
    ssaRename(variables, dom);
}


void poSSA_Destruction:: destruct(poFlowGraph& cfg)
{
    _web.findPhiWebs(cfg);

    std::unordered_map<int, int> phiWebVariable;

    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (int i = 0; i < int(bb->numInstructions()); i++)
        {
            auto& ins = bb->getInstruction(i);
            const int id = _web.find(ins.name());
            if (id != -1)
            {
                // Involved in a phi web; rename the variable
                const auto& it = phiWebVariable.find(id);
                if (it != phiWebVariable.end())
                {
                    ins.setName(it->second);
                }
                else
                {
                    phiWebVariable.insert(std::pair<int, int>(id, ins.name()));
                }
            }
        }

        bb = bb->getNext();
    }

    bb = cfg.getFirst();
    while (bb)
    {
        for (int i = 0; i < int(bb->numInstructions()); i++)
        {
            auto& ins = bb->getInstruction(i);
            const int left = ins.left();
            const int right = ins.right();

            if (isSpecialInstruction(ins.code()))
            {
                continue;
            }

            const int leftId = _web.find(left);
            const int rightId = _web.find(right);

            if (leftId != -1)
            {
                // Update the name to the new name
                ins.setLeft(phiWebVariable[leftId]);
            }
            if (rightId != -1)
            {
                // Update the name to the new name
                ins.setRight(phiWebVariable[rightId]);
            }
        }

        bb = bb->getNext();
    }
}


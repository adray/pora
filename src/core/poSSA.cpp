#include "poSSA.h"
#include "poModule.h"
#include "poDom.h"

using namespace po;

//==============
// poSSAPhi
//==============

poSSAPhi::poSSAPhi(const int name)
    :
    _name(name),
    _initialName(name)
{ }

void poSSAPhi::addValue(const int value)
{
    _rhs.push_back(value);
}

//=================
// poSSABasicBlock
//=================

void poSSABasicBlock::addPhi(const int name)
{
    _phis.push_back(poSSAPhi(name));
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
                    _ssa[blk].addPhi(var);
                    
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
        poSSABasicBlock& ssa = _ssa[i];
        for (poSSAPhi& phi : ssa.phis())
        {
            auto& phis = phi.values();
            int name = phis[0];
            for (int i = 0; i < phis.size() - 2; i++)
            {
                const int newName = _variableNames++;
                if (i == 0)
                {
                    bb->insertInstruction(poInstruction(newName, 0/*TODO*/, phis[i], phis[i + 1], IR_PHI), i);
                }
                else
                {
                    bb->insertInstruction(poInstruction(newName, 0/*TODO*/, name, phis[i + 1], IR_PHI), i);
                }
                name = newName;
            }
            bb->insertInstruction(poInstruction(phi.name(), 0/*TODO*/, name, phis[phis.size()-1], IR_PHI), int(phis.size())-2);
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

    for (poSSAPhi& phi : ssa.phis())
    {
        phi.setName(genName(phi.name()));
    }

    for (size_t i = 0; i < bb->numInstructions(); i++)
    {
        auto& ins = bb->getInstruction(int(i));
        if (ins.code() != IR_PHI)
        {
            if (ins.left() != -1 && ins.code() != IR_CONSTANT && ins.code() != IR_CALL && ins.code() != IR_BR)
            {
                ins.setLeft(getTopStack(ins.left()));
            }
            if (ins.right() != -1 && ins.code() != IR_CONSTANT && ins.code() != IR_CALL && ins.code() != IR_BR)
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
        for (poSSAPhi& phi : succ_ssa.phis())
        {
            phi.addValue(getTopStack(phi.initalName()));
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

    for (poSSAPhi& phi : ssa.phis())
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

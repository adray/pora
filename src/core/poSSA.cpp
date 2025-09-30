#include "poSSA.h"
#include "poModule.h"
#include "poDom.h"
#include "poLive.h"

#include <assert.h>

using namespace po;

#define NAME_BASE 1000 // Just pick a high number..

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
        if (func.hasAttribute(poAttributes::EXTERN))
        {
            continue;
        }

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
        std::unordered_set<int> defs; /* blocks containing a definition of var */
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

        // 2) Check where we might have phi nodes already inserted in the case the reconstruction

        for (int i = dom.start(); i < dom.num(); i++)
        {
            const auto& node = dom.get(i);
            poBasicBlock* bb = node.getBasicBlock();
            for (int j = 0; j < bb->numInstructions(); j++)
            {
                const auto& ins = bb->getInstruction(j);
                if (ins.code() == IR_PHI && ins.name() == var)
                {
                    // We have a phi node already inserted
                    blocksInsertedAt.insert(i);
                }
            }
        }

        // 3) Insert phi nodes at the dominance frontiers

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

void poSSA::rebaseNames(const int baseName, poDom& dom)
{
    for (int i = dom.start(); i < dom.num(); i++)
    {
        poBasicBlock* bb = dom.get(i).getBasicBlock();

        for (poPhi& phi : bb->phis())
        {
            phi.setName(phi.name() - baseName);
            
            auto& values = phi.values();
            for (size_t j = 0; j < values.size(); j++)
            {
                phi.setValue(int(j), values[j] - baseName);
            }
        }

        for (size_t j = 0; j < bb->numInstructions(); j++)
        {
            auto& ins = bb->getInstruction(int(j));
            ins.setName(ins.name() - baseName);
            if (ins.left() != -1 && !ins.isSpecialInstruction())
            {
                ins.setLeft(ins.left() - baseName);
            }
            if (ins.right() != -1 && !ins.isSpecialInstruction())
            {
                ins.setRight(ins.right() - baseName);
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

    const int baseName = NAME_BASE;
    _variableNames = baseName;
    ssaRename(dom, dom.start());

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

    rebaseNames(baseName, dom);
}

void poSSA::ssaRename(poDom& dom, int bb_id)
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
            if (ins.left() != -1 && !ins.isSpecialInstruction())
            {
                ins.setLeft(getTopStack(ins.left()));
            }
            if (ins.right() != -1 && !ins.isSpecialInstruction())
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

        for (poPhi& phi : succ_bb->phis())
        {
            const int name = getTopStack(phi.initialName());
            if (name != -1)
            {
                phi.addValue(name, bb);
            }
        }
    }

    for (size_t i = 0; i < succ.size(); i++)
    {
        ssaRename(dom, succ[i]);
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

//=================
// poSSA_Phi
//=================

poSSA_Phi::poSSA_Phi(poBasicBlock* bb, const int name)
    :
    _bb(bb),
    _name(name)
{
}


//====================
// poSSA_Reconstruct
//====================

poSSA_Reconstruct::poSSA_Reconstruct()
    :
    _variableNames(0)
{
}

void poSSA_Reconstruct::reconstruct(poFlowGraph& cfg, const std::vector<int>& variables)
{
    // Reset the state
    _defs.clear();
    _phis.clear();

    // First calculate dominator tree, dominator frontiers
    poDom dom;
    dom.compute(cfg);

    //
    // SSA reconstruction
    // This based on algorithm 5.1 SSA reconstruction driver in the SSA book
    //

    _variableNames = NAME_BASE;

    // Rename the variables which changed and are breaking SSA

    for (size_t i = 0; i < variables.size(); i++)
    {
        _defs.insert(std::pair<int, std::vector<poInstructionRef>>(variables[i], std::vector<poInstructionRef>()));
    }

    int baseRef = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (int i = 0; i < int(bb->numInstructions()); i++)
        {
            auto& ins = bb->getInstruction(i);
            for (int j = 0; j < int(variables.size()); j++)
            {
                const int var = variables[j];
                if (ins.name() == var)
                {
                    const int name = genName();
                    ins.setName(name);

                    std::vector<poInstructionRef>& defs = _defs[var];
                    defs.push_back(poInstructionRef(bb, baseRef + i, baseRef));
                }
            }
        }
        baseRef += int(bb->numInstructions());
        bb = bb->getNext();
    }

    // Scan uses of variables and update them as needed

    for (int i = 0; i < int(variables.size()); i++)
    {
        const int var = variables[i];

        int domId = dom.start();
        while (domId < dom.num())
        {
            poDomNode& node = dom.get(domId);
            poBasicBlock* bb = node.getBasicBlock();
            for (int j = 0; j < bb->numInstructions(); j++)
            {
                auto& ins = bb->getInstruction(j);
                if (!ins.isSpecialInstruction() &&
                    (ins.left() == var || ins.right() == var))
                {
                    reconstructUse(dom, domId, ins, var, j);
                }
            }
            domId++;
        }
    }

    rebuildPhiInstructions(dom);

    rebaseNames(cfg);
}

void poSSA_Reconstruct::rebaseNames(poFlowGraph& cfg)
{
    std::unordered_map<int, int> nameMap;

    int newName = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (int i = 0; i < int(bb->numInstructions()); i++)
        {
            auto& ins = bb->getInstruction(i);
            const int name = ins.name();
            if (nameMap.find(name) == nameMap.end())
            {
                nameMap.insert(std::pair<int, int>(name, newName));
                ins.setName(newName);
                newName++;
            }
            else
            {
                ins.setName(nameMap[name]);
            }

            if (ins.left() != -1 && !ins.isSpecialInstruction())
            {
                const int left = ins.left();
                if (nameMap.find(left) == nameMap.end())
                {
                    nameMap.insert(std::pair<int, int>(left, newName));
                    ins.setLeft(newName);
                    newName++;
                }
                else
                {
                    ins.setLeft(nameMap[left]);
                }
            }

            if (ins.right() != -1 && !ins.isSpecialInstruction())
            {
                const int right = ins.right();
                if (nameMap.find(right) == nameMap.end())
                {
                    nameMap.insert(std::pair<int, int>(right, newName));
                    ins.setRight(newName);
                    newName++;
                }
                else
                {
                    ins.setRight(nameMap[right]);
                }
            }
        }

        for (poPhi& phi : bb->phis())
        {
            const int name = phi.name();
            if (nameMap.find(name) == nameMap.end())
            {
                nameMap.insert(std::pair<int, int>(name, newName));
                phi.setName(newName);
                newName++;
            }
            else
            {
                phi.setName(nameMap[name]);
            }
            auto& values = phi.values();
            for (size_t j = 0; j < values.size(); j++)
            {
                const int value = values[j];
                if (nameMap.find(value) == nameMap.end())
                {
                    nameMap.insert(std::pair<int, int>(value, newName));
                    phi.setValue(int(j), newName);
                    newName++;
                }
                else
                {
                    phi.setValue(int(j), nameMap[value]);
                }
            }
        }

        bb = bb->getNext();
    }
}

int poSSA_Reconstruct::genName()
{
    const int newName = _variableNames++;
    return newName;
}

void poSSA_Reconstruct::reconstructUse(poDom& dom, const int node, poInstruction& inst, const int var, const int ref)
{
    poBasicBlock* block = nullptr;
    poBasicBlock* outBB = nullptr;
    int name = -1;

    std::vector<poInstructionRef>& defs = _defs[var];

    if (inst.code() == IR_PHI)
    {
        poDomNode& domNode = dom.get(node);
        poBasicBlock* bb = domNode.getBasicBlock();

        for (size_t i = 0; i < bb->phis().size(); i++)
        {
            poPhi& phi = bb->phis()[i];
            if (phi.name() == inst.name()) /* find the phi corresponding to the phi instruction */
            {
                const std::vector<int>& values = phi.values();
                for (int j = 0; j < int(values.size()); j++)
                {
                    if (values[j] == var)
                    {
                        block = phi.getBasicBlock()[j];
                        break;
                    }
                }

                if (block)
                {
                    break;
                }
            }
        }

        if (block)
        {
            int blockId = -1;
            for (int i = dom.start(); i < dom.num(); i++)
            {
                if (dom.get(i).getBasicBlock() == block)
                {
                    blockId = i;
                    break;
                }
            }

            assert(blockId != -1);
            name = findDefFromBottom(dom, dom.get(blockId), defs, var, inst.type(), &outBB);
        }
    }
    else
    {
        poDomNode& domNode = dom.get(node);
        block = domNode.getBasicBlock();

        for (int i = int(defs.size()) - 1; i >= 0; i--)
        {
            poInstructionRef& def = defs[i];
            if (def.getBasicBlock() == block &&
                def.getAdjustedRef() < ref)
            {
                name = def.getInstruction().name();
                outBB = def.getBasicBlock();
                break;
            }
        }

        if (name == -1)
        {
            /* Here we should check if there is a phi in our basic block
            * that was inserted with a name equal to the name of our use. */
            const auto& it = _phis.find(var);
            if (it != _phis.end())
            {
                for (poSSA_Phi& ssaPhi : it->second) /* get the basic block which contain a def */
                {
                    if (ssaPhi.getBasicBlock() != block)
                    {
                        continue;
                    }

                    for (poPhi& phi : ssaPhi.getBasicBlock()->phis()) /* look for a phi which name matches the name of the def */
                    {
                        if (phi.name() == ssaPhi.name())
                        {
                            name = phi.name();
                            outBB = ssaPhi.getBasicBlock();
                            break;
                        }
                    }

                    if (name != -1)
                    {
                        break;
                    }
                }
            }
        }

        if (name == -1)
        {
            // FindDefFromTop
            name = findDefFromTop(dom, domNode, defs, var, inst.type(), &outBB);
        }
    }

    if (inst.left() == var)
    {
        inst.setLeft(name);
    }
    else if (inst.right() == var)
    {
        inst.setRight(name);
    }
}

int poSSA_Reconstruct::findDefFromBottom(poDom& dom, poDomNode& node, const std::vector<poInstructionRef>& defs, const int var, const int type, poBasicBlock** outBB)
{
    // Scan backwards in the predecessor block for the variable definition

    int id = -1;
    for (int i = int(defs.size()) - 1; i >= 0; i--)
    {
        const poInstructionRef& def = defs[i];
        if (def.getBasicBlock() == node.getBasicBlock())
        {
            id = def.getInstruction().name();
            *outBB = def.getBasicBlock();
            break;
        }
    }

    /* We may need to check for newly inserted Phi nodes */

    if (id == -1)
    {
        const auto& phiDefs = _phis.find(var);
        if (phiDefs != _phis.end())
        {
            for (const poSSA_Phi& phi : phiDefs->second)
            {
                if (phi.getBasicBlock() == node.getBasicBlock())
                {
                    id = phi.name();
                    *outBB = phi.getBasicBlock();
                    break;
                }
            }
        }
    }

    if (id == -1)
    {
        // FindDefFromTop
        id = findDefFromTop(dom, node, defs, var, type, outBB);
    }

    return id;
}

int poSSA_Reconstruct::findDefFromTop(poDom& dom, poDomNode& node, const std::vector<poInstructionRef>& defs, const int var, const int type, poBasicBlock** outBB)
{
    /* There wasn't a def in the basic block we were evaluating - therefore we may need to insert a phi node */

    /* TODO: we may need to check the inserted phi defs as well */

    int id = -1;
    bool insertPhi = false;

    std::vector<int> defNodes;
    for (int i = 0; i < int(defs.size()); i++)
    {
        const poInstructionRef& def = defs[i];
        for (int j = dom.start(); j < dom.num(); j++)
        {
            const poDomNode& domNode = dom.get(j);
            if (domNode.getBasicBlock() == def.getBasicBlock())
            {
                defNodes.push_back(j);
                break;
            }
        }
    }

    std::unordered_set<int> idf;
    dom.iteratedDominanceFrontier(defNodes, idf);
    for (int item : idf)
    {
        const poDomNode& dfNode = dom.get(item);
        if (dfNode.getBasicBlock() == node.getBasicBlock())
        {
            insertPhi = true;
            break;
        }
    }

    if (insertPhi)
    {
        const int newName = genName();
        poBasicBlock* bb = node.getBasicBlock();
        const int phiPos = int(bb->phis().size());
        bb->addPhi(poPhi(newName, type));

        *outBB = bb;
        id = newName;
        
        const auto& it = _phis.find(var);
        if (it != _phis.end())
        {
            it->second.push_back(poSSA_Phi(bb, newName));
        }
        else
        {
            std::vector<poSSA_Phi> phis = { poSSA_Phi(bb, newName) };
            _phis.insert(std::pair<int, std::vector<poSSA_Phi>>(var, phis));
        }

        const std::vector<int>& predecessors = node.predecessors();
        for (int i = 0; i < int(predecessors.size()); i++)
        {
            poDomNode& predecessorNode = dom.get(predecessors[i]);
            poBasicBlock* predecessorBB = nullptr;

            const int name = findDefFromBottom(dom, predecessorNode, defs, var, type, &predecessorBB);
            bb->phis()[phiPos].addValue(name, predecessorBB);
        }
    }
    else
    {
        /* Use the immediate dominator i.e. the single node which immediately dominates this one. */
        const int imm = node.immediateDominator();

        id = findDefFromBottom(dom, dom.get(imm), defs, var, type, outBB);
    }

    return id;
}

void poSSA_Reconstruct::rebuildPhiInstructions(poDom& dom)
{
    for (int i = dom.start(); i < dom.num(); i++)
    {
        poBasicBlock* bb = dom.get(i).getBasicBlock();
        
        // Wipe all the phi instructions before rebuilding them
        while (bb->numInstructions() > 0 && bb->getInstruction(0).code() == IR_PHI)
        {
            bb->removeInstruction(0);
        }

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

//=========================
// poSSA_Destruction
//=========================

void poSSA_Destruction::destruct(poFlowGraph& cfg)
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

            if (ins.isSpecialInstruction())
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


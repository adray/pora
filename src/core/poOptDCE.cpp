#include "poOptDCE.h"
#include "poModule.h"
#include "poDom.h"

using namespace po;


void poOptDCE::optimize(poModule& module)
{
    for (poFunction& function : module.functions()) {
        if (function.hasAttribute(poAttributes::EXTERN)) {
            continue; // Skip extern functions
        }

        optimize(function);
    }
}

void poOptDCE::optimize(poDom& dom, const int id)
{
    _visitedNodes.insert(id);

    poDomNode& node = dom.get(id);

    // Add uses for any phi nodes

    poBasicBlock* bb = node.getBasicBlock();

    for (poPhi& phi : bb->phis())
    {
        for (int j = 0; j < int(phi.values().size()); j++)
        {
            _usedNames.insert(phi.values()[j]);
        }
    }

    /* Iterate over the control flow graph in post-order */

    const std::vector<int>& successors = node.successors();

    for (int i = 0; i <int(successors.size()); i++)
    {
        if (_visitedNodes.find(successors[i]) == _visitedNodes.end())
        {
            optimize(dom, successors[i]);
        }
    }

    // Perform dead code eliminate for this basic block

    int pos = int(bb->numInstructions()) - 1;
    while (pos >= 0)
    {
        auto& ins = bb->getInstruction(pos);
        switch (ins.code())
        {
        case IR_CMP: /* skip on these as they won't have uses */
        case IR_RETURN:
        case IR_ARG:
        case IR_BR:
        case IR_LOAD:
        case IR_STORE:
        case IR_CALL: /* call may have uses or not, but certainly can have side-effects */
            goto done;
        default:
            break;
        }

        if (_usedNames.find(ins.name()) != _usedNames.end())
        {
            goto done;
        }

        bb->removeInstruction(pos);
        pos--;
        continue;
    done:
        if (!ins.isSpecialInstruction())
        {
            if (ins.left() != -1) {
                _usedNames.insert(ins.left());
            }
            if (ins.right() != -1) {
                _usedNames.insert(ins.right());
            }
        }

        pos--;
    }
}

void poOptDCE::optimize(poFunction& function)
{
    _usedNames.clear();
    _visitedNodes.clear();

    poFlowGraph& cfg = function.cfg();

    poDom dom;
    dom.compute(cfg);

    optimize(dom, dom.start());
}

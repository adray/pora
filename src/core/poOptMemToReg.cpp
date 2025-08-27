#include "poOptMemToReg.h"
#include "poModule.h"
#include "poCFG.h"
#include "poDom.h"
#include "poSSA.h"

#include <assert.h>

using namespace po;

//
// poOptMemToReg_Alloca
//

poOptMemToReg_Alloca::poOptMemToReg_Alloca(const poInstructionRef& source)
    :
    _source(source),
    _promote(true)
{

}

void poOptMemToReg_Alloca::addUse(const poInstructionRef& ref)
{
    _uses.push_back(ref);

    const poInstruction& ins = ref.getInstruction();
    if (ref.getInstruction().code() != IR_PTR)
    {
        _promote = false;
    }
}

//
// poOptMemToReg
//

void poOptMemToReg::optimize(poModule& module)
{
    for (poNamespace& ns : module.namespaces())
    {
        for (poFunction& func : ns.functions())
        {
            if (!func.hasAttribute(poAttributes::EXTERN))
            {
                optimize(module, func.cfg());
            }
        }
    }
}

void poOptMemToReg::optimize(poModule& module, poFlowGraph& cfg)
{
    // 1. Compute the uses of ALLOCA instructions and determine if they can be promoted.

    _uses.analyze(cfg);
    _alloca.clear();

    int pos = 0;
    int baseRef = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        for (const poInstruction& ins : bb->instructions())
        {
            const int name = ins.name();
            if (ins.code() == IR_ALLOCA)
            {
                const poType& type = module.types()[ins.type()];
                if (!type.isPointer())
                {
                    // If the type is not a pointer, we cannot promote it
                    pos++;
                    continue;
                }

                const poType& baseType = module.types()[type.baseType()];
                if (baseType.baseType() == TYPE_OBJECT || baseType.isPointer() || baseType.isArray())
                {
                    // If the base type is an object or pointer, we cannot promote it
                    pos++;
                    continue;
                }

                if (_uses.hasUses(ins.name()))
                {
                    const std::vector<poInstructionRef>& uses = _uses.getUses(ins.name());
                    poOptMemToReg_Alloca al(poInstructionRef(bb, pos, baseRef));

                    for (const poInstructionRef& use : uses)
                    {
                        al.addUse(use);
                    }

                    _alloca.push_back(al);
                }
            }

            pos++;
        }

        bb = bb->getNext();
        baseRef = pos;
    }

    // 2. Replace the instructions of IR_ALLOCA, IR_LOAD, IR_STORE, IR_PTR with register based instructions

    for (poOptMemToReg_Alloca& al : _alloca)
    {
        if (al.promote())
        {
            // Replace IR_ALLOCA

            auto& allocaIns = al.source().getInstruction();
            auto& type = module.types()[allocaIns.type()];
            assert(type.isPointer());
            const int baseType = type.baseType();
            allocaIns.setLeft(-1);
            allocaIns.setRight(-1);
            allocaIns.setCode(IR_CONSTANT);
            allocaIns.setType(baseType);
            poConstantPool& pool = module.constants();
            int constant = -1;
            switch (baseType)
            {
            case TYPE_I64:
                constant = pool.getConstant(int(0));
                if (constant == -1)
                {
                    constant = pool.addConstant(int(0));
                }
                break;
            case TYPE_BOOLEAN:
            case TYPE_U8:
                constant = pool.getConstant((uint8_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(uint8_t(0));
                }
                break;
            case TYPE_U16:
                constant = pool.getConstant((uint16_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(uint16_t(0));
                }
                break;
            case TYPE_U32:
                constant = pool.getConstant((uint32_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(uint32_t(0));
                }
                break;
            case TYPE_U64:
                constant = pool.getConstant((uint64_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(uint64_t(0));
                }
                break;
            case TYPE_I32:
                constant = pool.getConstant((int32_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(int32_t(0));
                }
                break;
            case TYPE_I16:
                constant = pool.getConstant((int16_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(int16_t(0));
                }
                break;
            case TYPE_I8:
                constant = pool.getConstant((int8_t)0);
                if (constant == -1)
                {
                    constant = pool.addConstant(int8_t(0));
                }
                break;
            case TYPE_F32:
                constant = pool.getConstant(0.0f);
                if (constant == -1)
                {
                    constant = pool.addConstant(float(0));
                }
                break;
            case TYPE_F64:
                constant = pool.getConstant(0.0);
                if (constant == -1)
                {
                    constant = pool.addConstant(double(0));
                }
                break;
            default:
                break;
            }

            if (constant != -1)
            {
                allocaIns.setConstant(constant);

                for (poInstructionRef& use : al.uses())
                {
                    poInstruction& ins = use.getInstruction();

                    if (ins.code() == IR_PTR)
                    {
                        rewritePtr(ins);
                    }
                }
            }
        }
    }

    // 3. Get the variables affected before removing redundant instructions

    std::vector<int> variables;
    for (auto& al : _alloca)
    {
        variables.push_back(al.source().getInstruction().name());
    }

    // 4. Remove instructions no longer needed (name == -1)
    bb = cfg.getFirst();
    while (bb)
    {
        int pos = 0;
        while (pos < int(bb->instructions().size()))
        {
            auto& ins = bb->getInstruction(pos);
            if (ins.name() == -1)
            {
                bb->removeInstruction(pos);
            }
            else
            {
                pos++;
            }
        }

        bb = bb->getNext();
    }

    // 5: Phi node insertion, SSA rename
    poSSA_Reconstruct ssa;
    ssa.reconstruct(cfg, variables);
}

void poOptMemToReg::rewritePtr(poInstruction& ins)
{
    /* Find any load/store instructions which reference this */

    const int name = ins.name();
    if (!_uses.hasUses(name))
    {
        return;
    }

    const std::vector<poInstructionRef>& uses = _uses.getUses(name);

    for (const poInstructionRef& use : uses)
    {
        poInstruction& useIns = use.getInstruction();
        if (useIns.code() == IR_LOAD)
        {
            // Replace the load with a copy
            useIns.setCode(IR_COPY);
            useIns.setLeft(ins.left());
            useIns.setConstant(0);
            useIns.setRight(-1);
        }
        else if (useIns.code() == IR_STORE)
        {
            // Replace the store with a copy to ins.left()
            useIns.setCode(IR_COPY);
            useIns.setLeft(useIns.right());
            useIns.setRight(-1);
            useIns.setName(ins.left());
            useIns.setConstant(0);
        }
    }

    /* Finally, eliminate the IR_PTR instruction */

    ins.setName(-1);
    ins.setLeft(-1);
    ins.setRight(-1);
}


#include "poAnalyzer.h"
#include "po_x86_64.h"
#include "poModule.h"

#include <vector>
#include <iostream>

using namespace po;

static const int align(const int size)
{
    const int remainder = size % 16;
    if (remainder == 8)
    {
        return 0;
    }

    return 8;
}

void poAnalyzer:: checkCallSites(poModule& module, po_x86_64_flow_graph& cfg)
{
    // We need to align function calls to the calling convention. This involves passing in arguments via the stack.
    // 1) Loop through basic blocks and find call instructions
    // 2) For each call instruction check which arguments
    // the function requires and calculate the stack space needed
    // 3) Adjust the stack space allocated?
    // 4) Find the call sites again and insert instructions needed to copy to the stack
    // 5) We may need to adjust anything which uses the stack
 
    int argStackSize = 0;

    std::vector<po_x86_64_basic_block*>& basicBlocks = cfg.basicBlocks();
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t j = 0; j < ins.size(); j++)
        {
            po_x86_64_instruction& instr = ins[j];
            if (instr.opcode() != VMI_CALL)
            {
                continue;
            }

            const int id = instr.id();
            argStackSize = std::max(argStackSize, checkFunction(module, id)); 
        }
    }

    int stackAdjustment = 32 /* register homes */;
    if (basicBlocks.size() > 0)
    {
        po_x86_64_basic_block* bb = basicBlocks.front();

        int pushStackSize = 0;
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            po_x86_64_instruction& instr = ins[i];

            if (instr.opcode() == VMI_PUSH_REG)
            {
                pushStackSize += 8;
            }
        }

        int stackSize = 0;
        for (size_t i = 0; i < ins.size(); i++)
        {
            po_x86_64_instruction& instr = ins[i];

            if (instr.opcode() != VMI_SUB64_SRC_IMM_DST_REG)
            {
                continue;
            }

            if (instr.dstReg() != VM_REGISTER_ESP)
            {
                continue;
            }

            const int imm32 = argStackSize + instr.imm32();
            const int alignment = align(imm32 + pushStackSize);
            stackAdjustment += alignment + argStackSize;
            stackSize = imm32 + alignment;
            instr.setImm32(stackSize);

            //std::cout << "MaxStackSize: " << imm32 << " Push:" << pushStackSize << " Align: " << alignment << std::endl;
        }

        for (size_t i = 0; i < basicBlocks.size(); i++)
        {
            po_x86_64_basic_block* bb = basicBlocks[i];
            std::vector<po_x86_64_instruction>& ins = bb->instructions();
            for (size_t j = 0; j < ins.size(); j++)
            {
                po_x86_64_instruction& instr = ins[j];

                if (instr.opcode() != VMI_ADD64_SRC_IMM_DST_REG)
                {
                    continue;
                }

                if (instr.dstReg() != VM_REGISTER_ESP)
                {
                    continue;
                }

                instr.setImm32(stackSize);
            }
        }
    }

    // Fixup MOV instructions
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t j = 0; j < ins.size(); j++)
        {
            po_x86_64_instruction& instr = ins[j];
            if (instr.opcode() == VMI_MOV64_SRC_MEM_DST_REG)
            {
                if (instr.srcReg() != VM_REGISTER_ESP)
                {
                    continue;
                }

                instr.setImm32(instr.imm32() + stackAdjustment);
            }
            else if (instr.opcode() == VMI_MOV64_SRC_REG_DST_MEM)
            {
                if (instr.dstReg() != VM_REGISTER_ESP)
                {
                    continue;
                }

                instr.setImm32(instr.imm32() + stackAdjustment);
            }
        }
    }

    // Fixup array/struct loads, which use an add instruction to add an immediate value to the stack pointer
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t j = 0; j < ins.size(); j++)
        {
            po_x86_64_instruction& instr = ins[j];
            if (instr.opcode() != VMI_ADD64_SRC_REG_DST_REG &&
                instr.opcode() != VMI_MOV64_SRC_REG_DST_REG)
            {
                continue;
            }

            if (instr.srcReg() != VM_REGISTER_ESP)
            {
                continue;
            }

            if (j + 1 >= ins.size())
            {
                continue;
            }

            po_x86_64_instruction& imm_instr = ins[j + 1];
            if (imm_instr.opcode() != VMI_ADD64_SRC_IMM_DST_REG)
            {
                continue;
            }

            // We need to adjust the stack pointer for the arguments
            const int imm32 = imm_instr.imm32();
            imm_instr.setImm32(imm32 + stackAdjustment);
        }
    }

    fixFunctionCalls(module, cfg);
}

void poAnalyzer::fixFunctionCalls(poModule& module, po_x86_64_flow_graph& cfg)
{
    const int registerHomes = 32; // 8 * 4 for the register homes

    static const int generalArgs[] = { VM_ARG1, VM_ARG2, VM_ARG3, VM_ARG4, VM_ARG5, VM_ARG6 };
    static const int sseArgs[] = { VM_SSE_ARG1, VM_SSE_ARG2, VM_SSE_ARG3, VM_SSE_ARG4, VM_SSE_ARG5, VM_SSE_ARG6, VM_SSE_ARG7, VM_SSE_ARG8 };

    std::vector<po_x86_64_basic_block*>& basicBlocks = cfg.basicBlocks();
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t k = 0; k < ins.size(); k++)
        {
            po_x86_64_instruction& instr = ins[k];
            if (instr.opcode() != VMI_CALL)
            {
                continue;
            }

            const int id = instr.id();
            std::string symbol;
            if (!module.getSymbol(id, symbol)) { continue; }
            
            for (poNamespace& ns : module.namespaces())
            {
                for (poFunction& func : ns.functions())
                {
                    if (func.name() == symbol)
                    {
                        const std::vector<int>& args = func.args();
                        size_t argsDone = 0;
                        size_t pos = k - 1;
                        int stackPos = registerHomes + (int(args.size()) - VM_MAX_ARGS - 1) * 8;
                        while (argsDone < args.size())
                        {
                            const int argIndex = int(args.size() - argsDone) - 1;
                            const int typeId = args[argIndex];
                            const poType& type = module.types()[typeId];
                            auto& instr = ins[pos];

                            switch (typeId)
                            {
                                case TYPE_I64:
                                case TYPE_I32:
                                case TYPE_I16:
                                case TYPE_I8:
                                case TYPE_U64:
                                case TYPE_U32:
                                case TYPE_U16:
                                case TYPE_U8:
                                    if (argIndex >= VM_MAX_ARGS &&
                                        instr.dstReg() == VM_REGISTER_ESP)
                                    {
                                        instr.setImm32(stackPos);
                                        stackPos -= 8;
                                        argsDone++;
                                    }
                                    else if (instr.dstReg() == generalArgs[argIndex])
                                    {
                                        argsDone++;
                                    }
                                    break;
                                case TYPE_F32:
                                case TYPE_F64:
                                    if (argIndex >= VM_MAX_SSE_ARGS &&
                                        instr.dstReg() == VM_REGISTER_ESP)
                                    {
                                        instr.setImm32(stackPos);
                                        stackPos -= 8;
                                        argsDone++;
                                    }
                                    else if (instr.dstReg() == sseArgs[argIndex])
                                    {
                                        argsDone++;
                                    }
                                    break;
                                default:
                                    if (argIndex >= VM_MAX_ARGS &&
                                        instr.dstReg() == VM_REGISTER_ESP)
                                    {
                                        instr.setImm32(stackPos);
                                        stackPos -= 8;
                                        argsDone++;
                                    }
                                    else if (instr.dstReg() == generalArgs[argIndex])
                                    {
                                        argsDone++;
                                    }
                                    break;
                            }

                            pos--;
                         }
                    }
                }
            }
        }
    }
}

int poAnalyzer::checkFunction(poModule& module, const int id)
{
    int stackSize = 0;
    std::string symbol;
    if (!module.getSymbol(id, symbol))
    {
        return stackSize;
    }

    for (poNamespace& ns : module.namespaces())
    {
        for (poFunction& func : ns.functions())
        {
            if (func.name() != symbol)
            {
                continue;
            }

            const std::vector<int>& args = func.args();
            for (size_t i = 0; i < args.size(); i++)
            {
                const int typeId = args[i];
                const poType& type = module.types()[typeId];
                switch (typeId)
                {
                    case TYPE_I64:
                    case TYPE_I32:
                    case TYPE_I16:
                    case TYPE_I8:
                    case TYPE_U64:
                    case TYPE_U32:
                    case TYPE_U16:
                    case TYPE_U8:
                        if (i >= VM_MAX_ARGS)
                        {
                            stackSize += type.size();
                        }
                        break;
                    case TYPE_F32:
                    case TYPE_F64:
                        if (i >= VM_MAX_SSE_ARGS)
                        {
                            stackSize += type.size();
                        }
                        break;
                    default:
                        if (i >= VM_MAX_ARGS)
                        {
                            stackSize += type.size();
                        }
                        break;
                }
            }
        }
    }

    return stackSize;
}



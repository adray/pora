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
 
    int maxStackSize = 0;
      
    std::vector<po_x86_64_basic_block*>& basicBlocks = cfg.basicBlocks();
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            po_x86_64_instruction& instr = ins[i];
            if (instr.opcode() != VMI_CALL)
            {
                continue;
            }

            const int id = instr.id();
            maxStackSize = std::max(maxStackSize, checkFunction(module, id)); 
        }
    }

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

            const int imm32 = maxStackSize + instr.imm32();
            const int alignment = align(imm32 + pushStackSize);
            instr.setImm32(imm32 + alignment);

            std::cout << "MaxStackSize: " << imm32 << " Push:" << pushStackSize << " Align: " << alignment << std::endl;
        } 
    }

    fixFunctionCalls(module, cfg);
}

void poAnalyzer::fixFunctionCalls(poModule& module, po_x86_64_flow_graph& cfg)
{
    std::vector<po_x86_64_basic_block*>& basicBlocks = cfg.basicBlocks();
    for (size_t i = 0; i < basicBlocks.size(); i++)
    {
        po_x86_64_basic_block* bb = basicBlocks[i];
        std::vector<po_x86_64_instruction>& ins = bb->instructions();
        for (size_t i = 0; i < ins.size(); i++)
        {
            po_x86_64_instruction& instr = ins[i];
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
                        for (size_t j = 0; j < args.size(); j++)
                        {
                            const int typeId = args[j];
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
                                    if (j >= VM_MAX_ARGS)
                                    {
                                        auto& instr = ins[i - 1];
                                        instr.setImm32(instr.imm32() + 
                                                (int32_t)(j - VM_MAX_ARGS) * 8);
                                    }
                                    break;
                                case TYPE_F32:
                                case TYPE_F64:
                                    if (j >= VM_MAX_SSE_ARGS)
                                    {
                                        
                                    }
                                    break;
                                default:
                                    if (j >= VM_MAX_ARGS)
                                    {
                                        
                                    }
                                    break;
                                
                            }
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
    if (module.getSymbol(id, symbol))
    {
        for (poNamespace& ns : module.namespaces())
        {
           for (poFunction& func : ns.functions())
           {
               if (func.name() == symbol)
               {
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
        }
    }

    return stackSize;
}



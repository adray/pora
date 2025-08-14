#include "poCodeGen.h"
#include "poAsm.h"
#include "poPE.h"
#include <cstring>

#include <iostream>

using namespace po;

static void generatePadding(po_x86_64& x86_64)
{
    for (int i = 0; i < 8; i++)
    {
        x86_64.programData().push_back(0x0);
    }
}

static void generateAddInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_x64(i, VM_REGISTER_EAX);
    }
    
    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_x64(VM_REGISTER_EAX, i);
    }
    
    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_imm_to_reg_x64(i, 1);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_memory_to_reg_x64(VM_REGISTER_EAX, i, 16);
    }

    generatePadding(x86_64);
}

static void generateAddInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_8(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_8(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_8(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_8(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mul_reg_8(i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_umul_reg_8(i);
    }

    generatePadding(x86_64);
}

static void generateAddInstructions3(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_16(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_mem_to_reg_16(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_reg_to_reg_32(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_add_mem_to_reg_32(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);
}

static void generateSubInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_x64(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_x64(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_imm_to_reg_x64(i, 1);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_memory_to_reg_x64(VM_REGISTER_EAX, i, 16);
    }

    generatePadding(x86_64);
}

static void generateSubInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_16(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_mem_to_reg_16(i, VM_REGISTER_EAX, 0xFF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_reg_to_reg_32(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_sub_mem_to_reg_32(i, VM_REGISTER_EAX, 0xFF);
    }

    generatePadding(x86_64);
}

static void generateMulInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mul_reg_to_reg_x64(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mul_reg_to_reg_x64(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_umul_reg_to_reg_x64(i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_umul_reg_to_reg_32(i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_umul_reg_to_reg_16(i);
    }

    generatePadding(x86_64);
}

static void generateMulInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mul_reg_to_reg_16(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mul_reg_to_reg_32(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
}

static void generateMovInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_reg_to_reg_x64(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_imm_to_reg_x64(i, 1);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_memory_to_reg_x64(VM_REGISTER_EAX, i, 16);
    }

    generatePadding(x86_64);
}

static void generateMovInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_reg_to_reg_32(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_imm_to_reg_32(i, 1000);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_mem_to_reg_32(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_reg_to_reg_16(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_mem_to_reg_16(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
}

static void generateMovSXInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_16_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_16_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_16_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_32_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_32_mem_to_reg(VM_REGISTER_EAX, i, 0xFF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_32_mem_to_reg(i, VM_REGISTER_EAX, 0xFF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_64_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_64_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_8_to_64_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
}

static void generateMovSXInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_16_to_32_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_16_to_32_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_16_to_64_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_16_to_64_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_32_to_64_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_32_to_64_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movsx_32_to_64_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);
}

static void generateMovZXInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_16_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_16_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_16_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_32_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_32_mem_to_reg(VM_REGISTER_EAX, i, 0xFF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_32_mem_to_reg(i, VM_REGISTER_EAX, 0xFF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_64_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_64_reg_to_reg(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_8_to_64_mem_to_reg(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
}

static void generateMovZXInstructions2(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_16_to_32_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_16_to_32_mem_to_reg(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_16_to_64_reg_to_reg(i, VM_REGISTER_EAX);
    }

    generatePadding(x86_64);

    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_movzx_16_to_64_mem_to_reg(i, VM_REGISTER_EAX, 0xF);
    }

    generatePadding(x86_64);
}

static void generateCmpInstructions1(po_x86_64& x86_64)
{
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_cmp_reg_to_reg_32(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_cmp_mem_to_reg_32(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_cmp_reg_to_reg_16(VM_REGISTER_EAX, i);
    }

    generatePadding(x86_64);
    for (int i = VM_REGISTER_EAX; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_cmp_mem_to_reg_16(VM_REGISTER_EAX, i, 0xF);
    }

    generatePadding(x86_64);
}

static void generateUnarySubInstructions(po_x86_64& x86_64)
{
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_neg_reg_x64(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_neg_reg_32(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_neg_reg_16(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_neg_reg_8(i);
    }

    generatePadding(x86_64);
}

static void generateDivInstructions1(po_x86_64& x86_64)
{
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_div_reg_8(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_div_reg_16(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_div_reg_32(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_div_reg_x64(i);
    }

    generatePadding(x86_64);
}

static void generateDivInstructions2(po_x86_64& x86_64)
{
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_udiv_reg_8(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_udiv_reg_16(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_udiv_reg_32(i);
    }

    generatePadding(x86_64);
    for (int i = 0; i <= VM_REGISTER_R15; i++)
    {
        x86_64.mc_udiv_reg_x64(i);
    }

    generatePadding(x86_64);
}

int main(int numArgs, char** args)
{
    std::cout << "Running code gen test" << std::endl;

    po_x86_64 x86_64;
    //generateAddInstructions1(x86_64);
    //generateSubInstructions1(x86_64);
    //generateMulInstructions1(x86_64);
    //generateMulInstructions2(x86_64);
    //generateMovInstructions1(x86_64);
    //generateMovSXInstructions1(x86_64);
    //generateMovSXInstructions2(x86_64);
    //generateAddInstructions2(x86_64);
    //generateAddInstructions3(x86_64);
    //generateSubInstructions2(x86_64);
    //generateMovInstructions2(x86_64);
    //generateCmpInstructions1(x86_64);
    //generateDivInstructions1(x86_64);
    generateDivInstructions2(x86_64);

    //generateMovZXInstructions1(x86_64);
    //generateMovZXInstructions2(x86_64);
    //generateUnarySubInstructions(x86_64);

    poPortableExecutable exe;

    // Create the data sections
    exe.setEntryPoint(0x1000);
    exe.addSection(poSectionType::TEXT, 1024 * 4);
    exe.addSection(poSectionType::INITIALIZED, 1024);
    exe.addSection(poSectionType::UNINITIALIZED, 1024);
    exe.addSection(poSectionType::IDATA, 1024);
    exe.initializeSections();

    // Write program data
    std::memcpy(exe.textSection().data().data(), x86_64.programData().data(), x86_64.programData().size());
    //std::memcpy(exe.initializedDataSection().data().data(), initializedData.data(), initializedData.size());

    // Write the executable file
    exe.write("app.exe");

    std::cout << "Program compiled successfully: app.exe" << std::endl;

    return 0;
}

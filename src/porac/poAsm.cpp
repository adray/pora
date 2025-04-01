#include "poAsm.h"
#include <assert.h>

using namespace po;


//================

enum vm_register
{
    VM_REGISTER_EAX = 0x0,
    VM_REGISTER_ECX = 0x1,
    VM_REGISTER_EDX = 0x2,
    VM_REGISTER_EBX = 0x3,
    VM_REGISTER_ESP = 0x4,
    VM_REGISTER_EBP = 0x5,
    VM_REGISTER_ESI = 0x6,
    VM_REGISTER_EDI = 0x7,
    VM_REGISTER_R8 = 0x8,
    VM_REGISTER_R9 = 0x9,
    VM_REGISTER_R10 = 0xa,
    VM_REGISTER_R11 = 0xb,
    VM_REGISTER_R12 = 0xc,
    VM_REGISTER_R13 = 0xd,
    VM_REGISTER_R14 = 0xe,
    VM_REGISTER_R15 = 0xf,

    VM_REGISTER_MAX = 0x10
};

enum vm_sse_register
{
    VM_SSE_REGISTER_XMM0 = 0x0,
    VM_SSE_REGISTER_XMM1 = 0x1,
    VM_SSE_REGISTER_XMM2 = 0x2,
    VM_SSE_REGISTER_XMM3 = 0x3,
    VM_SSE_REGISTER_XMM4 = 0x4,
    VM_SSE_REGISTER_XMM5 = 0x5,
    VM_SSE_REGISTER_XMM6 = 0x6,
    VM_SSE_REGISTER_XMM7 = 0x7,
    VM_SSE_REGISTER_XMM8 = 0x8,
    VM_SSE_REGISTER_XMM9 = 0x9,
    VM_SSE_REGISTER_XMM10 = 0xa,
    VM_SSE_REGISTER_XMM11 = 0xb,
    VM_SSE_REGISTER_XMM12 = 0xc,
    VM_SSE_REGISTER_XMM13 = 0xd,
    VM_SSE_REGISTER_XMM14 = 0xe,
    VM_SSE_REGISTER_XMM15 = 0xf,

    VM_SSE_REGISTER_MAX = 0x10
};

#if WIN32
#define VM_ARG1 VM_REGISTER_ECX
#define VM_ARG2 VM_REGISTER_EDX
#define VM_ARG3 VM_REGISTER_R8
#define VM_ARG4 VM_REGISTER_R9
#define VM_ARG5 (-1)
#define VM_ARG6 (-1)
#define VM_SSE_ARG1 VM_SSE_REGISTER_XMM0
#define VM_SSE_ARG2 VM_SSE_REGISTER_XMM1
#define VM_SSE_ARG3 VM_SSE_REGISTER_XMM2
#define VM_SSE_ARG4 VM_SSE_REGISTER_XMM3
#define VM_SSE_ARG5 VM_SSE_REGISTER_XMM4
#define VM_SSE_ARG6 VM_SSE_REGISTER_XMM5
#define VM_SSE_ARG7 (-1)
#define VM_SSE_ARG8 (-1)
#define VM_MAX_ARGS 4
#define VM_MAX_SSE_ARGS 6
#else
#define VM_ARG1 VM_REGISTER_EDI
#define VM_ARG2 VM_REGISTER_ESI
#define VM_ARG3 VM_REGISTER_EDX
#define VM_ARG4 VM_REGISTER_ECX
#define VM_ARG5 VM_REGISTER_R8
#define VM_ARG6 VM_REGISTER_R9
#define VM_SSE_ARG1 VM_SSE_REGISTER_XMM0
#define VM_SSE_ARG2 VM_SSE_REGISTER_XMM1
#define VM_SSE_ARG3 VM_SSE_REGISTER_XMM2
#define VM_SSE_ARG4 VM_SSE_REGISTER_XMM3
#define VM_SSE_ARG5 VM_SSE_REGISTER_XMM4
#define VM_SSE_ARG6 VM_SSE_REGISTER_XMM5
#define VM_SSE_ARG7 VM_SSE_REGISTER_XMM6
#define VM_SSE_ARG8 VM_SSE_REGISTER_XMM7
#define VM_MAX_ARGS 6
#define VM_MAX_SSE_ARGS 8
#endif

enum vm_instruction_type
{
    VM_INSTRUCTION_NONE = 0x0,
    VM_INSTRUCTION_UNARY = 0x1,
    VM_INSTRUCTION_BINARY = 0x2
};

enum vm_instruction_code
{
    VM_INSTRUCTION_CODE_NONE = 0x0,
    VM_INSTRUCTION_CODE_DST_REGISTER = 0x1,
    VM_INSTRUCTION_CODE_DST_MEMORY = 0x2,
    VM_INSTRUCTION_CODE_IMMEDIATE = 0x4,
    VM_INSTRUCTION_CODE_SRC_REGISTER = 0x8,
    VM_INSTRUCTION_CODE_SRC_MEMORY = 0x10,
    VM_INSTRUCTION_CODE_OFFSET = 0x20
};

#define CODE_NONE (VM_INSTRUCTION_CODE_NONE)
#define CODE_UR (VM_INSTRUCTION_CODE_DST_REGISTER)
#define CODE_UM (VM_INSTRUCTION_CODE_DST_MEMORY)
#define CODE_UMO (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_UI (VM_INSTRUCTION_CODE_IMMEDIATE)
#define CODE_BRR (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_REGISTER)
#define CODE_BRM (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_SRC_MEMORY)
#define CODE_BMR (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER)
#define CODE_BMRO (VM_INSTRUCTION_CODE_DST_MEMORY | VM_INSTRUCTION_CODE_SRC_REGISTER | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_BRMO (VM_INSTRUCTION_CODE_SRC_MEMORY | VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_OFFSET)
#define CODE_BRI (VM_INSTRUCTION_CODE_DST_REGISTER | VM_INSTRUCTION_CODE_IMMEDIATE)

enum vm_instruction_encoding
{
    VMI_ENC_I = 0,      // destination R, source immediate
    VMI_ENC_MR = 1,     // destination R/M, source R
    VMI_ENC_RM = 2,     // destination R, source R/M
    VMI_ENC_MI = 3,     // destination M, source immediate
    VMI_ENC_M = 4,      // destination and source R/M
    VMI_ENC_OI = 5,     // destination opcode rd (w), source immediate
    VMI_ENC_D = 6,      // jump instruction
    VMI_ENC_A = 7,      // destination R, source R/M
    VMI_ENC_C = 8       // destination R/M, source R
};

enum vm_instructions
{
    VMI_ADD64_SRC_REG_DST_REG,
    VMI_ADD64_SRC_IMM_DST_REG,
    VMI_ADD64_SRC_MEM_DST_REG,
    VMI_ADD64_SRC_REG_DST_MEM,

    VMI_SUB64_SRC_REG_DST_REG,
    VMI_SUB64_SRC_IMM_DST_REG,
    VMI_SUB64_SRC_MEM_DST_REG,
    VMI_SUB64_SRC_REG_DST_MEM,

    VMI_MOV64_SRC_REG_DST_REG,
    VMI_MOV64_SRC_REG_DST_MEM,
    VMI_MOV64_SRC_MEM_DST_REG,
    VMI_MOV64_SRC_IMM_DST_REG,

    VMI_MOV32_SRC_IMM_DST_REG,

    VMI_MUL64_SRC_REG_DST_REG,
    VMI_MUL64_SRC_MEM_DST_REG,

    VMI_INC64_DST_MEM,
    VMI_INC64_DST_REG,

    VMI_DEC64_DST_MEM,
    VMI_DEC64_DST_REG,

    VMI_NEAR_RETURN,
    VMI_FAR_RETURN,

    VMI_CMP64_SRC_REG_DST_REG,
    VMI_CMP64_SRC_REG_DST_MEM,
    VMI_CMP64_SRC_MEM_DST_REG,

    VMI_J8,
    VMI_JE8,
    VMI_JNE8,
    VMI_JL8,
    VMI_JG8,
    VMI_JLE8,
    VMI_JGE8,
    VMI_JA64,

    VMI_J32,
    VMI_JE32,
    VMI_JNE32,
    VMI_JL32,
    VMI_JG32,
    VMI_JLE32,
    VMI_JGE32,

    VMI_NEG64_DST_MEM,
    VMI_NEG64_DST_REG,
    VMI_IDIV_SRC_REG,
    VMI_IDIV_SRC_MEM,

    // End of instructions
    VMI_MAX_INSTRUCTIONS
};

enum vm_sse_instructions
{
    VMI_SSE_MOVSD_SRC_REG_DST_REG,
    VMI_SSE_MOVSD_SRC_REG_DST_MEM,
    VMI_SSE_MOVSD_SRC_MEM_DST_REG,

    VMI_SSE_ADDPD_SRC_REG_DST_REG,

    VMI_SSE_ADDSD_SRC_REG_DST_REG,
    VMI_SSE_ADDSD_SRC_MEM_DST_REG,

    VMI_SSE_SUBSD_SRC_REG_DST_REG,
    VMI_SSE_SUBSD_SRC_MEM_DST_REG,

    VMI_SSE_MULSD_SRC_REG_DST_REG,
    VMI_SSE_MULSD_SRC_MEM_DST_REG,

    VMI_SSE_DIVSD_SRC_REG_DST_REG,
    VMI_SSE_DIVSD_SRC_MEM_DST_REG,

    VMI_SSE_CVTSI2SD_SRC_REG_DST_REG,
    VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG,

    VMI_SSE_UCOMISD_SRC_REG_DST_REG,
    VMI_SSE_UCOMISD_SRC_MEM_DST_REG,

    VMI_SSE_XORPD_SRC_REG_DST_REG,

    VMI_SSE_MOVSS_SRC_REG_DST_REG,
    VMI_SSE_MOVSS_SRC_REG_DST_MEM,
    VMI_SSE_MOVSS_SRC_MEM_DST_REG,

    VMI_SSE_ADDSS_SRC_REG_DST_REG,
    VMI_SSE_ADDSS_SRC_MEM_DST_REG,

    VMI_SSE_SUBSS_SRC_REG_DST_REG,
    VMI_SSE_SUBSS_SRC_MEM_DST_REG,

    VMI_SSE_MULSS_SRC_REG_DST_REG,
    VMI_SSE_MULSS_SRC_MEM_DST_REG,

    VMI_SSE_DIVSS_SRC_REG_DST_REG,
    VMI_SSE_DIVSS_SRC_MEM_DST_REG,

    VMI_SSE_CVTSI2SS_SRC_REG_DST_REG,
    VMI_SSE_CVTSI2SS_SRC_MEM_DST_REG,

    VMI_SSE_UCOMISS_SRC_REG_DST_REG,
    VMI_SSE_UCOMISS_SRC_MEM_DST_REG,

    VMI_SSE_XORPS_SRC_REG_DST_REG,

    // End of instructions
    VMI_SSE_MAX_INSTRUCTIONS
};

struct vm_instruction
{
    unsigned char rex;
    unsigned char ins;
    unsigned char subins;
    unsigned char type;
    unsigned char code;
    unsigned char enc;
};

struct vm_sse_instruction
{
    unsigned char rex;
    unsigned char ins1;
    unsigned char ins2;
    unsigned char ins3;
    unsigned char type;
    unsigned char code;
    unsigned char enc;
};

#define INS(rex, ins, subins, type, code, enc) \
    { (unsigned char)rex, (unsigned char)ins, (unsigned char)subins, (unsigned char)type, (unsigned char)code, (unsigned char)enc }

#define SSE_INS(rex, ins1, ins2, ins3, type, code, enc) \
    { (unsigned char)rex, (unsigned char)ins1, (unsigned char)ins2, (unsigned char)ins3, (unsigned char)type, (unsigned char)code, (unsigned char)enc }

#define VMI_UNUSED 0xFF

static constexpr vm_instruction gInstructions[VMI_MAX_INSTRUCTIONS] = {
    INS(0x48, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),     // VMI_ADD64_SRC_REG_DST_REG
    INS(0x48, 0x81, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_ADD64_SRC_IMM_DST_REG
    INS(0x48, 0x3, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),    // VMI_ADD64_SRC_MEM_DST_REG
    INS(0x48, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),    // VMI_ADD64_SRC_REG_DST_MEM

    INS(0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_SUB64_SRC_REG_DST_REG
    INS(0x48, 0x81, 5, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_SUB64_SRC_IMM_DST_REG
    INS(0x48, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_SUB64_SRC_MEM_DST_REG
    INS(0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_SUB64_SRC_REG_DST_MEM

    INS(0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_MOV64_SRC_REG_DST_REG
    INS(0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_MOV64_SRC_REG_DST_MEM
    INS(0x48, 0x8B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_MOV64_SRC_MEM_DST_REG
    INS(0x48, 0xC7, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_MOV64_SRC_IMM_DST_REG

    INS(0x0, 0xB8, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI),    // VMI_MOV32_SRC_IMM_DST_REG

    INS(0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_MUL64_SRC_REG_DST_REG
    INS(0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_MUL64_SRC_MEM_DST_REG

    INS(0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_INC64_DST_MEM
    INS(0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_INC64_DST_REG

    INS(0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_DEC64_DST_MEM
    INS(0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_DEC64_DST_REG

    INS(0x0, 0xC3, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_NEAR_RETURN
    INS(0x0, 0xCB, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_FAR_RETURN

    INS(0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_CMP64_SRC_REG_DST_REG
    INS(0x48, 0x39, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), // VMI_CMP64_SRC_REG_DST_MEM
    INS(0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_CMP64_SRC_MEM_DST_REG

    INS(0x0, 0xEB, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_J8
    INS(0x0, 0x74, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JE8
    INS(0x0, 0x75, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNE8
    INS(0x0, 0x72, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JL8,
    INS(0x0, 0x77, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JG8,
    INS(0x0, 0x76, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JLE8,
    INS(0x0, 0x73, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JGE8,
    INS(0x0, 0xFF, 0x4, VM_INSTRUCTION_CODE_OFFSET, CODE_UR, VMI_ENC_M), // VMI_JA64

    INS(0x0, 0xE9, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_J32
    INS(0x0, 0x0F, 0x84, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JE32
    INS(0x0, 0x0F, 0x85, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNE32
    INS(0x0, 0x0F, 0x82, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JL32,
    INS(0x0, 0x0F, 0x87, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JG32,
    INS(0x0, 0x0F, 0x86, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JLE32,
    INS(0x0, 0x0F, 0x83, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JGE32,

    INS(0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_NEG64_DST_MEM
    INS(0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_NEG64_DST_REG

    INS(0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV_SRC_REG
    INS(0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV_SRC_MEM
};

static constexpr vm_sse_instruction gInstructions_SSE[VMI_SSE_MAX_INSTRUCTIONS] = {
    SSE_INS(0x0, 0xF2, 0xF, 0x10, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_MOVSD_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF2, 0xF, 0x11, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_C), // VMI_SSE_MOVSD_SRC_REG_DST_MEM
    SSE_INS(0x0, 0xF2, 0xF, 0x10, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_MOVSD_SRC_MEM_DST_REG

    SSE_INS(0x0, 0x66, 0xF, 0x58, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_ADDPD_SRC_REG_DST_REG


    SSE_INS(0x0, 0xF2, 0xF, 0x58, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_ADDSD_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF2, 0xF, 0x58, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A), // VMI_SSE_ADDSD_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF2, 0xF, 0x5C, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_SUBSD_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF2, 0xF, 0x5C, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A), // VMI_SSE_SUBSD_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF2, 0xF, 0x59, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_MULSD_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF2, 0xF, 0x59, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A), // VMI_SSE_MULSD_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF2, 0xF, 0x5E, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_DIVSD_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF2, 0xF, 0x5E, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A), // VMI_SSE_DIVSD_SRC_MEM_DST_REG

    SSE_INS(0x48, 0xF2, 0xF, 0x2A, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_CVTSI2SD_SRC_REG_DST_REG
    SSE_INS(0x48, 0xF2, 0xF, 0x2A, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A), // VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG

    SSE_INS(0x0, 0x66, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A),   // VMI_SSE_UCOMISD_SRC_REG_DST_REG,
    SSE_INS(0x0, 0x66, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_A),    // VMI_SSE_UCOMISD_SRC_MEM_DST_REG,

    SSE_INS(0x0, 0x66, 0xF, 0x57, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_XORPD_SRC_REG_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x10, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_MOVSS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x11, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_C), // VMI_SSE_MOVSS_SRC_REG_DST_MEM
    SSE_INS(0x0, 0xF3, 0xF, 0x10, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_MOVSS_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x58, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_ADDSS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x58, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_ADDSS_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x5C, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_SUBSS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x5C, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_SUBSS_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x59, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_MULSS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x59, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_MULSS_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x5E, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_DIVSS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x5E, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_DIVSS_SRC_MEM_DST_REG

    SSE_INS(0x0, 0xF3, 0xF, 0x2A, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_CVTSI2SS_SRC_REG_DST_REG
    SSE_INS(0x0, 0xF3, 0xF, 0x2A, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_CVTSI2SS_SRC_MEM_DST_REG

    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_UCOMISS_SRC_REG_DST_REG
    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_UCOMISS_SRC_MEM_DST_REG

    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x57, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_XORPS_SRC_REG_DST_REG

};

//=====================
// R/M 0-2
// Op/Reg 3-5
// Mod 6-8
//=====================

static void vm_emit(const vm_instruction& ins, unsigned char* program, int& count)
{
    assert(ins.code == CODE_NONE);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;
}

static void vm_emit_ur(const vm_instruction& ins, unsigned char* program, int& count, char reg)
{
    assert(ins.code == CODE_UR);
    if (ins.rex > 0)
    {
        program[count++] = ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0);
    }
    if (ins.subins != VMI_UNUSED)
    {
        program[count++] = ins.ins;
        program[count++] = (ins.subins << 3) | ((reg % 8) & 0x7) | (0x3 << 6);
    }
    else
    {
        program[count++] = ins.ins | ((reg % 8) & 0x7);
    }
}

static void vm_emit_um(const vm_instruction& ins, unsigned char* program, int& count, char reg)
{
    assert(ins.code == CODE_UM);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (reg == VM_REGISTER_ESP)
    {
        program[count++] = ((ins.subins & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24; // SIB byte
    }
    else
    {
        program[count++] = ((ins.subins & 0x7) << 3) | (0x0 << 6) | (reg & 0x7);
    }
}

static void vm_emit_umo(const vm_instruction& ins, unsigned char* program, int& count, char reg, int offset)
{
    assert(ins.code == CODE_UMO);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (reg == VM_REGISTER_ESP)
    {
        program[count++] = ((ins.subins & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24; // SIB byte
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = ((ins.subins & 0x7) << 3) | (0x2 << 6) | (reg & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

static void vm_emit_bri(const vm_instruction& ins, unsigned char* program, int& count, char reg, int imm)
{
    assert(ins.code == CODE_BRI);
    if (ins.rex > 0)
    {
        program[count++] = ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0);
        program[count++] = ins.ins;
        program[count++] = (ins.subins << 3) | (reg % 8) | (0x3 << 6);
    }
    else
    {
        program[count++] = ins.ins | reg;
    }

    program[count++] = (unsigned char)(imm & 0xff);
    program[count++] = (unsigned char)((imm >> 8) & 0xff);
    program[count++] = (unsigned char)((imm >> 16) & 0xff);
    program[count++] = (unsigned char)((imm >> 24) & 0xff);
}

static void vm_emit_brr(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src)
{
    assert(ins.code == CODE_BRR);

    if (ins.enc == VMI_ENC_MR)
    {
        if (ins.rex > 0) { program[count++] = ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0) | (src >= VM_REGISTER_R8 ? 0x4 : 0x0); }
        program[count++] = ins.ins;
        if (ins.subins != VMI_UNUSED) { program[count++] = ins.subins; }

        program[count++] = 0xC0 | (((src % 8) & 0x7) << 0x3) | (((dst % 8) & 0x7) << 0);
    }
    else if (ins.enc == VMI_ENC_RM)
    {
        if (ins.rex > 0) { program[count++] = ins.rex | (dst >= VM_REGISTER_R8 ? 0x4 : 0x0) | (src >= VM_REGISTER_R8 ? 0x1 : 0x0); }
        program[count++] = ins.ins;
        if (ins.subins != VMI_UNUSED) { program[count++] = ins.subins; }

        program[count++] = 0xC0 | (((dst % 8) & 0x7) << 0x3) | (((src % 8) & 0x7) << 0);
    }
}

static void vm_emit_brm(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src)
{
    assert(ins.code == CODE_BRM);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = ((dst & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24; // SIB byte
    }
    else
    {
        program[count++] = ((dst & 0x7) << 3) | (0x0 << 6) | (src & 0x7);
    }
}

static void vm_emit_bmr(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src)
{
    assert(ins.code == CODE_BMR);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x0 << 6);
        program[count++] = 0x24; // SIB byte
    }
    else
    {
        program[count++] = ((src & 0x7) << 3) | (0x0 << 6) | (dst & 0x7);
    }
}

static void vm_emit_brmo(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src, int offset)
{
    assert(ins.code == CODE_BRMO);
    if (ins.rex > 0)
    {
        program[count++] = ins.rex | (dst >= VM_REGISTER_R8 ? 0x4 : 0x0) | (src >= VM_REGISTER_R8 ? 0x1 : 0x0);
    }
    program[count++] = ins.ins;
    if (ins.subins != VMI_UNUSED) { program[count++] = ins.subins; }

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = (((dst % 8) & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24; // SIB byte
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = (((dst % 8) & 0x7) << 3) | (0x2 << 6) | ((src % 8) & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

static void vm_emit_bmro(const vm_instruction& ins, unsigned char* program, int& count, char dst, char src, int offset)
{
    assert(ins.code == CODE_BMRO);
    if (ins.rex > 0)
    {
        program[count++] = ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0) | (src >= VM_REGISTER_R8 ? 0x4 : 0x0);
    }
    program[count++] = ins.ins;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = ((src & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24; // SIB byte
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = (((src % 8) & 0x7) << 3) | (0x2 << 6) | ((dst % 8) & 0x7);
        program[count++] = (unsigned char)(offset & 0xff);
        program[count++] = (unsigned char)((offset >> 8) & 0xff);
        program[count++] = (unsigned char)((offset >> 16) & 0xff);
        program[count++] = (unsigned char)((offset >> 24) & 0xff);
    }
}

static void vm_emit_ui(const vm_instruction& ins, unsigned char* program, int& count, char imm)
{
    assert(ins.code == CODE_UI);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;
    program[count++] = imm;
}

static void vm_emit_ui(const vm_instruction& ins, unsigned char* program, int& count, int imm)
{
    assert(ins.code == CODE_UI);
    if (ins.rex > 0) { program[count++] = ins.rex; }
    program[count++] = ins.ins;
    if (ins.subins != VMI_UNUSED)
    {
        program[count++] = ins.subins;
    }
    program[count++] = imm & 0xFF;
    program[count++] = (imm >> 8) & 0xFF;
    program[count++] = (imm >> 16) & 0xFF;
    program[count++] = (imm >> 24) & 0xFF;
}

//================

static void vm_emit_sse_brr(const vm_sse_instruction& ins, unsigned char* program, int& count, int src, int dst)
{
    assert(ins.code == CODE_BRR);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        program[count++] = ins.ins1;
    }
    unsigned char rex = ins.rex;
    if (src >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { program[count++] = rex; }
    program[count++] = ins.ins2;
    program[count++] = ins.ins3;

    program[count++] = (((dst % 8) & 0x7) << 3) | (0x3 << 6) | ((src % 8) & 0x7);
}

static void vm_emit_sse_brmo(const vm_sse_instruction& ins, unsigned char* program, int& count, int src, int dst, int src_offset)
{
    assert(ins.code == CODE_BRMO);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        program[count++] = ins.ins1;
    }
    unsigned char rex = ins.rex;
    if (src >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (dst >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { program[count++] = rex; }

    program[count++] = ins.ins2;
    program[count++] = ins.ins3;

    if (src == VM_REGISTER_ESP)
    {
        program[count++] = (((dst % 8) & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24; // SIB byte
        program[count++] = (unsigned char)(src_offset & 0xff);
        program[count++] = (unsigned char)((src_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((src_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = (((dst % 8) & 0x7) << 3) | (0x2 << 6) | ((src % 8) & 0x7);
        program[count++] = src_offset & 0xFF;
        program[count++] = (src_offset >> 8) & 0xFF;
        program[count++] = (src_offset >> 16) & 0xFF;
        program[count++] = (src_offset >> 24) & 0xFF;
    }
}

static void vm_emit_sse_bmro(const vm_sse_instruction& ins, unsigned char* program, int& count, int src, int dst, int dst_offset)
{
    assert(ins.code == CODE_BMRO);
    assert(ins.enc == VMI_ENC_A || ins.enc == VMI_ENC_C);
    if (ins.ins1 != VMI_UNUSED) {
        program[count++] = ins.ins1;
    }
    unsigned char rex = ins.rex;

    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (src >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { program[count++] = rex; }

    program[count++] = ins.ins2;
    program[count++] = ins.ins3;

    if (dst == VM_REGISTER_ESP)
    {
        program[count++] = (((src % 8) & 0x7) << 3) | 0x4 | (0x2 << 6);
        program[count++] = 0x24; // SIB byte
        program[count++] = (unsigned char)(dst_offset & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 8) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 16) & 0xff);
        program[count++] = (unsigned char)((dst_offset >> 24) & 0xff);
    }
    else
    {
        program[count++] = (((src % 8) & 0x7) << 3) | (0x2 << 6) | ((dst % 8) & 0x7);
        program[count++] = dst_offset & 0xFF;
        program[count++] = (dst_offset >> 8) & 0xFF;
        program[count++] = (dst_offset >> 16) & 0xFF;
        program[count++] = (dst_offset >> 24) & 0xFF;
    }
}

static void vm_emit_sse_brm(const vm_sse_instruction& ins, unsigned char* program, int& count, int dst, int disp32)
{
    assert(ins.code == CODE_BRMO);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        program[count++] = ins.ins1;
    }
    unsigned char rex = ins.rex;

    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { program[count++] = rex; }

    program[count++] = ins.ins2;
    program[count++] = ins.ins3;

    /* Relative RIP addressing (Displacement from end of instruction) */

    program[count++] = (((dst % 8) & 0x7) << 3) | 0x5 | (0x0 << 6);
    program[count++] = (unsigned char)(disp32 & 0xff);
    program[count++] = (unsigned char)((disp32 >> 8) & 0xff);
    program[count++] = (unsigned char)((disp32 >> 16) & 0xff);
    program[count++] = (unsigned char)((disp32 >> 24) & 0xff);
}

//================
#define VM_ALIGN_16(x) ((x + 0xf) & ~(0xf))

static void vm_reserve(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
}

static void vm_reserve2(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
}

static void vm_reserve3(unsigned char* program, int& count)
{
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
    program[count++] = 0x90; // nop
}

static void vm_cdq(unsigned char* program, int& count)
{
    program[count++] = 0x99;
}

static void vm_return(unsigned char* program, int& count)
{
    //vm_emit(table.ret, program, count);

    vm_emit(gInstructions[VMI_NEAR_RETURN], program, count);
}

static void vm_push_32(unsigned char* program, int& count, const int imm)
{
    program[count++] = 0x68;
    program[count++] = (unsigned char)(imm & 0xFF);
    program[count++] = (unsigned char)((imm >> 8) & 0xFF);
    program[count++] = (unsigned char)((imm >> 16) & 0xFF);
    program[count++] = (unsigned char)((imm >> 24) & 0xFF);
}

static void vm_push_reg(unsigned char* program, int& count, char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        program[count++] = 0x48 | 0x1;
    }

    program[count++] = 0x50 | ((reg % 8) & 0x7);
}

static void vm_pop_reg(unsigned char* program, int& count, char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        program[count++] = 0x48 | 0x1;
    }

    program[count++] = 0x58 | ((reg % 8) & 0x7);
}

static void vm_mov_imm_to_reg(unsigned char* program, int& count, char dst, int imm)
{
    vm_emit_bri(gInstructions[VMI_MOV32_SRC_IMM_DST_REG], program, count, dst, imm);
}

static void vm_mov_imm_to_reg_x64(unsigned char* program, int& count, char dst, long long imm)
{
    //vm_emit_bri(gInstructions[VMI_MOV64_SRC_IMM_DST_REG], program, count, dst, imm);

    program[count++] = 0x48 | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0);
    program[count++] = 0xB8 | (dst % 8);
    program[count++] = (unsigned char)(imm & 0xff);
    program[count++] = (unsigned char)((imm >> 8) & 0xff);
    program[count++] = (unsigned char)((imm >> 16) & 0xff);
    program[count++] = (unsigned char)((imm >> 24) & 0xff);
    program[count++] = (unsigned char)((imm >> 32) & 0xff);
    program[count++] = (unsigned char)((imm >> 40) & 0xff);
    program[count++] = (unsigned char)((imm >> 48) & 0xff);
    program[count++] = (unsigned char)((imm >> 56) & 0xff);
}

inline static void vm_mov_reg_to_memory_x64(unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    vm_emit_bmro(gInstructions[VMI_MOV64_SRC_REG_DST_MEM], program, count, dst, src, dst_offset);
}

inline static void vm_mov_memory_to_reg_x64(unsigned char* program, int& count, char dst, char src, int src_offset)
{
    vm_emit_brmo(gInstructions[VMI_MOV64_SRC_MEM_DST_REG], program, count, dst, src, src_offset);
}

inline static void vm_mov_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    vm_emit_brr(gInstructions[VMI_MOV64_SRC_REG_DST_REG], program, count, dst, src);
}

inline static void vm_add_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    vm_emit_brr(gInstructions[VMI_ADD64_SRC_REG_DST_REG], program, count, dst, src);
}

inline static void vm_add_memory_to_reg_x64(unsigned char* program, int& count, char dst, char src, int src_offset)
{
    vm_emit_brmo(gInstructions[VMI_ADD64_SRC_MEM_DST_REG], program, count, dst, src, src_offset);
}

inline static void vm_add_reg_to_memory_x64(unsigned char* program, int& count, char dst, char src, int dst_offset)
{
    vm_emit_bmro(gInstructions[VMI_ADD64_SRC_REG_DST_MEM], program, count, dst, src, dst_offset);
}

inline static void vm_add_imm_to_reg_x64(unsigned char* program, int& count, char dst, int imm)
{
    vm_emit_bri(gInstructions[VMI_ADD64_SRC_IMM_DST_REG], program, count, dst, imm);
}

inline static void vm_sub_reg_to_memory_x64(unsigned char* program, int& count, char dst, char src, int dst_offset)
{
    vm_emit_bmro(gInstructions[VMI_SUB64_SRC_REG_DST_MEM], program, count, dst, src, dst_offset);
}

inline static void vm_sub_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    vm_emit_brr(gInstructions[VMI_SUB64_SRC_REG_DST_REG], program, count, dst, src);
}

inline static void vm_sub_memory_to_reg_x64(unsigned char* program, int& count, char dst, char src, int src_offset)
{
    vm_emit_brmo(gInstructions[VMI_SUB64_SRC_MEM_DST_REG], program, count, dst, src, src_offset);
}

inline static void vm_sub_imm_to_reg_x64(unsigned char* program, int& count, char reg, int imm)
{
    vm_emit_bri(gInstructions[VMI_SUB64_SRC_IMM_DST_REG], program, count, reg, imm);
}

inline static void vm_mul_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    vm_emit_brr(gInstructions[VMI_MUL64_SRC_REG_DST_REG], program, count, dst, src);
}

inline static void vm_mul_memory_to_reg_x64(unsigned char* program, int& count, char dst, char src, int src_offset)
{
    vm_emit_brmo(gInstructions[VMI_MUL64_SRC_MEM_DST_REG], program, count, dst, src, src_offset);
}

inline static void vm_div_reg_x64(unsigned char* program, int& count, char reg)
{
    vm_emit_ur(gInstructions[VMI_IDIV_SRC_REG], program, count, reg);
}

inline static void vm_div_memory_x64(unsigned char* program, int& count, char src, int src_offset)
{
    vm_emit_umo(gInstructions[VMI_IDIV_SRC_MEM], program, count, src, src_offset);
}

inline static void vm_cmp_reg_to_reg_x64(unsigned char* program, int& count, char dst, char src)
{
    vm_emit_brr(gInstructions[VMI_CMP64_SRC_REG_DST_REG], program, count, dst, src);
}

inline static void vm_cmp_reg_to_memory_x64(unsigned char* program, int& count, char dst, int dst_offset, char src)
{
    vm_emit_bmro(gInstructions[VMI_CMP64_SRC_REG_DST_MEM], program, count, dst, src, dst_offset);
}

inline static void vm_cmp_memory_to_reg_x64(unsigned char* program, int& count, char dst, char src, int src_offset)
{
    vm_emit_brmo(gInstructions[VMI_CMP64_SRC_MEM_DST_REG], program, count, dst, src, src_offset);
}

inline static void vm_jump_unconditional_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_J8], program, count, imm);
}

inline static void vm_jump_equals_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JE8], program, count, imm);
}

inline static void vm_jump_not_equals_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JNE8], program, count, imm);
}

inline static void vm_jump_less_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JL8], program, count, imm);
}

inline static void vm_jump_less_equal_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JLE8], program, count, imm);
}

inline static void vm_jump_greater_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JG8], program, count, imm);
}

inline static void vm_jump_greater_equal_8(unsigned char* program, int& count, char imm)
{
    vm_emit_ui(gInstructions[VMI_JGE8], program, count, imm);
}

inline static void vm_jump_unconditional(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_J32], program, count, imm);
}

inline static void vm_jump_equals(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JE32], program, count, imm);
}

inline static void vm_jump_not_equals(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JNE32], program, count, imm);
}

inline static void vm_jump_less(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JL32], program, count, imm);
}

inline static void vm_jump_less_equal(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JLE32], program, count, imm);
}

inline static void vm_jump_greater(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JG32], program, count, imm);
}

inline static void vm_jump_greater_equal(unsigned char* program, int& count, int imm)
{
    vm_emit_ui(gInstructions[VMI_JGE32], program, count, imm);
}

inline static void vm_jump_absolute(unsigned char* program, int& count, char reg)
{
    vm_emit_ur(gInstructions[VMI_JA64], program, count, reg);
}

static void vm_call(unsigned char* program, int& count, int offset)
{
    offset -= 5;

    program[count++] = 0xE8;
    program[count++] = (unsigned char)(offset & 0xff);
    program[count++] = (unsigned char)((offset >> 8) & 0xff);
    program[count++] = (unsigned char)((offset >> 16) & 0xff);
    program[count++] = (unsigned char)((offset >> 24) & 0xff);
}

inline static void vm_call_absolute(unsigned char* program, int& count, int reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        program[count++] = 0x1 | (0x1 << 6);
    }

    program[count++] = 0xFF;
    program[count++] = (0x2 << 3) | (reg % 8) | (0x3 << 6);
}

inline static void vm_inc_reg_x64(unsigned char* program, int& count, int reg)
{
    vm_emit_ur(gInstructions[VMI_INC64_DST_REG], program, count, reg);
}

inline static void vm_inc_memory_x64(unsigned char* program, int& count, int reg, int offset)
{
    vm_emit_umo(gInstructions[VMI_INC64_DST_MEM], program, count, reg, offset);
}

inline static void vm_dec_reg_x64(unsigned char* program, int& count, int reg)
{
    vm_emit_ur(gInstructions[VMI_DEC64_DST_REG], program, count, reg);
}

inline static void vm_dec_memory_x64(unsigned char* program, int& count, int reg, int offset)
{
    vm_emit_umo(gInstructions[VMI_DEC64_DST_MEM], program, count, reg, offset);
}

inline static void vm_neg_memory_x64(unsigned char* program, int& count, int reg, int offset)
{
    vm_emit_umo(gInstructions[VMI_NEG64_DST_MEM], program, count, reg, offset);
}

inline static void vm_neg_reg_x64(unsigned char* program, int& count, int reg)
{
    vm_emit_ur(gInstructions[VMI_NEG64_DST_REG], program, count, reg);
}

inline static void vm_movsd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_movsd_reg_to_memory_x64(unsigned char* program, int& count, int dst, int src, int dst_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_MEM], program, count, src, dst, dst_offset);
#else
    vm_emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_MEM], program, count, src, dst, dst_offset);
#endif
}

inline static void vm_movsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int src_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#endif
}

inline static void vm_movsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int addr)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], program, count, dst, addr);
#else
    vm_emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], program, count, dst, addr);
#endif
}

inline static void vm_addsd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_addsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int src_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSS_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSD_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#endif
}

inline static void vm_subsd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_subsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int src_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSS_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSD_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#endif
}

inline static void vm_mulsd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_mulsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int dst_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSS_SRC_MEM_DST_REG], program, count, src, dst, dst_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSD_SRC_MEM_DST_REG], program, count, src, dst, dst_offset);
#endif
}

inline static void vm_divsd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_divsd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int dst_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSS_SRC_MEM_DST_REG], program, count, src, dst, dst_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSD_SRC_MEM_DST_REG], program, count, src, dst, dst_offset);
#endif
}

inline static void vm_cvtitod_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int src_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#endif
}

inline static void vm_cvtitod_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_ucmpd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

inline static void vm_ucmpd_memory_to_reg_x64(unsigned char* program, int& count, int dst, int src, int src_offset)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#else
    vm_emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_MEM_DST_REG], program, count, src, dst, src_offset);
#endif
}

inline static void vm_xorpd_reg_to_reg_x64(unsigned char* program, int& count, int dst, int src)
{
#ifdef USE_SUN_FLOAT
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPS_SRC_REG_DST_REG], program, count, src, dst);
#else
    vm_emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPD_SRC_REG_DST_REG], program, count, src, dst);
#endif
}

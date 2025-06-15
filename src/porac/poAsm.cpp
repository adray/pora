#include "poAsm.h"
#include "poModule.h"
#include "poRegLinear.h"
#include "poType.h"

#include "poLive.h"
#include "poDom.h"

#include <assert.h>
#include <sstream>
#include <iostream>
#include <cstring>

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

    VMI_ADD8_SRC_IMM_DST_REG,
    VMI_ADD8_SRC_REG_DST_REG,
    VMI_ADD8_SRC_MEM_DST_REG,
    VMI_ADD8_SRC_REG_DST_MEM,

    VMI_SUB64_SRC_REG_DST_REG,
    VMI_SUB64_SRC_IMM_DST_REG,
    VMI_SUB64_SRC_MEM_DST_REG,
    VMI_SUB64_SRC_REG_DST_MEM,

    VMI_SUB8_SRC_REG_DST_REG,
    VMI_SUB8_SRC_IMM_DST_REG,
    VMI_SUB8_SRC_REG_DST_MEM,
    VMI_SUB8_SRC_MEM_DST_REG,

    VMI_MOV64_SRC_REG_DST_REG,
    VMI_MOV64_SRC_REG_DST_MEM,
    VMI_MOV64_SRC_MEM_DST_REG,
    VMI_MOV64_SRC_IMM_DST_REG,

    VMI_MOV32_SRC_IMM_DST_REG,

    VMI_MOV8_SRC_IMM_DST_REG,
    VMI_MOV8_SRC_REG_DST_REG,
    VMI_MOV8_SRC_MEM_DST_REG,
    VMI_MOV8_SRC_REG_DST_MEM,

    VMI_MUL64_SRC_REG_DST_REG,
    VMI_MUL64_SRC_MEM_DST_REG,

    VMI_IMUL8_SRC_REG,
    VMI_MUL8_SRC_REG,

    VMI_INC64_DST_MEM,
    VMI_INC64_DST_REG,

    VMI_DEC64_DST_MEM,
    VMI_DEC64_DST_REG,

    VMI_NEAR_RETURN,
    VMI_FAR_RETURN,

    VMI_CMP64_SRC_REG_DST_REG,
    VMI_CMP64_SRC_REG_DST_MEM,
    VMI_CMP64_SRC_MEM_DST_REG,

    VMI_CMP8_SRC_REG_DST_REG,
    VMI_CMP8_SRC_MEM_DST_REG,
    VMI_CMP8_SRC_REG_DST_MEM,

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
    VMI_JNAE32, /*unsigned*/
    VMI_JNBE32, /*unsigned*/
    VMI_JNA32, /*unsigned*/
    VMI_JNB32, /*unsigned*/
    VMI_JG32, /*signed*/ /*0F 8F*/
    VMI_JGE32, /*signed*/ /*0F 8D*/
    VMI_JL32, /*signed*/ /*0F 8C*/
    VMI_JLE32, /*signed*/ /*0F 8E*/

    VMI_NEG8_DST_MEM,
    VMI_NEG8_DST_REG,

    VMI_NEG64_DST_MEM,
    VMI_NEG64_DST_REG,
    VMI_IDIV64_SRC_REG,
    VMI_IDIV64_SRC_MEM,

    VMI_IDIV8_SRC_REG,
    VMI_IDIV8_SRC_MEM,

    VMI_DIV8_SRC_REG,
    VMI_DIV8_SRC_MEM,

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

    INS(0x48, 0xB0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_ADD8_SRC_IMM_DST_REG,
    INS(0x48, 0x2, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_ADD8_SRC_REG_DST_REG,
    INS(0x48, 0x2, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_ADD8_SRC_MEM_DST_REG,
    INS(0x48, 0x0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), // VMI_ADD8_SRC_REG_DST_MEM,

    INS(0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_SUB64_SRC_REG_DST_REG
    INS(0x48, 0x81, 5, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_SUB64_SRC_IMM_DST_REG
    INS(0x48, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_SUB64_SRC_MEM_DST_REG
    INS(0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_SUB64_SRC_REG_DST_MEM

    INS(0x48, 0x28, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_SUB8_SRC_REG_DST_REG
    INS(0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_SUB8_SRC_IMM_DST_REG
    INS(0x48, 0x28, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR),  // VMI_SUB8_SRC_REG_DST_MEM
    INS(0x48, 0x2A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_SUB8_SRC_MEM_DST_REG

    INS(0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_MOV64_SRC_REG_DST_REG
    INS(0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_MOV64_SRC_REG_DST_MEM
    INS(0x48, 0x8B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_MOV64_SRC_MEM_DST_REG
    INS(0x48, 0xC7, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_MOV64_SRC_IMM_DST_REG

    INS(0x0, 0xB8, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI),    // VMI_MOV32_SRC_IMM_DST_REG

    INS(0x48, 0xB0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI), //VMI_MOV8_SRC_IMM_DST_REG
    INS(0x48, 0x88, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), //VMI_MOV8_SRC_REG_DST_REG,
    INS(0x48, 0x8A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), //VMI_MOV8_SRC_MEM_DST_REG,
    INS(0x48, 0x88, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), //VMI_MOV8_SRC_REG_DST_MEM,

    INS(0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_MUL64_SRC_REG_DST_REG
    INS(0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_MUL64_SRC_MEM_DST_REG

    INS(0x48, 0xF6, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_UR, VMI_ENC_M), // VMI_IMUL8_SRC_REG /*signed*/
    INS(0x40, 0xF6, 0x4, VM_INSTRUCTION_BINARY, CODE_UR, VMI_ENC_M), // VMI_MUL8_SRC_REG /*unsigned*/


    INS(0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_INC64_DST_MEM
    INS(0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_INC64_DST_REG

    INS(0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_DEC64_DST_MEM
    INS(0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_DEC64_DST_REG

    INS(0x0, 0xC3, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_NEAR_RETURN
    INS(0x0, 0xCB, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_FAR_RETURN

    INS(0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_CMP64_SRC_REG_DST_REG
    INS(0x48, 0x39, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), // VMI_CMP64_SRC_REG_DST_MEM
    INS(0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_CMP64_SRC_MEM_DST_REG

    INS(0x40, 0x38, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_CMP8_SRC_REG_DST_REG
    INS(0x40, 0x3A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_CMP8_SRC_MEM_DST_REG
    INS(0x40, 0x38, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), // VMI_CMP8_SRC_REG_DST_MEM

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
    INS(0x0, 0x0F, 0x85, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNE32, /*unsigned*/
    INS(0x0, 0x0F, 0x82, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNAE32, /*unsigned*/
    INS(0x0, 0x0F, 0x87, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNBE32, /*unsigned*/
    INS(0x0, 0x0F, 0x86, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNB32, /*unsigned*/
    INS(0x0, 0x0F, 0x83, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNA32, /*unsigned*/
    INS(0x0, 0x0F, 0x8F, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D),  //VMI_JG32, /*signed*/
    INS(0x0, 0x0F, 0x8D, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JGE32, /*signed*/
    INS(0x0, 0x0F, 0x8C, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JL32, /*signed*/
    INS(0x0, 0x0F, 0x8E, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JLE32, /*signed*/

    INS(0x40, 0xF6, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M), // VMI_NEG8_DST_MEM,
    INS(0x40, 0xF6, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_NEG8_DST_REG,

    INS(0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_NEG64_DST_MEM
    INS(0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_NEG64_DST_REG

    INS(0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV64_SRC_REG /* signed */
    INS(0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV64_SRC_MEM /* signed */

    INS(0x40, 0xF6, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV8_SRC_REG /* signed */
    INS(0x40, 0xF6, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV8_SRC_MEM /* signed */

    INS(0x40, 0xF6, 0x6, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV8_SRC_REG /* unsigned */
    INS(0x40, 0xF6, 0x6, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV8_SRC_MEM /* unsigned */
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

//=================
// poAsmJump
//=================

poAsmJump::poAsmJump(const int programDataPos, const int jumpType, const int size, poBasicBlock* bb, const int type)
    :
    _programDataPos(programDataPos),
    _jumpType(jumpType),
    _type(type),
    _size(size),
    _bb(bb),
    _applied(false)
{
}

//====================
// poAsmBasicBlock
//====================

poAsmBasicBlock::poAsmBasicBlock()
    :
    _pos(0)
{
}

//==============
// poAsmCall
//==============

poAsmCall::poAsmCall(const int pos, const int arity, const std::string& symbol)
    :
    _pos(pos),
    _arity(arity),
    _symbol(symbol)
{
}

//==================
// poAsmConstant
//==================

poAsmConstant::poAsmConstant(const int initializedDataPos, const int programDataPos)
    :
    _initializedDataPos(initializedDataPos),
    _programDataPos(programDataPos)
{
}


//=====================
// R/M 0-2
// Op/Reg 3-5
// Mod 6-8
//=====================

void poAsm::emit(const vm_instruction& ins)
{
    assert(ins.code == CODE_NONE);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);
}

void poAsm::emit_ur(const vm_instruction& ins, char reg)
{
    assert(ins.code == CODE_UR);
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0));
    }
    if (ins.subins != VMI_UNUSED)
    {
        _programData.push_back(ins.ins);
        _programData.push_back((ins.subins << 3) | ((reg % 8) & 0x7) | (0x3 << 6));
    }
    else
    {
        _programData.push_back(ins.ins | ((reg % 8) & 0x7));
    }
}

void poAsm::emit_um(const vm_instruction& ins, char reg)
{
    assert(ins.code == CODE_UM);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if (reg == VM_REGISTER_ESP)
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | (0x0 << 6) | (reg & 0x7));
    }
}

void poAsm::emit_umo(const vm_instruction& ins, char reg, int offset)
{
    assert(ins.code == CODE_UMO);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if (reg == VM_REGISTER_ESP)
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | 0x4 | (0x2 << 6));
        _programData.push_back(0x24); // SIB byte
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
    else
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | (0x2 << 6) | (reg & 0x7));
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
}

void poAsm::emit_bri(const vm_instruction& ins, char reg, int imm)
{
    assert(ins.code == CODE_BRI);
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0));
        _programData.push_back(ins.ins);
        _programData.push_back((ins.subins << 3) | (reg % 8) | (0x3 << 6));
    }
    else
    {
        _programData.push_back(ins.ins | reg);
    }

    _programData.push_back((unsigned char)(imm & 0xff));
    _programData.push_back((unsigned char)((imm >> 8) & 0xff));
    _programData.push_back((unsigned char)((imm >> 16) & 0xff));
    _programData.push_back((unsigned char)((imm >> 24) & 0xff));
}

void poAsm::emit_brr(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BRR);

    if (ins.enc == VMI_ENC_MR)
    {
        if (ins.rex > 0) { _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0) | (src >= VM_REGISTER_R8 ? 0x4 : 0x0)); }
        _programData.push_back(ins.ins);
        if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

        _programData.push_back(0xC0 | (((src % 8) & 0x7) << 0x3) | (((dst % 8) & 0x7) << 0));
    }
    else if (ins.enc == VMI_ENC_RM)
    {
        if (ins.rex > 0) { _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x4 : 0x0) | (src >= VM_REGISTER_R8 ? 0x1 : 0x0)); }
        _programData.push_back(ins.ins);
        if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

        _programData.push_back(0xC0 | (((dst % 8) & 0x7) << 0x3) | (((src % 8) & 0x7) << 0));
    }
}

void poAsm::emit_brm(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BRM);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if (src == VM_REGISTER_ESP)
    {
        _programData.push_back(((dst & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((dst & 0x7) << 3) | (0x0 << 6) | (src & 0x7));
    }
}

void poAsm::emit_bmr(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BMR);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if (dst == VM_REGISTER_ESP)
    {
        _programData.push_back(((src & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((src & 0x7) << 3) | (0x0 << 6) | (dst & 0x7));
    }
}

void poAsm::emit_brmo(const vm_instruction& ins, char dst, char src, int offset)
{
    assert(ins.code == CODE_BRMO);
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x4 : 0x0) | (src >= VM_REGISTER_R8 ? 0x1 : 0x0));
    }
    _programData.push_back(ins.ins);
    if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

    if (src == VM_REGISTER_ESP)
    {
        _programData.push_back((((dst % 8) & 0x7) << 3) | 0x4 | (0x2 << 6));
        _programData.push_back(0x24); // SIB byte
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
    else
    {
        _programData.push_back((((dst % 8) & 0x7) << 3) | (0x2 << 6) | ((src % 8) & 0x7));
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
}

void poAsm::emit_bmro(const vm_instruction& ins, char dst, char src, int offset)
{
    assert(ins.code == CODE_BMRO);
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0) | (src >= VM_REGISTER_R8 ? 0x4 : 0x0));
    }
    _programData.push_back(ins.ins);

    if (dst == VM_REGISTER_ESP)
    {
        _programData.push_back(((src & 0x7) << 3) | 0x4 | (0x2 << 6));
        _programData.push_back(0x24); // SIB byte
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
    else
    {
        _programData.push_back((((src % 8) & 0x7) << 3) | (0x2 << 6) | ((dst % 8) & 0x7));
        _programData.push_back((unsigned char)(offset & 0xff));
        _programData.push_back((unsigned char)((offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((offset >> 24) & 0xff));
    }
}

void poAsm::emit_ui(const vm_instruction& ins, char imm)
{
    assert(ins.code == CODE_UI);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);
    _programData.push_back(imm);
}

void poAsm::emit_ui(const vm_instruction& ins, int imm)
{
    assert(ins.code == CODE_UI);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);
    if (ins.subins != VMI_UNUSED)
    {
        _programData.push_back(ins.subins);
    }
    _programData.push_back(imm & 0xFF);
    _programData.push_back((imm >> 8) & 0xFF);
    _programData.push_back((imm >> 16) & 0xFF);
    _programData.push_back((imm >> 24) & 0xFF);
}

//================

void poAsm::emit_sse_brr(const vm_sse_instruction& ins, int src, int dst)
{
    assert(ins.code == CODE_BRR);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        _programData.push_back(ins.ins1);
    }
    unsigned char rex = ins.rex;
    if (src >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { _programData.push_back(rex); }
    _programData.push_back(ins.ins2);
    _programData.push_back(ins.ins3);

    _programData.push_back((((dst % 8) & 0x7) << 3) | (0x3 << 6) | ((src % 8) & 0x7));
}

void poAsm::emit_sse_brmo(const vm_sse_instruction& ins, int src, int dst, int src_offset)
{
    assert(ins.code == CODE_BRMO);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        _programData.push_back(ins.ins1);
    }
    unsigned char rex = ins.rex;
    if (src >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (dst >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { _programData.push_back(rex); }

    _programData.push_back(ins.ins2);
    _programData.push_back(ins.ins3);

    if (src == VM_REGISTER_ESP)
    {
        _programData.push_back((((dst % 8) & 0x7) << 3) | 0x4 | (0x2 << 6));
        _programData.push_back(0x24); // SIB byte
        _programData.push_back((unsigned char)(src_offset & 0xff));
        _programData.push_back((unsigned char)((src_offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((src_offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((src_offset >> 24) & 0xff));
    }
    else
    {
        _programData.push_back((((dst % 8) & 0x7) << 3) | (0x2 << 6) | ((src % 8) & 0x7));
        _programData.push_back(src_offset & 0xFF);
        _programData.push_back((src_offset >> 8) & 0xFF);
        _programData.push_back((src_offset >> 16) & 0xFF);
        _programData.push_back((src_offset >> 24) & 0xFF);
    }
}

void poAsm::emit_sse_bmro(const vm_sse_instruction& ins, int src, int dst, int dst_offset)
{
    assert(ins.code == CODE_BMRO);
    assert(ins.enc == VMI_ENC_A || ins.enc == VMI_ENC_C);
    if (ins.ins1 != VMI_UNUSED) {
        _programData.push_back(ins.ins1);
    }
    unsigned char rex = ins.rex;

    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x1 | (1 << 6); }
    if (src >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { _programData.push_back(rex); }

    _programData.push_back(ins.ins2);
    _programData.push_back(ins.ins3);

    if (dst == VM_REGISTER_ESP)
    {
        _programData.push_back((((src % 8) & 0x7) << 3) | 0x4 | (0x2 << 6));
        _programData.push_back(0x24); // SIB byte
        _programData.push_back((unsigned char)(dst_offset & 0xff));
        _programData.push_back((unsigned char)((dst_offset >> 8) & 0xff));
        _programData.push_back((unsigned char)((dst_offset >> 16) & 0xff));
        _programData.push_back((unsigned char)((dst_offset >> 24) & 0xff));
    }
    else
    {
        _programData.push_back((((src % 8) & 0x7) << 3) | (0x2 << 6) | ((dst % 8) & 0x7));
        _programData.push_back(dst_offset & 0xFF);
        _programData.push_back((dst_offset >> 8) & 0xFF);
        _programData.push_back((dst_offset >> 16) & 0xFF);
        _programData.push_back((dst_offset >> 24) & 0xFF);
    }
}

void poAsm::emit_sse_brm(const vm_sse_instruction& ins, int dst, int disp32)
{
    assert(ins.code == CODE_BRMO);
    assert(ins.enc == VMI_ENC_A);
    if (ins.ins1 != VMI_UNUSED) {
        _programData.push_back(ins.ins1);
    }
    unsigned char rex = ins.rex;

    if (dst >= VM_SSE_REGISTER_XMM8) { rex |= 0x4 | (1 << 6); }

    if (rex > 0) { _programData.push_back(rex); }

    _programData.push_back(ins.ins2);
    _programData.push_back(ins.ins3);

    /* Relative RIP addressing (Displacement from end of instruction) */

    _programData.push_back((((dst % 8) & 0x7) << 3) | 0x5 | (0x0 << 6));
    _programData.push_back((unsigned char)(disp32 & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 8) & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 16) & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 24) & 0xff));
}

//================

void poAsm::mc_reserve()
{
    _programData.push_back(0x90); // nop
}
void poAsm::mc_reserve2()
{
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
}
void poAsm::mc_reserve3()
{
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
}
void poAsm::mc_cdq()
{
    _programData.push_back(0x99);
}
void poAsm::mc_return()
{   
    //vm_emit(table.ret, program, count);

    emit(gInstructions[VMI_NEAR_RETURN]);
}
void poAsm::mc_push_32(const int imm)
{
    _programData.push_back(0x68);
    _programData.push_back((unsigned char)(imm & 0xFF));
    _programData.push_back((unsigned char)((imm >> 8) & 0xFF));
    _programData.push_back((unsigned char)((imm >> 16) & 0xFF));
    _programData.push_back((unsigned char)((imm >> 24) & 0xFF));
}
void poAsm::mc_push_reg(char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x48 | 0x1);
    }

    _programData.push_back(0x50 | ((reg % 8) & 0x7));
}
void poAsm::mc_pop_reg(char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x48 | 0x1);
    }

    _programData.push_back(0x58 | ((reg % 8) & 0x7));
}
void poAsm::mc_mov_imm_to_reg_8(char dst, char imm)
{
    auto& ins = gInstructions[VMI_MOV8_SRC_IMM_DST_REG];

    _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0));
    _programData.push_back(ins.ins | (dst % 8));
    _programData.push_back((unsigned char)(imm & 0xff));
}
void poAsm::mc_mov_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV8_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_mov_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_mov_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_MOV8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_add_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD8_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_add_imm_to_reg_8(char dst, char imm)
{
    // TODO: the immediate in emit_bri takes an int not a byte
    emit_bri(gInstructions[VMI_ADD8_SRC_IMM_DST_REG], dst, imm);
}
void poAsm::mc_add_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_add_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_sub_imm_to_reg_8(char dst, char imm)
{
    // TODO: the immediate in emit_bri takes an int not a byte
    emit_bri(gInstructions[VMI_SUB8_SRC_IMM_DST_REG], dst, imm);
}
void poAsm::mc_sub_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB8_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_sub_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_sub_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_SUB8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_mul_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_IMUL8_SRC_REG], reg);
}
void poAsm::mc_umul_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_MUL8_SRC_REG], reg);
}
void poAsm::mc_div_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_IDIV8_SRC_REG], reg);
}
void poAsm::mc_div_mem_8(char reg)
{
    emit_um(gInstructions[VMI_IDIV8_SRC_MEM], reg);
}
void poAsm::mc_udiv_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_DIV8_SRC_REG], reg);
}
void poAsm::mc_udiv_mem_8(char reg)
{
    emit_um(gInstructions[VMI_DIV8_SRC_MEM], reg);
}
void poAsm::mc_cmp_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP8_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_cmp_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_cmp_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_CMP8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_mov_imm_to_reg(char dst, int imm)
{
    emit_bri(gInstructions[VMI_MOV32_SRC_IMM_DST_REG], dst, imm);
}
void poAsm::mc_mov_imm_to_reg_x64(char dst, long long imm)
{ 
    //vm_emit_bri(gInstructions[VMI_MOV64_SRC_IMM_DST_REG], program, count, dst, imm);

    _programData.push_back(0x48 | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0));
    _programData.push_back(0xB8 | (dst % 8));
    _programData.push_back((unsigned char)(imm & 0xff));
    _programData.push_back((unsigned char)((imm >> 8) & 0xff));
    _programData.push_back((unsigned char)((imm >> 16) & 0xff));
    _programData.push_back((unsigned char)((imm >> 24) & 0xff));
    _programData.push_back((unsigned char)((imm >> 32) & 0xff));
    _programData.push_back((unsigned char)((imm >> 40) & 0xff));
    _programData.push_back((unsigned char)((imm >> 48) & 0xff));
    _programData.push_back((unsigned char)((imm >> 56) & 0xff));
}
void poAsm::mc_mov_reg_to_memory_x64(char dst, int dst_offset, char src)
{
    emit_bmro(gInstructions[VMI_MOV64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_mov_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_mov_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV64_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_add_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD64_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_add_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_add_reg_to_memory_x64(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_add_imm_to_reg_x64(char dst, int imm)
{
    emit_bri(gInstructions[VMI_ADD64_SRC_IMM_DST_REG], dst, imm);
}
void poAsm::mc_sub_reg_to_memory_x64(char dst, char src, int dst_offset) 
{
    emit_bmro(gInstructions[VMI_SUB64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_sub_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB64_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_sub_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_sub_imm_to_reg_x64(char reg, int imm)
{
    emit_bri(gInstructions[VMI_SUB64_SRC_IMM_DST_REG], reg, imm);
}
void poAsm::mc_mul_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_MUL64_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_mul_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MUL64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_div_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_IDIV64_SRC_REG], reg);
}
void poAsm::mc_div_memory_x64(char src, int src_offset)
{
    emit_umo(gInstructions[VMI_IDIV64_SRC_MEM], src, src_offset);
}
void poAsm::mc_cmp_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP64_SRC_REG_DST_REG], dst, src);
}
void poAsm::mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src)
{
    emit_bmro(gInstructions[VMI_CMP64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void poAsm::mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void poAsm::mc_jump_unconditional_8(int imm)
{
    emit_ui(gInstructions[VMI_J8], imm);
}
void poAsm::mc_jump_equals_8(int imm)
{
    emit_ui(gInstructions[VMI_JE8], imm);
}
void poAsm::mc_jump_not_equals_8(int imm)
{
    emit_ui(gInstructions[VMI_JNE8], imm);
}
void poAsm::mc_jump_less_8(int imm)
{
    emit_ui(gInstructions[VMI_JL8], imm);
}
void poAsm::mc_jump_less_equal_8(int imm)
{
    emit_ui(gInstructions[VMI_JLE8], imm);
}
void poAsm::mc_jump_greater_8(int imm)
{
    emit_ui(gInstructions[VMI_JG8], imm);
}
void poAsm::mc_jump_greater_equal_8(int imm) 
{
    emit_ui(gInstructions[VMI_JGE8], imm);
}
void poAsm::mc_jump_unconditional(int imm)
{
    emit_ui(gInstructions[VMI_J32], imm);
}
void poAsm::mc_jump_equals(int imm)
{
    emit_ui(gInstructions[VMI_JE32], imm);
}
void poAsm::mc_jump_not_equals(int imm)
{
    emit_ui(gInstructions[VMI_JNE32], imm);
}
void poAsm::mc_jump_less(int imm)
{
    emit_ui(gInstructions[VMI_JL32], imm);
}
void poAsm::mc_jump_less_equal(int imm)
{
    emit_ui(gInstructions[VMI_JLE32], imm);
}
void poAsm::mc_jump_greater(int imm)
{
    emit_ui(gInstructions[VMI_JG32], imm);
}
void poAsm::mc_jump_greater_equal(int imm)
{
    emit_ui(gInstructions[VMI_JGE32], imm);
}
void poAsm::mc_jump_not_above(int imm)
{
    emit_ui(gInstructions[VMI_JNA32], imm);
}
void poAsm::mc_jump_not_above_equal(int imm)
{
    emit_ui(gInstructions[VMI_JNAE32], imm);
}
void poAsm::mc_jump_not_below(int imm)
{
    emit_ui(gInstructions[VMI_JNB32], imm);
}
void poAsm::mc_jump_not_below_equal(int imm)
{
    emit_ui(gInstructions[VMI_JNBE32], imm);
}
void poAsm::mc_jump_absolute(int reg)
{
    emit_ur(gInstructions[VMI_JA64], reg);
}
void poAsm::mc_call(int offset)
{
    offset -= 5;

    _programData.push_back(0xE8);
    _programData.push_back((unsigned char)(offset & 0xff));
    _programData.push_back((unsigned char)((offset >> 8) & 0xff));
    _programData.push_back((unsigned char)((offset >> 16) & 0xff));
    _programData.push_back((unsigned char)((offset >> 24) & 0xff));
}
void poAsm::mc_call_absolute(int reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x1 | (0x1 << 6));
    }

    _programData.push_back(0xFF);
    _programData.push_back((0x2 << 3) | (reg % 8) | (0x3 << 6));
}
void poAsm::mc_inc_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_INC64_DST_REG], reg);
}
void poAsm::mc_inc_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_INC64_DST_MEM], reg, offset);
}
void poAsm::mc_dec_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_DEC64_DST_REG], reg);
}
void poAsm::mc_dec_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_DEC64_DST_MEM], reg, offset);
}
void poAsm::mc_neg_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_NEG64_DST_MEM], reg, offset);
}
void poAsm::mc_neg_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_NEG64_DST_REG], reg);
}
void poAsm::mc_neg_reg_8(int reg)
{
    emit_ur(gInstructions[VMI_NEG8_DST_REG], reg);
}
void poAsm::mc_movsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_movsd_reg_to_memory_x64(int dst, int src, int dst_offset)
{
    emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_MEM], src, dst, dst_offset);
}
void poAsm::mc_movsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_movsd_memory_to_reg_x64(int dst, int addr)
{
    emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], dst, addr);
}
void poAsm::mc_addsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_addsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_subsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_subsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_mulsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_mulsd_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSD_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void poAsm::mc_divsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_divsd_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSD_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void poAsm::mc_cvtitod_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_cvtitod_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_ucmpd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_ucmpd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_xorpd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPD_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_movss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_movss_reg_to_memory_x64(int dst, int src, int dst_offset)
{
    emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_MEM], src, dst, dst_offset);
}
void poAsm::mc_movss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_movss_memory_to_reg_x64(int dst, int addr)
{
    emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], dst, addr);
}
void poAsm::mc_addss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_addss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_subss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_subss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_mulss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_mulss_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSS_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void poAsm::mc_divss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_divss_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSS_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void poAsm::mc_cvtitos_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_cvtitos_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_ucmps_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_REG_DST_REG], src, dst);
}
void poAsm::mc_ucmps_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void poAsm::mc_xorps_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPS_SRC_REG_DST_REG], src, dst);
}

//================

void poAsm::ir_element_ptr(poModule& module, poRegLinear& linear, const poInstruction& ins)
{
    // We need to get the pointer to the variable (left) optionally adding the variable (right) and a the memory offset.

    const int dst = linear.getRegisterByVariable(ins.name());
    const int slot = linear.getStackSlotByVariable(ins.left()); /* assume the pointer is the stack position */

    const poType& type = module.types()[ins.type()];
    const int size = type.size();

    const int offset = slot * 8;
    if (ins.right() != -1)
    {
        const int operand = linear.getRegisterByVariable(ins.right());

        // TODO: change to LEA? (load effective address)

        mc_mov_imm_to_reg_x64(dst, size);
        mc_mul_reg_to_reg_x64(dst, operand);

        mc_add_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        mc_add_imm_to_reg_x64(dst, offset);
    }
    else
    {
        mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        mc_add_imm_to_reg_x64(dst, offset);
    }
}

void poAsm::ir_ptr(poModule& module, poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const poType& type = module.types()[ins.type()];
    const int slot = linear.getStackSlotByVariable(ins.left()); /* assume the pointer is the stack position */
    if (slot != -1)
    {
        // We need to get the pointer to the variable (left) optionally adding the variable (right) and the memory offset.

        const int offset = slot * 8 + ins.memOffset();
        mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        mc_add_imm_to_reg_x64(dst, offset);
    }
    else
    {

        // We are just adding a memory offset to the existing pointer

        const int src = linear.getRegisterByVariable(ins.left());
        mc_mov_reg_to_reg_x64(dst, src);
        mc_add_imm_to_reg_x64(dst, ins.memOffset());
    }
}

void poAsm::ir_load(poRegLinear& linear, const poInstruction& ins)
{
    // We need to mov data from the ptr address to the destination register

    const int dst = linear.getRegisterByVariable(ins.name());
    const int dst_sse = linear.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX;

    const int src = linear.getRegisterByVariable(ins.left());

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I8:
    case TYPE_U8:
        mc_mov_memory_to_reg_x64(dst, src, 0);
        break;
    case TYPE_F64:
        mc_movsd_memory_to_reg_x64(dst_sse, src, 0);
        break;
    case TYPE_F32:
        mc_movss_memory_to_reg_x64(dst_sse, src, 0);
        break;
    }
}

void poAsm::ir_store(poRegLinear& linear, const poInstruction& ins)
{
    // We need to mov data from the source register to the destination address

    const int src = linear.getRegisterByVariable(ins.right());
    const int src_sse = src - VM_REGISTER_MAX;

    const int dst = linear.getRegisterByVariable(ins.left());

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I8:
    case TYPE_U8:
        mc_mov_reg_to_memory_x64(dst, 0, src);
        break;
    case TYPE_F64:
        mc_movsd_reg_to_memory_x64(dst, src_sse, 0);
        break;
    case TYPE_F32:
        mc_movss_reg_to_memory_x64(dst, src_sse, 0);
        break;
    }

}

void poAsm::ir_add(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src1 = linear.getRegisterByVariable(ins.left());
    const int src2 = linear.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst == src2) { mc_add_reg_to_reg_x64(dst, src1); }
        else if (dst == src1) { mc_add_reg_to_reg_x64(dst, src2); }
        else {
            mc_mov_reg_to_reg_x64(dst, src1);
            mc_add_reg_to_reg_x64(dst, src2);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_addss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_addsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1) { mc_mov_reg_to_reg_8(dst, src1); }
        mc_add_reg_to_reg_8(dst, src2);
        break;
    }
}

void poAsm::ir_sub(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src1 = linear.getRegisterByVariable(ins.left());
    const int src2 = linear.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst != src1) {
            mc_mov_reg_to_reg_x64(dst, src1);
        }
        mc_sub_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_subss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_subsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1)
        {
            mc_mov_reg_to_reg_8(dst, src1);
        }
        mc_sub_reg_to_reg_8(dst, src2);
        break;
    }
}

void poAsm::ir_mul(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src1 = linear.getRegisterByVariable(ins.left());
    const int src2 = linear.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
        if (dst != src1) {
            mc_mov_reg_to_reg_x64(dst, src1);
        }
        mc_mul_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_mulss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_mulsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
        mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        mc_mul_reg_8(src2);
        mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        mc_umul_reg_8(src2);
        mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        break;
    }
}

void poAsm::ir_div(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src1 = linear.getRegisterByVariable(ins.left());
    const int src2 = linear.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
        mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        mc_mov_imm_to_reg_x64(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        mc_div_reg_x64(src2);
        mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I8:
        mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        mc_div_reg_8(src2);
        mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        mc_udiv_reg_8(src2);
        mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_divss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        mc_divsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    }
}

void poAsm::ir_cmp(poRegLinear& linear, const poInstruction& ins)
{
    const int src1 = linear.getRegisterByVariable(ins.left());
    const int src2 = linear.getRegisterByVariable(ins.right());

    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        mc_cmp_reg_to_reg_x64(src1, src2);
        break;
    case TYPE_U8:
    case TYPE_I8:
        mc_cmp_reg_to_reg_8(src1, src2);
        break;
    case TYPE_F64:
        mc_ucmpd_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    case TYPE_F32:
        mc_ucmps_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    }
}

void poAsm::ir_br(poRegLinear& linear, const poInstruction& ins, poBasicBlock* bb)
{
    poBasicBlock* targetBB = bb->getBranch();

    // Check if the targetBB has been visited already, is which case 

    poAsmBasicBlock& targetAsmBB = _basicBlocks[_basicBlockMap[targetBB]];
    if (targetAsmBB.getPos() != 0)
    {
        const int imm = targetAsmBB.getPos() - (int(_programData.size()) + 5/*TODO*/);

        ir_jump(ins.left(), imm, ins.type());
    }
    else
    {
        // Insert patch

        const int pos = int(_programData.size());

        /* Insert after we have got the position */
        if (ins.left() == IR_JUMP_UNCONDITIONAL)
        {
            mc_jump_unconditional(0);
        }
        else
        {
            mc_jump_equals(0);
        }

        const int size = int(_programData.size()) - pos;

        poAsmBasicBlock* asmBB = &_basicBlocks[_basicBlockMap[bb]];
        asmBB->jumps().push_back(poAsmJump(pos, ins.left(), size, targetBB, ins.type()));
    }
}

void poAsm::ir_copy(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src = linear.getRegisterByVariable(ins.left());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src = src - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src)
        {
            mc_mov_reg_to_reg_x64(dst, src);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src)
        {
            mc_movss_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    case TYPE_F64:
        if (sse_dst != sse_src)
        {
            mc_movsd_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    default:
        break;
    }
}

void poAsm::ir_constant(poConstantPool& constants, poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int dstSSE = dst - VM_REGISTER_MAX;

    if (ins.constant() == -1)
    {
        std::stringstream ss;
        ss << "Internal Error: Malformed constant instruction " << ins.name();
        setError(ss.str());
        return;
    }

    switch (ins.type())
    {
    case TYPE_I64:
        mc_mov_imm_to_reg_x64(dst, constants.getI64(ins.constant()));
        break;
    case TYPE_U64:
        mc_mov_imm_to_reg_x64(dst, constants.getU64(ins.constant()));
        break;
    case TYPE_I32:
        mc_mov_imm_to_reg(dst, constants.getI32(ins.constant()));
        break;
    case TYPE_U32:
        mc_mov_imm_to_reg(dst, int(constants.getU32(ins.constant())));
        break;
    case TYPE_I8:
        mc_mov_imm_to_reg_8(dst, constants.getI8(ins.constant()));
        break;
    case TYPE_U8:
        mc_mov_imm_to_reg_8(dst, char(constants.getU8(ins.constant())));
        break;
    case TYPE_F32:
        mc_movss_memory_to_reg_x64(dstSSE, 0);
        addInitializedData(constants.getF32(ins.constant()), -int(sizeof(int32_t))); // insert patch
        break;
    case TYPE_F64:
        mc_movsd_memory_to_reg_x64(dstSSE, 0);
        addInitializedData(constants.getF64(ins.constant()), -int(sizeof(int32_t))); // insert patch
        break;
    default:
        break;
    }
}

void poAsm::ir_ret(poRegLinear& linear, const poInstruction& ins)
{
    const int left = ins.left();
    if (left != -1)
    {
        switch (ins.type())
        {
        case TYPE_I64:
        case TYPE_I32:
        case TYPE_I8:
            mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left));
            break;
        case TYPE_F64:
            mc_movsd_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            mc_movss_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        default:
            break;
        }
    }

    // Epilogue
    generateEpilogue(linear);

    mc_return();
}

void poAsm::ir_unary_minus(poRegLinear& linear, const poInstruction& ins)
{
    const int src = linear.getRegisterByVariable(ins.left());
    const int dst = linear.getRegisterByVariable(ins.name());

    const int src_sse = src - VM_REGISTER_MAX;
    const int dst_sse = dst - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst != src)
        {
            mc_mov_reg_to_reg_x64(dst, src);
        }
        mc_neg_reg_x64(dst);
        break;
    case TYPE_F64:
        if (dst_sse != src_sse)
        {
            mc_movsd_reg_to_reg_x64(dst_sse, src_sse);
        }
        mc_xorpd_reg_to_reg_x64(dst_sse, dst_sse);
        mc_subsd_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_F32:
        if (dst_sse != src_sse)
        {
            mc_movss_reg_to_reg_x64(dst_sse, src_sse);
        }
        mc_xorps_reg_to_reg_x64(dst_sse, dst_sse);
        mc_subss_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src)
        {
            mc_mov_reg_to_reg_x64(dst, src);
        }
        mc_neg_reg_8(dst);
        break;
    default:
        break;
    }
}

void poAsm::ir_call(poModule& module, poRegLinear& linear, const poInstruction& ins, const std::vector<poInstruction>& args)
{
    static const int generalArgs[] = { VM_ARG1, VM_ARG2, VM_ARG3, VM_ARG4, VM_ARG5, VM_ARG6 };
    static const int sseArgs[] = { VM_SSE_ARG1, VM_SSE_ARG2, VM_SSE_ARG3, VM_SSE_ARG4, VM_SSE_ARG5, VM_SSE_ARG6, VM_SSE_ARG7, VM_SSE_ARG8 };

    const auto& types = module.types();
    const int numArgs = ins.left();
    int slot = 0;
    for (int i = 0; i < numArgs; i++)
    {
        const auto& type = types[args[i].type()];
        const int baseType = type.baseType();
        const int size = type.size();
        int src_slot = 0;

        switch (args[i].type()) {
        case TYPE_I64:
        case TYPE_I32:
        case TYPE_I8:
        case TYPE_U64:
        case TYPE_U32:
        case TYPE_U8:
            mc_mov_reg_to_reg_x64(generalArgs[i], linear.getRegisterByVariable(args[i].left()));
            break;
        case TYPE_F64:
            mc_movsd_reg_to_reg_x64(sseArgs[i], linear.getRegisterByVariable(args[i].left()) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            mc_movss_reg_to_reg_x64(sseArgs[i], linear.getRegisterByVariable(args[i].left()) - VM_REGISTER_MAX);
            break;
        default:
            src_slot = linear.getStackSlotByVariable(args[i].left());
            if (src_slot != -1)
            {
                mc_mov_reg_to_reg_x64(generalArgs[i], VM_REGISTER_ESP);
                mc_add_imm_to_reg_x64(generalArgs[i], src_slot * 8);
            }
            else
            {
                mc_mov_reg_to_reg_x64(generalArgs[i], linear.getRegisterByVariable(args[i].left()));
            }
            break;
        }
    }

    const int symbol = ins.right();
    std::string symbolName;
    module.getSymbol(symbol, symbolName);

    // Add patch this call

    _calls.push_back(poAsmCall(int(_programData.size()), numArgs, symbolName));
    mc_call(0);

    switch (ins.type())
    {
    case TYPE_U8:
    case TYPE_I8:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I64:
    case TYPE_U64:
        mc_mov_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()), VM_REGISTER_EAX);
        break;
    case TYPE_F64:
        mc_movsd_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    case TYPE_F32:
        mc_movss_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    }
}

void poAsm::ir_param(poModule& module, poRegLinear& linear, const poInstruction& ins)
{
    int slot = 0;
    const int dst = linear.getRegisterByVariable(ins.name());
    const int dst_sse = dst - VM_REGISTER_MAX;
    static const int generalArgs[] = { VM_ARG1, VM_ARG2, VM_ARG3, VM_ARG4, VM_ARG5, VM_ARG6 };
    static const int sseArgs[] = { VM_SSE_ARG1, VM_SSE_ARG2, VM_SSE_ARG3, VM_SSE_ARG4, VM_SSE_ARG5, VM_SSE_ARG6, VM_SSE_ARG7, VM_SSE_ARG8 };
    const int param = ins.left();
    const poType& type = module.types()[ins.type()];
    const int size = type.size();
    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I8:
    case TYPE_U8:
        if (param < VM_MAX_ARGS)
        {
            const int src = generalArgs[param];
            mc_mov_reg_to_reg_x64(dst, src);
        }
        break;
    case TYPE_F64:
        mc_movsd_reg_to_reg_x64(dst_sse, sseArgs[param]);
        break;
    case TYPE_F32:
        mc_movss_reg_to_reg_x64(dst_sse, sseArgs[param]);
        break;
    default:
        mc_mov_reg_to_reg_x64(dst, generalArgs[param]);
        break;
    }
}

//================

poAsm::poAsm()
    :
    _entryPoint(-1),
    _isError(false),
    _prologueSize(0)
{
}

bool poAsm::ir_jump(int jump, int imm, int type)
{
    if (jump == IR_JUMP_UNCONDITIONAL)
    {
        mc_jump_unconditional(imm);
        return true;
    }

    switch (type)
    {
    case TYPE_I64:
    case TYPE_I32:
    case TYPE_I8:
        switch (jump)
        {
        case IR_JUMP_EQUALS:
            mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            mc_jump_greater(imm);
            break;
        case IR_JUMP_LESS:
            mc_jump_less(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            mc_jump_greater_equal(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            mc_jump_less_equal(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            mc_jump_not_equals(imm);
            break;
        default:
            return false;
        }
        break;
    case TYPE_F64:
    case TYPE_F32:
    case TYPE_U64:
    case TYPE_U32:
    case TYPE_U8:
        switch (jump)
        {
        case IR_JUMP_EQUALS:
            mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            mc_jump_not_below_equal(imm);
            break;
        case IR_JUMP_LESS:
            mc_jump_not_above_equal(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            mc_jump_not_below(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            mc_jump_not_above(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            mc_jump_not_equals(imm);
            break;
        default:
            return false;
        }
        break;
    }

    return true;
}

void poAsm::patchJump(const poAsmJump& jump)
{
    const int pos = int(_programData.size());
    const int size = jump.getSize();
    const int imm = pos - (jump.getProgramDataPos() + size);

    if (!ir_jump(jump.getJumpType(), imm, jump.getType()))
    {
        return;
    }

    memcpy(_programData.data() + jump.getProgramDataPos(), _programData.data() + pos, size);
    _programData.resize(_programData.size() - size);
}

void po::poAsm::patchForwardJumps(po::poBasicBlock* bb)
{
    auto& incoming = bb->getIncoming();
    for (poBasicBlock* inc : incoming)
    {
        auto& asmBB = _basicBlocks[_basicBlockMap.find(inc)->second];
        auto& jumps = asmBB.jumps();
        for (auto& jump : jumps)
        {
            if (!jump.isApplied() && jump.getBasicBlock() == bb)
            {
                jump.setApplied(true);
                patchJump(jump);
            }
        }
    }
}

void poAsm::scanBasicBlocks(poFlowGraph& cfg)
{
    _basicBlocks.clear();
    _basicBlockMap.clear();

    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        auto& asmBB = _basicBlocks.emplace_back();
        _basicBlockMap.insert(std::pair<poBasicBlock*, int>(bb, int(_basicBlocks.size()) - 1));

        bb = bb->getNext();
    }
}

static const int align(const int size)
{
    const int remainder = size % 16;
    if (remainder == 8)
    {
        return 0;
    }

    return 8;
}

void poAsm::generatePrologue(poRegLinear& linear)
{
    // Push all non-volatile general purpose registers which are used
    int numPushed = 0;
    for (int i = 0; i < VM_REGISTER_MAX; i++)
    {
        if (!linear.isVolatile(i) && linear.isRegisterSet(i))
        {
            mc_push_reg(i);
            numPushed++;
        }
    }

    const int size = 8 * numPushed + 8 * linear.stackSize();
    const int alignment = align(size);
    _prologueSize = alignment + size;
    const int resize = _prologueSize - 8 * numPushed;

    if (resize > 0)
    {
        mc_sub_imm_to_reg_x64(VM_REGISTER_ESP, resize);
    }
}

void poAsm::generateEpilogue(poRegLinear& linear)
{
    // Pop all non-volatile general purpose registers which are used (in backwards order)
    int numToPop = 0;
    for (int i = VM_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (!linear.isVolatile(i) && linear.isRegisterSet(i))
        {
            numToPop++;
        }
    }

    const int size = _prologueSize - 8 * numToPop;
    if (size > 0)
    {
        mc_add_imm_to_reg_x64(VM_REGISTER_ESP, size);
    }

    for (int i = VM_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (!linear.isVolatile(i) && linear.isRegisterSet(i))
        {
            mc_pop_reg(i);
        }
    }
}

void poAsm::dump(const poRegLinear& linear, poFlowGraph& cfg)
{
    std::cout << "###############" << std::endl;
    std::cout << "Stack Size: " << linear.stackSize() << std::endl;

    poBasicBlock* bb = cfg.getFirst();
    int index = 0;
    int pos = 0;
    while (bb)
    {
       std::cout << "Basic Block " << index << ":" << std::endl;
       poAsmBasicBlock& abb = _basicBlocks[_basicBlockMap[bb]];

       for (int i = 0; i < bb->numInstructions(); i++)
       {
           poRegSpill spill;
           if (linear.spillAt(pos, &spill))
           {
               std::cout << "SPILL " << spill.spillRegister() << std::endl;
           }

           const poInstruction& ins = bb->getInstruction(i);
           std::cout << ins.name() << " " << linear.getRegisterByVariable(ins.name()) << std::endl;
           pos++;
       }

        bb = bb->getNext();
        index++;
    }

    std::cout << "###############" << std::endl;
}

void poAsm::spill(poRegLinear& linear, const int pos)
{
    poRegSpill spill;
    if (linear.spillAt(pos, &spill))
    {
        const int spillVariable = spill.spillVariable();
        const int slot = spill.spillStackSlot();
        mc_mov_reg_to_memory_x64(VM_REGISTER_ESP, -8 * (slot + 1), spill.spillRegister());
    }
}

void poAsm::restore(const poInstruction& ins, poRegLinear& linear, const int pos)
{
    switch (ins.type())
    {
    case IR_CALL: /* CALL, BR, PARAM, CONSTANT have special handling */
    case IR_BR:
    case IR_PARAM:
    case IR_CONSTANT:
    case IR_PHI: /* PHIs are not used at this point */
        return;
    }

    /* if a variable we want to use has been spilled, we need to restore it */

    if (ins.left() != -1)
    {
        //const int spillSlot = _stackAlloc.findSlot(ins.left());
        //mc_mov_memory_to_reg_x64(linear.)
    }

    if (ins.right() != -1)
    {
        //const int spillSlot = _stackAlloc.findSlot(ins.right());

    }
}

void poAsm::generate(poModule& module, poFlowGraph& cfg)
{
    poRegLinear linear(module);
    linear.setNumRegisters(VM_REGISTER_MAX + VM_SSE_REGISTER_MAX);
    
    linear.setVolatile(VM_REGISTER_ESP, true);
    linear.setVolatile(VM_REGISTER_EBP, true);
    linear.setVolatile(VM_REGISTER_EAX, true);
    linear.setVolatile(VM_REGISTER_R10, true);
    linear.setVolatile(VM_REGISTER_R11, true);

    linear.setVolatile(VM_ARG1, true);
    linear.setVolatile(VM_ARG2, true);
    linear.setVolatile(VM_ARG3, true);
    linear.setVolatile(VM_ARG4, true);

#if VM_MAX_ARGS == 6
    linear.setVolatile(VM_ARG5, true);
    linear.setVolatile(VM_ARG6, true);
#endif

    linear.setVolatile(VM_SSE_ARG1 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG2 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG3 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG4 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG5 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG6 + VM_REGISTER_MAX, true);

#if VM_MAX_SSE_ARGS == 8
    linear.setVolatile(VM_SSE_ARG7 + VM_REGISTER_MAX, true);
    linear.setVolatile(VM_SSE_ARG8 + VM_REGISTER_MAX, true);
#endif

    for (int i = 0; i < VM_REGISTER_MAX + VM_SSE_REGISTER_MAX; i++)
    {
        linear.setType(i, i < VM_REGISTER_MAX ? poRegType::General : poRegType::SSE);
    }

    linear.allocateRegisters(cfg);
    
    scanBasicBlocks(cfg);
    
    dump(linear, cfg);

    // Prologue
    generatePrologue(linear);

    // Generate the machine code
    //
    poBasicBlock* bb = cfg.getFirst();
    int pos = 0;
    while (bb)
    {
        _basicBlocks[_basicBlockMap[bb]].setPos(int(_programData.size()));
        patchForwardJumps(bb);

        auto& instructions = bb->instructions();
        for (int i = 0; i < int(instructions.size()); i++)
        {
            auto& ins = instructions[i];

            spill(linear, pos);
            restore(ins, linear, pos);

            switch (ins.code())
            {
            case IR_ELEMENT_PTR:
                ir_element_ptr(module, linear, ins);
                break;
            case IR_PTR:
                ir_ptr(module, linear, ins);
                break;
            case IR_LOAD:
                ir_load(linear, ins);
                break;
            case IR_STORE:
                ir_store(linear, ins);
                break;
            case IR_ADD:
                ir_add(linear, ins);
                break;
            case IR_ARG:
                // do nothing
                break;
            case IR_BR:
                ir_br(linear, ins, bb);
                break;
            case IR_CALL:
            {
                std::vector<poInstruction> args;
                for (int j = 0; j < ins.left(); j++)
                {
                    args.push_back(instructions[i + j + 1]);
                }

                ir_call(module, linear, ins, args);
                i += ins.left();
            }
                break;
            case IR_CMP:
                ir_cmp(linear, ins);
                break;
            case IR_CONSTANT:
                ir_constant(module.constants(), linear, ins);
                break;
            case IR_COPY:
                ir_copy(linear, ins);
                break;
            case IR_DIV:
                ir_div(linear, ins);
                break;
            case IR_MUL:
                ir_mul(linear, ins);
                break;
            case IR_PARAM:
                ir_param(module, linear, ins);
                break;
            case IR_PHI:
                // do nothing
                break;
            case IR_RETURN:
                ir_ret(linear, ins);
                break;
            case IR_SUB:
                ir_sub(linear, ins);
                break;
            case IR_UNARY_MINUS:
                ir_unary_minus(linear, ins);
                break;
            }

            pos++;
        }

        bb = bb->getNext();
    }
}

void poAsm::generate(poModule& module)
{
    std::vector<poNamespace>& namespaces = module.namespaces();
    for (poNamespace& ns : namespaces)
    {
        std::vector<poFunction>& functions = ns.functions();
        for (poFunction& function : functions)
        {
            _mapping.insert(std::pair<std::string, int>(function.name(), int(_programData.size())));

            generate(module, function.cfg());
        }
    }

    patchCalls();

    const auto& main = _mapping.find("main");
    if (main != _mapping.end())
    {
        _entryPoint = int(_programData.size());
        mc_sub_imm_to_reg_x64(VM_REGISTER_EBP, 8);
        const int mainImm = main->second - int(_programData.size());
        mc_call(mainImm);
        mc_add_imm_to_reg_x64(VM_REGISTER_EBP, 8);
        mc_return();
    }
    else
    {
        setError("No entry point defined.");
    }
}

void poAsm::setError(const std::string& errorText)
{
    if (!_isError)
    {
        _isError = true;
        _errorText = errorText;
    }
}

void poAsm::addInitializedData(const float f32, const int programDataOffset)
{
    _constants.push_back(poAsmConstant(int(_initializedData.size()), int(_programData.size()) + programDataOffset));

    const int32_t data = *reinterpret_cast<const int32_t*>(&f32);
    _initializedData.push_back(data & 0xFF);
    _initializedData.push_back((data >> 8) & 0xFF);
    _initializedData.push_back((data >> 16) & 0xFF);
    _initializedData.push_back((data >> 24) & 0xFF);
}

void poAsm::addInitializedData(const double f64, const int programDataOffset)
{
    _constants.push_back(poAsmConstant(int(_initializedData.size()), int(_programData.size() + programDataOffset)));

    const int64_t data = *reinterpret_cast<const int64_t*>(&f64);
    _initializedData.push_back(data & 0xFF);
    _initializedData.push_back((data >> 8) & 0xFF);
    _initializedData.push_back((data >> 16) & 0xFF);
    _initializedData.push_back((data >> 24) & 0xFF);
    _initializedData.push_back((data >> 32) & 0xFF);
    _initializedData.push_back((data >> 40) & 0xFF);
    _initializedData.push_back((data >> 48) & 0xFF);
    _initializedData.push_back((data >> 56) & 0xFF);
}

void poAsm::patchCalls()
{
    for (poAsmCall& call : _calls)
    {
        const int pos = call.getPos();
        const std::string& symbol = call.getSymbol();
        const auto& it = _mapping.find(symbol);

        if (it != _mapping.end())
        {
            const int size = 5;
            const int dataPos = int(_programData.size());
            mc_call(it->second - pos);
            memcpy(_programData.data() + pos, _programData.data() + dataPos, size);
            _programData.resize(_programData.size() - size);
        }
        else
        {
            std::stringstream ss;
            ss << "Unsolved symbol " << symbol;
            setError(ss.str());
        }
    }
}

void poAsm::link(const int programDataPos, const int initializedDataPos)
{
    for (auto& constant : _constants)
    {
        const int disp32 = (constant.getInitializedDataPos() + initializedDataPos) - (programDataPos + constant.getProgramDataPos() + int(sizeof(int32_t)));
        _programData[constant.getProgramDataPos()] = disp32 & 0xFF;
        _programData[constant.getProgramDataPos() + 1] = (disp32 >> 8) & 0xFF;
        _programData[constant.getProgramDataPos() + 2] = (disp32 >> 16) & 0xFF;
        _programData[constant.getProgramDataPos() + 3] = (disp32 >> 24) & 0xFF;
    }
}

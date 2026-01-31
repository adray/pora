#include "po_x86_64.h"
#include <assert.h>
#include <iostream>

using namespace po;

//================

static constexpr const char* registers[] = {
    "RAX",
    "RCX",
    "RDX",
    "RBX",
    "RSP",
    "RBP",
    "RSI",
    "RDI",
    "R8",
    "R9",
    "R10",
    "R11",
    "R12",
    "R13",
    "R14",
    "R15",
};

static constexpr const char* sse_registers[] = {
    "XMM0",
    "XMM1",
    "XMM2",
    "XMM3",
    "XMM4",
    "XMM5",
    "XMM6",
    "XMM7",
    "XMM8",
    "XMM9",
    "XMM10",
    "XMM11",
    "XMM12",
    "XMM13",
    "XMM14",
    "XMM15",
};

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
    VMI_ENC_C = 8,      // destination R/M, source R
    VMI_ENC_Z0 = 9,     // no operands
    VMI_ENC_MC = 10,    // destination R/M, source CL
};

struct vm_instruction
{
    unsigned char legacy;
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

#define INS(legacy, rex, ins, subins, type, code, enc) \
    { (unsigned char)legacy, (unsigned char)rex, (unsigned char)ins, (unsigned char)subins, (unsigned char)type, (unsigned char)code, (unsigned char)enc }

#define SSE_INS(rex, ins1, ins2, ins3, type, code, enc) \
    { (unsigned char)rex, (unsigned char)ins1, (unsigned char)ins2, (unsigned char)ins3, (unsigned char)type, (unsigned char)code, (unsigned char)enc }

#define VMI_UNUSED 0xFF

static constexpr vm_instruction gInstructions[VMI_MAX_INSTRUCTIONS] = {
    INS(0x0, 0x48, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),     // VMI_ADD64_SRC_REG_DST_REG
    INS(0x0, 0x48, 0x81, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_ADD64_SRC_IMM_DST_REG
    INS(0x0, 0x48, 0x3, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),    // VMI_ADD64_SRC_MEM_DST_REG
    INS(0x0, 0x48, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),    // VMI_ADD64_SRC_REG_DST_MEM

    INS(0x0, 0x0, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_ADD32_SRC_REG_DST_REG
    INS(0x0, 0x0, 0x81, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_ADD32_SRC_IMM_DST_REG
    INS(0x0, 0x0, 0x3, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_ADD32_SRC_MEM_DST_REG
    INS(0x0, 0x0, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_RM), // VMI_ADD32_SRC_REG_DST_MEM

    INS(0x66, 0x0, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_ADD16_SRC_REG_DST_REG
    INS(0x66, 0x0, 0x81, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_ADD16_SRC_IMM_DST_REG
    INS(0x66, 0x0, 0x3, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_ADD16_SRC_MEM_DST_REG
    INS(0x66, 0x0, 0x1, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_RM), // VMI_ADD16_SRC_REG_DST_MEM

    INS(0x0, 0x48, 0xB0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_ADD8_SRC_IMM_DST_REG,
    INS(0x0, 0x48, 0x2, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_ADD8_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0x2, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_ADD8_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0x0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), // VMI_ADD8_SRC_REG_DST_MEM,

    INS(0x0, 0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_SUB64_SRC_REG_DST_REG
    INS(0x0, 0x48, 0x81, 5, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_SUB64_SRC_IMM_DST_REG
    INS(0x0, 0x48, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_SUB64_SRC_MEM_DST_REG
    INS(0x0, 0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_SUB64_SRC_REG_DST_MEM

    INS(0x0, 0x0, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),   //VMI_SUB32_SRC_REG_DST_REG,
    INS(0x0, 0x0, 0x81, 0x5, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),          //VMI_SUB32_SRC_IMM_DST_REG,
    INS(0x0, 0x0, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   //VMI_SUB32_SRC_MEM_DST_REG,
    INS(0x0, 0x0, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   //VMI_SUB32_SRC_REG_DST_MEM,

    INS(0x66, 0x0, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),   // VMI_SUB16_SRC_REG_DST_REG,
    INS(0x66, 0x0, 0x81, 0x5, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),          // VMI_SUB16_SRC_IMM_DST_REG,
    INS(0x66, 0x0, 0x2B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),  // VMI_SUB16_SRC_MEM_DST_REG,
    INS(0x66, 0x0, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),  // VMI_SUB16_SRC_REG_DST_MEM,

    INS(0x0, 0x48, 0x28, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_SUB8_SRC_REG_DST_REG
    INS(0x0, 0x48, 0x29, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI), // VMI_SUB8_SRC_IMM_DST_REG
    INS(0x0, 0x48, 0x28, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR),  // VMI_SUB8_SRC_REG_DST_MEM
    INS(0x0, 0x48, 0x2A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_SUB8_SRC_MEM_DST_REG

    INS(0x0, 0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),    // VMI_MOV64_SRC_REG_DST_REG
    INS(0x0, 0x48, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),   // VMI_MOV64_SRC_REG_DST_MEM
    INS(0x0, 0x48, 0x8B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_MOV64_SRC_MEM_DST_REG
    INS(0x0, 0x48, 0xC7, 0x0, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),    // VMI_MOV64_SRC_IMM_DST_REG

    INS(0x0, 0x0, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),      //VMI_MOV_SRC_REG_DST_REG,
    INS(0x0, 0x0, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),     //VMI_MOV32_SRC_REG_DST_MEM,
    INS(0x0, 0x0, 0x8B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),     //VMI_MOV32_SRC_MEM_DST_REG,
    INS(0x0, 0x0, 0xB8, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI),       // VMI_MOV32_SRC_IMM_DST_REG

    INS(0x66, 0x0, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR),// VMI_MOV16_SRC_REG_DST_REG,
    INS(0x66, 0x0, 0x89, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR),// VMI_MOV16_SRC_REG_DST_MEM,
    INS(0x66, 0x0, 0x8B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),// VMI_MOV16_SRC_MEM_DST_REG,
    INS(0x66, 0x0, 0xB8, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI),// VMI_MOV16_SRC_IMM_DST_REG,

    INS(0x0, 0x48, 0xB0, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_OI), //VMI_MOV8_SRC_IMM_DST_REG
    INS(0x0, 0x48, 0x88, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), //VMI_MOV8_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0x8A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), //VMI_MOV8_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0x88, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), //VMI_MOV8_SRC_REG_DST_MEM,

    INS(0x0, 0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_IMUL64_SRC_REG_DST_REG /* signed */
    INS(0x0, 0x48, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_IMUL64_SRC_MEM_DST_REG /* signed */

    INS(0x0, 0x0, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_IMUL32_SRC_REG_DST_REG, /* signed */
    INS(0x0, 0x0, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_IMUL32_SRC_MEM_DST_REG, /* signed */

    INS(0x66, 0x0, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_IMUL16_SRC_REG_DST_REG, /* signed */
    INS(0x66, 0x0, 0x0F, 0xAF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_IMUL16_SRC_MEM_DST_REG, /* signed */

    INS(0x0, 0x48, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_MUL64_SRC_REG_DST_REG, /*unsigned*/
    INS(0x0, 0x48, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UM, VMI_ENC_M), // VMI_MUL64_SRC_MEM_DST_REG, /*unsigned*/

    INS(0x0, 0x0, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_MUL32_SRC_REG_DST_REG, /*unsigned*/
    INS(0x0, 0x0, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UM, VMI_ENC_M), // VMI_MUL32_SRC_MEM_DST_REG, /*unsigned*/

    INS(0x66, 0x0, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_MUL16_SRC_REG_DST_REG, /*unsigned*/
    INS(0x66, 0x0, 0xF7, 0x4, VM_INSTRUCTION_UNARY, CODE_UM, VMI_ENC_M), // VMI_MUL16_SRC_MEM_DST_REG, /*unsigned*/

    INS(0x0, 0x40, 0xF6, 0x5, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_IMUL8_SRC_REG /*signed*/
    INS(0x0, 0x40, 0xF6, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_MUL8_SRC_REG /*unsigned*/


    INS(0x0, 0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_INC64_DST_MEM
    INS(0x0, 0x48, 0xFF, 0x0, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_INC64_DST_REG

    INS(0x0, 0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    // VMI_DEC64_DST_MEM
    INS(0x0, 0x48, 0xFF, 0x1, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    // VMI_DEC64_DST_REG

    INS(0x0, 0x0, 0xC3, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_NEAR_RETURN
    INS(0x0, 0x0, 0xCB, VMI_UNUSED, VM_INSTRUCTION_NONE, CODE_NONE, 0x0),           // VMI_FAR_RETURN

    INS(0x0, 0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), // VMI_CMP64_SRC_REG_DST_REG
    INS(0x0, 0x48, 0x39, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), // VMI_CMP64_SRC_REG_DST_MEM
    INS(0x0, 0x48, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), // VMI_CMP64_SRC_MEM_DST_REG

    INS(0x0, 0x0, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), //VMI_CMP32_SRC_REG_DST_REG,
    INS(0x0, 0x0, 0x39, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), //VMI_CMP32_SRC_REG_DST_MEM,
    INS(0x0, 0x0, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), //VMI_CMP32_SRC_MEM_DST_REG,

    INS(0x66, 0x0, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM), //VMI_CMP16_SRC_REG_DST_REG,
    INS(0x66, 0x0, 0x39, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMRO, VMI_ENC_MR), //VMI_CMP16_SRC_REG_DST_MEM,
    INS(0x66, 0x0, 0x3B, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM), //VMI_CMP16_SRC_MEM_DST_REG,

    INS(0x0, 0x40, 0x38, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_MR), // VMI_CMP8_SRC_REG_DST_REG
    INS(0x0, 0x40, 0x3A, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRM, VMI_ENC_RM), // VMI_CMP8_SRC_MEM_DST_REG
    INS(0x0, 0x40, 0x38, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), // VMI_CMP8_SRC_REG_DST_MEM

    INS(0x0, 0x0, 0xEB, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_J8
    INS(0x0, 0x0, 0x74, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JE8
    INS(0x0, 0x0, 0x75, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNE8
    INS(0x0, 0x0, 0x72, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JL8,
    INS(0x0, 0x0, 0x77, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JG8,
    INS(0x0, 0x0, 0x76, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JLE8,
    INS(0x0, 0x0, 0x73, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JGE8,
    INS(0x0, 0x0, 0xFF, 0x4, VM_INSTRUCTION_CODE_OFFSET, CODE_UR, VMI_ENC_M), // VMI_JA64

    INS(0x0, 0x0, 0xE9, VMI_UNUSED, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_J32
    INS(0x0, 0x0, 0x0F, 0x84, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JE32
    INS(0x0, 0x0, 0x0F, 0x85, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNE32, /*unsigned*/
    INS(0x0, 0x0, 0x0F, 0x82, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNAE32, /*unsigned*/
    INS(0x0, 0x0, 0x0F, 0x87, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNBE32, /*unsigned*/
    INS(0x0, 0x0, 0x0F, 0x86, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNB32, /*unsigned*/
    INS(0x0, 0x0, 0x0F, 0x83, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), // VMI_JNA32, /*unsigned*/
    INS(0x0, 0x0, 0x0F, 0x8F, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D),  //VMI_JG32, /*signed*/
    INS(0x0, 0x0, 0x0F, 0x8D, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JGE32, /*signed*/
    INS(0x0, 0x0, 0x0F, 0x8C, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JL32, /*signed*/
    INS(0x0, 0x0, 0x0F, 0x8E, VM_INSTRUCTION_CODE_OFFSET, CODE_UI, VMI_ENC_D), //VMI_JLE32, /*signed*/

    INS(0x0, 0x40, 0xF6, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M), // VMI_NEG8_DST_MEM,
    INS(0x0, 0x40, 0xF6, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M), // VMI_NEG8_DST_REG,

    INS(0x66, 0x0, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    //VMI_NEG16_DST_MEM,
    INS(0x66, 0x0, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    //VMI_NEG16_DST_REG,

    INS(0x0, 0x0, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),   //VMI_NEG32_DST_MEM,
    INS(0x0, 0x0, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),   //VMI_NEG32_DST_REG,

    INS(0x0, 0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_NEG64_DST_MEM
    INS(0x0, 0x48, 0xF7, 0x3, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_NEG64_DST_REG

    INS(0x0, 0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV64_SRC_REG /* signed */
    INS(0x0, 0x48, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV64_SRC_MEM /* signed */

    INS(0x0, 0x0, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    //VMI_IDIV32_SRC_REG, /* signed */
    INS(0x0, 0x0, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),   //VMI_IDIV32_SRC_MEM, /* signed */

    INS(0x66, 0x0, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),    //VMI_IDIV16_SRC_REG, /* signed */
    INS(0x66, 0x0, 0xF7, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),   //VMI_IDIV16_SRC_MEM, /* signed */

    INS(0x0, 0x40, 0xF6, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_IDIV8_SRC_REG /* signed */
    INS(0x0, 0x40, 0xF6, 0x7, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_IDIV8_SRC_MEM /* signed */

    INS(0x0, 0x48, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     //VMI_DIV64_SRC_REG, /* unsigned */
    INS(0x0, 0x48, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    //VMI_DIV64_SRC_MEM, /* unsigned */

    INS(0x0, 0x0, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     //VMI_DIV32_SRC_REG, /* unsigned */
    INS(0x0, 0x0, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    //VMI_DIV32_SRC_MEM, /* unsigned */

    INS(0x66, 0x0, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     //VMI_DIV16_SRC_REG, /* unsigned */
    INS(0x66, 0x0, 0xF7, 0x6, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),    //VMI_DIV16_SRC_MEM, /* unsigned */

    INS(0x0, 0x40, 0xF6, 0x6, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_M),     // VMI_DIV8_SRC_REG /* unsigned */
    INS(0x0, 0x40, 0xF6, 0x6, VM_INSTRUCTION_UNARY, CODE_UMO, VMI_ENC_M),     // VMI_DIV8_SRC_MEM /* unsigned */

    INS(0x66, 0x40, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),  // VMI_MOVSX_8_TO_64_SRC_REG_DST_REG
    INS(0x66, 0x40, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),  // VMI_MOVSX_8_TO_16_SRC_MEM_DST_REG,
    INS(0x0, 0x40, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),  // VMI_MOVSX_8_TO_32_SRC_REG_DST_REG,
    INS(0x0, 0x40, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),  // VMI_MOVSX_8_TO_32_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),  // VMI_MOVSX_8_TO_64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0xF, 0xBE, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),  // VMI_MOVSX_8_TO_64_SRC_MEM_DST_REG,

    INS(0x0, 0x0, 0xF, 0xBF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),    // VMI_MOVSX_16_TO_32_SRC_REG_DST_REG,
    INS(0x0, 0x0, 0xF, 0xBF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),    // VMI_MOVSX_16_TO_32_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0xF, 0xBF, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),   // VMI_MOVSX_16_TO_64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0xF, 0xBF, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_MOVSX_16_TO_64_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0x63, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),   // VMI_MOVSX_32_TO_64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0x63, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),   // VMI_MOVSX_32_TO_64_SRC_MEM_DST_REG,

    INS(0x66, 0x40, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),//VMI_MOVZX_8_TO_16_SRC_REG_DST_REG,
    INS(0x66, 0x40, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),//VMI_MOVZX_8_TO_16_SRC_MEM_DST_REG,
    INS(0x0, 0x40, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),//VMI_MOVZX_8_TO_32_SRC_REG_DST_REG,
    INS(0x0, 0x40, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),//VMI_MOVZX_8_TO_32_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),//VMI_MOVZX_8_TO_64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0xF, 0xB6, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),//VMI_MOVZX_8_TO_64_SRC_MEM_DST_REG,

    INS(0x0, 0x0, 0xF, 0xB7, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),//VMI_MOVZX_16_TO_32_SRC_REG_DST_REG,
    INS(0x0, 0x0, 0xF, 0xB7, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),//VMI_MOVZX_16_TO_32_SRC_MEM_DST_REG,
    INS(0x0, 0x48, 0xF, 0xB7, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),//VMI_MOVZX_16_TO_64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0xF, 0xB7, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_RM),//VMI_MOVZX_16_TO_64_SRC_MEM_DST_REG,

    INS(0x0, 0x48, 0x8D, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_RM),    // VMI_LEA64_SRC_REG_DST_REG,
    INS(0x0, 0x48, 0x8D, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_RM),    // VMI_LEA64_SRC_MEM_DST_REG,

    INS(0x0, 0x48, 0xC1, 0x4, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAL64_SRC_IMM_DST_REG, // left shift
    INS(0x0, 0x0, 0xC1, 0x4, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAL32_SRC_IMM_DST_REG,
    INS(0x66, 0x0, 0xC1, 0x4, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAL16_SRC_IMM_DST_REG,
    INS(0x0, 0x40, 0xC0, 0x4, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAL8_SRC_IMM_DST_REG,

    INS(0x0, 0x48, 0xC1, 0x7, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAR64_SRC_IMM_DST_REG, // right shift
    INS(0x0, 0x0, 0xC1, 0x7, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAR32_SRC_IMM_DST_REG,
    INS(0x66, 0x0, 0xC1, 0x7, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAR16_SRC_IMM_DST_REG,
    INS(0x0, 0x40, 0xC0, 0x7, VM_INSTRUCTION_BINARY, CODE_BRI, VMI_ENC_MI),// VMI_SAR8_SRC_IMM_DST_REG,

    INS(0x0, 0x48, 0xD3, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAL64_SRC_REG_DST_REG, // left shift
    INS(0x0, 0x0, 0xD3, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAL32_SRC_REG_DST_REG,
    INS(0x66, 0x0, 0xD3, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAL16_SRC_REG_DST_REG,
    INS(0x0, 0x40, 0xD2, 0x4, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAL8_SRC_REG_DST_REG,

    INS(0x0, 0x48, 0xD3, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAR64_SRC_REG_DST_REG, // right shift
    INS(0x0, 0x0, 0xD3, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAR32_SRC_REG_DST_REG,
    INS(0x66, 0x0, 0xD3, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAR16_SRC_REG_DST_REG,
    INS(0x0, 0x40, 0xD2, 0x7, VM_INSTRUCTION_UNARY, CODE_UR, VMI_ENC_MC),// VMI_SAR8_SRC_REG_DST_REG,

    INS(0x0, 0x48, 0x98, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_Z0),// VMI_CDQE

    INS(0x0, 0x0, 0x50, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_C), // VMI_PUSH_REG, (not implemented)
    INS(0x0, 0x0, 0x48, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_C), // VMI_POP_REG, (not implemented)
    INS(0x0, 0x0, 0x0, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_C), // VMI_JUMP_MEM, (not implemented)
    INS(0x0, 0x0, 0x0, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_C), // JMI_CALL_MEM, (not implemented)
    INS(0x0, 0x0, 0x0, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_C), // VMI_CALL (not implemented)
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

    SSE_INS(0x48, 0xF2, 0xF, 0x2D, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A),   // VMI_SSE_CVTSD2SI_SRC_REG_DST_REG

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

    SSE_INS(0x48, 0xF3, 0xF, 0x2C, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A),   // VMI_SSE_CVTSS2SI_SRC_REG_DST_REG,

    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_UCOMISS_SRC_REG_DST_REG
    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x2E, VM_INSTRUCTION_BINARY, CODE_BRMO, VMI_ENC_A), // VMI_SSE_UCOMISS_SRC_MEM_DST_REG

    SSE_INS(0x0, VMI_UNUSED, 0xF, 0x57, VM_INSTRUCTION_BINARY, CODE_BRR, VMI_ENC_A), // VMI_SSE_XORPS_SRC_REG_DST_REG

};

//=====================
// R/M 0-2
// Op/Reg 3-5
// Mod 6-8
//=====================

void po_x86_64::emit(const vm_instruction& ins)
{
    assert(ins.code == CODE_NONE);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);
}

void po_x86_64::emit_ur(const vm_instruction& ins, char reg)
{
    assert(ins.code == CODE_UR);
    if (ins.legacy > 0)
    {
        _programData.push_back(ins.legacy);
    }
    int rex = ins.rex;
    if (reg >= VM_REGISTER_R8)
    {
        rex |= 0x40 | 0x1;
    }
    if (rex > 0)
    {
        _programData.push_back(rex);
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

void po_x86_64::emit_um(const vm_instruction& ins, char reg)
{
    assert(ins.code == CODE_UM);
    if (ins.legacy > 0)
    {
        _programData.push_back(ins.legacy);
    }
    int rex = ins.rex;
    if (reg >= VM_REGISTER_R8)
    {
        rex |= 0x40 | 0x1;
    }
    if (rex > 0)
    {
        _programData.push_back(rex);
    }
    _programData.push_back(ins.ins);

    if ((reg % 8) == VM_REGISTER_ESP)
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((ins.subins & 0x7) << 3) | (0x0 << 6) | (reg & 0x7));
    }
}

void po_x86_64::emit_umo(const vm_instruction& ins, char reg, int offset)
{
    assert(ins.code == CODE_UMO);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if ((reg % 8) == VM_REGISTER_ESP)
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

void po_x86_64::emit_bri(const vm_instruction& ins, char reg, int imm)
{
    assert(ins.code == CODE_BRI);
    if (ins.legacy > 0)
    {
        _programData.push_back(ins.legacy);
    }
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0));
        _programData.push_back(ins.ins);
        _programData.push_back((ins.subins << 3) | (reg % 8) | (0x3 << 6));
    }
    else
    {
        if (reg >= VM_REGISTER_R8)
        {
            _programData.push_back(0x40 | 0x1);
        }

        if (ins.subins != VMI_UNUSED)
        {
            _programData.push_back(ins.ins);
            _programData.push_back((ins.subins << 3) | (reg % 8) | (0x3 << 6));
        }
        else
        {
            _programData.push_back(ins.ins | (reg % 8));
        }
    }

    _programData.push_back((unsigned char)(imm & 0xff));
    _programData.push_back((unsigned char)((imm >> 8) & 0xff));
    _programData.push_back((unsigned char)((imm >> 16) & 0xff));
    _programData.push_back((unsigned char)((imm >> 24) & 0xff));
}

void po_x86_64::emit_bri(const vm_instruction& ins, char reg, char imm)
{
    assert(ins.code == CODE_BRI);
    if (ins.legacy > 0)
    {
        _programData.push_back(ins.legacy);
    }
    if (ins.rex > 0)
    {
        _programData.push_back(ins.rex | (reg >= VM_REGISTER_R8 ? 0x1 : 0x0));
        _programData.push_back(ins.ins);
        _programData.push_back((ins.subins << 3) | (reg % 8) | (0x3 << 6));
    }
    else
    {
        if (reg >= VM_REGISTER_R8)
        {
            _programData.push_back(0x40 | 0x1);
        }

        if (ins.subins != VMI_UNUSED)
        {
            _programData.push_back(ins.ins );
            _programData.push_back((ins.subins << 3) | (reg % 8) | (0x3 << 6));
        }
        else
        {
            _programData.push_back(ins.ins | (reg % 8));
        }
    }

    _programData.push_back((unsigned char)(imm & 0xff));
}

void po_x86_64::emit_brr(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BRR);

    if (ins.legacy) { _programData.push_back(ins.legacy); }

    if (ins.enc == VMI_ENC_MR)
    {
        unsigned char rex = ins.rex;
        if (dst >= VM_REGISTER_R8) { rex |= 0x1 | (1 << 6); }
        if (src >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }
        if (rex > 0) { _programData.push_back(rex); }
        _programData.push_back(ins.ins);
        if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

        _programData.push_back(0xC0 | (((src % 8) & 0x7) << 0x3) | (((dst % 8) & 0x7) << 0));
    }
    else if (ins.enc == VMI_ENC_RM)
    {
        unsigned char rex = ins.rex;
        if (dst >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }
        if (src >= VM_REGISTER_R8) { rex |= 0x1 | (1 << 6); }
        if (rex > 0) { _programData.push_back(rex); }
        _programData.push_back(ins.ins);
        if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

        _programData.push_back(0xC0 | (((dst % 8) & 0x7) << 0x3) | (((src % 8) & 0x7) << 0));
    }
}

void po_x86_64::emit_brm(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BRM);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if ((src % 8) == VM_REGISTER_ESP)
    {
        _programData.push_back(((dst & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((dst & 0x7) << 3) | (0x0 << 6) | (src & 0x7));
    }
}

void po_x86_64::emit_bmr(const vm_instruction& ins, char dst, char src)
{
    assert(ins.code == CODE_BMR);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);

    if ((dst % 8) == VM_REGISTER_ESP)
    {
        _programData.push_back(((src & 0x7) << 3) | 0x4 | (0x0 << 6));
        _programData.push_back(0x24); // SIB byte
    }
    else
    {
        _programData.push_back(((src & 0x7) << 3) | (0x0 << 6) | (dst & 0x7));
    }
}

void po_x86_64::emit_brmo(const vm_instruction& ins, char dst, char src, int offset)
{
    assert(ins.code == CODE_BRMO);
    if (ins.legacy) { _programData.push_back(ins.legacy); }

    unsigned char rex = ins.rex;
    if (dst >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }
    if (src >= VM_REGISTER_R8) { rex |= 0x1 | (1 << 6); }
    if (rex > 0) { _programData.push_back(rex); }
    _programData.push_back(ins.ins);
    if (ins.subins != VMI_UNUSED) { _programData.push_back(ins.subins); }

    if ((src % 8) == VM_REGISTER_ESP)
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

void po_x86_64::emit_bmro(const vm_instruction& ins, char dst, char src, int offset)
{
    assert(ins.code == CODE_BMRO);
    if (ins.legacy) { _programData.push_back(ins.legacy); }

    unsigned char rex = ins.rex;
    if (dst >= VM_REGISTER_R8) { rex |= 0x1 | (1 << 6); }
    if (src >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }
    if (rex > 0) { _programData.push_back(rex); }
    _programData.push_back(ins.ins);

    if ((dst % 8) == VM_REGISTER_ESP)
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

void po_x86_64::emit_brr_disp(const vm_instruction& ins, int reg, int disp32)
{
    assert(ins.code == CODE_BRR ||
        ins.code == CODE_BRMO ||
        ins.code == CODE_BMRO ||
        ins.code == CODE_BMR ||
        ins.code == CODE_BRM);
    assert(ins.enc == VMI_ENC_RM || ins.enc == VMI_ENC_MR);
    if (ins.legacy) { _programData.push_back(ins.legacy); }

    unsigned char rex = ins.rex;

    if (reg >= VM_REGISTER_R8) { rex |= 0x4 | (1 << 6); }
    if (rex > 0) { _programData.push_back(rex); }

    if (ins.ins != VMI_UNUSED) {
        _programData.push_back(ins.ins);
    }

    /* Relative RIP addressing (Displacement from end of instruction) */

    _programData.push_back((((reg % 8) & 0x7) << 3) | 0x5 | (0x0 << 6));
    _programData.push_back((unsigned char)(disp32 & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 8) & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 16) & 0xff));
    _programData.push_back((unsigned char)((disp32 >> 24) & 0xff));
}

void po_x86_64::emit_ui(const vm_instruction& ins, char imm)
{
    assert(ins.code == CODE_UI);
    if (ins.rex > 0) { _programData.push_back(ins.rex); }
    _programData.push_back(ins.ins);
    _programData.push_back(imm);
}

void po_x86_64::emit_ui(const vm_instruction& ins, int imm)
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


void po_x86_64_Lower::op_imm(const int opcode, const int imm)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, -1, imm));
}

void po_x86_64_Lower::unaryop_imm(const int dst, const int opcode, const char imm)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst, imm));
}
void po_x86_64_Lower::unaryop_imm(const int dst, const int opcode, const short imm)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst, imm));
}
void po_x86_64_Lower::unaryop_imm(const int dst, const int opcode, const int32_t imm)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst, imm));
}
void po_x86_64_Lower::unaryop_imm(const int dst, const int opcode, const int64_t imm)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst, imm));
}

void po_x86_64_Lower::unaryop(const int dst, const int opcode, const int offset)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst, offset));
}

void po_x86_64_Lower::unaryop(const int dst, const int opcode)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, -1, dst));
}

void po_x86_64_Lower::binop(const int src, const int dst, const int opcode)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, src, dst));
}

void po_x86_64_Lower::binop(const int src, const int dst, const int opcode, const int offset)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, opcode, src, dst, offset));
}

void po_x86_64_Lower::sse_binop(const int src, const int dst, const int opcode)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(true, opcode, src, dst));
}

void po_x86_64_Lower::sse_binop(const int src, const int dst, const int opcode, const int offset)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(true, opcode, src, dst, offset));
}

void po_x86_64_Lower::sse_unaryop(const int dst, const int opcode, const int offset)
{
    _cfg.getLast()->instructions().push_back(po_x86_64_instruction(true, opcode, -1, dst, offset));
}

void po_x86_64_Lower::mc_reserve() {}
void po_x86_64_Lower::mc_reserve2() {}
void po_x86_64_Lower::mc_reserve3() {}
void po_x86_64_Lower::mc_cdq() {}
void po_x86_64_Lower::mc_return() { op_imm(VMI_NEAR_RETURN, 0); }
void po_x86_64_Lower::mc_push_32(const int imm) {}
void po_x86_64_Lower::mc_push_reg(char reg) { unaryop(reg, VMI_PUSH_REG); }
void po_x86_64_Lower::mc_pop_reg(char reg) { unaryop(reg, VMI_POP_REG); }

/* 8-bit operations */

void po_x86_64_Lower::mc_mov_imm_to_reg_8(char dst, char imm) { unaryop_imm(dst, VMI_MOV8_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_mov_reg_to_reg_8(char dst, char src) { binop(src, dst, VMI_MOV8_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mov_memory_to_reg_8(char dst, char src, int src_offset) { binop(src, dst, VMI_MOV8_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mov_reg_to_memory_8(char dst, char src, int dst_offset) { binop(src, dst, VMI_MOV8_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mov_memory_to_reg_8(char dst, int addr) { binop(-1, dst, VMI_MOV8_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_mov_reg_to_memory_8(char src, int addr) { binop(src, -1, VMI_MOV8_SRC_REG_DST_MEM, addr); }
void po_x86_64_Lower::mc_add_reg_to_reg_8(char dst, char src) { binop(src, dst, VMI_ADD8_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_add_imm_to_reg_8(char dst, char imm) { unaryop_imm(dst, VMI_ADD8_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_add_memory_to_reg_8(char dst, char src, int src_offset) { binop(src, dst, VMI_ADD8_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_add_reg_to_memory_8(char dst, char src, int dst_offset) { binop(src, dst, VMI_ADD8_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_sub_imm_to_reg_8(char dst, char imm) { unaryop_imm(dst, VMI_SUB8_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sub_reg_to_reg_8(char dst, char src) { binop(src, dst, VMI_SUB8_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sub_memory_to_reg_8(char dst, char src, int src_offset) { binop(src, dst, VMI_SUB8_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_sub_reg_to_memory_8(char dst, char src, int dst_offset) { binop(src, dst, VMI_SUB8_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mul_reg_8(char reg) { unaryop(reg, VMI_IMUL8_SRC_REG); }
void po_x86_64_Lower::mc_umul_reg_8(char reg) { unaryop(reg, VMI_MUL8_SRC_REG); }
void po_x86_64_Lower::mc_div_reg_8(char reg) { unaryop(reg, VMI_IDIV8_SRC_REG); }
void po_x86_64_Lower::mc_div_mem_8(char reg) { unaryop(reg, VMI_IDIV8_SRC_MEM); }
void po_x86_64_Lower::mc_udiv_reg_8(char reg) { unaryop(reg, VMI_DIV8_SRC_REG); }
void po_x86_64_Lower::mc_udiv_mem_8(char reg) { unaryop(reg, VMI_DIV8_SRC_MEM); }
void po_x86_64_Lower::mc_cmp_reg_to_reg_8(char dst, char src) { binop(src, dst, VMI_CMP8_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cmp_memory_to_reg_8(char dst, char src, int src_offset) { binop(src, dst, VMI_CMP8_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cmp_reg_to_memory_8(char dst, char src, int dst_offset) { binop(src, dst, VMI_SUB8_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_neg_reg_8(int reg) { unaryop(reg, VMI_NEG8_DST_REG); }
void po_x86_64_Lower::mc_sar_imm_to_reg_8(char reg, char imm) { unaryop_imm(reg, VMI_SAR8_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sal_imm_to_reg_8(char reg, char imm) { unaryop_imm(reg, VMI_SAL8_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sar_reg_8(char reg) { unaryop(reg, VMI_SAR8_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sal_reg_8(char reg) { unaryop(reg, VMI_SAL8_SRC_REG_DST_REG); }

/* 16-bit operations */

void po_x86_64_Lower::mc_mov_imm_to_reg_16(char dst, short imm) { unaryop_imm(dst, VMI_MOV16_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_add_reg_to_reg_16(char dst, char src) { binop(src, dst, VMI_ADD16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_add_mem_to_reg_16(char dst, char src, int src_offset) { binop(src, dst, VMI_ADD16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_add_reg_to_mem_16(char dst, char src, int dst_offset) { binop(src, dst, VMI_ADD16_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_sub_reg_to_reg_16(char dst, char src) { binop(src, dst, VMI_SUB16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sub_mem_to_reg_16(char dst, char src, int src_offset) { binop(src, dst, VMI_SUB16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_sub_reg_to_mem_16(char dst, char src, int dst_offset) { binop(src, dst, VMI_SUB16_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mul_reg_to_reg_16(char dst, char src) { binop(src, dst, VMI_IMUL16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mul_memory_to_reg_16(char dst, char src, int src_offset) { binop(src, dst, VMI_IMUL16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_umul_reg_to_reg_16(char reg) { unaryop(reg, VMI_MUL16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_umul_memory_to_reg_16(char reg, int offset) { unaryop(reg, VMI_MUL16_SRC_MEM_DST_REG, offset); }
void po_x86_64_Lower::mc_div_reg_16(char reg) { unaryop(reg, VMI_IDIV16_SRC_REG); }
void po_x86_64_Lower::mc_div_memory_16(char src, int src_offset) { unaryop(src, VMI_IDIV16_SRC_MEM, src_offset); }
void po_x86_64_Lower::mc_udiv_reg_16(char reg) { unaryop(reg, VMI_DIV16_SRC_REG); }
void po_x86_64_Lower::mc_udiv_mem_16(char reg) { unaryop(reg, VMI_DIV16_SRC_MEM); }
void po_x86_64_Lower::mc_mov_reg_to_reg_16(char dst, char src) { binop(src, dst, VMI_MOV16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mov_mem_to_reg_16(char dst, char src, int src_offset) { binop(src, dst, VMI_MOV16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mov_reg_to_mem_16(char dst, char src, int dst_offset) { binop(src, dst, VMI_MOV16_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mov_mem_to_reg_16(char dst, int addr) { binop(-1, dst, VMI_MOV16_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_mov_reg_to_mem_16(char src, int addr) { binop(src, -1, VMI_MOV16_SRC_REG_DST_MEM, addr); }
void po_x86_64_Lower::mc_cmp_reg_to_reg_16(char dst, char src) { binop(src, dst, VMI_CMP16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cmp_mem_to_reg_16(char dst, char src, int src_offset) { binop(src, dst, VMI_CMP16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cmp_reg_to_mem_16(char dst, char src, int dst_offset) { binop(src, dst, VMI_MOV16_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_neg_reg_16(int reg) { unaryop(reg, VMI_NEG16_DST_REG); }
void po_x86_64_Lower::mc_sar_imm_to_reg_16(char reg, char imm) { unaryop_imm(reg, VMI_SAR16_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sal_imm_to_reg_16(char reg, char imm) { unaryop_imm(reg, VMI_SAL16_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sar_reg_16(char reg) { unaryop(reg, VMI_SAR16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sal_reg_16(char reg) { unaryop(reg, VMI_SAL16_SRC_REG_DST_REG); }

/* 32-bit operations */

void po_x86_64_Lower::mc_add_reg_to_reg_32(char dst, char src) { binop(src, dst, VMI_ADD32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_add_mem_to_reg_32(char dst, char src, int src_offset) { binop(src, dst, VMI_ADD32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_add_reg_to_mem_32(char dst, char src, int dst_offset) { binop(src, dst, VMI_ADD32_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_sub_reg_to_reg_32(char dst, char src) { binop(src, dst, VMI_SUB32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sub_mem_to_reg_32(char dst, char src, int src_offset) { binop(src, dst, VMI_SUB32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_sub_reg_to_mem_32(char dst, char src, int dst_offset) { binop(src, dst, VMI_SUB32_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mul_reg_to_reg_32(char dst, char src) { binop(src, dst, VMI_IMUL32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mul_memory_to_reg_32(char dst, char src, int src_offset) { binop(src, dst, VMI_IMUL32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_umul_reg_to_reg_32(char reg) { unaryop(reg, VMI_MUL32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_umul_memory_to_reg_32(char reg, int offset) { unaryop(reg, VMI_MUL32_SRC_MEM_DST_REG, offset); }
void po_x86_64_Lower::mc_div_reg_32(char reg) { unaryop(reg, VMI_IDIV32_SRC_REG); }
void po_x86_64_Lower::mc_div_memory_32(char src, int src_offset) { unaryop(src, VMI_IDIV32_SRC_MEM, src_offset); }
void po_x86_64_Lower::mc_udiv_reg_32(char reg) { unaryop(reg, VMI_DIV32_SRC_REG); }
void po_x86_64_Lower::mc_udiv_mem_32(char reg) { unaryop(reg, VMI_DIV32_SRC_MEM); }
void po_x86_64_Lower::mc_mov_reg_to_reg_32(char dst, char src) { binop(src, dst, VMI_MOV32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mov_mem_to_reg_32(char dst, char src, int src_offset) { binop(src, dst, VMI_MOV32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mov_reg_to_mem_32(char dst, char src, int dst_offset) { binop(src, dst, VMI_MOV32_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mov_imm_to_reg_32(char dst, int imm) { unaryop_imm(dst, VMI_MOV32_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_mov_mem_to_reg_32(char dst, int addr) { binop(-1, dst, VMI_MOV32_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_mov_reg_to_mem_32(char src, int addr) { binop(src, -1, VMI_MOV32_SRC_REG_DST_MEM, addr); }
void po_x86_64_Lower::mc_cmp_reg_to_reg_32(char dst, char src) { binop(src, dst, VMI_CMP32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cmp_mem_to_reg_32(char dst, char src, int src_offset) { binop(src, dst, VMI_CMP32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cmp_reg_to_mem_32(char dst, char src, int dst_offset) { binop(src, dst, VMI_CMP32_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_neg_reg_32(int reg) { unaryop(reg, VMI_NEG32_DST_REG); }
void po_x86_64_Lower::mc_sar_imm_to_reg_32(char reg, char imm) { unaryop_imm(reg, VMI_SAR32_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sal_imm_to_reg_32(char reg, char imm) { unaryop_imm(reg, VMI_SAL32_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sar_reg_32(char reg) { unaryop(reg, VMI_SAR32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sal_reg_32(char reg) { unaryop(reg, VMI_SAL32_SRC_REG_DST_REG); }

/*64-bit operations */

void po_x86_64_Lower::mc_mov_imm_to_reg_x64(char dst, long long imm) { unaryop_imm(dst, VMI_MOV64_SRC_IMM_DST_REG, (int64_t)imm); }
void po_x86_64_Lower::mc_mov_reg_to_memory_x64(char dst, int dst_offset, char src) { binop(src, dst, VMI_MOV64_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_mov_memory_to_reg_x64(char dst, char src, int src_offset) { binop(src, dst, VMI_MOV64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mov_memory_to_reg_x64(char dst, int addr) { binop(-1, dst, VMI_MOV64_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_mov_reg_to_memory_x64(char src, int addr) { binop(src, -1, VMI_MOV64_SRC_REG_DST_MEM, addr); };
void po_x86_64_Lower::mc_mov_reg_to_reg_x64(char dst, char src) { binop(src, dst, VMI_MOV64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_add_reg_to_reg_x64(char dst, char src) { binop(src, dst, VMI_ADD64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_add_memory_to_reg_x64(char dst, char src, int src_offset) { binop(src, dst, VMI_ADD64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_add_reg_to_memory_x64(char dst, char src, int dst_offset) { binop(src, dst, VMI_ADD64_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_add_imm_to_reg_x64(char dst, int imm) { unaryop(dst, VMI_ADD64_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sub_reg_to_memory_x64(char dst, char src, int dst_offset) { binop(src, dst, VMI_SUB64_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_sub_reg_to_reg_x64(char dst, char src) { binop(src, dst, VMI_SUB64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sub_memory_to_reg_x64(char dst, char src, int src_offset) { binop(src, dst, VMI_SUB64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_sub_imm_to_reg_x64(char reg, int imm) { unaryop(reg, VMI_SUB64_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_mul_reg_to_reg_x64(char dst, char src) { binop(src, dst, VMI_IMUL64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mul_memory_to_reg_x64(char dst, char src, int src_offset) { binop(src, dst, VMI_IMUL64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_umul_reg_to_reg_x64(char reg) { unaryop(reg, VMI_MUL64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_umul_memory_to_reg_x64(char reg, int offset) { unaryop(reg, VMI_MUL64_SRC_MEM_DST_REG, offset); }
void po_x86_64_Lower::mc_div_reg_x64(char reg) { unaryop(reg, VMI_IDIV64_SRC_REG); }
void po_x86_64_Lower::mc_div_memory_x64(char src, int src_offset) { unaryop(src, VMI_IDIV64_SRC_MEM, src_offset); }
void po_x86_64_Lower::mc_udiv_reg_x64(char reg) { unaryop(reg, VMI_DIV64_SRC_REG); }
void po_x86_64_Lower::mc_udiv_mem_x64(char reg) { unaryop(reg, VMI_DIV64_SRC_MEM); }
void po_x86_64_Lower::mc_cmp_reg_to_reg_x64(char dst, char src) { binop(src, dst, VMI_CMP64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src) { binop(src, dst, VMI_CMP64_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset) { binop(src, dst, VMI_CMP64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_inc_reg_x64(int reg) { unaryop(reg, VMI_INC64_DST_REG); }
void po_x86_64_Lower::mc_inc_memory_x64(int reg, int offset) { unaryop(reg, VMI_INC64_DST_MEM, offset); }
void po_x86_64_Lower::mc_dec_reg_x64(int reg) { unaryop(reg, VMI_DEC64_DST_REG); }
void po_x86_64_Lower::mc_dec_memory_x64(int reg, int offset) { unaryop(reg, VMI_DEC64_DST_MEM, offset); }
void po_x86_64_Lower::mc_neg_memory_x64(int reg, int offset) { unaryop(reg, VMI_NEG64_DST_MEM, offset); }
void po_x86_64_Lower::mc_neg_reg_x64(int reg) { unaryop(reg, VMI_NEG64_DST_REG); }
void po_x86_64_Lower::mc_lea_reg_to_reg_x64(char dst, int addr) { binop(-1, dst, VMI_LEA64_SRC_REG_DST_REG, addr); }
void po_x86_64_Lower::mc_sar_imm_to_reg_x64(char reg, char imm) { unaryop_imm(reg, VMI_SAR64_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sal_imm_to_reg_x64(char reg, char imm) { unaryop_imm(reg, VMI_SAL64_SRC_IMM_DST_REG, imm); }
void po_x86_64_Lower::mc_sar_reg_x64(char reg) { unaryop(reg, VMI_SAR64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_sal_reg_x64(char reg) { unaryop(reg, VMI_SAL64_SRC_REG_DST_REG); }

/* Jump operations */

void po_x86_64_Lower::mc_jump_unconditional_8(int imm) { op_imm(VMI_J8, imm); }
void po_x86_64_Lower::mc_jump_equals_8(int imm) { op_imm(VMI_JE8, imm); }
void po_x86_64_Lower::mc_jump_not_equals_8(int imm) { op_imm(VMI_JNE8, imm); }
void po_x86_64_Lower::mc_jump_less_8(int imm) { op_imm(VMI_JL8, imm); }
void po_x86_64_Lower::mc_jump_less_equal_8(int imm) { op_imm(VMI_JLE8, imm); }
void po_x86_64_Lower::mc_jump_greater_8(int imm) { op_imm(VMI_JG8, imm); }
void po_x86_64_Lower::mc_jump_greater_equal_8(int imm) { op_imm(VMI_JGE8, imm); }
void po_x86_64_Lower::mc_jump_unconditional(int imm) { op_imm(VMI_J32, imm); }
void po_x86_64_Lower::mc_jump_equals(int imm) { op_imm(VMI_JE32, imm); }
void po_x86_64_Lower::mc_jump_not_equals(int imm) { op_imm(VMI_JNE32, imm); }
void po_x86_64_Lower::mc_jump_less(int imm) { op_imm(VMI_JL32, imm); }
void po_x86_64_Lower::mc_jump_less_equal(int imm) { op_imm(VMI_JLE32, imm); }
void po_x86_64_Lower::mc_jump_greater(int imm) { op_imm(VMI_JG32, imm); }
void po_x86_64_Lower::mc_jump_greater_equal(int imm) { op_imm(VMI_JGE32, imm); }
void po_x86_64_Lower::mc_jump_not_above(int imm) { op_imm(VMI_JNA32, imm); }
void po_x86_64_Lower::mc_jump_not_above_equal(int imm) { op_imm(VMI_JNAE32, imm); }
void po_x86_64_Lower::mc_jump_not_below(int imm) { op_imm(VMI_JNB32, imm); }
void po_x86_64_Lower::mc_jump_not_below_equal(int imm) { op_imm(VMI_JNBE32, imm); }
void po_x86_64_Lower::mc_jump_absolute(int reg) {  }
void po_x86_64_Lower::mc_jump_memory(int offset) { op_imm(VMI_JUMP_MEM, offset); }
void po_x86_64_Lower::mc_call_memory(int offset) { op_imm(VMI_CALL_MEM, offset); }
void po_x86_64_Lower::mc_call(int offset) { op_imm(VMI_CALL, offset); }
void po_x86_64_Lower::mc_call_absolute(int reg) { }

/* Sign extend operations */

void po_x86_64_Lower::mc_movsx_8_to_16_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_8_TO_16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_8_to_16_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_8_TO_16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsx_8_to_32_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_8_TO_32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_8_to_32_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_8_TO_32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsx_8_to_64_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_8_TO_64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_8_to_64_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_8_TO_64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsx_16_to_32_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_16_TO_32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_16_to_32_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_16_TO_32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsx_16_to_64_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_16_TO_64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_16_to_64_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_16_TO_64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsx_32_to_64_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVSX_32_TO_64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsx_32_to_64_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVSX_32_TO_64_SRC_MEM_DST_REG, src_offset); }

/* Zero extend operations */

void po_x86_64_Lower::mc_movzx_8_to_16_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVZX_8_TO_16_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movzx_8_to_16_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVZX_8_TO_16_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movzx_8_to_32_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVZX_8_TO_32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movzx_8_to_32_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVZX_8_TO_32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movzx_8_to_64_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVZX_8_TO_64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movzx_8_to_64_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVZX_8_TO_64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movzx_16_to_32_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVZX_16_TO_32_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movzx_16_to_32_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVZX_16_TO_32_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movzx_16_to_64_reg_to_reg(char dst, char src) { binop(src, dst, VMI_MOVZX_16_TO_64_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movzx_16_to_64_mem_to_reg(char dst, char src, int src_offset) { binop(src, dst, VMI_MOVZX_16_TO_64_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cdqe() { _cfg.getLast()->instructions().push_back(po_x86_64_instruction(false, VMI_CDQE, -1, -1)); }

/* Floating point operations */

void po_x86_64_Lower::mc_movsd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_MOVSD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movsd_reg_to_memory_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_MOVSD_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_movsd_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_MOVSD_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movsd_memory_to_reg_x64(int dst, int addr) { sse_unaryop(dst, VMI_SSE_MOVSD_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_addsd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_ADDSD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_addsd_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_ADDSD_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_subsd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_SUBSD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_subsd_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_MOVSD_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mulsd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_MULSD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mulsd_memory_to_reg_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_MULSD_SRC_MEM_DST_REG, dst_offset); }
void po_x86_64_Lower::mc_divsd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_DIVSD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_divsd_memory_to_reg_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_DIVSD_SRC_MEM_DST_REG, dst_offset); }
void po_x86_64_Lower::mc_cvtitod_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cvtitod_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_CVTSI2SD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cvtsdsi_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_CVTSD2SI_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_ucmpd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_UCOMISD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_ucmpd_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_UCOMISD_SRC_MEM_DST_REG); }
void po_x86_64_Lower::mc_xorpd_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_XORPD_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movss_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_MOVSS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_movss_reg_to_memory_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_MOVSS_SRC_REG_DST_MEM, dst_offset); }
void po_x86_64_Lower::mc_movss_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_MOVSS_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_movss_memory_to_reg_x64(int dst, int addr) { sse_unaryop(dst, VMI_SSE_MOVSS_SRC_MEM_DST_REG, addr); }
void po_x86_64_Lower::mc_addss_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_ADDSS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_addss_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_ADDSS_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_subss_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_SUBSS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_subss_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_SUBSS_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_mulss_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_MULSS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_mulss_memory_to_reg_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_MULSS_SRC_MEM_DST_REG, dst_offset); }
void po_x86_64_Lower::mc_divss_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_DIVSS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_divss_memory_to_reg_x64(int dst, int src, int dst_offset) { sse_binop(src, dst, VMI_SSE_DIVSS_SRC_MEM_DST_REG, dst_offset); }
void po_x86_64_Lower::mc_cvtitos_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG, src_offset); }
void po_x86_64_Lower::mc_cvtitos_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_CVTSI2SS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_cvtsssi_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_CVTSS2SI_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_ucmps_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_UCOMISS_SRC_REG_DST_REG); }
void po_x86_64_Lower::mc_ucmps_memory_to_reg_x64(int dst, int src, int src_offset) { sse_binop(src, dst, VMI_SSE_UCOMISS_SRC_MEM_DST_REG); }
void po_x86_64_Lower::mc_xorps_reg_to_reg_x64(int dst, int src) { sse_binop(src, dst, VMI_SSE_XORPS_SRC_REG_DST_REG); }

void po_x86_64_Lower::dump() const
{
    std::cout << "===========================" << std::endl;

    for (size_t i = 0; i < _cfg.basicBlocks().size(); i++)
    {
        auto& bb = _cfg.basicBlocks()[i];
        std::cout << "Basic Block: " << i << std::endl;

        for (const auto& ins : bb->instructions())
        {
            if (ins.isSSE())
            {
                const auto& sse_ins = gInstructions_SSE[ins.opcode()];
                if (sse_ins.code == CODE_BRR)
                {
                    std::cout << ins.opcode() << " " << (ins.dstReg() >= 0 ? sse_registers[ins.dstReg()] : "") << " " << (ins.srcReg() >= 0 ? sse_registers[ins.srcReg()] : "") << " " << std::endl;
                }
                else if (sse_ins.code == CODE_BRMO ||
                    sse_ins.code == CODE_BRM)
                {
                    std::cout << ins.opcode() << " " << (ins.dstReg() >= 0 ? sse_registers[ins.dstReg()] : "") << " " << (ins.srcReg() >= 0 ? registers[ins.srcReg()] : "") << " " << std::endl;
                }
                else if (sse_ins.code == CODE_BMRO ||
                    sse_ins.code == CODE_BMR)
                {
                    std::cout << ins.opcode() << " "<< (ins.dstReg() >= 0 ? registers[ins.dstReg()] : "") << " "  << (ins.srcReg() >= 0 ? sse_registers[ins.srcReg()] : "") << " " << std::endl;
                }
                else
                {
                    std::cout << ins.opcode() << std::endl;
                }
            }
            else
            {
                std::cout << ins.opcode() << " " << (ins.dstReg() >= 0 ? registers[ins.dstReg()] : "") << " " << (ins.srcReg() >= 0 ? registers[ins.srcReg()] : "") << " " << std::endl;
            }
        }
    }
    
    std::cout << "===========================" << std::endl;
}

//================

void po_x86_64::emit_sse_brr(const vm_sse_instruction& ins, int src, int dst)
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

void po_x86_64::emit_sse_brmo(const vm_sse_instruction& ins, int src, int dst, int src_offset)
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

    if ((src % 8) == VM_REGISTER_ESP)
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

void po_x86_64::emit_sse_bmro(const vm_sse_instruction& ins, int src, int dst, int dst_offset)
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

    if ((dst % 8) == VM_REGISTER_ESP)
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

void po_x86_64::emit_sse_brm(const vm_sse_instruction& ins, int dst, int disp32)
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

void po_x86_64::emit(const po_x86_64_instruction& instruction)
{
    if (instruction.isSSE())
    {
        const auto& sse_ins = gInstructions_SSE[instruction.opcode()];
        switch (sse_ins.code) {
            case CODE_BRR:
                emit_sse_brr(sse_ins, instruction.srcReg(), instruction.dstReg());
                break;
            case CODE_BRMO:
                emit_sse_brmo(sse_ins, instruction.srcReg(), instruction.dstReg(), instruction.imm32());
                break;
            case CODE_BMRO:
                emit_sse_bmro(sse_ins, instruction.srcReg(), instruction.dstReg(), instruction.imm32());
                break;
            case CODE_BRM:
                emit_sse_brm(sse_ins, instruction.dstReg(), instruction.imm32());
                break;
            default:
                abort();
                break;
        }
    }
    else
    {
        const auto& ins = gInstructions[instruction.opcode()];
        switch (ins.code) {
            case CODE_BRR:
                emit_brr(ins, instruction.dstReg(), instruction.srcReg());
                break;
            case CODE_BRMO:
                emit_brmo(ins, instruction.dstReg(), instruction.srcReg(), instruction.imm32());
                break;
            case CODE_BMRO:
                emit_bmro(ins, instruction.dstReg(), instruction.srcReg(), instruction.imm32());
                break;
            case CODE_BRM:
                emit_brm(ins, instruction.dstReg(), instruction.imm32());
                break;
            case CODE_BMR:
                emit_bmr(ins, instruction.dstReg(), instruction.imm32());
                break;
            //case CODE_BRI:
            //    emit_bri(ins, instruction.dstReg(), instruction.imm32());
            //    break;
            case CODE_UR:
                assert(instruction.dstReg() >= 0);
                emit_ur(ins, instruction.dstReg());
                break;
            case CODE_UI:
                emit_ui(ins, instruction.imm32());
                break;
            default:
                abort();
                break;
        }
    }
}

void po_x86_64::emit_jump(const int opcode, const int imm)
{
    switch (opcode)
    {
    case VMI_J32:
        mc_jump_unconditional(imm);
        break;
    case VMI_JE8:
        mc_jump_equals_8(imm);
        break;
    case VMI_JNE8:
        mc_jump_not_equals_8(imm);
        break;
    case VMI_JL8:
        mc_jump_less_8(imm);
        break;
    case VMI_JLE8:
        mc_jump_less_equal_8(imm);
        break;
    case VMI_JG8:
        mc_jump_greater_8(imm);
        break;
    case VMI_JGE8:
        mc_jump_greater_equal_8(imm);
        break;
    case VMI_JL32:
        mc_jump_less(imm);
        break;
    case VMI_JLE32:
        mc_jump_less_equal(imm);
        break;
    case VMI_JG32:
        mc_jump_greater(imm);
        break;
    case VMI_JGE32:
        mc_jump_greater_equal(imm);
        break;
    case VMI_JE32:
        mc_jump_equals(imm);
        break;
    case VMI_JNE32:
        mc_jump_not_equals(imm);
        break;
    case VMI_JNA32:
        mc_jump_not_above(imm);
        break;
    case VMI_JNAE32:
        mc_jump_not_above_equal(imm);
        break;
    case VMI_JNB32:
        mc_jump_not_below(imm);
        break;
    case VMI_JNBE32:
        mc_jump_not_below_equal(imm);
        break;
    default:
        abort();
        break;
    }
}

void po_x86_64::mc_reserve()
{
    _programData.push_back(0x90); // nop
}
void po_x86_64::mc_reserve2()
{
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
}
void po_x86_64::mc_reserve3()
{
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
    _programData.push_back(0x90); // nop
}
void po_x86_64::mc_cdq()
{
    _programData.push_back(0x99);
}
void po_x86_64::mc_return()
{
    emit(gInstructions[VMI_NEAR_RETURN]);
}
void po_x86_64::mc_push_32(const int imm)
{
    _programData.push_back(0x68);
    _programData.push_back((unsigned char)(imm & 0xFF));
    _programData.push_back((unsigned char)((imm >> 8) & 0xFF));
    _programData.push_back((unsigned char)((imm >> 16) & 0xFF));
    _programData.push_back((unsigned char)((imm >> 24) & 0xFF));
}
void po_x86_64::mc_push_reg(char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x48 | 0x1);
    }

    _programData.push_back(0x50 | ((reg % 8) & 0x7));
}
void po_x86_64::mc_pop_reg(char reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x48 | 0x1);
    }

    _programData.push_back(0x58 | ((reg % 8) & 0x7));
}
void po_x86_64::mc_mov_imm_to_reg_8(char dst, char imm)
{
    auto& ins = gInstructions[VMI_MOV8_SRC_IMM_DST_REG];

    _programData.push_back(ins.rex | (dst >= VM_REGISTER_R8 ? 0x1 : 0x0));
    _programData.push_back(ins.ins | (dst % 8));
    _programData.push_back((unsigned char)(imm & 0xff));
}
void po_x86_64::mc_mov_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV8_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mov_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_mov_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_MOV8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mov_memory_to_reg_8(char dst, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV8_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_mov_reg_to_memory_8(char src, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV8_SRC_REG_DST_MEM], src, addr);
}
void po_x86_64::mc_add_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD8_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_add_imm_to_reg_8(char dst, char imm)
{
    emit_bri(gInstructions[VMI_ADD8_SRC_IMM_DST_REG], dst, imm);
}
void po_x86_64::mc_add_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_add_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_sub_imm_to_reg_8(char dst, char imm)
{
    emit_bri(gInstructions[VMI_SUB8_SRC_IMM_DST_REG], dst, imm);
}
void po_x86_64::mc_sub_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB8_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_sub_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_sub_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_SUB8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mul_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_IMUL8_SRC_REG], reg);
}
void po_x86_64::mc_umul_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_MUL8_SRC_REG], reg);
}
void po_x86_64::mc_div_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_IDIV8_SRC_REG], reg);
}
void po_x86_64::mc_div_mem_8(char reg)
{
    emit_um(gInstructions[VMI_IDIV8_SRC_MEM], reg);
}
void po_x86_64::mc_udiv_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_DIV8_SRC_REG], reg);
}
void po_x86_64::mc_udiv_mem_8(char reg)
{
    emit_um(gInstructions[VMI_DIV8_SRC_MEM], reg);
}
void po_x86_64::mc_cmp_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP8_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_cmp_memory_to_reg_8(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP8_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_cmp_reg_to_memory_8(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_CMP8_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mov_imm_to_reg_16(char dst, short imm)
{
    auto& ins = gInstructions[VMI_MOV16_SRC_IMM_DST_REG];

    _programData.push_back(ins.legacy);
    
    int rex = ins.rex;
    if (dst >= VM_REGISTER_R8)
    {
        rex |= 0x40 | 0x1;
    }
    if (rex > 0)
    {
        _programData.push_back(rex);
    }

    _programData.push_back(ins.ins | (dst % 8));
    _programData.push_back((unsigned char)(imm & 0xff));
    _programData.push_back((unsigned char)((imm << 8) & 0xff));
}
void po_x86_64::mc_add_reg_to_reg_16(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_add_mem_to_reg_16(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_add_reg_to_mem_16(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD16_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_sub_reg_to_reg_16(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_sub_mem_to_reg_16(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_sub_reg_to_mem_16(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_SUB16_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mul_reg_to_reg_16(char dst, char src)
{
    emit_brr(gInstructions[VMI_IMUL16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mul_memory_to_reg_16(char dst, char src, int src_offset)
{
    emit_bmro(gInstructions[VMI_IMUL16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_umul_reg_to_reg_16(char reg)
{
    emit_ur(gInstructions[VMI_MUL16_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_umul_memory_to_reg_16(char reg, int offset)
{
    emit_umo(gInstructions[VMI_MUL16_SRC_MEM_DST_REG], reg, offset);
}
void po_x86_64::mc_div_reg_16(char reg)
{
    emit_ur(gInstructions[VMI_IDIV16_SRC_REG], reg);
}
void po_x86_64::mc_div_memory_16(char src, int src_offset)
{
    emit_umo(gInstructions[VMI_IDIV16_SRC_MEM], src, src_offset);
}
void po_x86_64::mc_udiv_reg_16(char reg)
{
    emit_ur(gInstructions[VMI_DIV16_SRC_REG], reg);
}
void po_x86_64::mc_udiv_mem_16(char reg)
{
    emit_um(gInstructions[VMI_DIV16_SRC_MEM], reg);
}
void po_x86_64::mc_mov_reg_to_reg_16(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mov_mem_to_reg_16(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_mov_reg_to_mem_16(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_MOV16_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mov_mem_to_reg_16(char dst, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV16_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_mov_reg_to_mem_16(char src, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV16_SRC_REG_DST_MEM], src, addr);
}
void po_x86_64::mc_cmp_reg_to_reg_16(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_cmp_mem_to_reg_16(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_cmp_reg_to_mem_16(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_CMP16_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_neg_reg_16(int reg)
{
    emit_ur(gInstructions[VMI_NEG16_DST_REG], reg);
}
void po_x86_64::mc_sar_imm_to_reg_16(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAR16_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sal_imm_to_reg_16(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAL16_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sar_reg_16(char reg)
{
    emit_ur(gInstructions[VMI_SAR16_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_sal_reg_16(char reg)
{
    emit_ur(gInstructions[VMI_SAL16_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_add_reg_to_reg_32(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_add_mem_to_reg_32(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_add_reg_to_mem_32(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD32_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_sub_reg_to_reg_32(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_sub_mem_to_reg_32(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_sub_reg_to_mem_32(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_SUB32_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mul_reg_to_reg_32(char dst, char src)
{
    emit_brr(gInstructions[VMI_IMUL32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mul_memory_to_reg_32(char dst, char src, int src_offset)
{
    emit_bmro(gInstructions[VMI_IMUL32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_umul_reg_to_reg_32(char reg)
{
    emit_ur(gInstructions[VMI_MUL32_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_umul_memory_to_reg_32(char reg, int offset)
{
    emit_umo(gInstructions[VMI_MUL32_SRC_MEM_DST_REG], reg, offset);
}
void po_x86_64::mc_div_reg_32(char reg)
{
    emit_ur(gInstructions[VMI_IDIV32_SRC_REG], reg);
}
void po_x86_64::mc_div_memory_32(char src, int src_offset)
{
    emit_umo(gInstructions[VMI_IDIV32_SRC_MEM], src, src_offset);
}
void po_x86_64::mc_udiv_reg_32(char reg)
{
    emit_ur(gInstructions[VMI_DIV32_SRC_REG], reg);
}
void po_x86_64::mc_udiv_mem_32(char reg)
{
    emit_um(gInstructions[VMI_DIV32_SRC_MEM], reg);
}
void po_x86_64::mc_mov_reg_to_reg_32(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mov_mem_to_reg_32(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_mov_reg_to_mem_32(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_MOV32_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mov_mem_to_reg_32(char dst, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV32_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_mov_reg_to_mem_32(char src, int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV32_SRC_REG_DST_MEM], src, addr);
}
void po_x86_64::mc_mov_imm_to_reg_32(char dst, int imm)
{
    emit_bri(gInstructions[VMI_MOV32_SRC_IMM_DST_REG], dst, imm);
}
void po_x86_64::mc_cmp_reg_to_reg_32(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_cmp_mem_to_reg_32(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_cmp_reg_to_mem_32(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_CMP32_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_neg_reg_32(int reg)
{
    emit_ur(gInstructions[VMI_NEG32_DST_REG], reg);
}
void po_x86_64::mc_sar_imm_to_reg_32(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAR32_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sal_imm_to_reg_32(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAL32_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sar_reg_32(char reg)
{
    emit_ur(gInstructions[VMI_SAR32_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_sal_reg_32(char reg)
{
    emit_ur(gInstructions[VMI_SAL32_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_mov_imm_to_reg_x64(char dst, long long imm)
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
void po_x86_64::mc_mov_reg_to_memory_x64(char dst, int dst_offset, char src)
{
    emit_bmro(gInstructions[VMI_MOV64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_mov_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOV64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_mov_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOV64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mov_reg_to_memory_x64(const int addr, char src)
{
    emit_brr_disp(gInstructions[VMI_MOV64_SRC_REG_DST_MEM], src, addr);
}
void po_x86_64::mc_mov_memory_to_reg_x64(const char dst, const int addr)
{
    emit_brr_disp(gInstructions[VMI_MOV64_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_add_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_add_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_ADD64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_add_reg_to_memory_x64(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_ADD64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_add_imm_to_reg_x64(char dst, int imm)
{
    emit_bri(gInstructions[VMI_ADD64_SRC_IMM_DST_REG], dst, imm);
}
void po_x86_64::mc_sub_reg_to_memory_x64(char dst, char src, int dst_offset)
{
    emit_bmro(gInstructions[VMI_SUB64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_sub_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_SUB64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_sub_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_SUB64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_sub_imm_to_reg_x64(char reg, int imm)
{
    emit_bri(gInstructions[VMI_SUB64_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_mul_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_IMUL64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_mul_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_IMUL64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_umul_reg_to_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_MUL64_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_umul_memory_to_reg_x64(char reg, int offset)
{
    emit_umo(gInstructions[VMI_MUL64_SRC_MEM_DST_REG], reg, offset);
}
void po_x86_64::mc_div_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_IDIV64_SRC_REG], reg);
}
void po_x86_64::mc_div_memory_x64(char src, int src_offset)
{
    emit_umo(gInstructions[VMI_IDIV64_SRC_MEM], src, src_offset);
}
void po_x86_64::mc_udiv_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_DIV64_SRC_REG], reg);
}
void po_x86_64::mc_udiv_mem_x64(char reg)
{
    emit_um(gInstructions[VMI_DIV64_SRC_MEM], reg);
}
void po_x86_64::mc_cmp_reg_to_reg_x64(char dst, char src)
{
    emit_brr(gInstructions[VMI_CMP64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src)
{
    emit_bmro(gInstructions[VMI_CMP64_SRC_REG_DST_MEM], dst, src, dst_offset);
}
void po_x86_64::mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_CMP64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_jump_unconditional_8(int imm)
{
    emit_ui(gInstructions[VMI_J8], imm);
}
void po_x86_64::mc_jump_equals_8(int imm)
{
    emit_ui(gInstructions[VMI_JE8], imm);
}
void po_x86_64::mc_jump_not_equals_8(int imm)
{
    emit_ui(gInstructions[VMI_JNE8], imm);
}
void po_x86_64::mc_jump_less_8(int imm)
{
    emit_ui(gInstructions[VMI_JL8], imm);
}
void po_x86_64::mc_jump_less_equal_8(int imm)
{
    emit_ui(gInstructions[VMI_JLE8], imm);
}
void po_x86_64::mc_jump_greater_8(int imm)
{
    emit_ui(gInstructions[VMI_JG8], imm);
}
void po_x86_64::mc_jump_greater_equal_8(int imm)
{
    emit_ui(gInstructions[VMI_JGE8], imm);
}
void po_x86_64::mc_jump_unconditional(int imm)
{
    emit_ui(gInstructions[VMI_J32], imm);
}
void po_x86_64::mc_jump_equals(int imm)
{
    emit_ui(gInstructions[VMI_JE32], imm);
}
void po_x86_64::mc_jump_not_equals(int imm)
{
    emit_ui(gInstructions[VMI_JNE32], imm);
}
void po_x86_64::mc_jump_less(int imm)
{
    emit_ui(gInstructions[VMI_JL32], imm);
}
void po_x86_64::mc_jump_less_equal(int imm)
{
    emit_ui(gInstructions[VMI_JLE32], imm);
}
void po_x86_64::mc_jump_greater(int imm)
{
    emit_ui(gInstructions[VMI_JG32], imm);
}
void po_x86_64::mc_jump_greater_equal(int imm)
{
    emit_ui(gInstructions[VMI_JGE32], imm);
}
void po_x86_64::mc_jump_not_above(int imm)
{
    emit_ui(gInstructions[VMI_JNA32], imm);
}
void po_x86_64::mc_jump_not_above_equal(int imm)
{
    emit_ui(gInstructions[VMI_JNAE32], imm);
}
void po_x86_64::mc_jump_not_below(int imm)
{
    emit_ui(gInstructions[VMI_JNB32], imm);
}
void po_x86_64::mc_jump_not_below_equal(int imm)
{
    emit_ui(gInstructions[VMI_JNBE32], imm);
}
void po_x86_64::mc_jump_absolute(int reg)
{
    emit_ur(gInstructions[VMI_JA64], reg);
}
void po_x86_64::mc_jump_memory(int offset)
{
    offset -= 6;

    _programData.push_back(0xFF);
    _programData.push_back((0x4 << 3) | 0x5);
    _programData.push_back((unsigned char)(offset & 0xff));
    _programData.push_back((unsigned char)((offset >> 8) & 0xff));
    _programData.push_back((unsigned char)((offset >> 16) & 0xff));
    _programData.push_back((unsigned char)((offset >> 24) & 0xff));
}
void po_x86_64::mc_call_memory(int offset)
{
    offset -= 6;

    _programData.push_back(0xFF);
    _programData.push_back(0x15);
    _programData.push_back((unsigned char)(offset & 0xff));
    _programData.push_back((unsigned char)((offset >> 8) & 0xff));
    _programData.push_back((unsigned char)((offset >> 16) & 0xff));
    _programData.push_back((unsigned char)((offset >> 24) & 0xff));
}
void po_x86_64::mc_call(int offset)
{
    offset -= 5;

    _programData.push_back(0xE8);
    _programData.push_back((unsigned char)(offset & 0xff));
    _programData.push_back((unsigned char)((offset >> 8) & 0xff));
    _programData.push_back((unsigned char)((offset >> 16) & 0xff));
    _programData.push_back((unsigned char)((offset >> 24) & 0xff));
}
void po_x86_64::mc_call_absolute(int reg)
{
    if (reg >= VM_REGISTER_R8)
    {
        _programData.push_back(0x1 | (0x1 << 6));
    }

    _programData.push_back(0xFF);
    _programData.push_back((0x2 << 3) | (reg % 8) | (0x3 << 6));
}
void po_x86_64::mc_inc_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_INC64_DST_REG], reg);
}
void po_x86_64::mc_inc_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_INC64_DST_MEM], reg, offset);
}
void po_x86_64::mc_dec_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_DEC64_DST_REG], reg);
}
void po_x86_64::mc_dec_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_DEC64_DST_MEM], reg, offset);
}
void po_x86_64::mc_neg_memory_x64(int reg, int offset)
{
    emit_umo(gInstructions[VMI_NEG64_DST_MEM], reg, offset);
}
void po_x86_64::mc_neg_reg_x64(int reg)
{
    emit_ur(gInstructions[VMI_NEG64_DST_REG], reg);
}
void po_x86_64::mc_lea_reg_to_reg_x64(char dst, int addr)
{
    emit_brr_disp(gInstructions[VMI_LEA64_SRC_REG_DST_REG], dst, addr);
}
void po_x86_64::mc_sar_imm_to_reg_x64(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAR64_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sal_imm_to_reg_x64(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAL64_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sar_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_SAR64_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_sal_reg_x64(char reg)
{
    emit_ur(gInstructions[VMI_SAL64_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_neg_reg_8(int reg)
{
    emit_ur(gInstructions[VMI_NEG8_DST_REG], reg);
}
void po_x86_64::mc_sar_imm_to_reg_8(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAR8_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sal_imm_to_reg_8(char reg, char imm)
{
    emit_bri(gInstructions[VMI_SAL8_SRC_IMM_DST_REG], reg, imm);
}
void po_x86_64::mc_sar_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_SAR8_SRC_REG_DST_REG], reg);
}
void po_x86_64::mc_sal_reg_8(char reg)
{
    emit_ur(gInstructions[VMI_SAL8_SRC_REG_DST_REG], reg);
}

void po_x86_64::mc_movsx_8_to_16_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_8_TO_16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_8_to_16_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_8_TO_16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movsx_8_to_32_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_8_TO_32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_8_to_32_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_8_TO_32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movsx_8_to_64_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_8_TO_64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_8_to_64_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_8_TO_64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movsx_16_to_32_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_16_TO_32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_16_to_32_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_16_TO_32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movsx_16_to_64_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_16_TO_64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_16_to_64_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_16_TO_64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movsx_32_to_64_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVSX_32_TO_64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movsx_32_to_64_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVSX_32_TO_64_SRC_MEM_DST_REG], dst, src, src_offset);
}

void po_x86_64::mc_movzx_8_to_16_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVZX_8_TO_16_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movzx_8_to_16_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVZX_8_TO_16_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movzx_8_to_32_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVZX_8_TO_32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movzx_8_to_32_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVZX_8_TO_32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movzx_8_to_64_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVZX_8_TO_64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movzx_8_to_64_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVZX_8_TO_64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movzx_16_to_32_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVZX_16_TO_32_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movzx_16_to_32_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVZX_16_TO_32_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_movzx_16_to_64_reg_to_reg(char dst, char src)
{
    emit_brr(gInstructions[VMI_MOVZX_16_TO_64_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_movzx_16_to_64_mem_to_reg(char dst, char src, int src_offset)
{
    emit_brmo(gInstructions[VMI_MOVZX_16_TO_64_SRC_MEM_DST_REG], dst, src, src_offset);
}
void po_x86_64::mc_cdqe()
{
    emit(gInstructions[VMI_CDQE]);
}


void po_x86_64::mc_movsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_movsd_reg_to_memory_x64(int dst, int src, int dst_offset)
{
    emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSD_SRC_REG_DST_MEM], src, dst, dst_offset);
}
void po_x86_64::mc_movsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_movsd_memory_to_reg_x64(int dst, int addr)
{
    emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSD_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_addsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_addsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_subsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_subsd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_mulsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_mulsd_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSD_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void po_x86_64::mc_divsd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_divsd_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSD_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void po_x86_64::mc_cvtitod_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_cvtitod_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_cvtsdsi_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSD2SI_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_ucmpd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_ucmpd_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISD_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_xorpd_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPD_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_movss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_movss_reg_to_memory_x64(int dst, int src, int dst_offset)
{
    emit_sse_bmro(gInstructions_SSE[VMI_SSE_MOVSS_SRC_REG_DST_MEM], src, dst, dst_offset);
}
void po_x86_64::mc_movss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_movss_memory_to_reg_x64(int dst, int addr)
{
    emit_sse_brm(gInstructions_SSE[VMI_SSE_MOVSS_SRC_MEM_DST_REG], dst, addr);
}
void po_x86_64::mc_addss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_ADDSS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_addss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_ADDSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_subss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_SUBSS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_subss_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_SUBSS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_mulss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_MULSS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_mulss_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_MULSS_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void po_x86_64::mc_divss_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_DIVSS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_divss_memory_to_reg_x64(int dst, int src, int dst_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_DIVSS_SRC_MEM_DST_REG], src, dst, dst_offset);
}
void po_x86_64::mc_cvtitos_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_cvtitos_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSI2SS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_cvtsssi_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_CVTSS2SI_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_ucmps_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_REG_DST_REG], src, dst);
}
void po_x86_64::mc_ucmps_memory_to_reg_x64(int dst, int src, int src_offset)
{
    emit_sse_brmo(gInstructions_SSE[VMI_SSE_UCOMISS_SRC_MEM_DST_REG], src, dst, src_offset);
}
void po_x86_64::mc_xorps_reg_to_reg_x64(int dst, int src)
{
    emit_sse_brr(gInstructions_SSE[VMI_SSE_XORPS_SRC_REG_DST_REG], src, dst);
}


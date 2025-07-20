#include "po_x86_64.h"
#include <assert.h>

using namespace po;

//================

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
};

enum vm_instructions
{
    VMI_ADD64_SRC_REG_DST_REG,
    VMI_ADD64_SRC_IMM_DST_REG,
    VMI_ADD64_SRC_MEM_DST_REG,
    VMI_ADD64_SRC_REG_DST_MEM,

    VMI_ADD32_SRC_REG_DST_REG,
    VMI_ADD32_SRC_IMM_DST_REG,
    VMI_ADD32_SRC_MEM_DST_REG,
    VMI_ADD32_SRC_REG_DST_MEM,

    VMI_ADD16_SRC_REG_DST_REG,
    VMI_ADD16_SRC_IMM_DST_REG,
    VMI_ADD16_SRC_MEM_DST_REG,
    VMI_ADD16_SRC_REG_DST_MEM,

    VMI_ADD8_SRC_IMM_DST_REG,
    VMI_ADD8_SRC_REG_DST_REG,
    VMI_ADD8_SRC_MEM_DST_REG,
    VMI_ADD8_SRC_REG_DST_MEM,

    VMI_SUB64_SRC_REG_DST_REG,
    VMI_SUB64_SRC_IMM_DST_REG,
    VMI_SUB64_SRC_MEM_DST_REG,
    VMI_SUB64_SRC_REG_DST_MEM,

    VMI_SUB32_SRC_REG_DST_REG,
    VMI_SUB32_SRC_IMM_DST_REG,
    VMI_SUB32_SRC_MEM_DST_REG,
    VMI_SUB32_SRC_REG_DST_MEM,

    VMI_SUB16_SRC_REG_DST_REG,
    VMI_SUB16_SRC_IMM_DST_REG,
    VMI_SUB16_SRC_MEM_DST_REG,
    VMI_SUB16_SRC_REG_DST_MEM,

    VMI_SUB8_SRC_REG_DST_REG,
    VMI_SUB8_SRC_IMM_DST_REG,
    VMI_SUB8_SRC_REG_DST_MEM,
    VMI_SUB8_SRC_MEM_DST_REG,

    VMI_MOV64_SRC_REG_DST_REG,
    VMI_MOV64_SRC_REG_DST_MEM,
    VMI_MOV64_SRC_MEM_DST_REG,
    VMI_MOV64_SRC_IMM_DST_REG,

    VMI_MOV32_SRC_REG_DST_REG,
    VMI_MOV32_SRC_REG_DST_MEM,
    VMI_MOV32_SRC_MEM_DST_REG,
    VMI_MOV32_SRC_IMM_DST_REG,

    VMI_MOV16_SRC_REG_DST_REG,
    VMI_MOV16_SRC_REG_DST_MEM,
    VMI_MOV16_SRC_MEM_DST_REG,
    VMI_MOV16_SRC_IMM_DST_REG,

    VMI_MOV8_SRC_IMM_DST_REG,
    VMI_MOV8_SRC_REG_DST_REG,
    VMI_MOV8_SRC_MEM_DST_REG,
    VMI_MOV8_SRC_REG_DST_MEM,

    VMI_IMUL64_SRC_REG_DST_REG,
    VMI_IMUL64_SRC_MEM_DST_REG,

    VMI_IMUL32_SRC_REG_DST_REG,
    VMI_IMUL32_SRC_MEM_DST_REG,

    VMI_IMUL16_SRC_REG_DST_REG,
    VMI_IMUL16_SRC_MEM_DST_REG,

    VMI_MUL64_SRC_REG_DST_REG,
    VMI_MUL64_SRC_MEM_DST_REG,

    VMI_MUL32_SRC_REG_DST_REG,
    VMI_MUL32_SRC_MEM_DST_REG,

    VMI_MUL16_SRC_REG_DST_REG,
    VMI_MUL16_SRC_MEM_DST_REG,

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

    VMI_CMP32_SRC_REG_DST_REG,
    VMI_CMP32_SRC_REG_DST_MEM,
    VMI_CMP32_SRC_MEM_DST_REG,

    VMI_CMP16_SRC_REG_DST_REG,
    VMI_CMP16_SRC_REG_DST_MEM,
    VMI_CMP16_SRC_MEM_DST_REG,

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

    VMI_NEG16_DST_MEM,
    VMI_NEG16_DST_REG,

    VMI_NEG32_DST_MEM,
    VMI_NEG32_DST_REG,

    VMI_NEG64_DST_MEM,
    VMI_NEG64_DST_REG,
    VMI_IDIV64_SRC_REG,
    VMI_IDIV64_SRC_MEM,

    VMI_IDIV32_SRC_REG,
    VMI_IDIV32_SRC_MEM,

    VMI_IDIV16_SRC_REG,
    VMI_IDIV16_SRC_MEM,

    VMI_IDIV8_SRC_REG,
    VMI_IDIV8_SRC_MEM,

    VMI_DIV64_SRC_REG,
    VMI_DIV64_SRC_MEM,

    VMI_DIV32_SRC_REG,
    VMI_DIV32_SRC_MEM,

    VMI_DIV16_SRC_REG,
    VMI_DIV16_SRC_MEM,

    VMI_DIV8_SRC_REG,
    VMI_DIV8_SRC_MEM,

    VMI_MOVSX_8_TO_16_SRC_REG_DST_REG,
    VMI_MOVSX_8_TO_16_SRC_MEM_DST_REG,
    VMI_MOVSX_8_TO_32_SRC_REG_DST_REG,
    VMI_MOVSX_8_TO_32_SRC_MEM_DST_REG,
    VMI_MOVSX_8_TO_64_SRC_REG_DST_REG,
    VMI_MOVSX_8_TO_64_SRC_MEM_DST_REG,

    VMI_MOVSX_16_TO_32_SRC_REG_DST_REG,
    VMI_MOVSX_16_TO_32_SRC_MEM_DST_REG,
    VMI_MOVSX_16_TO_64_SRC_REG_DST_REG,
    VMI_MOVSX_16_TO_64_SRC_MEM_DST_REG,

    VMI_MOVSX_32_TO_64_SRC_REG_DST_REG,
    VMI_MOVSX_32_TO_64_SRC_MEM_DST_REG,

    VMI_MOVZX_8_TO_16_SRC_REG_DST_REG,
    VMI_MOVZX_8_TO_16_SRC_MEM_DST_REG,
    VMI_MOVZX_8_TO_32_SRC_REG_DST_REG,
    VMI_MOVZX_8_TO_32_SRC_MEM_DST_REG,
    VMI_MOVZX_8_TO_64_SRC_REG_DST_REG,
    VMI_MOVZX_8_TO_64_SRC_MEM_DST_REG,

    VMI_MOVZX_16_TO_32_SRC_REG_DST_REG,
    VMI_MOVZX_16_TO_32_SRC_MEM_DST_REG,
    VMI_MOVZX_16_TO_64_SRC_REG_DST_REG,
    VMI_MOVZX_16_TO_64_SRC_MEM_DST_REG,

    VMI_CDQE,

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
    INS(0x0, 0x48, 0x88, VMI_UNUSED, VM_INSTRUCTION_BINARY, CODE_BMR, VMI_ENC_MR), //VMI_MOV8_SRC_REG_DST_MEM,

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

    INS(0x0, 0x48, 0x98, 0x0, VM_INSTRUCTION_NONE, CODE_NONE, VMI_ENC_Z0),// VMI_CDQE
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

        _programData.push_back(ins.ins | (reg % 8));
    }

    _programData.push_back((unsigned char)(imm & 0xff));
    _programData.push_back((unsigned char)((imm >> 8) & 0xff));
    _programData.push_back((unsigned char)((imm >> 16) & 0xff));
    _programData.push_back((unsigned char)((imm >> 24) & 0xff));
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
    //vm_emit(table.ret, program, count);

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
void po_x86_64::mc_add_reg_to_reg_8(char dst, char src)
{
    emit_brr(gInstructions[VMI_ADD8_SRC_REG_DST_REG], dst, src);
}
void po_x86_64::mc_add_imm_to_reg_8(char dst, char imm)
{
    // TODO: the immediate in emit_bri takes an int not a byte
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
    // TODO: the immediate in emit_bri takes an int not a byte
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
void po_x86_64::mc_neg_reg_8(int reg)
{
    emit_ur(gInstructions[VMI_NEG8_DST_REG], reg);
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


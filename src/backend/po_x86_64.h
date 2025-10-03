#pragma once
#include <vector>
#include <cstdint>

struct vm_instruction;
struct vm_sse_instruction;

namespace po
{
    class po_x86_64_basic_block;

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

        VMI_LEA64_SRC_REG_DST_REG,
        VMI_LEA64_SRC_MEM_DST_REG,

        VMI_SAL64_SRC_IMM_DST_REG, // left shift
        VMI_SAL32_SRC_IMM_DST_REG,
        VMI_SAL16_SRC_IMM_DST_REG,
        VMI_SAL8_SRC_IMM_DST_REG,

        VMI_SAR64_SRC_IMM_DST_REG, // right shift
        VMI_SAR32_SRC_IMM_DST_REG,
        VMI_SAR16_SRC_IMM_DST_REG,
        VMI_SAR8_SRC_IMM_DST_REG,

        VMI_SAL64_SRC_REG_DST_REG, // left shift
        VMI_SAL32_SRC_REG_DST_REG,
        VMI_SAL16_SRC_REG_DST_REG,
        VMI_SAL8_SRC_REG_DST_REG,

        VMI_SAR64_SRC_REG_DST_REG, // right shift
        VMI_SAR32_SRC_REG_DST_REG,
        VMI_SAR16_SRC_REG_DST_REG,
        VMI_SAR8_SRC_REG_DST_REG,

        VMI_CDQE,

        VMI_PUSH_REG,
        VMI_POP_REG,
        VMI_JUMP_MEM,
        VMI_CALL_MEM,
        VMI_CALL,

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

        VMI_SSE_CVTSD2SI_SRC_REG_DST_REG,

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

        VMI_SSE_CVTSS2SI_SRC_REG_DST_REG,

        VMI_SSE_UCOMISS_SRC_REG_DST_REG,
        VMI_SSE_UCOMISS_SRC_MEM_DST_REG,

        VMI_SSE_XORPS_SRC_REG_DST_REG,

        // End of instructions
        VMI_SSE_MAX_INSTRUCTIONS
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
#define VM_SSE_ARG5 (-1)
#define VM_SSE_ARG6 (-1)
#define VM_SSE_ARG7 (-1)
#define VM_SSE_ARG8 (-1)
#define VM_MAX_ARGS 4
#define VM_MAX_SSE_ARGS 4
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

    class po_x86_64_instruction
    {
    public:
        po_x86_64_instruction(bool isSSE, int opcode, int srcReg, int dstReg)
            : _isSSE(isSSE), _opcode(opcode), _srcReg(srcReg), _dstReg(dstReg), _imm8(0), _id(-1) {
        }
        po_x86_64_instruction(bool isSSE, int opcode, int srcReg, int dstReg, int8_t imm)
            : _isSSE(isSSE), _opcode(opcode), _srcReg(srcReg), _dstReg(dstReg), _imm8(imm), _id(-1) {
        }
        po_x86_64_instruction(bool isSSE, int opcode, int srcReg, int dstReg, int16_t imm)
            : _isSSE(isSSE), _opcode(opcode), _srcReg(srcReg), _dstReg(dstReg), _imm16(imm), _id(-1) {
        }
        po_x86_64_instruction(bool isSSE, int opcode, int srcReg, int dstReg, int32_t imm)
            : _isSSE(isSSE), _opcode(opcode), _srcReg(srcReg), _dstReg(dstReg), _imm32(imm), _id(-1) {}
        po_x86_64_instruction(bool isSSE, int opcode, int srcReg, int dstReg, int64_t imm)
            : _isSSE(isSSE), _opcode(opcode), _srcReg(srcReg), _dstReg(dstReg), _imm64(imm), _id(-1) {
        }

        inline void setId(int id) { _id = id; }

        inline const int id() const { return _id; }
        inline const int opcode() const { return _opcode; }
        inline const int srcReg() const { return _srcReg; }
        inline const int dstReg() const { return _dstReg; }
        
        inline const int8_t imm8() const { return _imm8; }
        inline const int16_t imm16() const { return _imm16; }
        inline const int32_t imm32() const { return _imm32; }
        inline const int64_t imm64() const { return _imm64; }

        inline void setImm8(const int8_t imm8) { _imm8 = imm8; }
        inline void setImm16(const int16_t imm16) { _imm16 = imm16; }
        inline void setImm32(const int32_t imm32) { _imm32 = imm32; }
        inline void setImm64(const int64_t imm64) { _imm64 = imm64; }

        inline const bool isSSE() const { return _isSSE; }

    private:
        bool _isSSE; // Indicates if the instruction is an SSE instruction
        int _opcode; // The opcode of the instruction
        int _srcReg; // Source register
        int _dstReg; // Destination register
        int _id;    // id for an assoicated constant or function

        union
        {
            int8_t _imm8;
            int16_t _imm16;
            int32_t _imm32;
            int64_t _imm64;
        };
    };

    class po_x86_64_jump
    {
    public:
        po_x86_64_jump()
            :
            _patched(false),
            _programDataPos(-1),
            _jumpType(0),
            _size(0),
            _bb(nullptr)
        {
        }

        inline const bool isPatched() const { return _patched; }
        inline void setPatched(bool patched) { _patched = patched; }

        inline void setProgramDataPos(int pos) { _programDataPos = pos; }
        inline void setJumpType(int type) { _jumpType = type; }
        inline void setSize(int size) { _size = size; }
        inline void setBasicBlock(po_x86_64_basic_block* bb) { _bb = bb; }

        inline const int getProgramDataPos() const { return _programDataPos; }
        inline const int getJumpType() const { return _jumpType; }
        inline const int getSize() const { return _size; }
        inline const po_x86_64_basic_block* getBasicBlock() const { return _bb; }

    private:
        bool _patched; // Indicates if the jump has been patched
        int _programDataPos;
        int _jumpType;
        int _size;
        po_x86_64_basic_block* _bb;
    };

    class po_x86_64_basic_block
    {
    public:
        po_x86_64_basic_block()
            :
            _nextBlock(nullptr),
            _prevBlock(nullptr),
            _jumpTarget(nullptr),
            _programDataPos(-1)
        {
        }

        inline std::vector<po_x86_64_instruction>& instructions() { return _instructions; }
        inline const std::vector<po_x86_64_instruction>& instructions() const { return _instructions; }

        inline std::vector<po_x86_64_basic_block*>& incomingBlocks() { return _incomingBlocks; }
        inline po_x86_64_jump& jump() { return _jump; }

        inline po_x86_64_basic_block* nextBlock() { return _nextBlock; }
        inline po_x86_64_basic_block* prevBlock() { return _prevBlock; }
        inline po_x86_64_basic_block* jumpBlock() { return _jumpTarget; }
        inline po_x86_64_basic_block* jumpBlock() const { return _jumpTarget; }
        
        inline void setNextBlock(po_x86_64_basic_block* bb) { _nextBlock = bb; }
        inline void setPrevBlock(po_x86_64_basic_block* bb) { _prevBlock = bb; }
        inline void setJumpTarget(po_x86_64_basic_block* bb) { _jumpTarget = bb; }

        inline int programDataPos() const { return _programDataPos; }
        inline void setProgramDataPos(int pos) { _programDataPos = pos; }

    private:
        std::vector<po_x86_64_instruction> _instructions; // Instructions in the basic block
        std::vector<po_x86_64_basic_block*> _incomingBlocks; // Incoming blocks to this basic block
        po_x86_64_jump _jump; // Jump from this basic block
        po_x86_64_basic_block* _nextBlock; // Pointer to the next basic block
        po_x86_64_basic_block* _prevBlock; // Pointer to the previous basic block
        po_x86_64_basic_block* _jumpTarget; // Pointer to the jump target block, if any
        int _programDataPos;
    };

    class po_x86_64_flow_graph
    {
    public:
        inline std::vector<po_x86_64_basic_block*>& basicBlocks() { return _basicBlocks; }
        inline const std::vector<po_x86_64_basic_block*>& basicBlocks() const { return _basicBlocks; }
        void addBasicBlock(po_x86_64_basic_block* block) { _basicBlocks.push_back(block); }
        po_x86_64_basic_block* getLast() { return _basicBlocks.back(); }
        inline void clear() { _basicBlocks.clear(); }
    private:
        std::vector<po_x86_64_basic_block*> _basicBlocks; // List of basic blocks in the flow graph
    };

    class po_x86_64_Lower
    {
    public:
        //===================================
        // High level machine code routines
        //===================================

        void mc_reserve();
        void mc_reserve2();
        void mc_reserve3();
        void mc_cdq();
        void mc_return();
        void mc_push_32(const int imm);
        void mc_push_reg(char reg);
        void mc_pop_reg(char reg);

        /* 8-bit operations */

        void mc_mov_imm_to_reg_8(char dst, char imm);
        void mc_mov_reg_to_reg_8(char dst, char src);
        void mc_mov_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_mov_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_add_reg_to_reg_8(char dst, char src);
        void mc_add_imm_to_reg_8(char dst, char imm);
        void mc_add_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_add_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_sub_imm_to_reg_8(char dst, char imm);
        void mc_sub_reg_to_reg_8(char dst, char src);
        void mc_sub_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_sub_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_mul_reg_8(char reg);
        void mc_umul_reg_8(char reg);
        void mc_div_reg_8(char reg);
        void mc_div_mem_8(char reg);
        void mc_udiv_reg_8(char reg);
        void mc_udiv_mem_8(char reg);
        void mc_cmp_reg_to_reg_8(char dst, char src);
        void mc_cmp_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_cmp_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_neg_reg_8(int reg);
        void mc_sar_imm_to_reg_8(char reg, char imm);
        void mc_sal_imm_to_reg_8(char reg, char imm);
        void mc_sar_reg_8(char reg);
        void mc_sal_reg_8(char reg);

        /* 16-bit operations */

        void mc_mov_imm_to_reg_16(char dst, short imm);
        void mc_add_reg_to_reg_16(char dst, char src);
        void mc_add_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_add_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_16(char dst, char src);
        void mc_sub_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_sub_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_mul_reg_to_reg_16(char dst, char src);
        void mc_mul_memory_to_reg_16(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_16(char reg);
        void mc_umul_memory_to_reg_16(char reg, int offset);
        void mc_div_reg_16(char reg);
        void mc_div_memory_16(char src, int src_offset);
        void mc_udiv_reg_16(char reg);
        void mc_udiv_mem_16(char reg);
        void mc_mov_reg_to_reg_16(char dst, char src);
        void mc_mov_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_mov_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_cmp_reg_to_reg_16(char dst, char src);
        void mc_cmp_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_cmp_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_neg_reg_16(int reg);
        void mc_sar_imm_to_reg_16(char reg, char imm);
        void mc_sal_imm_to_reg_16(char reg, char imm);
        void mc_sar_reg_16(char reg);
        void mc_sal_reg_16(char reg);

        /* 32-bit operations */

        void mc_add_reg_to_reg_32(char dst, char src);
        void mc_add_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_add_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_32(char dst, char src);
        void mc_sub_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_sub_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_mul_reg_to_reg_32(char dst, char src);
        void mc_mul_memory_to_reg_32(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_32(char reg);
        void mc_umul_memory_to_reg_32(char reg, int offset);
        void mc_div_reg_32(char reg);
        void mc_div_memory_32(char src, int src_offset);
        void mc_udiv_reg_32(char reg);
        void mc_udiv_mem_32(char reg);
        void mc_mov_reg_to_reg_32(char dst, char src);
        void mc_mov_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_mov_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_mov_imm_to_reg_32(char dst, int imm);
        void mc_cmp_reg_to_reg_32(char dst, char src);
        void mc_cmp_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_cmp_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_neg_reg_32(int reg);
        void mc_sar_imm_to_reg_32(char reg, char imm);
        void mc_sal_imm_to_reg_32(char reg, char imm);
        void mc_sar_reg_32(char reg);
        void mc_sal_reg_32(char reg);

        /*64-bit operations */

        void mc_mov_imm_to_reg_x64(char dst, long long imm);
        void mc_mov_reg_to_memory_x64(char dst, int dst_offset, char src);
        void mc_mov_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_mov_reg_to_reg_x64(char dst, char src);
        void mc_add_reg_to_reg_x64(char dst, char src);
        void mc_add_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_add_reg_to_memory_x64(char dst, char src, int dst_offset);
        void mc_add_imm_to_reg_x64(char dst, int imm);
        void mc_sub_reg_to_memory_x64(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_x64(char dst, char src);
        void mc_sub_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_sub_imm_to_reg_x64(char reg, int imm);
        void mc_mul_reg_to_reg_x64(char dst, char src);
        void mc_mul_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_x64(char reg);
        void mc_umul_memory_to_reg_x64(char reg, int offset);
        void mc_div_reg_x64(char reg);
        void mc_div_memory_x64(char src, int src_offset);
        void mc_udiv_reg_x64(char reg);
        void mc_udiv_mem_x64(char reg);
        void mc_cmp_reg_to_reg_x64(char dst, char src);
        void mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src);
        void mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_inc_reg_x64(int reg);
        void mc_inc_memory_x64(int reg, int offset);
        void mc_dec_reg_x64(int reg);
        void mc_dec_memory_x64(int reg, int offset);
        void mc_neg_memory_x64(int reg, int offset);
        void mc_neg_reg_x64(int reg);
        void mc_lea_reg_to_reg_x64(char dst, int addr);
        void mc_sar_imm_to_reg_x64(char reg, char imm);
        void mc_sal_imm_to_reg_x64(char reg, char imm);
        void mc_sar_reg_x64(char reg);
        void mc_sal_reg_x64(char reg);

        /* Jump operations */

        void mc_jump_unconditional_8(int imm);
        void mc_jump_equals_8(int imm);
        void mc_jump_not_equals_8(int imm);
        void mc_jump_less_8(int imm);
        void mc_jump_less_equal_8(int imm);
        void mc_jump_greater_8(int imm);
        void mc_jump_greater_equal_8(int imm);
        void mc_jump_unconditional(int imm);
        void mc_jump_equals(int imm);
        void mc_jump_not_equals(int imm);
        void mc_jump_less(int imm);
        void mc_jump_less_equal(int imm);
        void mc_jump_greater(int imm);
        void mc_jump_greater_equal(int imm);
        void mc_jump_not_above(int imm);
        void mc_jump_not_above_equal(int imm);
        void mc_jump_not_below(int imm);
        void mc_jump_not_below_equal(int imm);
        void mc_jump_absolute(int reg);
        void mc_jump_memory(int offset);
        void mc_call_memory(int offset);
        void mc_call(int offset);
        void mc_call_absolute(int reg);

        /* Sign extend operations */

        void mc_movsx_8_to_16_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_16_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_8_to_32_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_8_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_16_to_32_reg_to_reg(char dst, char src);
        void mc_movsx_16_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_16_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_16_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_32_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_32_to_64_mem_to_reg(char dst, char src, int src_offset);

        /* Zero extend operations */

        void mc_movzx_8_to_16_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_16_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_8_to_32_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_8_to_64_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_16_to_32_reg_to_reg(char dst, char src);
        void mc_movzx_16_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_16_to_64_reg_to_reg(char dst, char src);
        void mc_movzx_16_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_cdqe();

        /* Floating point operations */

        void mc_movsd_reg_to_reg_x64(int dst, int src);
        void mc_movsd_reg_to_memory_x64(int dst, int src, int dst_offset);
        void mc_movsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_movsd_memory_to_reg_x64(int dst, int addr);
        void mc_addsd_reg_to_reg_x64(int dst, int src);
        void mc_addsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_subsd_reg_to_reg_x64(int dst, int src);
        void mc_subsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_mulsd_reg_to_reg_x64(int dst, int src);
        void mc_mulsd_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_divsd_reg_to_reg_x64(int dst, int src);
        void mc_divsd_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_cvtitod_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_cvtitod_reg_to_reg_x64(int dst, int src);
        void mc_cvtsdsi_reg_to_reg_x64(int dst, int src);
        void mc_ucmpd_reg_to_reg_x64(int dst, int src);
        void mc_ucmpd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_xorpd_reg_to_reg_x64(int dst, int src);
        void mc_movss_reg_to_reg_x64(int dst, int src);
        void mc_movss_reg_to_memory_x64(int dst, int src, int dst_offset);
        void mc_movss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_movss_memory_to_reg_x64(int dst, int addr);
        void mc_addss_reg_to_reg_x64(int dst, int src);
        void mc_addss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_subss_reg_to_reg_x64(int dst, int src);
        void mc_subss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_mulss_reg_to_reg_x64(int dst, int src);
        void mc_mulss_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_divss_reg_to_reg_x64(int dst, int src);
        void mc_divss_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_cvtitos_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_cvtitos_reg_to_reg_x64(int dst, int src);
        void mc_cvtsssi_reg_to_reg_x64(int dst, int src);
        void mc_ucmps_reg_to_reg_x64(int dst, int src);
        void mc_ucmps_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_xorps_reg_to_reg_x64(int dst, int src);

        inline po_x86_64_flow_graph& cfg() { return _cfg; }
        void dump() const;

    private:
        po_x86_64_flow_graph _cfg;

        void op_imm(const int opcode, const int imm);

        void unaryop_imm(const int dst, const int opcode, const char imm);
        void unaryop_imm(const int dst, const int opcode, const short imm);
        void unaryop_imm(const int dst, const int opcode, const int32_t imm);
        void unaryop_imm(const int dst, const int opcode, const int64_t imm);

        void unaryop(const int dst, const int opcode);
        void unaryop(const int dst, const int opcode, const int offset);
        void binop(const int src, const int dst, const int opcode);
        void binop(const int src, const int dst, const int opcode, const int offset);

        void sse_binop(const int src, const int dst, const int opcode);
        void sse_binop(const int src, const int dst, const int opcode, const int offset);
        void sse_unaryop(const int dst, const int opcode, const int offset);
    };

    class po_x86_64
    {
    public:
        inline std::vector<unsigned char>& programData() { return _programData; }
        inline const std::vector<unsigned char>& programData() const { return _programData; }

        void emit(const po_x86_64_instruction& instruction);
        void emit_jump(const int opcode, const int offset);

        //===================================
        // High level machine code routines
        //===================================

        void mc_reserve();
        void mc_reserve2();
        void mc_reserve3();
        void mc_cdq();
        void mc_return();
        void mc_push_32(const int imm);
        void mc_push_reg(char reg);
        void mc_pop_reg(char reg);

        /* 8-bit operations */

        void mc_mov_imm_to_reg_8(char dst, char imm);
        void mc_mov_reg_to_reg_8(char dst, char src);
        void mc_mov_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_mov_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_add_reg_to_reg_8(char dst, char src);
        void mc_add_imm_to_reg_8(char dst, char imm);
        void mc_add_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_add_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_sub_imm_to_reg_8(char dst, char imm);
        void mc_sub_reg_to_reg_8(char dst, char src);
        void mc_sub_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_sub_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_mul_reg_8(char reg);
        void mc_umul_reg_8(char reg);
        void mc_div_reg_8(char reg);
        void mc_div_mem_8(char reg);
        void mc_udiv_reg_8(char reg);
        void mc_udiv_mem_8(char reg);
        void mc_cmp_reg_to_reg_8(char dst, char src);
        void mc_cmp_memory_to_reg_8(char dst, char src, int src_offset);
        void mc_cmp_reg_to_memory_8(char dst, char src, int dst_offset);
        void mc_neg_reg_8(int reg);
        void mc_sar_imm_to_reg_8(char reg, char imm);
        void mc_sal_imm_to_reg_8(char reg, char imm);
        void mc_sar_reg_8(char reg);
        void mc_sal_reg_8(char reg);

        /* 16-bit operations */

        void mc_mov_imm_to_reg_16(char dst, short imm);
        void mc_add_reg_to_reg_16(char dst, char src);
        void mc_add_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_add_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_16(char dst, char src);
        void mc_sub_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_sub_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_mul_reg_to_reg_16(char dst, char src);
        void mc_mul_memory_to_reg_16(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_16(char reg);
        void mc_umul_memory_to_reg_16(char reg, int offset);
        void mc_div_reg_16(char reg);
        void mc_div_memory_16(char src, int src_offset);
        void mc_udiv_reg_16(char reg);
        void mc_udiv_mem_16(char reg);
        void mc_mov_reg_to_reg_16(char dst, char src);
        void mc_mov_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_mov_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_cmp_reg_to_reg_16(char dst, char src);
        void mc_cmp_mem_to_reg_16(char dst, char src, int src_offset);
        void mc_cmp_reg_to_mem_16(char dst, char src, int dst_offset);
        void mc_neg_reg_16(int reg);
        void mc_sar_imm_to_reg_16(char reg, char imm);
        void mc_sal_imm_to_reg_16(char reg, char imm);
        void mc_sar_reg_16(char reg);
        void mc_sal_reg_16(char reg);

        /* 32-bit operations */

        void mc_add_reg_to_reg_32(char dst, char src);
        void mc_add_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_add_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_32(char dst, char src);
        void mc_sub_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_sub_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_mul_reg_to_reg_32(char dst, char src);
        void mc_mul_memory_to_reg_32(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_32(char reg);
        void mc_umul_memory_to_reg_32(char reg, int offset);
        void mc_div_reg_32(char reg);
        void mc_div_memory_32(char src, int src_offset);
        void mc_udiv_reg_32(char reg);
        void mc_udiv_mem_32(char reg);
        void mc_mov_reg_to_reg_32(char dst, char src);
        void mc_mov_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_mov_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_mov_imm_to_reg_32(char dst, int imm);
        void mc_cmp_reg_to_reg_32(char dst, char src);
        void mc_cmp_mem_to_reg_32(char dst, char src, int src_offset);
        void mc_cmp_reg_to_mem_32(char dst, char src, int dst_offset);
        void mc_neg_reg_32(int reg);
        void mc_sar_imm_to_reg_32(char reg, char imm);
        void mc_sal_imm_to_reg_32(char reg, char imm);
        void mc_sar_reg_32(char reg);
        void mc_sal_reg_32(char reg);

        /*64-bit operations */

        void mc_mov_imm_to_reg_x64(char dst, long long imm);
        void mc_mov_reg_to_memory_x64(char dst, int dst_offset, char src);
        void mc_mov_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_mov_reg_to_reg_x64(char dst, char src);
        void mc_add_reg_to_reg_x64(char dst, char src);
        void mc_add_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_add_reg_to_memory_x64(char dst, char src, int dst_offset);
        void mc_add_imm_to_reg_x64(char dst, int imm);
        void mc_sub_reg_to_memory_x64(char dst, char src, int dst_offset);
        void mc_sub_reg_to_reg_x64(char dst, char src);
        void mc_sub_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_sub_imm_to_reg_x64(char reg, int imm);
        void mc_mul_reg_to_reg_x64(char dst, char src);
        void mc_mul_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_umul_reg_to_reg_x64(char reg);
        void mc_umul_memory_to_reg_x64(char reg, int offset);
        void mc_div_reg_x64(char reg);
        void mc_div_memory_x64(char src, int src_offset);
        void mc_udiv_reg_x64(char reg);
        void mc_udiv_mem_x64(char reg);
        void mc_cmp_reg_to_reg_x64(char dst, char src);
        void mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src);
        void mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset);
        void mc_inc_reg_x64(int reg);
        void mc_inc_memory_x64(int reg, int offset);
        void mc_dec_reg_x64(int reg);
        void mc_dec_memory_x64(int reg, int offset);
        void mc_neg_memory_x64(int reg, int offset);
        void mc_neg_reg_x64(int reg);
        void mc_lea_reg_to_reg_x64(char dst, int addr);
        void mc_sar_imm_to_reg_x64(char reg, char imm);
        void mc_sal_imm_to_reg_x64(char reg, char imm);
        void mc_sar_reg_x64(char reg);
        void mc_sal_reg_x64(char reg);

        /* Jump operations */

        void mc_jump_unconditional_8(int imm);
        void mc_jump_equals_8(int imm);
        void mc_jump_not_equals_8(int imm);
        void mc_jump_less_8(int imm);
        void mc_jump_less_equal_8(int imm);
        void mc_jump_greater_8(int imm);
        void mc_jump_greater_equal_8(int imm);
        void mc_jump_unconditional(int imm);
        void mc_jump_equals(int imm);
        void mc_jump_not_equals(int imm);
        void mc_jump_less(int imm);
        void mc_jump_less_equal(int imm);
        void mc_jump_greater(int imm);
        void mc_jump_greater_equal(int imm);
        void mc_jump_not_above(int imm);
        void mc_jump_not_above_equal(int imm);
        void mc_jump_not_below(int imm);
        void mc_jump_not_below_equal(int imm);
        void mc_jump_absolute(int reg);
        void mc_jump_memory(int offset);
        void mc_call_memory(int offset);
        void mc_call(int offset);
        void mc_call_absolute(int reg);
        
        /* Sign extend operations */

        void mc_movsx_8_to_16_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_16_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_8_to_32_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_8_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_8_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_16_to_32_reg_to_reg(char dst, char src);
        void mc_movsx_16_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_16_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_16_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movsx_32_to_64_reg_to_reg(char dst, char src);
        void mc_movsx_32_to_64_mem_to_reg(char dst, char src, int src_offset);

        /* Zero extend operations */

        void mc_movzx_8_to_16_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_16_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_8_to_32_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_8_to_64_reg_to_reg(char dst, char src);
        void mc_movzx_8_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_16_to_32_reg_to_reg(char dst, char src);
        void mc_movzx_16_to_32_mem_to_reg(char dst, char src, int src_offset);
        void mc_movzx_16_to_64_reg_to_reg(char dst, char src);
        void mc_movzx_16_to_64_mem_to_reg(char dst, char src, int src_offset);
        void mc_cdqe();

        /* Floating point operations */

        void mc_movsd_reg_to_reg_x64(int dst, int src);
        void mc_movsd_reg_to_memory_x64(int dst, int src, int dst_offset);
        void mc_movsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_movsd_memory_to_reg_x64(int dst, int addr);
        void mc_addsd_reg_to_reg_x64(int dst, int src);
        void mc_addsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_subsd_reg_to_reg_x64(int dst, int src);
        void mc_subsd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_mulsd_reg_to_reg_x64(int dst, int src);
        void mc_mulsd_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_divsd_reg_to_reg_x64(int dst, int src);
        void mc_divsd_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_cvtitod_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_cvtitod_reg_to_reg_x64(int dst, int src);
        void mc_cvtsdsi_reg_to_reg_x64(int dst, int src);
        void mc_ucmpd_reg_to_reg_x64(int dst, int src);
        void mc_ucmpd_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_xorpd_reg_to_reg_x64(int dst, int src);
        void mc_movss_reg_to_reg_x64(int dst, int src);
        void mc_movss_reg_to_memory_x64(int dst, int src, int dst_offset);
        void mc_movss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_movss_memory_to_reg_x64(int dst, int addr);
        void mc_addss_reg_to_reg_x64(int dst, int src);
        void mc_addss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_subss_reg_to_reg_x64(int dst, int src);
        void mc_subss_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_mulss_reg_to_reg_x64(int dst, int src);
        void mc_mulss_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_divss_reg_to_reg_x64(int dst, int src);
        void mc_divss_memory_to_reg_x64(int dst, int src, int dst_offset);
        void mc_cvtitos_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_cvtitos_reg_to_reg_x64(int dst, int src);
        void mc_cvtsssi_reg_to_reg_x64(int dst, int src);
        void mc_ucmps_reg_to_reg_x64(int dst, int src);
        void mc_ucmps_memory_to_reg_x64(int dst, int src, int src_offset);
        void mc_xorps_reg_to_reg_x64(int dst, int src);

    private:
        //============================================
        // Low level machine code generation routines
        //============================================

        void emit(const vm_instruction& ins);
        void emit_ur(const vm_instruction& ins, char reg);
        void emit_um(const vm_instruction& ins, char reg);
        void emit_umo(const vm_instruction& ins, char reg, int offset);
        void emit_bri(const vm_instruction& ins, char reg, int imm);
        void emit_bri(const vm_instruction& ins, char reg, char imm);
        void emit_brr(const vm_instruction& ins, char dst, char src);
        void emit_brm(const vm_instruction& ins, char dst, char src);
        void emit_bmr(const vm_instruction& ins, char dst, char src);
        void emit_brmo(const vm_instruction& ins, char dst, char src, int offset);
        void emit_bmro(const vm_instruction& ins, char dst, char src, int offset);
        void emit_brr_disp(const vm_instruction& ins, int dst, int disp32);
        void emit_ui(const vm_instruction& ins, char imm);
        void emit_ui(const vm_instruction& ins, int imm);

        void emit_sse_brr(const vm_sse_instruction& ins, int src, int dst);
        void emit_sse_brmo(const vm_sse_instruction& ins, int src, int dst, int src_offset);
        void emit_sse_bmro(const vm_sse_instruction& ins, int src, int dst, int dst_offset);
        void emit_sse_brm(const vm_sse_instruction& ins, int dst, int disp32);

        std::vector<unsigned char> _programData;
    };
}

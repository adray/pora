#pragma once
#include <vector>

struct vm_instruction;
struct vm_sse_instruction;

namespace po
{
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

    class po_x86_64
    {
    public:
        inline std::vector<unsigned char>& programData() { return _programData; }
        inline const std::vector<unsigned char>& programData() const { return _programData; }

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
        void emit_brr(const vm_instruction& ins, char dst, char src);
        void emit_brm(const vm_instruction& ins, char dst, char src);
        void emit_bmr(const vm_instruction& ins, char dst, char src);
        void emit_brmo(const vm_instruction& ins, char dst, char src, int offset);
        void emit_bmro(const vm_instruction& ins, char dst, char src, int offset);
        void emit_ui(const vm_instruction& ins, char imm);
        void emit_ui(const vm_instruction& ins, int imm);

        void emit_sse_brr(const vm_sse_instruction& ins, int src, int dst);
        void emit_sse_brmo(const vm_sse_instruction& ins, int src, int dst, int src_offset);
        void emit_sse_bmro(const vm_sse_instruction& ins, int src, int dst, int dst_offset);
        void emit_sse_brm(const vm_sse_instruction& ins, int dst, int disp32);

        std::vector<unsigned char> _programData;
    };
}

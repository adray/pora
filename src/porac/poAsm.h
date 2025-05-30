#pragma once
#include "poPhiWeb.h"

#include <string>
#include <vector>
#include <unordered_map>

struct vm_instruction;
struct vm_sse_instruction;

namespace po
{
    class poFlowGraph;
    class poModule;
    class poInstruction;
    class poRegLinear;
    class poConstantPool;
    class poBasicBlock;

    class poAsmJump
    {
    public:
        poAsmJump(const int programDataPos, const int jumpType, const int size, poBasicBlock* bb, const int type);

        inline const int getProgramDataPos() const { return _programDataPos; }
        inline const int getJumpType() const { return _jumpType; }
        inline const int getSize() const { return _size; }
        inline const int getType() const { return _type; }
        inline const poBasicBlock* getBasicBlock() const { return _bb; }
        inline const bool isApplied() const { return _applied; }
        inline void setApplied(bool applied) { _applied = applied; }

    private:
        bool _applied;
        int _programDataPos;
        int _jumpType;
        int _type;
        int _size;
        poBasicBlock* _bb;
    };

    class poAsmCopy
    {
    public:
        poAsmCopy(const int src, const int dst)
            :
            _src(src),
            _dst(dst)
        {
        }

        inline const int dst() const { return _dst; }
        inline const int src() const { return _src; }

    private:
        int _src;
        int _dst;
    };

    class poAsmBasicBlock
    {
    public:
        poAsmBasicBlock();
        inline std::vector<poAsmJump>& jumps() { return _jumps; }
        inline const int getPos() const { return _pos; }
        inline void setPos(const int pos) { _pos = pos; }

    private:
        int _pos;
        std::vector<poAsmJump> _jumps;
    };

    class poAsmCall
    {
    public:
        poAsmCall(const int pos, const int arity, const std::string& symbol);

        inline const int getPos() const { return _pos; }
        inline const int getArity() const { return _arity; }
        inline const std::string& getSymbol() const { return _symbol; }

    private:
        int _pos;
        int _arity;
        std::string _symbol;
    };

    class poAsmConstant
    {
    public:
        poAsmConstant(const int initializedDataPos, const int programDataPos);
        inline int getInitializedDataPos() const { return _initializedDataPos; }
        inline int getProgramDataPos() const { return _programDataPos; }

    private:
        int _initializedDataPos;
        int _programDataPos;
    };

    class poAsm
    {
    public:
        poAsm();
        void generate(poModule& module);
        void link(const int programDataPos, const int initializedDataPos);

        inline const std::vector<unsigned char>& programData() const { return _programData; }
        inline const std::vector<unsigned char>& initializedData() const { return _initializedData; }
        inline const int entryPoint() const { return _entryPoint; }
        inline bool isError() const { return _isError; }
        inline const std::string& errorText() const { return _errorText; }

    private:
        void dump(const poRegLinear& linear, poFlowGraph& cfg);

        void spill(poRegLinear& linear, const int pos);
        void restore(const poInstruction& ins, poRegLinear& linear, const int pos);

        void generate(poModule& module, poFlowGraph& cfg);
        void patchForwardJumps(poBasicBlock* bb);
        void scanBasicBlocks(poFlowGraph& cfg);
        void patchJump(const poAsmJump& jump);
        void patchCalls();
        void generatePrologue(poRegLinear& linear);
        void generateEpilogue(poRegLinear& linear);
        void setError(const std::string& errorText);
        void addInitializedData(const float f32, const int programDataOffset);
        void addInitializedData(const double f64, const int programDataOffset);

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
        void mc_mov_imm_to_reg(char dst, int imm);
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
        void mc_div_reg_x64(char reg);
        void mc_div_memory_x64(char src, int src_offset);
        void mc_cmp_reg_to_reg_x64(char dst, char src);
        void mc_cmp_reg_to_memory_x64(char dst, int dst_offset, char src);
        void mc_cmp_memory_to_reg_x64(char dst, char src, int src_offset);
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
        void mc_call(int offset);
        void mc_call_absolute(int reg);
        void mc_inc_reg_x64(int reg);
        void mc_inc_memory_x64(int reg, int offset);
        void mc_dec_reg_x64(int reg);
        void mc_dec_memory_x64(int reg, int offset);
        void mc_neg_memory_x64(int reg, int offset);
        void mc_neg_reg_x64(int reg);
        void mc_neg_reg_8(int reg);
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

        //=====================================
        // IR to machine code routines
        //=====================================

        void ir_load(poRegLinear& linear, const poInstruction& ins, const int instructionIndex);
        void ir_add(poRegLinear& linear, const poInstruction& ins);
        void ir_sub(poRegLinear& linear, const poInstruction& ins);
        void ir_mul(poRegLinear& linear, const poInstruction& ins);
        void ir_div(poRegLinear& linear, const poInstruction& ins);
        void ir_cmp(poRegLinear& linear, const poInstruction& ins);
        void ir_br(poRegLinear& linear, const poInstruction& ins, poBasicBlock* bb);
        void ir_constant(poConstantPool& constants, poRegLinear& linear, const poInstruction& ins);
        void ir_copy(poRegLinear& linear, const poInstruction& ins);
        void ir_ret(poRegLinear& linear, const poInstruction& ins);
        void ir_unary_minus(poRegLinear& linear, const poInstruction& ins);
        void ir_call(poModule& module, poRegLinear& linear, const poInstruction& ins, const std::vector<poInstruction>& args);
        void ir_param(poRegLinear& linear, const poInstruction& ins);
        bool ir_jump(const int jump, const int imm, const int type);

        std::unordered_map<std::string, int> _mapping;
        std::vector<poAsmCall> _calls;
        std::vector<poAsmConstant> _constants;
        std::unordered_map<poBasicBlock*, int> _basicBlockMap;
        std::vector<poAsmBasicBlock> _basicBlocks;
        std::vector<unsigned char> _programData;
        std::vector<unsigned char> _initializedData;
        poPhiWeb _web;
        int _entryPoint;
        bool _isError;
        std::string _errorText;
        int _prologueSize;
    };
}

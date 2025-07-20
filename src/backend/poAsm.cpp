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

//==================
// poAsmExternCall
//==================

poAsmExternCall::poAsmExternCall(const std::string& name, const int programPos)
    :
    _name(name),
    _programPos(programPos)
{
}

//================

void poAsm::ir_element_ptr(poModule& module, poRegLinear& linear, const poInstruction& ins)
{
    // We need to get the pointer to the variable (left) optionally adding the variable (right) and a memory offset.

    const int dst = linear.getRegisterByVariable(ins.name());
    const int slot = linear.getStackSlotByVariable(ins.left()); /* assume the pointer is the stack position */

    const poType& type = module.types()[ins.type()];
    const int size = type.size();

    const int offset = slot * 8;
    if (ins.right() != -1)
    {
        const int operand = linear.getRegisterByVariable(ins.right());

        // TODO: change to LEA? (load effective address)

        _x86_64.mc_mov_imm_to_reg_x64(dst, size);
        _x86_64.mc_mul_reg_to_reg_x64(dst, operand);

        _x86_64.mc_add_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        if (offset > 0)
        {
            _x86_64.mc_add_imm_to_reg_x64(dst, offset);
        }
    }
    else
    {
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        if (offset > 0)
        {
            _x86_64.mc_add_imm_to_reg_x64(dst, offset);
        }
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
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        if (offset > 0)
        {
            _x86_64.mc_add_imm_to_reg_x64(dst, offset);
        }
    }
    else
    {
        //
        // We are just adding a memory offset to the existing pointer

        const int src = linear.getRegisterByVariable(ins.left());
        _x86_64.mc_mov_reg_to_reg_x64(dst, src);
        if (ins.memOffset() > 0)
        {
            _x86_64.mc_add_imm_to_reg_x64(dst, ins.memOffset());
        }
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
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_I8:
    case TYPE_U8:
        _x86_64.mc_mov_memory_to_reg_x64(dst, src, 0);
        break;
    case TYPE_F64:
        _x86_64.mc_movsd_memory_to_reg_x64(dst_sse, src, 0);
        break;
    case TYPE_F32:
        _x86_64.mc_movss_memory_to_reg_x64(dst_sse, src, 0);
        break;
    default:
        /* copy pointer value */
        _x86_64.mc_mov_memory_to_reg_x64(dst, src, 0);
        break;
    }
}

void poAsm::ir_store(poRegLinear& linear, const poInstruction& ins)
{
    // We need to mov data from the source register to the destination address

    const int src = linear.getRegisterByVariable(ins.right());
    const int src_sse = src - VM_REGISTER_MAX;

    const int dst = linear.getRegisterByVariable(ins.left());
    const int src_slot = linear.getStackSlotByVariable(ins.right());

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_I8:
    case TYPE_U8:
        _x86_64.mc_mov_reg_to_memory_x64(dst, 0, src);
        break;
    case TYPE_F64:
        _x86_64.mc_movsd_reg_to_memory_x64(dst, src_sse, 0);
        break;
    case TYPE_F32:
        _x86_64.mc_movss_reg_to_memory_x64(dst, src_sse, 0);
        break;
    default:
        /* copy pointer value (stack) */
        if (src_slot != -1)
        {
            _x86_64.mc_mov_reg_to_memory_x64(dst, src_slot * 8, VM_REGISTER_ESP);
        }
        else
        {
            _x86_64.mc_mov_reg_to_memory_x64(dst, 0, src);
        }
        break;
    }

}

void poAsm::ir_zero_extend(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src = linear.getRegisterByVariable(ins.left());

    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension

    switch (srcType)
    {
    case TYPE_U32:
        if (src != VM_REGISTER_EAX) { _x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src); }
        _x86_64.mc_cdqe();
        if (dst != VM_REGISTER_EAX) { _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX); }
        break;
    case TYPE_U16:
        switch (ins.type())
        {
        case TYPE_U64:
            _x86_64.mc_movsx_16_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_U32:
            _x86_64.mc_movsx_16_to_32_reg_to_reg(dst, src);
            break;
        }
        break;
    case TYPE_U8:
        switch (ins.type())
        {
        case TYPE_U64:
            _x86_64.mc_movzx_8_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_U32:
            _x86_64.mc_movzx_8_to_32_reg_to_reg(dst, src);
            break;
        case TYPE_U16:
            _x86_64.mc_movzx_8_to_16_reg_to_reg(dst, src);
            break;
        }
        break;
    }
}

void poAsm::ir_sign_extend(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src = linear.getRegisterByVariable(ins.left());

    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension

    switch (srcType)
    {
    case TYPE_I32:
        _x86_64.mc_movsx_32_to_64_reg_to_reg(dst, src);
        break;
    case TYPE_I16:
        switch (ins.type())
        {
        case TYPE_I64:
            _x86_64.mc_movsx_16_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_I32:
            _x86_64.mc_movsx_16_to_32_reg_to_reg(dst, src);
            break;
        }
        break;
    case TYPE_I8:
        switch (ins.type())
        {
        case TYPE_I64:
            _x86_64.mc_movsx_8_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_I32:
            _x86_64.mc_movsx_8_to_32_reg_to_reg(dst, src);
            break;
        case TYPE_I16:
            _x86_64.mc_movsx_8_to_16_reg_to_reg(dst, src);
            break;
        }
        break;
    }
}

void poAsm::ir_bitwise_cast(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src = linear.getRegisterByVariable(ins.left());

    if (src != dst)
    {
        _x86_64.mc_mov_reg_to_reg_x64(dst, src);
    }
}

void poAsm::ir_convert(poRegLinear& linear, const poInstruction& ins)
{
    const int dst = linear.getRegisterByVariable(ins.name());
    const int src = linear.getRegisterByVariable(ins.left());
    const int dst_sse = dst - VM_REGISTER_MAX;

    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension

    /* First, perform sign extension on signed source types */

    switch (srcType)
    {
    case TYPE_I32:
        _x86_64.mc_movsx_32_to_64_reg_to_reg(src, src);
        break;
    case TYPE_I16:
        _x86_64.mc_movsx_16_to_64_reg_to_reg(src, src);
        break;
    case TYPE_I8:
        _x86_64.mc_movsx_8_to_64_reg_to_reg(src, src);
        break;
    default:
        break;
    }

    /* Then convert to the desired type */

    switch (srcType)
    {
    case TYPE_I64:
    case TYPE_I32:
    case TYPE_I16:
    case TYPE_I8:
        switch (ins.type())
        {
        case TYPE_F64:
            _x86_64.mc_cvtitod_reg_to_reg_x64(dst_sse, src);
            break;
        case TYPE_F32:
            _x86_64.mc_cvtitos_reg_to_reg_x64(dst_sse, src);
            break;
        }
        break;
    //case TYPE_U64: // TODO: needs special handling
    //case TYPE_U32:
    //case TYPE_U16:
    //case TYPE_U8:
    //    switch (ins.type())
    //    {
    //        case TYPE_F64:
    //            _x86_64.mc_cvtitod_reg_to_reg_x64(dst_sse, src);
    //            break;
    //        case TYPE_F32:
    //            _x86_64.mc_cvtitos_reg_to_reg_x64(dst_sse, src);
    //            break;
    //    }
    //    break;
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
        if (dst == src2) { _x86_64.mc_add_reg_to_reg_x64(dst, src1); }
        else if (dst == src1) { _x86_64.mc_add_reg_to_reg_x64(dst, src2); }
        else {
            _x86_64.mc_mov_reg_to_reg_x64(dst, src1);
            _x86_64.mc_add_reg_to_reg_x64(dst, src2);
        }
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst == src2) { _x86_64.mc_add_reg_to_reg_32(dst, src1); }
        else if (dst == src1) { _x86_64.mc_add_reg_to_reg_32(dst, src2); }
        else {
            _x86_64.mc_mov_reg_to_reg_32(dst, src1);
            _x86_64.mc_add_reg_to_reg_32(dst, src2);
        }
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst == src2) { _x86_64.mc_add_reg_to_reg_16(dst, src1); }
        else if (dst == src1) { _x86_64.mc_add_reg_to_reg_16(dst, src2); }
        else {
            _x86_64.mc_mov_reg_to_reg_16(dst, src1);
            _x86_64.mc_add_reg_to_reg_16(dst, src2);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_addss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_addsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1) { _x86_64.mc_mov_reg_to_reg_8(dst, src1); }
        _x86_64.mc_add_reg_to_reg_8(dst, src2);
        break;
    default:
        assert(false);
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
            _x86_64.mc_mov_reg_to_reg_x64(dst, src1);
        }
        _x86_64.mc_sub_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst != src1) {
            _x86_64.mc_mov_reg_to_reg_32(dst, src1);
        }
        _x86_64.mc_sub_reg_to_reg_32(dst, src2);
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst != src1) {
            _x86_64.mc_mov_reg_to_reg_16(dst, src1);
        }
        _x86_64.mc_sub_reg_to_reg_16(dst, src2);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_subss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_subsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1)
        {
            _x86_64.mc_mov_reg_to_reg_8(dst, src1);
        }
        _x86_64.mc_sub_reg_to_reg_8(dst, src2);
        break;
    default:
        assert(false);
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
            _x86_64.mc_mov_reg_to_reg_x64(dst, src1);
        }
        _x86_64.mc_mul_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_U64:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        }
        _x86_64.mc_umul_reg_to_reg_x64(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_I32:
        if (dst != src1) {
            _x86_64.mc_mov_reg_to_reg_32(dst, src1);
        }
        _x86_64.mc_mul_reg_to_reg_32(dst, src2);
        break;
    case TYPE_U32:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        }
        _x86_64.mc_umul_reg_to_reg_32(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_I16:
        if (dst != src1) {
            _x86_64.mc_mov_reg_to_reg_16(dst, src1);
        }
        _x86_64.mc_mul_reg_to_reg_16(dst, src2);
        break;
    case TYPE_U16:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        }
        _x86_64.mc_umul_reg_to_reg_16(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_mulss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_mulsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
        _x86_64.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64.mc_mul_reg_8(src2);
        _x86_64.mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        }
        _x86_64.mc_umul_reg_8(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64.mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        }
        break;
    default:
        assert(false);
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
        _x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_x64(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_div_reg_x64(src2);
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U64:
        _x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_x64(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_udiv_reg_x64(src2);
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I32:
        _x86_64.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_32(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_div_reg_32(src2);
        _x86_64.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U32:
        _x86_64.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_32(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_udiv_reg_32(src2);
        _x86_64.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I16:
        _x86_64.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_16(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_div_reg_16(src2);
        _x86_64.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U16:
        _x86_64.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        _x86_64.mc_mov_imm_to_reg_16(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64.mc_udiv_reg_16(src2);
        _x86_64.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I8:
        _x86_64.mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        _x86_64.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64.mc_div_reg_8(src2);
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        _x86_64.mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        _x86_64.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64.mc_udiv_reg_8(src2);
        _x86_64.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_divss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64.mc_divsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    default:
        assert(false);
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
        _x86_64.mc_cmp_reg_to_reg_x64(src1, src2);
        break;
    case TYPE_I32:
    case TYPE_U32:
        _x86_64.mc_cmp_reg_to_reg_32(src1, src2);
        break;
    case TYPE_I16:
    case TYPE_U16:
        _x86_64.mc_cmp_reg_to_reg_16(src1, src2);
        break;
    case TYPE_U8:
    case TYPE_I8:
        _x86_64.mc_cmp_reg_to_reg_8(src1, src2);
        break;
    case TYPE_F64:
        _x86_64.mc_ucmpd_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    case TYPE_F32:
        _x86_64.mc_ucmps_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    default:
        assert(false);
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
        const int imm = targetAsmBB.getPos() - (int(_x86_64.programData().size()) + 5/*TODO*/);

        ir_jump(ins.left(), imm, ins.type());
    }
    else
    {
        // Insert patch

        const int pos = int(_x86_64.programData().size());

        /* Insert after we have got the position */
        if (ins.left() == IR_JUMP_UNCONDITIONAL)
        {
            _x86_64.mc_jump_unconditional(0);
        }
        else
        {
            _x86_64.mc_jump_equals(0);
        }

        const int size = int(_x86_64.programData().size()) - pos;

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
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src)
        {
            _x86_64.mc_mov_reg_to_reg_x64(dst, src);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src)
        {
            _x86_64.mc_movss_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    case TYPE_F64:
        if (sse_dst != sse_src)
        {
            _x86_64.mc_movsd_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    default:
        assert(false);
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
        _x86_64.mc_mov_imm_to_reg_x64(dst, constants.getI64(ins.constant()));
        break;
    case TYPE_U64:
        _x86_64.mc_mov_imm_to_reg_x64(dst, constants.getU64(ins.constant()));
        break;
    case TYPE_I32:
        _x86_64.mc_mov_imm_to_reg_32(dst, constants.getI32(ins.constant()));
        break;
    case TYPE_U32:
        _x86_64.mc_mov_imm_to_reg_32(dst, int(constants.getU32(ins.constant())));
        break;
    case TYPE_I16:
        _x86_64.mc_mov_imm_to_reg_16(dst, constants.getI16(ins.constant()));
        break;
    case TYPE_U16:
        _x86_64.mc_mov_imm_to_reg_16(dst, char(constants.getU16(ins.constant())));
        break;
    case TYPE_I8:
        _x86_64.mc_mov_imm_to_reg_8(dst, constants.getI8(ins.constant()));
        break;
    case TYPE_U8:
        _x86_64.mc_mov_imm_to_reg_8(dst, char(constants.getU8(ins.constant())));
        break;
    case TYPE_F32:
        _x86_64.mc_movss_memory_to_reg_x64(dstSSE, 0);
        addInitializedData(constants.getF32(ins.constant()), -int(sizeof(int32_t))); // insert patch
        break;
    case TYPE_F64:
        _x86_64.mc_movsd_memory_to_reg_x64(dstSSE, 0);
        addInitializedData(constants.getF64(ins.constant()), -int(sizeof(int32_t))); // insert patch
        break;
    default:
        assert(false);
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
        case TYPE_U64:
        case TYPE_I32:
        case TYPE_U32:
        case TYPE_I16:
        case TYPE_U16:
        case TYPE_I8:
        case TYPE_U8:
            _x86_64.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left));
            break;
        case TYPE_F64:
            _x86_64.mc_movsd_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            _x86_64.mc_movss_reg_to_reg_x64(VM_REGISTER_EAX, linear.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        default:
            assert(false);
            break;
        }
    }

    // Epilogue
    generateEpilogue(linear);

    _x86_64.mc_return();
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
            _x86_64.mc_mov_reg_to_reg_x64(dst, src);
        }
        _x86_64.mc_neg_reg_x64(dst);
        break;
    case TYPE_F64:
        if (dst_sse != src_sse)
        {
            _x86_64.mc_movsd_reg_to_reg_x64(dst_sse, src_sse);
        }
        _x86_64.mc_xorpd_reg_to_reg_x64(dst_sse, dst_sse);
        _x86_64.mc_subsd_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_F32:
        if (dst_sse != src_sse)
        {
            _x86_64.mc_movss_reg_to_reg_x64(dst_sse, src_sse);
        }
        _x86_64.mc_xorps_reg_to_reg_x64(dst_sse, dst_sse);
        _x86_64.mc_subss_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src)
        {
            _x86_64.mc_mov_reg_to_reg_x64(dst, src);
        }
        _x86_64.mc_neg_reg_8(dst);
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst != src)
        {
            _x86_64.mc_mov_reg_to_reg_16(dst, src);
        }
        _x86_64.mc_neg_reg_16(dst);
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst != src)
        {
            _x86_64.mc_mov_reg_to_reg_32(dst, src);
        }
        _x86_64.mc_neg_reg_32(dst);
        break;
    default:
        assert(false);
        break;
    }
}

void poAsm::ir_call(poModule& module, poRegLinear& linear, const poInstruction& ins, const std::vector<poInstruction>& args)
{
    /* TODO: handle calls with > VM_MAX_ARGS */

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
        case TYPE_I16:
        case TYPE_I8:
        case TYPE_U64:
        case TYPE_U32:
        case TYPE_U16:
        case TYPE_U8:
            _x86_64.mc_mov_reg_to_reg_x64(generalArgs[i], linear.getRegisterByVariable(args[i].left()));
            break;
        case TYPE_F64:
            _x86_64.mc_movsd_reg_to_reg_x64(sseArgs[i], linear.getRegisterByVariable(args[i].left()) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            _x86_64.mc_movss_reg_to_reg_x64(sseArgs[i], linear.getRegisterByVariable(args[i].left()) - VM_REGISTER_MAX);
            break;
        default:
            src_slot = linear.getStackSlotByVariable(args[i].left());
            if (src_slot != -1)
            {
                _x86_64.mc_mov_reg_to_reg_x64(generalArgs[i], VM_REGISTER_ESP);
                _x86_64.mc_add_imm_to_reg_x64(generalArgs[i], src_slot * 8);
            }
            else
            {
                _x86_64.mc_mov_reg_to_reg_x64(generalArgs[i], linear.getRegisterByVariable(args[i].left()));
            }
            break;
        }
    }

    const int symbol = ins.right();
    std::string symbolName;
    module.getSymbol(symbol, symbolName);

    // Add patch for this call

    _calls.push_back(poAsmCall(int(_x86_64.programData().size()), numArgs, symbolName));
    _x86_64.mc_call(0);

    switch (ins.type())
    {
    case TYPE_U8:
    case TYPE_I8:
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I64:
    case TYPE_U64:
        _x86_64.mc_mov_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()), VM_REGISTER_EAX);
        break;
    case TYPE_F64:
        _x86_64.mc_movsd_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    case TYPE_F32:
        _x86_64.mc_movss_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    case TYPE_VOID:
        break;
    default:
        _x86_64.mc_mov_reg_to_reg_x64(linear.getRegisterByVariable(ins.name()), VM_REGISTER_EAX);
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
    case TYPE_I16:
    case TYPE_U16:
    case TYPE_I32:
    case TYPE_U32:
    case TYPE_I8:
    case TYPE_U8:
        if (param < VM_MAX_ARGS)
        {
            const int src = generalArgs[param];
            _x86_64.mc_mov_reg_to_reg_x64(dst, src);
        }
        break;
    case TYPE_F64:
        _x86_64.mc_movsd_reg_to_reg_x64(dst_sse, sseArgs[param]);
        break;
    case TYPE_F32:
        _x86_64.mc_movss_reg_to_reg_x64(dst_sse, sseArgs[param]);
        break;
    default:
        _x86_64.mc_mov_reg_to_reg_x64(dst, generalArgs[param]);
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
        _x86_64.mc_jump_unconditional(imm);
        return true;
    }

    switch (type)
    {
    case TYPE_I64:
    case TYPE_I32:
    case TYPE_I16:
    case TYPE_I8:
        switch (jump)
        {
        case IR_JUMP_EQUALS:
            _x86_64.mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            _x86_64.mc_jump_greater(imm);
            break;
        case IR_JUMP_LESS:
            _x86_64.mc_jump_less(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            _x86_64.mc_jump_greater_equal(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            _x86_64.mc_jump_less_equal(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            _x86_64.mc_jump_not_equals(imm);
            break;
        default:
            return false;
        }
        break;
    case TYPE_F64:
    case TYPE_F32:
    case TYPE_U64:
    case TYPE_U32:
    case TYPE_U16:
    case TYPE_U8:
        switch (jump)
        {
        case IR_JUMP_EQUALS:
            _x86_64.mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            _x86_64.mc_jump_not_below_equal(imm);
            break;
        case IR_JUMP_LESS:
            _x86_64.mc_jump_not_above_equal(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            _x86_64.mc_jump_not_below(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            _x86_64.mc_jump_not_above(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            _x86_64.mc_jump_not_equals(imm);
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
    const int pos = int(_x86_64.programData().size());
    const int size = jump.getSize();
    const int imm = pos - (jump.getProgramDataPos() + size);

    if (!ir_jump(jump.getJumpType(), imm, jump.getType()))
    {
        return;
    }

    memcpy(_x86_64.programData().data() + jump.getProgramDataPos(), _x86_64.programData().data() + pos, size);
    _x86_64.programData().resize(_x86_64.programData().size() - size);
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
            _x86_64.mc_push_reg(i);
            numPushed++;
        }
    }

    const int size = 8 * numPushed + 8 * linear.stackSize();
    const int alignment = align(size);
    _prologueSize = alignment + size;
    const int resize = _prologueSize - 8 * numPushed;

    if (resize > 0)
    {
        _x86_64.mc_sub_imm_to_reg_x64(VM_REGISTER_ESP, resize);
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
        _x86_64.mc_add_imm_to_reg_x64(VM_REGISTER_ESP, size);
    }

    for (int i = VM_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (!linear.isVolatile(i) && linear.isRegisterSet(i))
        {
            _x86_64.mc_pop_reg(i);
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
        _x86_64.mc_mov_reg_to_memory_x64(VM_REGISTER_ESP, -8 * (slot + 1), spill.spillRegister());
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
        //_x86_64.mc_mov_memory_to_reg_x64(linear.)
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
        _basicBlocks[_basicBlockMap[bb]].setPos(int(_x86_64.programData().size()));
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
            case IR_SIGN_EXTEND:
                ir_sign_extend(linear, ins);
                break;
            case IR_ZERO_EXTEND:
                ir_zero_extend(linear, ins);
                break;
            case IR_BITWISE_CAST:
                ir_bitwise_cast(linear, ins);
                break;
            case IR_CONVERT:
                ir_convert(linear, ins);
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

void poAsm::generateExternStub(poModule& module, poFlowGraph& cfg)
{
    _x86_64.mc_jump_memory(0); // Placeholder for the extern function call
}

void poAsm::generate(poModule& module)
{
    std::vector<poNamespace>& namespaces = module.namespaces();
    for (poNamespace& ns : namespaces)
    {
        std::vector<poFunction>& functions = ns.functions();
        for (poFunction& function : functions)
        {
            _mapping.insert(std::pair<std::string, int>(function.name(), int(_x86_64.programData().size())));

            if (function.hasAttribute(poAttributes::EXTERN))
            {
                _externCalls.push_back(poAsmExternCall(function.name(), int(_x86_64.programData().size()) + 2 /* get the position of the displacement */));
                _imports.insert(std::pair<std::string, int>(function.name(), 0));
                generateExternStub(module, function.cfg());
            }
            else
            {
                generate(module, function.cfg());
            }
        }
    }

    patchCalls();

    const auto& main = _mapping.find("main");
    if (main != _mapping.end())
    {
        _entryPoint = int(_x86_64.programData().size());
        _x86_64.mc_sub_imm_to_reg_x64(VM_REGISTER_EBP, 8);
        const int mainImm = main->second - int(_x86_64.programData().size());
        _x86_64.mc_call(mainImm);
        _x86_64.mc_add_imm_to_reg_x64(VM_REGISTER_EBP, 8);
        _x86_64.mc_return();
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
    _constants.push_back(poAsmConstant(int(_initializedData.size()), int(_x86_64.programData().size()) + programDataOffset));

    const int32_t data = *reinterpret_cast<const int32_t*>(&f32);
    _initializedData.push_back(data & 0xFF);
    _initializedData.push_back((data >> 8) & 0xFF);
    _initializedData.push_back((data >> 16) & 0xFF);
    _initializedData.push_back((data >> 24) & 0xFF);
}

void poAsm::addInitializedData(const double f64, const int programDataOffset)
{
    _constants.push_back(poAsmConstant(int(_initializedData.size()), int(_x86_64.programData().size() + programDataOffset)));

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
            const int dataPos = int(_x86_64.programData().size());
            _x86_64.mc_call(it->second - pos);
            memcpy(_x86_64.programData().data() + pos, _x86_64.programData().data() + dataPos, size);
            _x86_64.programData().resize(_x86_64.programData().size() - size);
        }
        else
        {
            std::stringstream ss;
            ss << "Unresolved symbol " << symbol;
            setError(ss.str());
        }
    }
}

void poAsm::link(const int programDataPos, const int initializedDataPos)
{
    for (auto& constant : _constants)
    {
        const int disp32 = (constant.getInitializedDataPos() + initializedDataPos) - (programDataPos + constant.getProgramDataPos() + int(sizeof(int32_t)));
        _x86_64.programData()[constant.getProgramDataPos()] = disp32 & 0xFF;
        _x86_64.programData()[constant.getProgramDataPos() + 1] = (disp32 >> 8) & 0xFF;
        _x86_64.programData()[constant.getProgramDataPos() + 2] = (disp32 >> 16) & 0xFF;
        _x86_64.programData()[constant.getProgramDataPos() + 3] = (disp32 >> 24) & 0xFF;
    }

    for (auto& externCall : _externCalls)
    {
        const int disp32 = _imports[externCall.name()] - (externCall.programPos() + programDataPos + int(sizeof(int32_t)));
        _x86_64.programData()[externCall.programPos()] = disp32 & 0xFF;
        _x86_64.programData()[externCall.programPos() + 1] = (disp32 >> 8) & 0xFF;
        _x86_64.programData()[externCall.programPos() + 2] = (disp32 >> 16) & 0xFF;
        _x86_64.programData()[externCall.programPos() + 3] = (disp32 >> 24) & 0xFF;
    }
}

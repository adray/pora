#include "poAsm.h"
#include "poModule.h"
#include "poRegLinear.h"
#include "poRegGraph.h"
#include "poType.h"

#include "poLive.h"
#include "poDom.h"
#include "poAnalyzer.h"

#include <assert.h>
#include <sstream>
#include <iostream>
#include <cstring>

using namespace po;

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

void poAsm::ir_element_ptr(poModule& module, PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    // We need to get the pointer to the variable (left) optionally adding the variable (right) and a memory offset.

    const int dst = allocator.getRegisterByVariable(ins.name());
    const int slot = allocator.getStackSlotByVariable(ins.left()); /* assume the pointer is the stack position */

    const poType& type = module.types()[ins.type()];
    assert(type.isPointer());

    const poType& baseType = module.types()[type.baseType()];
    const int size = baseType.size();

    if (slot != -1)
    {
        const int offset = slot * 8;
        if (ins.right() != -1)
        {
            const int operand = allocator.getRegisterByVariable(ins.right());

            // TODO: change to LEA? (load effective address)
            // ESP + offset + operand * size

            _x86_64_lower.mc_mov_imm_to_reg_x64(dst, size); //emit_unary_instruction(dst, VMI_MOV64_SRC_IMM_DST_REG);
            _x86_64_lower.mc_mul_reg_to_reg_x64(dst, operand);

            _x86_64_lower.mc_add_reg_to_reg_x64(dst, VM_REGISTER_ESP);
            _x86_64_lower.mc_add_imm_to_reg_x64(dst, offset);
        }
        else
        {
            // ESP + offset

            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
            _x86_64_lower.mc_add_imm_to_reg_x64(dst, offset);
        }
    }
    else
    {
        if (ins.right() != -1)
        {
            const int operand = allocator.getRegisterByVariable(ins.right());
            const int left = allocator.getRegisterByVariable(ins.left());

            // TODO: change to LEA? (load effective address)
            // left + operand * size

            _x86_64_lower.mc_mov_imm_to_reg_x64(dst, size);
            _x86_64_lower.mc_mul_reg_to_reg_x64(dst, operand);

            _x86_64_lower.mc_add_reg_to_reg_x64(dst, left);
        }
        else
        {
            const int left = allocator.getRegisterByVariable(ins.left());
            _x86_64_lower.mc_mov_imm_to_reg_x64(dst, size);
            _x86_64_lower.mc_add_reg_to_reg_x64(dst, left);
        }
    }
}

void poAsm::ir_ptr(poModule& module, PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const poType& type = module.types()[ins.type()];
    const int slot = allocator.getStackSlotByVariable(ins.left()); /* assume the pointer is the stack position */
    const int src = allocator.getRegisterByVariable(ins.left());
    if (src != -1)
    {
        //
        // We are just adding a memory offset to the existing pointer

        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src);
        if (ins.memOffset() > 0)
        {
            _x86_64_lower.mc_add_imm_to_reg_x64(dst, ins.memOffset());
        }
    }
    else if (slot != -1)
    {
        // We need to get the pointer to the variable (left) optionally adding the variable (right) and the memory offset.

        const int offset = slot * 8 + ins.memOffset();
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_ESP);
        _x86_64_lower.mc_add_imm_to_reg_x64(dst, offset);
    }
}

void poAsm::ir_load(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    // We need to mov data from the ptr address to the destination register

    const int dst = allocator.getRegisterByVariable(ins.name());
    const int dst_sse = allocator.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX;

    const int src = allocator.getRegisterByVariable(ins.left());

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
        _x86_64_lower.mc_mov_memory_to_reg_x64(dst, src, 0);
        break;
    case TYPE_F64:
        _x86_64_lower.mc_movsd_memory_to_reg_x64(dst_sse, src, 0);
        break;
    case TYPE_F32:
        _x86_64_lower.mc_movss_memory_to_reg_x64(dst_sse, src, 0);
        break;
    default:
        /* copy pointer value */
        assert(dst != -1 && src != -1);
        _x86_64_lower.mc_mov_memory_to_reg_x64(dst, src, 0);
        break;
    }
}

void poAsm::ir_store(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    // We need to mov data from the source register to the destination address

    const int src = allocator.getRegisterByVariable(ins.right());
    const int src_sse = src - VM_REGISTER_MAX;

    const int dst = allocator.getRegisterByVariable(ins.left());
    const int src_slot = allocator.getStackSlotByVariable(ins.right());

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        _x86_64_lower.mc_mov_reg_to_memory_x64(dst, 0, src);
        break;
    case TYPE_I32:
    case TYPE_U32:
        _x86_64_lower.mc_mov_reg_to_mem_32(dst, src, 0);
        break;
    case TYPE_I16:
    case TYPE_U16:
        _x86_64_lower.mc_mov_reg_to_mem_16(dst, src, 0);
        break;
    case TYPE_I8:
    case TYPE_U8:
        _x86_64_lower.mc_mov_reg_to_memory_8(dst, src, 0);
        break;
    case TYPE_F64:
        _x86_64_lower.mc_movsd_reg_to_memory_x64(dst, src_sse, 0);
        break;
    case TYPE_F32:
        _x86_64_lower.mc_movss_reg_to_memory_x64(dst, src_sse, 0);
        break;
    default:
        /* copy pointer value (stack) */
        if (src_slot != -1)
        {
            assert(dst != -1);
            _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, VM_REGISTER_ESP);
            _x86_64_lower.mc_add_imm_to_reg_x64(VM_REGISTER_EAX, src_slot * 8);
            _x86_64_lower.mc_mov_reg_to_memory_x64(dst, 0, VM_REGISTER_EAX);

            // originally was:
            //_x86_64_lower.mc_mov_reg_to_memory_x64(dst, src_slot * 8, VM_REGISTER_ESP);
        }
        else
        {
            assert(src != -1 && dst != -1);
            _x86_64_lower.mc_mov_reg_to_memory_x64(dst, 0, src);
        }
        break;
    }

}

void poAsm::ir_zero_extend(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src = allocator.getRegisterByVariable(ins.left());

    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension

    switch (srcType)
    {
    case TYPE_U32:
        if (src != VM_REGISTER_EAX) { _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src); }
        _x86_64_lower.mc_cdqe();
        if (dst != VM_REGISTER_EAX) { _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX); }
        break;
    case TYPE_U16:
        switch (ins.type())
        {
        case TYPE_U64:
            _x86_64_lower.mc_movsx_16_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_U32:
            _x86_64_lower.mc_movsx_16_to_32_reg_to_reg(dst, src);
            break;
        }
        break;
    case TYPE_U8:
        switch (ins.type())
        {
        case TYPE_U64:
            _x86_64_lower.mc_movzx_8_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_U32:
            _x86_64_lower.mc_movzx_8_to_32_reg_to_reg(dst, src);
            break;
        case TYPE_U16:
            _x86_64_lower.mc_movzx_8_to_16_reg_to_reg(dst, src);
            break;
        }
        break;
    }
}

void poAsm::ir_sign_extend(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src = allocator.getRegisterByVariable(ins.left());

    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension

    switch (srcType)
    {
    case TYPE_I32:
    case TYPE_U32:
        _x86_64_lower.mc_movsx_32_to_64_reg_to_reg(dst, src);
        break;
    case TYPE_I16:
    case TYPE_U16:
        switch (ins.type())
        {
        case TYPE_I64:
            _x86_64_lower.mc_movsx_16_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_I32:
            _x86_64_lower.mc_movsx_16_to_32_reg_to_reg(dst, src);
            break;
        }
        break;
    case TYPE_U8:
    case TYPE_I8:
        switch (ins.type())
        {
        case TYPE_I64:
            _x86_64_lower.mc_movsx_8_to_64_reg_to_reg(dst, src);
            break;
        case TYPE_I32:
            _x86_64_lower.mc_movsx_8_to_32_reg_to_reg(dst, src);
            break;
        case TYPE_I16:
            _x86_64_lower.mc_movsx_8_to_16_reg_to_reg(dst, src);
            break;
        }
        break;
    default:
        setError("Internal Error: malformed sign extend instruction.");
        break;
    }
}

void poAsm::ir_bitwise_cast(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src = allocator.getRegisterByVariable(ins.left());

    if (dst == -1 || src == -1)
    {
        setError("Internal Error: malformed bitwise cast instruction.");
        return;
    }

    if (src != dst)
    {
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src);
    }
}

void poAsm::ir_convert(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int srcType = ins.memOffset(); // Using memOffset to store source type for sign extension
    
    if (srcType == TYPE_F32 ||
        srcType == TYPE_F64)
    {
        const int dst = allocator.getRegisterByVariable(ins.name());
        const int src = allocator.getRegisterByVariable(ins.left());
        const int src_sse = src - VM_REGISTER_MAX;

        switch (ins.type())
        {
        case TYPE_I64:
        case TYPE_I32:
        case TYPE_I16:
        case TYPE_I8:
            switch (srcType)
            {
            case TYPE_F64:
                _x86_64_lower.mc_cvtsdsi_reg_to_reg_x64(dst, src_sse);
                break;
            case TYPE_F32:
                _x86_64_lower.mc_cvtsssi_reg_to_reg_x64(dst, src_sse);
                break;
            }
            break;
        }

        // TODO: handle unsigned conversion from float (needs special handling)
        // TODO: handle float to float conversions (f32 to f64 and f64 to f32)?
    }
    else
    {
        const int dst = allocator.getRegisterByVariable(ins.name());
        const int src = allocator.getRegisterByVariable(ins.left());
        const int dst_sse = dst - VM_REGISTER_MAX;


        /* First, perform sign extension on signed source types */

        switch (srcType)
        {
        case TYPE_I32:
            _x86_64_lower.mc_movsx_32_to_64_reg_to_reg(src, src);
            break;
        case TYPE_I16:
            _x86_64_lower.mc_movsx_16_to_64_reg_to_reg(src, src);
            break;
        case TYPE_I8:
            _x86_64_lower.mc_movsx_8_to_64_reg_to_reg(src, src);
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
                _x86_64_lower.mc_cvtitod_reg_to_reg_x64(dst_sse, src);
                break;
            case TYPE_F32:
                _x86_64_lower.mc_cvtitos_reg_to_reg_x64(dst_sse, src);
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
}

void poAsm::ir_add(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src1 = allocator.getRegisterByVariable(ins.left());
    const int src2 = allocator.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst == src2) { _x86_64_lower.mc_add_reg_to_reg_x64(dst, src1); }
        else if (dst == src1) { _x86_64_lower.mc_add_reg_to_reg_x64(dst, src2); }
        else {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src1);
            _x86_64_lower.mc_add_reg_to_reg_x64(dst, src2);
        }
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst == src2) { _x86_64_lower.mc_add_reg_to_reg_32(dst, src1); }
        else if (dst == src1) { _x86_64_lower.mc_add_reg_to_reg_32(dst, src2); }
        else {
            _x86_64_lower.mc_mov_reg_to_reg_32(dst, src1);
            _x86_64_lower.mc_add_reg_to_reg_32(dst, src2);
        }
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst == src2) { _x86_64_lower.mc_add_reg_to_reg_16(dst, src1); }
        else if (dst == src1) { _x86_64_lower.mc_add_reg_to_reg_16(dst, src2); }
        else {
            _x86_64_lower.mc_mov_reg_to_reg_16(dst, src1);
            _x86_64_lower.mc_add_reg_to_reg_16(dst, src2);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_addss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_addsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1) { _x86_64_lower.mc_mov_reg_to_reg_8(dst, src1); }
        _x86_64_lower.mc_add_reg_to_reg_8(dst, src2);
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed add instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_sub(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src1 = allocator.getRegisterByVariable(ins.left());
    const int src2 = allocator.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src1);
        }
        _x86_64_lower.mc_sub_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_32(dst, src1);
        }
        _x86_64_lower.mc_sub_reg_to_reg_32(dst, src2);
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_16(dst, src1);
        }
        _x86_64_lower.mc_sub_reg_to_reg_16(dst, src2);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_subss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_subsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src1)
        {
            _x86_64_lower.mc_mov_reg_to_reg_8(dst, src1);
        }
        _x86_64_lower.mc_sub_reg_to_reg_8(dst, src2);
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed sub instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_mul(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src1 = allocator.getRegisterByVariable(ins.left());
    const int src2 = allocator.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src1);
        }
        _x86_64_lower.mc_mul_reg_to_reg_x64(dst, src2);
        break;
    case TYPE_U64:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        }
        _x86_64_lower.mc_umul_reg_to_reg_x64(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_I32:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_32(dst, src1);
        }
        _x86_64_lower.mc_mul_reg_to_reg_32(dst, src2);
        break;
    case TYPE_U32:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        }
        _x86_64_lower.mc_umul_reg_to_reg_32(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_I16:
        if (dst != src1) {
            _x86_64_lower.mc_mov_reg_to_reg_16(dst, src1);
        }
        _x86_64_lower.mc_mul_reg_to_reg_16(dst, src2);
        break;
    case TYPE_U16:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        }
        _x86_64_lower.mc_umul_reg_to_reg_16(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_mulss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_mulsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_I8:
        _x86_64_lower.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mul_reg_8(src2);
        _x86_64_lower.mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        if (src1 != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        }
        _x86_64_lower.mc_umul_reg_8(src2);
        if (dst != VM_REGISTER_EAX) {
            _x86_64_lower.mc_mov_reg_to_reg_8(dst, VM_REGISTER_EAX);
        }
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed mul instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_div(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src1 = allocator.getRegisterByVariable(ins.left());
    const int src2 = allocator.getRegisterByVariable(ins.right());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
        _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_x64(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_div_reg_x64(src2);
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U64:
        _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_x64(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_udiv_reg_x64(src2);
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I32:
        _x86_64_lower.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_32(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_div_reg_32(src2);
        _x86_64_lower.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U32:
        _x86_64_lower.mc_mov_reg_to_reg_32(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_32(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_udiv_reg_32(src2);
        _x86_64_lower.mc_mov_reg_to_reg_32(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I16:
        _x86_64_lower.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_16(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_div_reg_16(src2);
        _x86_64_lower.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U16:
        _x86_64_lower.mc_mov_reg_to_reg_16(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_mov_imm_to_reg_16(VM_REGISTER_EDX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_udiv_reg_16(src2);
        _x86_64_lower.mc_mov_reg_to_reg_16(dst, VM_REGISTER_EAX);
        break;
    case TYPE_I8:
        _x86_64_lower.mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_div_reg_8(src2);
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_U8:
        _x86_64_lower.mc_mov_imm_to_reg_x64(VM_REGISTER_EAX, 0); // TODO: replace with XOR?
        _x86_64_lower.mc_mov_reg_to_reg_8(VM_REGISTER_EAX, src1);
        _x86_64_lower.mc_udiv_reg_8(src2);
        _x86_64_lower.mc_mov_reg_to_reg_x64(dst, VM_REGISTER_EAX);
        break;
    case TYPE_F32:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movss_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_divss_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    case TYPE_F64:
        if (sse_dst != sse_src1) { _x86_64_lower.mc_movsd_reg_to_reg_x64(sse_dst, sse_src1); }
        _x86_64_lower.mc_divsd_reg_to_reg_x64(sse_dst, sse_src2);
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed div instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_cmp(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int src1 = allocator.getRegisterByVariable(ins.left());
    const int src2 = allocator.getRegisterByVariable(ins.right());

    const int sse_src1 = src1 - VM_REGISTER_MAX;
    const int sse_src2 = src2 - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        _x86_64_lower.mc_cmp_reg_to_reg_x64(src1, src2);
        break;
    case TYPE_I32:
    case TYPE_U32:
        _x86_64_lower.mc_cmp_reg_to_reg_32(src1, src2);
        break;
    case TYPE_I16:
    case TYPE_U16:
        _x86_64_lower.mc_cmp_reg_to_reg_16(src1, src2);
        break;
    case TYPE_U8:
    case TYPE_I8:
        _x86_64_lower.mc_cmp_reg_to_reg_8(src1, src2);
        break;
    case TYPE_F64:
        _x86_64_lower.mc_ucmpd_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    case TYPE_F32:
        _x86_64_lower.mc_ucmps_reg_to_reg_x64(sse_src1, sse_src2);
        break;
    default:
        //_x86_64_lower.mc_cmp_reg_to_reg_x64(src1, src2);
        std::stringstream ss;
        ss << "Internal Error: Malformed compare instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_br(PO_ALLOCATOR& allocator, const poInstruction& ins, poBasicBlock* bb)
{
    po_x86_64_basic_block* abb = _basicBlockMap[bb];
    if (bb->getBranch())
    {
        abb->setJumpTarget(_basicBlockMap[bb->getBranch()]);
        abb->jumpBlock()->incomingBlocks().push_back(abb);
    }

    // Generate the jump instruction, the immediate value is 0 as we will patch it later
    ir_jump(ins.left(), 0, ins.type());
}

void poAsm::ir_copy(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int src = allocator.getRegisterByVariable(ins.left());

    const int sse_dst = dst - VM_REGISTER_MAX;
    const int sse_src = src - VM_REGISTER_MAX;
    
    if (dst < 0 || src < 0)
    {
        std::stringstream ss;
        ss << "Internal Error: Malformed copy instruction " << ins.name();
        setError(ss.str());
        return;
    }

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
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src);
        }
        break;
    case TYPE_F32:
        if (sse_dst != sse_src)
        {
            _x86_64_lower.mc_movss_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    case TYPE_F64:
        if (sse_dst != sse_src)
        {
            _x86_64_lower.mc_movsd_reg_to_reg_x64(sse_dst, sse_src);
        }
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed copy instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_constant(poModule& module, poConstantPool& constants, PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
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
        _x86_64_lower.mc_mov_imm_to_reg_x64(dst, constants.getI64(ins.constant()));
        break;
    case TYPE_U64:
        _x86_64_lower.mc_mov_imm_to_reg_x64(dst, constants.getU64(ins.constant()));
        break;
    case TYPE_I32:
        _x86_64_lower.mc_mov_imm_to_reg_32(dst, constants.getI32(ins.constant()));
        break;
    case TYPE_U32:
        _x86_64_lower.mc_mov_imm_to_reg_32(dst, int(constants.getU32(ins.constant())));
        break;
    case TYPE_I16:
        _x86_64_lower.mc_mov_imm_to_reg_16(dst, constants.getI16(ins.constant()));
        break;
    case TYPE_U16:
        _x86_64_lower.mc_mov_imm_to_reg_16(dst, char(constants.getU16(ins.constant())));
        break;
    case TYPE_I8:
        _x86_64_lower.mc_mov_imm_to_reg_8(dst, constants.getI8(ins.constant()));
        break;
    case TYPE_U8:
        _x86_64_lower.mc_mov_imm_to_reg_8(dst, char(constants.getU8(ins.constant())));
        break;
    case TYPE_F32:
        _x86_64_lower.mc_movss_memory_to_reg_x64(dstSSE, 0);
        _x86_64_lower.cfg().getLast()->instructions().back().setId(ins.constant());
        break;
    case TYPE_F64:
        _x86_64_lower.mc_movsd_memory_to_reg_x64(dstSSE, 0);
        _x86_64_lower.cfg().getLast()->instructions().back().setId(ins.constant());
        break;
    default:
        {
            poType& type = module.types()[ins.type()];
            if (type.isPointer())
            {
                _x86_64_lower.mc_lea_reg_to_reg_x64(dst, 0);
                _x86_64_lower.cfg().getLast()->instructions().back().setId(ins.constant());
                return;
            }
        }

        std::stringstream ss;
        ss << "Internal Error: Malformed constant instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_ret(poModule& module, PO_ALLOCATOR& allocator, const poInstruction& ins)
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
            _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, allocator.getRegisterByVariable(left));
            break;
        case TYPE_F64:
            _x86_64_lower.mc_movsd_reg_to_reg_x64(VM_REGISTER_EAX, allocator.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            _x86_64_lower.mc_movss_reg_to_reg_x64(VM_REGISTER_EAX, allocator.getRegisterByVariable(left) - VM_REGISTER_MAX);
            break;
        default:
            if (left != -1)
            {
                poType& type = module.types()[ins.type()];
                if (type.isPointer())
                {
                    _x86_64_lower.mc_mov_reg_to_reg_x64(VM_REGISTER_EAX, allocator.getRegisterByVariable(left));
                    break;
                }
            }
            std::stringstream ss;
            ss << "Internal Error: Malformed return instruction " << ins.name();
            setError(ss.str());
            break;
        }
    }

    // Epilogue
    generateEpilogue(allocator);

    _x86_64_lower.mc_return();
}

void poAsm::ir_unary_minus(PO_ALLOCATOR& allocator, const poInstruction& ins)
{
    const int src = allocator.getRegisterByVariable(ins.left());
    const int dst = allocator.getRegisterByVariable(ins.name());

    const int src_sse = src - VM_REGISTER_MAX;
    const int dst_sse = dst - VM_REGISTER_MAX;

    switch (ins.type())
    {
    case TYPE_I64:
    case TYPE_U64:
        if (dst != src)
        {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src);
        }
        _x86_64_lower.mc_neg_reg_x64(dst);
        break;
    case TYPE_F64:
        if (dst_sse != src_sse)
        {
            _x86_64_lower.mc_movsd_reg_to_reg_x64(dst_sse, src_sse);
        }
        _x86_64_lower.mc_xorpd_reg_to_reg_x64(dst_sse, dst_sse);
        _x86_64_lower.mc_subsd_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_F32:
        if (dst_sse != src_sse)
        {
            _x86_64_lower.mc_movss_reg_to_reg_x64(dst_sse, src_sse);
        }
        _x86_64_lower.mc_xorps_reg_to_reg_x64(dst_sse, dst_sse);
        _x86_64_lower.mc_subss_reg_to_reg_x64(dst_sse, src_sse);
        break;
    case TYPE_I8:
    case TYPE_U8:
        if (dst != src)
        {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, src);
        }
        _x86_64_lower.mc_neg_reg_8(dst);
        break;
    case TYPE_I16:
    case TYPE_U16:
        if (dst != src)
        {
            _x86_64_lower.mc_mov_reg_to_reg_16(dst, src);
        }
        _x86_64_lower.mc_neg_reg_16(dst);
        break;
    case TYPE_I32:
    case TYPE_U32:
        if (dst != src)
        {
            _x86_64_lower.mc_mov_reg_to_reg_32(dst, src);
        }
        _x86_64_lower.mc_neg_reg_32(dst);
        break;
    default:
        std::stringstream ss;
        ss << "Internal Error: Malformed unary minus instruction " << ins.name();
        setError(ss.str());
        break;
    }
}

void poAsm::ir_call(poModule& module, PO_ALLOCATOR& allocator, const poInstruction& ins, const int pos, const std::vector<poInstruction>& args)
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
        int reg = -1;

        const int argPos = pos + i + 1;

#ifdef REG_LINEAR
        spill(allocator, argPos);
#endif
        restore(allocator, argPos);

        switch (args[i].type()) {
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
                _x86_64_lower.mc_mov_reg_to_memory_x64(VM_REGISTER_ESP, 0, allocator.getRegisterByVariable(args[i].left(), argPos));
                continue;
            }
            _x86_64_lower.mc_mov_reg_to_reg_x64(generalArgs[i], allocator.getRegisterByVariable(args[i].left(), argPos));
            break;
        case TYPE_F64:
            if (i >= VM_MAX_SSE_ARGS) {
                _x86_64_lower.mc_movsd_reg_to_memory_x64(VM_REGISTER_ESP, allocator.getRegisterByVariable(args[i].left(), argPos) - VM_REGISTER_MAX, 0);
                continue;
            }
            _x86_64_lower.mc_movsd_reg_to_reg_x64(sseArgs[i], allocator.getRegisterByVariable(args[i].left(), argPos) - VM_REGISTER_MAX);
            break;
        case TYPE_F32:
            if (i >= VM_MAX_SSE_ARGS)
            {
                _x86_64_lower.mc_movss_reg_to_memory_x64(VM_REGISTER_ESP, allocator.getRegisterByVariable(args[i].left(), argPos) - VM_REGISTER_MAX, 0);
                continue;
            }
            _x86_64_lower.mc_movss_reg_to_reg_x64(sseArgs[i], allocator.getRegisterByVariable(args[i].left(), argPos) - VM_REGISTER_MAX);
            break;
        default:
            if (i >= VM_MAX_ARGS) {
                _x86_64_lower.mc_mov_reg_to_memory_x64(VM_REGISTER_ESP, 0, allocator.getRegisterByVariable(args[i].left(), argPos));
                continue;
            }

            reg = allocator.getRegisterByVariable(args[i].left(), argPos);
            if (reg != -1)
            {
                _x86_64_lower.mc_mov_reg_to_reg_x64(generalArgs[i], reg);
            }
            else
            {
                src_slot = allocator.getStackSlotByVariable(args[i].left());
                assert(src_slot != -1);
                _x86_64_lower.mc_mov_reg_to_reg_x64(generalArgs[i], VM_REGISTER_ESP);
                _x86_64_lower.mc_add_imm_to_reg_x64(generalArgs[i], src_slot * 8);
            }

            break;
        }

#ifdef REG_GRAPH
        spill(allocator, argPos);
#endif
    }

    const int symbol = ins.right();

    _x86_64_lower.mc_call(0);
    _x86_64_lower.cfg().getLast()->instructions().back().setId(symbol);

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
        _x86_64_lower.mc_mov_reg_to_reg_x64(allocator.getRegisterByVariable(ins.name()), VM_REGISTER_EAX);
        break;
    case TYPE_F64:
        _x86_64_lower.mc_movsd_reg_to_reg_x64(allocator.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    case TYPE_F32:
        _x86_64_lower.mc_movss_reg_to_reg_x64(allocator.getRegisterByVariable(ins.name()) - VM_REGISTER_MAX, VM_SSE_REGISTER_XMM0);
        break;
    case TYPE_VOID:
        break;
    default:
        _x86_64_lower.mc_mov_reg_to_reg_x64(allocator.getRegisterByVariable(ins.name()), VM_REGISTER_EAX);
        break;
    }
}

static int inline getArgOffset(const int argIndex, const int prologueSize)
{
    // Note the size of the register homes of the previous stack frame will be added later by the call site analyzer

    return 8 /*return address*/ + (argIndex - VM_MAX_ARGS) * 8 + prologueSize;
}

void poAsm::ir_param(poModule& module, PO_ALLOCATOR& allocator, const poInstruction& ins, const int numArgs)
{
    const int dst = allocator.getRegisterByVariable(ins.name());
    const int dst_sse = dst - VM_REGISTER_MAX;
    static const int generalArgs[] = { VM_ARG1, VM_ARG2, VM_ARG3, VM_ARG4, VM_ARG5, VM_ARG6 };
    static const int sseArgs[] = { VM_SSE_ARG1, VM_SSE_ARG2, VM_SSE_ARG3, VM_SSE_ARG4, VM_SSE_ARG5, VM_SSE_ARG6, VM_SSE_ARG7, VM_SSE_ARG8 };
    const int param = ins.left();
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
        if (param < VM_MAX_ARGS)
        {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, generalArgs[param]);
        }
        else
        {
            _x86_64_lower.mc_mov_memory_to_reg_x64(dst, VM_REGISTER_ESP, getArgOffset(param, _prologueSize));
        }
        break;
    case TYPE_F64:
        if (param < VM_MAX_SSE_ARGS)
        {
            _x86_64_lower.mc_movsd_reg_to_reg_x64(dst_sse, sseArgs[param]);
        }
        else
        {
            _x86_64_lower.mc_movsd_memory_to_reg_x64(dst_sse, VM_REGISTER_ESP, getArgOffset(param, _prologueSize));
        }
        break;
    case TYPE_F32:
        if (param < VM_MAX_SSE_ARGS)
        {
            _x86_64_lower.mc_movss_reg_to_reg_x64(dst_sse, sseArgs[param]);
        }
        else
        {
            _x86_64_lower.mc_movss_memory_to_reg_x64(dst_sse, VM_REGISTER_ESP, getArgOffset(param, _prologueSize));
        }
        break;
    default:
        if (param < VM_MAX_ARGS)
        {
            _x86_64_lower.mc_mov_reg_to_reg_x64(dst, generalArgs[param]);
        }
        else
        {
            _x86_64_lower.mc_mov_memory_to_reg_x64(dst, VM_REGISTER_ESP, getArgOffset(param, _prologueSize));
        }
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
        _x86_64_lower.mc_jump_unconditional(imm);
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
            _x86_64_lower.mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            _x86_64_lower.mc_jump_greater(imm);
            break;
        case IR_JUMP_LESS:
            _x86_64_lower.mc_jump_less(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            _x86_64_lower.mc_jump_greater_equal(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            _x86_64_lower.mc_jump_less_equal(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            _x86_64_lower.mc_jump_not_equals(imm);
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
            _x86_64_lower.mc_jump_equals(imm);
            break;
        case IR_JUMP_GREATER:
            _x86_64_lower.mc_jump_not_below_equal(imm);
            break;
        case IR_JUMP_LESS:
            _x86_64_lower.mc_jump_not_above_equal(imm);
            break;
        case IR_JUMP_GREATER_EQUALS:
            _x86_64_lower.mc_jump_not_below(imm);
            break;
        case IR_JUMP_LESS_EQUALS:
            _x86_64_lower.mc_jump_not_above(imm);
            break;
        case IR_JUMP_NOT_EQUALS:
            _x86_64_lower.mc_jump_not_equals(imm);
            break;
        default:
            return false;
        }
        break;
    }

    return true;
}

void poAsm::patchJump(po_x86_64_basic_block* jump)
{
    const int pos = int(_x86_64.programData().size());
    const int size = jump->jump().getSize();
    const int imm = pos - (jump->jump().getProgramDataPos() + size);

    _x86_64.emit_jump(jump->jump().getJumpType(), imm);

    std::memcpy(_x86_64.programData().data() + jump->jump().getProgramDataPos(), _x86_64.programData().data() + pos, size);
    _x86_64.programData().resize(_x86_64.programData().size() - size);
}

void poAsm::patchForwardJumps(po_x86_64_basic_block* bb)
{
    auto& incoming = bb->incomingBlocks();
    for (po_x86_64_basic_block* inc : incoming)
    {
        auto& jump = inc->jump();
        if (jump.getProgramDataPos() != -1 &&
            !jump.isPatched() &&
            jump.getBasicBlock() == bb)
        {
            jump.setPatched(true);
            patchJump(inc);
        }
    }
}

void poAsm::scanBasicBlocks(poFlowGraph& cfg)
{
    _basicBlockMap.clear();

    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        po_x86_64_basic_block* asmBB = new po_x86_64_basic_block();
        _basicBlockMap.insert(std::pair<poBasicBlock*, po_x86_64_basic_block*>(bb, asmBB));

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

void poAsm::generatePrologue(PO_ALLOCATOR& allocator)
{
    // Push all non-volatile general purpose registers which are used
    int numPushed = 0;
    for (int i = 0; i < VM_REGISTER_MAX; i++)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            _x86_64_lower.mc_push_reg(i);
            numPushed++;
        }
    }

    int sseRegisters = 0;
    for (int i = VM_REGISTER_MAX; i < VM_REGISTER_MAX + VM_SSE_REGISTER_MAX; i++)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            sseRegisters++;
        }
    }

    const int size = 8 * numPushed + 8 * allocator.stackSize() + 32 /* register homes */ + sseRegisters * 8;
    const int alignment = align(size);
    _prologueSize = alignment + size;
    const int resize = _prologueSize - 8 * numPushed;

    if (resize > 0)
    {
        _x86_64_lower.mc_sub_imm_to_reg_x64(VM_REGISTER_ESP, resize);
    }

    int count = 0;
    for (int i = VM_REGISTER_MAX; i < VM_REGISTER_MAX + VM_SSE_REGISTER_MAX; i++)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            _x86_64_lower.mc_movsd_reg_to_memory_x64(VM_REGISTER_ESP, i - VM_REGISTER_MAX, resize - 8 * count - 8 - 32/*register homes*/);
            count++;
        }
    }
}

void poAsm::emitJump(po_x86_64_basic_block* bb)
{
    po_x86_64_basic_block* targetBB =  bb->jumpBlock();

    // Check if the targetBB has been visited already, is which case we know its position

    if (targetBB->programDataPos() != -1)
    {
        const int imm = targetBB->programDataPos() - (int(_x86_64.programData().size()) + 5/*TODO*/);

        _x86_64.emit_jump(bb->instructions().back().opcode(), imm);
    }
    else
    {
        // Insert patch

        const int pos = int(_x86_64.programData().size());
        const int jumpType = bb->instructions().back().opcode();

        /* Insert after we have got the position */
        if (jumpType == VMI_J32)
        {
            _x86_64.mc_jump_unconditional(0);
        }
        else
        {
            _x86_64.mc_jump_equals(0);
        }

        const int size = int(_x86_64.programData().size()) - pos;

        auto& jump = bb->jump();
        jump.setBasicBlock(targetBB);
        jump.setJumpType(jumpType);
        jump.setSize(size);
        jump.setProgramDataPos(pos);
    }
}

void poAsm::generateEpilogue(PO_ALLOCATOR& allocator)
{
    // Calculate how many non-volatile registers we need to pop
    int numToPop = 0;
    for (int i = VM_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            numToPop++;
        }
    }

    const int size = _prologueSize - 8 * numToPop;

    int count = 0;
    for (int i = VM_REGISTER_MAX; i < VM_REGISTER_MAX + VM_SSE_REGISTER_MAX; i++)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            _x86_64_lower.mc_movsd_memory_to_reg_x64(i - VM_REGISTER_MAX, VM_REGISTER_ESP, size - 8 * count - 8 - 32/*register homes*/);
            count++;
        }
    }

    if (size > 0)
    {
        _x86_64_lower.mc_add_imm_to_reg_x64(VM_REGISTER_ESP, size);
    }

    // Pop all non-volatile general purpose registers which are used (in backwards order)
    for (int i = VM_REGISTER_MAX - 1; i >= 0; i--)
    {
        if (!allocator.isVolatile(i) && allocator.isRegisterSet(i))
        {
            _x86_64_lower.mc_pop_reg(i);
        }
    }
}

void poAsm::dump(const PO_ALLOCATOR& allocator, poRegLinearIterator& iterator, poFlowGraph& cfg)
{
    std::cout << "###############" << std::endl;
    std::cout << "Stack Size: " << allocator.stackSize() << std::endl;

    poBasicBlock* bb = cfg.getFirst();
    int index = 0;
    int pos = 0;
    while (bb)
    {
       std::cout << "Basic Block " << index << ":" << std::endl;

       for (int i = 0; i < bb->numInstructions(); i++)
       {
           poRegSpill spill;
           int spillPos = 0;
           while (allocator.spillAt(pos, spillPos++, &spill))
           {
               std::cout << "SPILL r" << spill.spillRegister() << ":s" << spill.spillStackSlot() << " var:" << spill.spillVariable() << std::endl;
           }

           poRegRestore restore;
           int restorePos = 0;
           while (allocator.restoreAt(pos, restorePos++, &restore))
           {
               std::cout << "RESTORE r" << restore.restoreRegister() << ":s" << restore.restoreStackSlot() << " var:" << restore.restoreVariable() << std::endl;
           }

           const poInstruction& ins = bb->getInstruction(i);
           int reg = allocator.getRegisterByVariable(ins.name());
           if (reg != -1)
           {
               std::cout << ins.name() << " r" << reg << " var:" << ins.name() << std::endl;
           }
           else
           {
               std::cout << ins.name() << std::endl;
           }
           pos++;
           iterator.next();
       }

        bb = bb->getNext();
        index++;
    }

    std::cout << "###############" << std::endl;
}

void poAsm::spill(PO_ALLOCATOR& allocator, const int pos)
{
    poRegSpill spill;
    int spillPos = 0;
    while (allocator.spillAt(pos, spillPos++, &spill))
    {
        const int slot = spill.spillStackSlot();
        if (spill.spillRegister() < VM_REGISTER_MAX)
        {
            _x86_64_lower.mc_mov_reg_to_memory_x64(VM_REGISTER_ESP, 8 * slot, spill.spillRegister());
        }
        else
        {
            _x86_64_lower.mc_movsd_reg_to_memory_x64(VM_REGISTER_ESP, spill.spillRegister() - VM_REGISTER_MAX, slot * 8);
        }
    }
}

void poAsm::restore(PO_ALLOCATOR& allocator, const int pos)
{
    /* if a variable we want to use has been spilled, we need to restore it */

    poRegRestore restore;
    int restorePos = 0;
    while (allocator.restoreAt(pos, restorePos++, &restore))
    {
        const int restoreSlot = restore.restoreStackSlot();
        if (restoreSlot != -1)
        {
            if (restore.restoreRegister() < VM_REGISTER_MAX)
            {
                _x86_64_lower.mc_mov_memory_to_reg_x64(restore.restoreRegister(), VM_REGISTER_ESP, restoreSlot * 8);
            }
            else
            {
                _x86_64_lower.mc_movsd_memory_to_reg_x64(restore.restoreRegister() - VM_REGISTER_MAX, VM_REGISTER_ESP, restoreSlot * 8);
            }
        }
    }
}

void poAsm::generate(poModule& module, poFlowGraph& cfg, const int numArgs)
{
    PO_ALLOCATOR allocator(module);
    allocator.setNumRegisters(VM_REGISTER_MAX + VM_SSE_REGISTER_MAX);
    
    allocator.setVolatile(VM_REGISTER_ESP, true);
    allocator.setVolatile(VM_REGISTER_EBP, true);
    allocator.setVolatile(VM_REGISTER_EAX, true);
    allocator.setVolatile(VM_REGISTER_R10, true);
    allocator.setVolatile(VM_REGISTER_R11, true);

    allocator.setVolatile(VM_ARG1, true);
    allocator.setVolatile(VM_ARG2, true);
    allocator.setVolatile(VM_ARG3, true);
    allocator.setVolatile(VM_ARG4, true);

#if VM_MAX_ARGS == 6
    allocator.setVolatile(VM_ARG5, true);
    allocator.setVolatile(VM_ARG6, true);
#endif

    allocator.setVolatile(VM_SSE_ARG1 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG2 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG3 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG4 + VM_REGISTER_MAX, true);

#ifdef _WIN32
    allocator.setVolatile(VM_SSE_REGISTER_XMM4 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_REGISTER_XMM5 + VM_REGISTER_MAX, true);
#endif

#if VM_MAX_SSE_ARGS == 8
    allocator.setVolatile(VM_SSE_ARG5 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG6 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG7 + VM_REGISTER_MAX, true);
    allocator.setVolatile(VM_SSE_ARG8 + VM_REGISTER_MAX, true);
#endif

    for (int i = 0; i < VM_REGISTER_MAX + VM_SSE_REGISTER_MAX; i++)
    {
        allocator.setType(i, i < VM_REGISTER_MAX ? poRegType::General : poRegType::SSE);
    }

    allocator.allocateRegisters(cfg);
    
    scanBasicBlocks(cfg);
    
    allocator.iterator().reset();
    dump(allocator, allocator.iterator(), cfg);

    allocator.iterator().reset();

    // Lower into machine code cfg
    // 
    _x86_64_lower.cfg().clear();
    po_x86_64_basic_block* asmBB = new po_x86_64_basic_block();
    _x86_64_lower.cfg().addBasicBlock(asmBB);

    // Prologue
    generatePrologue(allocator);

    int pos = 0;
    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        asmBB = _basicBlockMap[bb];
        if (_x86_64_lower.cfg().getLast() != asmBB)
        {
            _x86_64_lower.cfg().addBasicBlock(asmBB);
        }

        auto& instructions = bb->instructions();
        for (int i = 0; i < int(instructions.size()); i++)
        {
            auto& ins = instructions[i];

            assert(ins.code() != IR_ARG); // args are handled in the call
            
#ifdef REG_LINEAR
            spill(allocator, pos);
#endif
            restore(allocator, pos);

            switch (ins.code())
            {
            case IR_ELEMENT_PTR:
                ir_element_ptr(module, allocator, ins);
                break;
            case IR_PTR:
                ir_ptr(module, allocator, ins);
                break;
            case IR_LOAD:
                ir_load(allocator, ins);
                break;
            case IR_STORE:
                ir_store(allocator, ins);
                break;
            case IR_SIGN_EXTEND:
                ir_sign_extend(allocator, ins);
                break;
            case IR_ZERO_EXTEND:
                ir_zero_extend(allocator, ins);
                break;
            case IR_BITWISE_CAST:
                ir_bitwise_cast(allocator, ins);
                break;
            case IR_CONVERT:
                ir_convert(allocator, ins);
                break;
            case IR_ADD:
                ir_add(allocator, ins);
                break;
            case IR_ARG:
                // do nothing
                continue;
            case IR_BR:
                ir_br(allocator, ins, bb);
                break;
            case IR_CALL:
            {
                std::vector<poInstruction> args;
                for (int j = 0; j < ins.left(); j++)
                {
                    args.push_back(instructions[i + j + 1]);
                }

                ir_call(module, allocator, ins, pos, args);
#ifdef REG_GRAPH
                spill(allocator, pos); // do the spill for the call
#endif
                i += ins.left();
                pos += ins.left();
                allocator.iterator().advance(ins.left());
            }
                break;
            case IR_CMP:
                ir_cmp(allocator, ins);
                break;
            case IR_CONSTANT:
                ir_constant(module, module.constants(), allocator, ins);
                break;
            case IR_COPY:
                ir_copy(allocator, ins);
                break;
            case IR_DIV:
                ir_div(allocator, ins);
                break;
            case IR_MUL:
                ir_mul(allocator, ins);
                break;
            case IR_PARAM:
                ir_param(module, allocator, ins, numArgs);
                break;
            case IR_PHI:
                // do nothing
                break;
            case IR_RETURN:
                ir_ret(module, allocator, ins);
                break;
            case IR_SUB:
                ir_sub(allocator, ins);
                break;
            case IR_UNARY_MINUS:
                ir_unary_minus(allocator, ins);
                break;
            }

#ifdef REG_GRAPH
            spill(allocator, pos);
#endif

            pos++;
            allocator.iterator().next();
        }
        
        bb = bb->getNext();
    }

    poAnalyzer an;
    an.checkCallSites(module, _x86_64_lower.cfg());

    // Generate the machine code
    //

    _x86_64_lower.dump();
    generateMachineCode(module);
}

void poAsm::generateMachineCode(poModule& module)
{
    poConstantPool& constants = module.constants();

    po_x86_64_basic_block* asmBB = nullptr;
    for (size_t i = 0; i < _x86_64_lower.cfg().basicBlocks().size(); i++)
    {
        asmBB = _x86_64_lower.cfg().basicBlocks()[i];
        asmBB->setProgramDataPos(int(_x86_64.programData().size()));
        patchForwardJumps(asmBB);

        for (auto& ins : asmBB->instructions())
        {
            const int pos = int(_x86_64.programData().size());
            if (ins.isSSE())
            {
                switch (ins.opcode())
                {
                case VMI_SSE_MOVSD_SRC_MEM_DST_REG:
                    if (ins.id() != -1)
                    {
                        _x86_64.mc_movsd_memory_to_reg_x64(ins.dstReg(), 0);
                        addInitializedData(constants.getF64(ins.id()), -int(sizeof(int32_t))); // insert patch
                    }
                    else
                    {
                        _x86_64.mc_movsd_memory_to_reg_x64(ins.dstReg(), ins.srcReg(), ins.imm32());
                    }
                    break;
                case VMI_SSE_MOVSS_SRC_MEM_DST_REG:
                    if (ins.id() != -1)
                    {
                        _x86_64.mc_movss_memory_to_reg_x64(ins.dstReg(), 0);
                        addInitializedData(constants.getF32(ins.id()), -int(sizeof(int32_t))); // insert patch
                    }
                    else
                    {
                        _x86_64.mc_movss_memory_to_reg_x64(ins.dstReg(), ins.srcReg(), ins.imm32());
                    }
                    break;
                default:
                    _x86_64.emit(ins);
                    break;
                }
            }
            else
            {
                switch (ins.opcode())
                {
                case VMI_CALL:
                case VMI_CALL_MEM:
                {
                    const int symbol = ins.id();
                    std::string symbolName;
                    module.getSymbol(symbol, symbolName);

                    // Add patch for this call

                    _calls.push_back(poAsmCall(int(_x86_64.programData().size()), 0/*numArgs*/, symbolName));
                    _x86_64.mc_call(0); // Placeholder for the call
                }
                break;
                case VMI_PUSH_REG:
                    _x86_64.mc_push_reg(ins.dstReg());
                    break;
                case VMI_POP_REG:
                    _x86_64.mc_pop_reg(ins.dstReg());
                    break;
                case VMI_NEAR_RETURN:
                    _x86_64.mc_return();
                    break;
                case VMI_ADD64_SRC_IMM_DST_REG:
                    _x86_64.mc_add_imm_to_reg_x64(ins.dstReg(), int(ins.imm64()));
                    break;
                case VMI_MOV64_SRC_IMM_DST_REG:
                    _x86_64.mc_mov_imm_to_reg_x64(ins.dstReg(), ins.imm64());
                    break;
                case VMI_SUB64_SRC_IMM_DST_REG:
                    _x86_64.mc_sub_imm_to_reg_x64(ins.dstReg(), int(ins.imm64()));
                    break;
                case VMI_MOV32_SRC_IMM_DST_REG:
                    _x86_64.mc_mov_imm_to_reg_32(ins.dstReg(), ins.imm32());
                    break;
                case VMI_MOV16_SRC_IMM_DST_REG:
                    _x86_64.mc_mov_imm_to_reg_16(ins.dstReg(), ins.imm16());
                    break;
                case VMI_ADD8_SRC_IMM_DST_REG:
                    _x86_64.mc_add_imm_to_reg_8(ins.dstReg(), ins.imm8());
                    break;
                case VMI_MOV8_SRC_IMM_DST_REG:
                    _x86_64.mc_mov_imm_to_reg_8(ins.dstReg(), ins.imm8());
                    break;
                case VMI_SUB8_SRC_IMM_DST_REG:
                    _x86_64.mc_sub_imm_to_reg_8(ins.dstReg(), ins.imm8());
                    break;
                case VMI_MOV8_SRC_REG_DST_MEM:
                    _x86_64.mc_mov_reg_to_memory_8(ins.dstReg(), ins.srcReg(), ins.imm32());
                    break;
                case VMI_LEA64_SRC_REG_DST_REG:
                    _x86_64.mc_lea_reg_to_reg_x64(ins.dstReg(), 0);
                    addInitializedData(constants.getString(ins.id()), -int(sizeof(int32_t))); // insert patch
                    break;
                case VMI_J8:
                case VMI_JE8:
                case VMI_JG8:
                case VMI_JGE8:
                case VMI_JL8:
                case VMI_JLE8:
                case VMI_J32:
                case VMI_JE32:
                case VMI_JG32:
                case VMI_JGE32:
                case VMI_JL32:
                case VMI_JLE32:
                case VMI_JNA32:
                case VMI_JNB32:
                case VMI_JNBE32:
                case VMI_JNE32:
                case VMI_JNAE32:
                    emitJump(asmBB);
                    break;
                default:
                    _x86_64.emit(ins);
                    break;
                }
            }
        }
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
                generate(module, function.cfg(), int(function.args().size()));
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

void poAsm::addInitializedData(const std::string& str, const int programDataOffset)
{
    _constants.push_back(poAsmConstant(int(_initializedData.size()), int(_x86_64.programData().size() + programDataOffset)));

    for (char c : str)
    {
        _initializedData.push_back(c);
    }
    _initializedData.push_back(0); // null terminator
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

#pragma once
#include "poPhiWeb.h"
#include "po_x86_64.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace po
{
    class poFlowGraph;
    class poModule;
    class poInstruction;
    class poRegLinear;
    class poRegLinearIterator;
    class poConstantPool;
    class poBasicBlock;

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
        inline const int getPos() const { return _pos; }
        inline void setPos(const int pos) { _pos = pos; }

    private:
        int _pos;
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

    class poAsmExternCall
    {
    public:
        poAsmExternCall(const std::string& name, const int programPos);

        const std::string& name() const { return _name; }
        const int programPos() const { return _programPos; }

    private:
        int _programPos;
        std::string _name;
    };

    class poAsm
    {
    public:
        poAsm();
        void generate(poModule& module);
        void link(const int programDataPos, const int initializedDataPos);

        inline std::unordered_map<std::string, int>& imports() { return _imports; }
        inline const std::vector<unsigned char>& programData() const { return _x86_64.programData(); }
        inline const std::vector<unsigned char>& initializedData() const { return _initializedData; }
        inline const int entryPoint() const { return _entryPoint; }
        inline bool isError() const { return _isError; }
        inline const std::string& errorText() const { return _errorText; }

    private:
        void dump(const poRegLinear& linear, poRegLinearIterator& iterator, poFlowGraph& cfg);

        void spill(poRegLinear& linear, const int pos);
        void restore(poRegLinear& linear, const int pos);

        void generate(poModule& module, poFlowGraph& cfg, const int numArgs);
        void generateMachineCode(poModule& module);
        void generateExternStub(poModule& module, poFlowGraph& cfg);
        void patchForwardJumps(po_x86_64_basic_block* bb);
        void scanBasicBlocks(poFlowGraph& cfg);
        void patchJump(po_x86_64_basic_block* jump);
        void patchCalls();
        void generatePrologue(poRegLinear& linear);
        void generateEpilogue(poRegLinear& linear);
        void setError(const std::string& errorText);
        void addInitializedData(const float f32, const int programDataOffset);
        void addInitializedData(const double f64, const int programDataOffset);

        void emitJump(po_x86_64_basic_block* bb);

        //=====================================
        // IR to machine code routines
        //=====================================

        void ir_element_ptr(poModule& module, poRegLinear& linear, const poInstruction& ins);
        void ir_ptr(poModule& module, poRegLinear& linear, const poInstruction& ins);
        void ir_store(poRegLinear& linear, const poInstruction& ins);
        void ir_load(poRegLinear& linear, const poInstruction& ins);
        void ir_zero_extend(poRegLinear& linear, const poInstruction& ins);
        void ir_sign_extend(poRegLinear& linear, const poInstruction& ins);
        void ir_bitwise_cast(poRegLinear& linear, const poInstruction& ins);
        void ir_convert(poRegLinear& linear, const poInstruction& ins);
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
        void ir_call(poModule& module, poRegLinear& linear, const poInstruction& ins, const int pos, const std::vector<poInstruction>& args);
        void ir_param(poModule& module, poRegLinear& linear, const poInstruction& ins, const int numArgs);
        bool ir_jump(const int jump, const int imm, const int type);

        std::unordered_map<std::string, int> _mapping;
        std::vector<poAsmCall> _calls;
        std::vector<poAsmExternCall> _externCalls;
        std::unordered_map<std::string, int> _imports;
        std::vector<poAsmConstant> _constants;
        std::unordered_map<poBasicBlock*, po_x86_64_basic_block*> _basicBlockMap;
        std::vector<unsigned char> _initializedData;
        poPhiWeb _web;
        po_x86_64 _x86_64;
        po_x86_64_Lower _x86_64_lower;
        int _entryPoint;
        bool _isError;
        std::string _errorText;
        int _prologueSize;
    };
}

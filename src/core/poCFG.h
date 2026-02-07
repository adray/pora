#pragma once
#include <vector>
#include <cstdint>

/* 
* Flow graph of basic blocks for a single function.
*/

namespace po
{
    class poBasicBlock;
    class poPhi;

    constexpr int IR_CONSTANT = 0x1;
    constexpr int IR_PARAM = 0x2;
    constexpr int IR_PHI = 0x3;
    constexpr int IR_ADD = 0x10;
    constexpr int IR_SUB = 0x11;
    constexpr int IR_MUL = 0x12;
    constexpr int IR_DIV = 0x13;
    constexpr int IR_AND = 0x14;
    constexpr int IR_OR = 0x15;
    constexpr int IR_UNARY_MINUS = 0x16;
    constexpr int IR_LEFT_SHIFT = 0x17;
    constexpr int IR_RIGHT_SHIFT = 0x18;
    constexpr int IR_MODULO = 0x19;
    constexpr int IR_CMP = 0x21;
    constexpr int IR_BR = 0x22;
    constexpr int IR_CALL = 0x30;
    constexpr int IR_ARG = 0x31;
    constexpr int IR_RETURN = 0x32;
    constexpr int IR_COPY = 0x33;
    constexpr int IR_SIGN_EXTEND = 0x34;
    constexpr int IR_ZERO_EXTEND = 0x35;
    constexpr int IR_BITWISE_CAST = 0x36;
    constexpr int IR_CONVERT = 0x37;
    constexpr int IR_ALLOCA = 0x40;
    constexpr int IR_MALLOC = 0x41;
    constexpr int IR_LOAD = 0x42;
    constexpr int IR_STORE = 0x43;
    constexpr int IR_PTR = 0x44;
    constexpr int IR_ELEMENT_PTR = 0x45;

    constexpr int IR_LOAD_GLOBAL = 0x50;
    constexpr int IR_STORE_GLOBAL = 0x51;

    constexpr int IR_JUMP_UNCONDITIONAL = 0x0;
    constexpr int IR_JUMP_EQUALS = 0x1;
    constexpr int IR_JUMP_NOT_EQUALS = 0x2;
    constexpr int IR_JUMP_LESS = 0x3;
    constexpr int IR_JUMP_GREATER = 0x4;
    constexpr int IR_JUMP_GREATER_EQUALS = 0x5;
    constexpr int IR_JUMP_LESS_EQUALS = 0x6;

    class poInstruction
    {
    public:
        poInstruction(const int32_t name, const int16_t type, const int16_t left, const int16_t right, const int16_t code);
        poInstruction(const int32_t name, const int16_t type, const int16_t constant, const int16_t code);
        poInstruction(const int32_t name, const int16_t type, const int16_t left, const int16_t right, const int16_t memOffset, const int16_t code);
        inline int16_t type() const { return _type; }
        inline int16_t left() const { return _left; }
        inline int16_t right() const { return _right; }
        inline int16_t code() const { return _code; }
        inline int16_t constant() const { return _constant; }
        inline int16_t memOffset() const { return _memOffset; }
        inline int32_t name() const { return _name; }
        inline void setName(const int32_t name) { _name = name; }
        inline void setLeft(const int32_t left) { _left = left; }
        inline void setRight(const int32_t right) { _right = right; }
        inline void setCode(const int16_t code) { _code = code; }
        inline void setType(const int16_t type) { _type = type; }
        inline void setConstant(const int16_t constant) { _constant = constant; }

        bool isSpecialInstruction() const;

    private:
        int32_t _name;
        int16_t _type;
        int16_t _left;
        int16_t _right;
        union
        {
            int16_t _constant;
            int16_t _memOffset;
        };
        int16_t _code;
    };

    class poInstructionRef
    {
    public:
        poInstructionRef(poBasicBlock* bb, const int ref, const int baseRef);
        inline poBasicBlock* getBasicBlock() const { return _bb; }
        inline const int getRef() const { return _ref; }
        inline const int getAdjustedRef() const { return _ref - _baseRef; }
        poInstruction& getInstruction() const;

    private:
        poBasicBlock* _bb;
        int _ref;
        int _baseRef;
    };

    class poPhi
    {
    public:
        poPhi(const int name, const int type);
        void addValue(const int value, poBasicBlock* bb);
        inline void setValue(const int index, const int value) { _rhs[index] = value; }
        inline const std::vector<int>& values() const { return _rhs; }
        inline void setName(const int name) { _name = name; }
        inline const int name() const { return _name; }
        inline const int initialName() const { return _initialName; }
        inline void setType(const int type) { _type = type; }
        inline const int getType() const { return _type; }
        inline const std::vector<poBasicBlock*>& getBasicBlock() const { return _bb; }

    private:
        int _initialName;
        int _name;
        int _type;
        std::vector<int> _rhs;
        std::vector<poBasicBlock*> _bb;
    };

    class poBasicBlock
    {
    public:
        poBasicBlock();

        inline void setNext(poBasicBlock* next) { _next = next; }
        inline void setPrev(poBasicBlock* prev) { _prev = prev; }
        inline void setBranch(poBasicBlock* branch, const bool unconditional)
        {
            _branch = branch;
            _unconditionalBranch = unconditional;
        }

        inline poBasicBlock* getNext() const { return _next; }
        inline poBasicBlock* getPrev() const { return _prev; }
        inline poBasicBlock* getBranch() const { return _branch; }
        inline const bool unconditionalBranch() const { return _unconditionalBranch; }

        inline void addInstruction(const poInstruction& ins) { return _ins.push_back(ins); }
        inline void insertInstruction(const poInstruction& ins, const int index) { _ins.insert(_ins.begin() + index, ins); }
        inline void removeInstruction(const int index) { _ins.erase(_ins.begin() + index); }
        inline const size_t numInstructions() const { return _ins.size(); }
        inline const std::vector<poInstruction>& instructions() const { return _ins; }
        inline poInstruction& getInstruction(const int index) {
            return _ins[index];
        }

        inline void addPhi(const poPhi& phi) { _phis.push_back(phi); }
        inline std::vector<poPhi>& phis() { return _phis; }

        inline void addIncoming(poBasicBlock* bb) { _incoming.push_back(bb); }
        inline void removeIncoming(poBasicBlock* bb) { std::erase(_incoming, bb); }
        inline const std::vector<poBasicBlock*>& getIncoming() const { return _incoming; }

    private:
        poBasicBlock* _next;
        poBasicBlock* _prev;
        poBasicBlock* _branch;
        bool _unconditionalBranch;
        std::vector<poBasicBlock*> _incoming;
        std::vector<poInstruction> _ins;
        std::vector<poPhi> _phis;
    };

    class poFlowGraph
    {
    public:
        void addBasicBlock(poBasicBlock* bb);
        void insertBasicBlock(poBasicBlock* insertAfter, poBasicBlock* bb);
        void removeBasicBlock(poBasicBlock* bb);
        poBasicBlock* getBasicBlock(int index) const;

        inline const size_t numBlocks() const { return _blocks.size(); }
        poBasicBlock* getLast() const;
        poBasicBlock* getFirst() const;

        void optimize();

        //~poFlowGraph();

    private:
        std::vector<poBasicBlock*> _blocks;
    };
}

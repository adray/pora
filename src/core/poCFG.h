#pragma once
#include <vector>
#include <cstdint>

/* 
* Flow graph of basic blocks for a single function.
*/

namespace po
{
    class poBasicBlock;

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
    constexpr int IR_CMP = 0x21;
    constexpr int IR_BR = 0x22;
    constexpr int IR_CALL = 0x30;
    constexpr int IR_ARG = 0x31;
    constexpr int IR_RETURN = 0x32;

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
        poInstruction(const int32_t name, const int8_t type, const int16_t left, const int16_t right, const int16_t code);
        poInstruction(const int32_t name, const int8_t type, const int16_t constant, const int16_t code);
        inline int8_t type() const { return _type; }
        inline int16_t left() const { return _left; }
        inline int16_t right() const { return _right; }
        inline int16_t code() const { return _code; }
        inline int16_t constant() const { return _constant; }
        inline int32_t name() const { return _name; }
        inline void setName(const int32_t name) { _name = name; }
        inline void setLeft(const int32_t left) { _left = left; }
        inline void setRight(const int32_t right) { _right = right; }

    private:
        int32_t _name;
        int8_t _padding;
        int8_t _type;
        union
        {
            int16_t _left;
            int16_t _constant;
        };
        int16_t _right;
        int16_t _code;
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

        inline void addIncoming(poBasicBlock* bb) { _incoming.push_back(bb); }
        //inline void removeIncoming(poBasicBlock* bb);
        inline const std::vector<poBasicBlock*>& getIncoming() const { return _incoming; }

    private:
        poBasicBlock* _next;
        poBasicBlock* _prev;
        poBasicBlock* _branch;
        bool _unconditionalBranch;
        std::vector<poBasicBlock*> _incoming;
        std::vector<poInstruction> _ins;
    };

    class poFlowGraph
    {
    public:
        void addBasicBlock(poBasicBlock* bb);
        //void insertBasicBlock(int index, poBasicBlock* bb);
        void removeBasicBlock(poBasicBlock* bb);
        poBasicBlock* getBasicBlock(int index) const;

        inline const size_t numBlocks() const { return _blocks.size(); }
        poBasicBlock* getLast() const;
        poBasicBlock* getFirst() const;

        void optimize();

    private:
        std::vector<poBasicBlock*> _blocks;
    };
}

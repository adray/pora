#pragma once
#include "poLex.h"

namespace po
{
    enum class poNodeType
    {
        POINTER, /* poUnaryNode */
        ADD, /* poBinaryNode */
        SUB, /* poBinaryNode */
        MUL, /* poBinaryNode */
        DIV, /* poBinaryNode */
        UNARY_SUB, /* poUnaryNode */
        OR, /* poBinaryNode */
        AND, /* poBinaryNode */
        IF, /* poListNode */
        ELSE, /* poUnaryNode */
        WHILE, /* poListNode */
        CALL, /* poListNode */
        ASSIGNMENT, /* poBinaryNode */
        VARIABLE, /* poNode */
        PARAMETER, /* poUnaryNode */
        ARGS, /* poListNode */
        TYPE, /* poNode */
        DECL, /* poUnaryNode */
        CMP_EQUALS, /* poBinaryNode */
        CMP_NOT_EQUALS, /* poBinaryNode */
        CMP_LESS, /* poBinaryNode */
        CMP_GREATER, /* poBinaryNode */
        CMP_LESS_EQUALS, /* poBinaryNode */
        CMP_GREATER_EQUALS, /* poBinaryNode */
        CONSTANT, /* poConstantNode */
        MEMBER, /* poUnaryNode */
        STATEMENT, /* poUnaryNode */
        RETURN, /* poUnaryNode */
        FUNCTION, /* poListNode */
        BODY, /* poListNode */
        CLASS, /* poListNode */
        STRUCT, /* poListNode */
        NAMESPACE, /* poListNode */
        MODULE, /* poListNode */
        IMPORT /* poNode */
    };

    class poNode
    {
    public:
        poNode(poNodeType type, const poToken& token);
        inline poNodeType type() { return _type; }
        inline poToken& token() { return _token; }

    private:
        poNodeType _type;
        poToken _token;
    };

    class poConstantNode : public poNode
    {
    public:
        poConstantNode(poNodeType type, const poToken& token);
        poConstantNode(poNodeType type, const poToken& token, int64_t value);
        poConstantNode(poNodeType type, const poToken& token, uint8_t value);
        poConstantNode(poNodeType type, const poToken& token, uint64_t value);
        inline const int64_t i64() const { return _i64; }
        inline const uint8_t u8() const { return _u8; }
        inline const double f64() const { return _f64; }
        inline const float f32() const { return _f32; }
        inline const uint64_t u64() const { return _u64; }

        inline const int type() const { return _type; }
    private:
        union
        {
            int64_t _i64;
            int32_t _i32;
            int8_t _i8;
            uint64_t _u64;
            uint32_t _u32;
            uint8_t _u8;
            float _f32;
            double _f64;
        };
        int _type;
        std::string _str;
    };

    class poUnaryNode : public poNode
    {
    public:
        poUnaryNode(poNodeType type, poNode* child, const poToken& token);
        inline poNode* child() { return _child; }
        inline void setChild(poNode* child) { _child = child; }
    private:
        poNode* _child;
    };

    class poBinaryNode : public poNode
    {
    public:
        poBinaryNode(poNodeType type, poNode* left, poNode* right, const poToken& token);
        inline poNode* left() { return _left; }
        inline poNode* right() { return _right; }
        inline void setLeft(poNode* left) { _left = left; }
        inline void setRight(poNode* right) { _right = right; }
    private:
        poNode* _left;
        poNode* _right;
    };

    class poListNode : public poNode
    {
    public:
        poListNode(poNodeType type, const std::vector<poNode*>& nodes, const poToken& token);
        inline std::vector<poNode*>& list() { return _list; }
    private:
        std::vector<poNode*> _list;
    };
}

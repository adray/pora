#include "poAST.h"
#include "poTypeChecker.h"

using namespace po;

poNode::poNode(poNodeType type, const poToken& token)
    :
    _type(type),
    _token(token)
{
}

poConstantNode::poConstantNode(poNodeType type, const poToken& token)
    :
    poNode(type, token),
    _type(0),
    _i64(0)
{
    switch (token.token())
    {
    case poTokenType::U8:
        _u8 = token.u8();
        _type = TYPE_U8;
        break;
    case poTokenType::U32:
        _u32 = token.u32();
        _type = TYPE_U32;
        break;
    case poTokenType::U64:
        _u64 = token.u64();
        _type = TYPE_U64;
        break;
    case poTokenType::I8:
        _i8 = token.i8();
        _type = TYPE_I8;
        break;
    case poTokenType::I32:
        _i32 = token.i32();
        _type = TYPE_I32;
        break;
    case poTokenType::I64:
        _i64 = token.i64();
        _type = TYPE_I64;
        break;
    //case poTokenType::F32:
    //    _f32 = token.f32();
    //    _type = TYPE_U8;
    //    break;
    case poTokenType::STRING:
        _str = token.string();
        //_type = TYPE_;
        break;
    }
}

poConstantNode::poConstantNode(poNodeType type, const poToken& token, int64_t value)
    :
    poNode(type, token),
    _i64(value),
    _type(TYPE_I64)
{
}

poConstantNode::poConstantNode(poNodeType type, const poToken& token, uint8_t value)
    :
    poNode(type, token),
    _u8(value),
    _type(TYPE_U8)
{
}

poUnaryNode::poUnaryNode(poNodeType type, poNode* child, const poToken& token)
    :
    poNode(type, token),
    _child(child)
{
}

poBinaryNode::poBinaryNode(poNodeType type, poNode* left, poNode* right, const poToken& token)
    :
    poNode(type, token),
    _left(left),
    _right(right)
{
}


poListNode::poListNode(poNodeType type, const std::vector<poNode*>& nodes, const poToken& token)
    :
    poNode(type, token),
    _list(nodes)
{
}


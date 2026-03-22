#include "poAST.h"
#include "poType.h"

using namespace po;

poNode::poNode(poNodeType type, const poToken& token)
    :
    _type(type),
    _token(token)
{
}

poNode* poNode::clone() {
    return new poNode(_type, _token);
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
    case poTokenType::F32:
        _f32 = token.f32();
        _type = TYPE_F32;
        break;
    case poTokenType::F64:
        _f64 = token.f64();
        _type = TYPE_F64;
        break;
    case poTokenType::STRING:
        _str = token.string();
        _type = TYPE_STRING;
        break;
    case poTokenType::TRUE:
        _u8 = 1;
        _type = TYPE_BOOLEAN;
        break;
    case poTokenType::FALSE:
        _u8 = 0;
        _type = TYPE_BOOLEAN;
        break;
    case poTokenType::CHAR:
        _u8 = token.character();
        _type = TYPE_U8;
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

poConstantNode::poConstantNode(poNodeType type, const poToken& token, uint64_t value)
    :
    poNode(type, token),
    _u64(value),
    _type(TYPE_U64)
{
}

poConstantNode::poConstantNode(poNodeType type, const poToken& token, uint8_t value)
    :
    poNode(type, token),
    _u8(value),
    _type(TYPE_U8)
{
}

poNode* poConstantNode::clone() {
    poNode* clone = nullptr;
    switch (_type) {
    case TYPE_I64:
        clone = new poConstantNode(
            type(),
            token(),
            _i64);
        break;
    case TYPE_U64:
        clone = new poConstantNode(
            type(),
            token(),
            _u64);
        break;
    case TYPE_U8:
        clone = new poConstantNode(
            type(),
            token(),
            _u8);
        break;
    default:
        clone = new poConstantNode(
            type(),
            token());
    }

    return clone;
}

poUnaryNode::poUnaryNode(poNodeType type, poNode* child, const poToken& token)
    :
    poNode(type, token),
    _child(child)
{
}

poNode* poUnaryNode::clone() {
    poNode* clone = nullptr;
    if (_child) {
        clone = _child->clone();
    }
    return new poUnaryNode(type(), clone, token());
}

poBinaryNode::poBinaryNode(poNodeType type, poNode* left, poNode* right, const poToken& token)
    :
    poNode(type, token),
    _left(left),
    _right(right)
{
}

poNode* poBinaryNode::clone() {
    return new poBinaryNode(
        type(),
        _left ->clone(),
        _right->clone(),
        token()
    );
}

poBinaryNode:: ~poBinaryNode() {
    delete _left;
    delete _right;

    _left = nullptr;
    _right = nullptr;
}

poListNode::poListNode(poNodeType type, const std::vector<poNode*>& nodes, const poToken& token)
    :
    poNode(type, token),
    _list(nodes)
{
}

poNode* poListNode::clone() {
    std::vector<poNode*> clones;
    for (poNode* node : _list) {
        clones.push_back(node->clone());
    }

    return new poListNode(
        type(),
        clones,
        token()
    );
}

poListNode:: ~poListNode() {
    for (poNode* node : _list) {
        delete node;
    }
    _list.clear();
}

poArrayNode::poArrayNode(const int64_t arraySize, poNode* node, const poNodeType type, const poToken& token)
    :
    poUnaryNode(type, node, token),
    _arraySize(arraySize)
{
}

poNode* poArrayNode::clone() {
    return new poArrayNode(
        _arraySize,
        child()->clone(),
        type(),
        token()
    );
}

poArrayAccessor::poArrayAccessor(poNode* accessor, poNode* child, const poNodeType type, const poToken& token)
    :
    _child(child),
    _accessor(accessor),
    _dereference(false),
    poNode(type, token)
{
}

poNode* poArrayAccessor::clone() {
    poArrayAccessor* clone = new poArrayAccessor(
        _accessor->clone(),
        _child->clone(),
        type(),
        token()
    );
    clone->setDereference(_dereference);
    return clone;
}

poArrayAccessor:: ~poArrayAccessor() {
    delete _child;
    delete _accessor;

    _child = nullptr;
    _accessor = nullptr;
}

poPointerNode::poPointerNode(const poNodeType type, poNode* child, const poToken& token, const int count)
    :
    poUnaryNode(type, child, token),
    _count(count)
{
}

poNode* poPointerNode::clone() {
    return new poPointerNode(
        type(),
        child()->clone(),
        token(),
        _count
    );
}

poNode* poAttributeNode::clone() {
    return new poAttributeNode(_attributes, token(), child()->clone());
}

poNode*  poResolverNode:: clone() {
    return new poResolverNode(token(), _path);
}

poNode* poGenericNode::clone() {
    std::vector<poNode*> clones;
    for (poNode* node : _parameters) {
        clones.push_back(node->clone());
    }

    return new poGenericNode(
        child(),
        clones,
        token());
}

poGenericNode:: ~poGenericNode()
{
    for (poNode* child : _parameters) {
        delete child;
    }
    _parameters.clear();
}

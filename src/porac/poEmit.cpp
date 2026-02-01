#include "poEmit.h"
#include "poAST.h"
#include "poModule.h"
#include <assert.h>
#include <algorithm>
#include <iostream>

using namespace po;

constexpr int EMIT_ERROR = -1;
constexpr int QUALIFIER_NONE = 0;
constexpr int QUALIFIER_CONST = 1;

//=====================
// Variable
//=====================

poVariable::poVariable(const int id, const int type, const int size)
    :
    _id(id),
    _type(type),
    _size(size)
{
}

//=========================
// poConditionGraphNode
//=========================

poConditionGraphNode::poConditionGraphNode(int id, poNode* ast)
    :
    _id(id),
    _success(-1),
    _fail(-1),
    _ast(ast),
    _terminator(true),
    _emitted(false),
    _bb(nullptr)
{
}

poConditionGraphNode::poConditionGraphNode(int id, int success, int fail, poNode* ast, const bool terminator)
    :
    _id(id),
    _success(success),
    _fail(fail),
    _ast(ast),
    _terminator(terminator),
    _emitted(false),
    _bb(nullptr)
{
}

//=====================
// ConditionGraph
//=====================

poConditionGraph::poConditionGraph()
    :
    _root(-1),
    _success(-1),
    _fail(-1)
{
}

int poConditionGraph::createNode(poNode* ast)
{
    auto& node = _nodes.emplace_back(poConditionGraphNode(int(_nodes.size()), ast));
    return node.id();
}

int poConditionGraph::createNode(poNode* ast, int success, int fail)
{
    auto& node = _nodes.emplace_back(poConditionGraphNode(int(_nodes.size()), success, fail, ast, success == _success && fail == _fail));
    return node.id();
}

int poConditionGraph::buildAnd(poNode* ast, int success, int fail)
{
    poBinaryNode* expr = static_cast<poBinaryNode*>(ast);
    const int right = build(expr->right(), success, fail);
    const int left = build(expr->left(), right, fail);

    return left;
}

int poConditionGraph::buildOr(poNode* ast, int success, int fail)
{
    poBinaryNode* expr = static_cast<poBinaryNode*>(ast);
    const int right = build(expr->right(), success, fail);
    const int left = build(expr->left(), success, right);

    return left;
}

int poConditionGraph::build(poNode* ast, int success, int fail)
{
    switch (ast->type())
    {
    case poNodeType::AND:
        return buildAnd(ast, success, fail);
    case poNodeType::OR:
        return buildOr(ast, success, fail);
    default:
        return createNode(ast, success, fail);
    }

    return -1;
}

void poConditionGraph::buildGraph(poNode* ast)
{
    _nodes.clear();
    _success = createNode(nullptr);
    _fail = createNode(nullptr);
    _root = build(ast, _success, _fail);
}

void poConditionGraph::computePredecessors()
{
    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& node = _nodes[i];
        node.predecessors().clear();
    }

    for (size_t i = 0; i < _nodes.size(); i++)
    {
        auto& node = _nodes[i];
        if (node.fail() != -1)
        {
            _nodes[node.fail()].predecessors().push_back(int(i));
        }
        if (node.success() != -1)
        {
            _nodes[node.success()].predecessors().push_back(int(i));
        }
    }
}

//============
// Helpers
//============

static bool checkPredecessors(poConditionGraph& graph, poConditionGraphNode& node)
{
    bool success = true;
    for (int pred : node.predecessors())
    {
        if (!graph.at(pred).isEmitted())
        {
            success = false;
            break;
        }
    }
    return success;
}

static int getCompare(poNode* node)
{
    switch (node->type())
    {
    case poNodeType::CMP_EQUALS:
        return IR_JUMP_EQUALS;
    case poNodeType::CMP_NOT_EQUALS:
        return IR_JUMP_NOT_EQUALS;
    case poNodeType::CMP_LESS:
        return IR_JUMP_LESS;
    case poNodeType::CMP_GREATER:
        return IR_JUMP_GREATER;
    case poNodeType::CMP_LESS_EQUALS:
        return IR_JUMP_LESS_EQUALS;
    case poNodeType::CMP_GREATER_EQUALS:
        return IR_JUMP_GREATER_EQUALS;
    }
    return IR_JUMP_UNCONDITIONAL;
}

static int flipJump(int jump)
{
    switch (jump)
    {
    case IR_JUMP_EQUALS:
        return IR_JUMP_NOT_EQUALS;
    case IR_JUMP_NOT_EQUALS:
        return IR_JUMP_EQUALS;
    case IR_JUMP_LESS:
        return IR_JUMP_GREATER_EQUALS;
    case IR_JUMP_GREATER:
        return IR_JUMP_LESS_EQUALS;
    case IR_JUMP_LESS_EQUALS:
        return IR_JUMP_GREATER;
    case IR_JUMP_GREATER_EQUALS:
        return IR_JUMP_LESS;
    }
    return IR_JUMP_UNCONDITIONAL;
}

//===================
// poEmitter
//===================

poEmitter::poEmitter(poModule& module)
    :
    _module(module),
    _instructionCount(0)
{
}

void poEmitter::emitInstruction(const poInstruction& instruction, poBasicBlock* bb)
{
    bb->addInstruction(instruction);
    _types.push_back(instruction.type());
    _qualifiers.push_back(QUALIFIER_NONE);
    _arraySize.push_back(0);
}

int poEmitter::emitConstant(const int64_t i64, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(i64);
    if (id == -1) { id = constants.addConstant(i64); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_I64, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const uint64_t u64, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(u64);
    if (id == -1) { id = constants.addConstant(u64); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_U64, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const int32_t i32, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(i32);
    if (id == -1) { id = constants.addConstant(i32); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_I32, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const uint32_t u32, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(u32);
    if (id == -1) { id = constants.addConstant(u32); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_U32, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const int16_t i16, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(i16);
    if (id == -1) { id = constants.addConstant(i16); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_I16, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const uint16_t u16, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(u16);
    if (id == -1) { id = constants.addConstant(u16); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_U16, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const int8_t i8, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(i8);
    if (id == -1) { id = constants.addConstant(i8); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_I8, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const uint8_t u8, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(u8);
    if (id == -1) { id = constants.addConstant(u8); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_U8, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const float f32, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(f32);
    if (id == -1) { id = constants.addConstant(f32); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_F32, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const double f64, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(f64);
    if (id == -1) { id = constants.addConstant(f64); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, TYPE_F64, id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const std::string& str, poFlowGraph& cfg)
{
    poConstantPool& constants = _module.constants();
    int id = constants.getConstant(str);
    if (id == -1) { id = constants.addConstant(str); }
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, getPointerType(TYPE_U8), id, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConstant(const int type, const int constantId, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, type, constantId, IR_CONSTANT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitAdd(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_ADD), cfg.getLast());
    return instructionId;
}

int poEmitter::emitSub(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_SUB), cfg.getLast());
    return instructionId;
}

int poEmitter::emitMul(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_MUL), cfg.getLast());
    return instructionId;
}

int poEmitter::emitDiv(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_DIV), cfg.getLast());
    return instructionId;
}

int poEmitter::emitLeftShift(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_LEFT_SHIFT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitRightShift(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_RIGHT_SHIFT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitModulo(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_MODULO), cfg.getLast());
    return instructionId;
}

int poEmitter::emitAnd(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_AND), cfg.getLast());
    return instructionId;
}

int poEmitter::emitOr(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_OR), cfg.getLast());
    return instructionId;
}

int poEmitter::emitUnaryMinus(const int type, const int left, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), -1, IR_UNARY_MINUS), cfg.getLast());
    return instructionId;
}

int poEmitter::emitCmp(const int type, const int left, const int right, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(left), int16_t(right), IR_CMP), cfg.getLast());
    return instructionId;
}

int poEmitter::emitCall(const int returnType, const int numArgs, const int symbolId, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(returnType), int16_t(numArgs), int16_t(symbolId), IR_CALL), cfg.getLast());
    return instructionId;
}

int poEmitter::emitArg(const int type, const int arg, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(arg), -1, IR_ARG), cfg.getLast());
    return instructionId;
}

int poEmitter::emitReturn(const int type, const int value, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(type), int16_t(value), -1, IR_RETURN), cfg.getLast());
    return instructionId;
}

int poEmitter::emitReturn(poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(TYPE_VOID), -1, -1, IR_RETURN), cfg.getLast());
    return instructionId;
}

int poEmitter::emitSignExtend(const int dstType, const int srcType, const int value, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(dstType), int16_t(value), -1, int16_t(srcType),IR_SIGN_EXTEND), cfg.getLast());
    return instructionId;
}

int poEmitter::emitZeroExtend(const int dstType, const int srcType, const int value, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(dstType), int16_t(value), -1, int16_t(srcType), IR_ZERO_EXTEND), cfg.getLast());
    return instructionId;
}

int poEmitter::emitBitwiseCast(const int dstType, const int srcType, const int value, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(dstType), int16_t(value), -1, int16_t(srcType), IR_BITWISE_CAST), cfg.getLast());
    return instructionId;
}

int poEmitter::emitConvert(const int dstType, const int srcType, const int value, poFlowGraph& cfg)
{
    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, int16_t(dstType), int16_t(value), -1, int16_t(srcType), IR_CONVERT), cfg.getLast());
    return instructionId;
}

int poEmitter::emitAlloca(const int type, poBasicBlock* bb)
{
    const int id = _instructionCount;
    const int ptrType = getPointerType(type);
    emitInstruction(poInstruction(_instructionCount++, ptrType, 1 /* num elements */, -1, IR_ALLOCA), bb);
    return id;
}

int poEmitter::emitAlloca(const int type, const int varName, poBasicBlock* bb)
{
    const int ptrType = getPointerType(type);
    bb->addInstruction(poInstruction(varName, ptrType, 1 /* num elements */, -1, IR_ALLOCA));
    return varName;
}

int poEmitter::emitPtr(const int type, const int value, const int offset, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, value, -1, offset, IR_PTR), cfg.getLast());
    return id;
}

int poEmitter::emitPtr(const int type, const int value, const int dynamicOffset, const int offset, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, value, dynamicOffset, offset, IR_PTR), cfg.getLast());
    return id;
}

int poEmitter::emitElementPtr(const int type, const int value, const int element, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, value, element, IR_ELEMENT_PTR), cfg.getLast());
    return id;
}

int poEmitter::emitLoad(const int type, const int ptr, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, ptr, -1, IR_LOAD), cfg.getLast());
    return id;
}

int poEmitter::emitStore(const int type, const int ptr, const int value, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, ptr, value, IR_STORE), cfg.getLast());
    return id;
}

int poEmitter::emitBranch(const int branchType, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, 0, branchType, -1, IR_BR), cfg.getLast());
    return id;
}

int poEmitter::emitBranch(const int type, const int branchType, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, branchType, -1, IR_BR), cfg.getLast());
    return id;
}

int poEmitter::emitParam(const int type, const int paramIndex, poFlowGraph& cfg)
{
    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, type, paramIndex, -1, IR_PARAM), cfg.getLast());
    return id;
}

int poEmitter::emitStoreGlobal(const int type, const int value, const int globalId, poFlowGraph& cfg)
{
    const int store = _instructionCount++;
    emitInstruction(poInstruction(store, type, -1, value, globalId, IR_STORE_GLOBAL), cfg.getLast());
    return store;
}

int poEmitter::emitLoadGlobal(const int type, const int globalId, poFlowGraph& cfg)
{
    const int load = _instructionCount++;
    emitInstruction(poInstruction(load, type, -1, -1, globalId, IR_LOAD_GLOBAL), cfg.getLast());
    return load;
}

void poEmitter::reset()
{
    _types.clear();
    _qualifiers.clear();
    _arraySize.clear();
    _instructionCount = 0;
}

int poEmitter::addVariable(const int type, const int qualifier)
{
    const int id = _instructionCount++;
    _types.push_back(type);
    _qualifiers.push_back(qualifier);
    _arraySize.push_back(0);
    return id;
}

int poEmitter::addVariable(const int type, const int qualifier, const int arraySize)
{
    const int id = _instructionCount++;
    _types.push_back(type);
    _qualifiers.push_back(qualifier);
    _arraySize.push_back(arraySize);
    return id;
}

int poEmitter::getArrayType(const int baseType, const int arrayRank)
{
    return _module.getArrayType(baseType);
}

int poEmitter::getPointerType(const int baseType)
{
    if (baseType == -1)
    {
        return -1;
    }

    const int ptr = _module.getPointerType(baseType);
    if (ptr == -1)
    {
        const poType& type = _module.types()[baseType];

        const int id = int(_module.types().size());
        poType pointerType(id, baseType, type.name() + "*");
        pointerType.setPointer(true);
        pointerType.setSize(8);
        pointerType.setAlignment(8);
        _module.addType(pointerType);

        return id;
    }

    return ptr;
}

//===================
// CodeGenerator
//===================

poCodeGenerator::poCodeGenerator(poModule& module)
    :
    _module(module),
    _emitter(module),
    _returnInstruction(-1),
    _thisInstruction(-1),
    _arrayConstant(-1),
    _incrementConstant(-1),
    _elementPtr(-1),
    _preHeaderBB(nullptr),
    _headerBB(nullptr),
    _loopBodyBB(nullptr),
    _loopEndBB(nullptr),
    _pointerType(-1),
    _loopIndexLoadBody(-1),
    _loopIndexPtrBody(-1),
    _loopIndexVar(-1),
    _returnType(-1),
    _isError(false),
    _errorCol(0),
    _errorLine(0),
    _errorFile(0)
{
}

void poCodeGenerator::setError(const std::string& errorText)
{
    if (!_isError)
    {
        _errorText = errorText;
        _isError = true;
        _errorCol = -1;
        _errorLine = -1;
        _errorFile = -1;
    }
}

void poCodeGenerator::setError(const std::string& errorText, const poToken& token)
{
    if (!_isError)
    {
        _errorText = errorText;
        _isError = true;
        _errorCol = token.column();
        _errorLine = token.line();
        _errorFile = token.fileId();
    }
}

void poCodeGenerator::generate(const std::vector<poNode*>& ast)
{
    for (poNode* node : ast)
    {
        getModules(node);
    }

    for (auto& ns : _module.namespaces())
    {
        _namespace = ns.name();

        // Emit static functions
        for (const int staticFunction : ns.functions())
        {
            poFunction& function = _module.functions()[staticFunction];
            const std::string fullName = ns.name() + "::" + function.name();
            const auto& node = _functions.find(fullName);
            if (node != _functions.end() &&
                node->second->type() == poNodeType::FUNCTION)
            {
                _imports.clear();
                _imports.push_back(_namespace);
                const std::vector<poNode*> importNodes = _functionImports.find(fullName)->second;
                for (poNode* importNode : importNodes)
                {
                    _imports.push_back(importNode->token().string());
                }

                emitFunction(node->second, function);
            }
        }

        for (const int type : ns.types())
        {
            // Emit constructors
            const int numConstructors = int(_module.types()[type].constructors().size());
            for (int i = 0; i < numConstructors; i++)
            {
                const poType& typeData = _module.types()[type];
                const std::vector<poConstructor>& constructors = typeData.constructors();
                auto& constructor = constructors[i];
                const std::string fullName = ns.name() + "::" + typeData.name() + "::" + typeData.name();

                if (constructor.isDefault())
                {
                    _imports.clear();
                    emitDefaultConstructor(_module.functions()[constructor.id()], type);
                }
                else
                {
                    _imports.clear();
                    _imports.push_back(_namespace);
                    const std::vector<poNode*> importNodes = _functionImports.find(fullName)->second;
                    for (poNode* importNode : importNodes)
                    {
                        _imports.push_back(importNode->token().string());
                    }

                    const auto& node = _functions.find(fullName);
                    if (node != _functions.end() &&
                        node->second->type() == poNodeType::CONSTRUCTOR)
                    {
                        emitFunction(node->second, _module.functions()[constructor.id()]);
                    }
                }
            }

            // Emit member functions
            const int numMethods = int(_module.types()[type].functions().size());
            for (int i = 0; i < numMethods; i++)
            {
                const poType& typeData = _module.types()[type];
                const std::vector<poMemberFunction>& methods = typeData.functions();
                auto& method = methods[i];
                const std::string fullName = ns.name() + "::" + typeData.name() + "::" + method.name();
                const auto& node = _functions.find(fullName);
                if (node != _functions.end() &&
                    node->second->type() == poNodeType::FUNCTION)
                {
                    _imports.clear();
                    _imports.push_back(_namespace);
                    const std::vector<poNode*> importNodes = _functionImports.find(fullName)->second;
                    for (poNode* importNode : importNodes)
                    {
                        _imports.push_back(importNode->token().string());
                    }

                    emitFunction(node->second, _module.functions()[method.id()]);
                }
            }
        }
    }
}

int poCodeGenerator::getArrayType(const int baseType, const int arrayRank)
{
    return _module.getArrayType(baseType);
}

int poCodeGenerator::getPointerType(const int baseType)
{
    if (baseType == -1)
    {
        return -1;
    }

    const int ptr = _module.getPointerType(baseType);
    if (ptr == -1)
    {
        const poType& type = _module.types()[baseType];

        const int id = int(_module.types().size());
        poType pointerType(id, baseType, type.name() + "*");
        pointerType.setPointer(true);
        pointerType.setSize(8);
        pointerType.setAlignment(8);
        _module.addType(pointerType);

        return id;
    }

    return ptr;
}

int poCodeGenerator::getTypeSize(const int type)
{
    return _module.types()[type].size();
}

int poCodeGenerator::getType(const poToken& token)
{
    int type = TYPE_VOID;
    switch (token.token())
    {
    case poTokenType::I64_TYPE:
        type = TYPE_I64;
        break;
    case poTokenType::I32_TYPE:
        type = TYPE_I32;
        break;
    case poTokenType::I16_TYPE:
        type = TYPE_I16;
        break;
    case poTokenType::I8_TYPE:
        type = TYPE_I8;
        break;
    case poTokenType::U64_TYPE:
        type = TYPE_U64;
        break;
    case poTokenType::U32_TYPE:
        type = TYPE_U32;
        break;
    case poTokenType::U16_TYPE:
        type = TYPE_U16;
        break;
    case poTokenType::U8_TYPE:
        type = TYPE_U8;
        break;
    case poTokenType::F64_TYPE:
        type = TYPE_F64;
        break;
    case poTokenType::F32_TYPE:
        type = TYPE_F32;
        break;
    case poTokenType::VOID:
        type = TYPE_VOID;
        break;
    case poTokenType::BOOLEAN:
        type = TYPE_BOOLEAN;
        break;
    case poTokenType::OBJECT:
        type = TYPE_OBJECT;
        break;
    default:
        type = _module.getTypeFromName(token.string());
        break;
    }
    return type;
}

int poCodeGenerator::getVariable(const std::string& name)
{
    int var = -1;
    const auto& it = _variables.find(name);
    if (it != _variables.end())
    {
        var = it->second.id();
    }
    return var;
}

int poCodeGenerator::getOrAddVariable(const std::string& name, const int type, const int qualifiers, const int size)
{
    int var = -1;
    const auto& it = _variables.find(name);
    if (it != _variables.end())
    {
        var = it->second.id();
    }
    else
    {
        var = _emitter.addVariable(type, qualifiers, size);
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type, size)));
    }
    return var;
}

int poCodeGenerator::addVariable(const std::string& name, const int type, const int qualifiers, const int size)
{
    /* TODO: shadowing variables is broken? */

    int var = -1;
    const auto& it = _variables.find(name);
    if (it == _variables.end())
    {
        var = _emitter.addVariable(type, qualifiers, size);
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type, size)));
    }
    return var;
}

void poCodeGenerator::emitArrayCopy(const int src, const int dst, poFlowGraph& cfg, const int size)
{
    const poType srcType = _module.types()[_emitter.getType(src)];
    const poType dstType = _module.types()[_emitter.getType(dst)];
    assert(srcType.id() == dstType.id());
    assert(srcType.isArray());

    const poType baseType = _module.types()[srcType.baseType()];
    const int pointerType = getPointerType(srcType.id());

    for (int i = 0; i < size; i++)
    {
        const int ptr = _emitter.emitPtr(pointerType, src, i * baseType.size(), cfg);
        const int load = _emitter.emitLoad(baseType.id(), ptr, cfg);
        const int dstPtr = _emitter.emitPtr(pointerType, dst, i * baseType.size(), cfg);
        _emitter.emitStore(baseType.id(), dstPtr, load, cfg);
    }
}

void poCodeGenerator::emitCopy(const int src, const int dst, poFlowGraph& cfg)
{
    /* copy an object */

    const poType& srcType = _module.types()[_emitter.getType(src)];
    const poType& dstType = _module.types()[_emitter.getType(dst)];
    assert(srcType.id() == dstType.id());
    assert(srcType.isPointer());

    const poType& baseType = _module.types()[srcType.baseType()];
    const int numCopies = std::max(1, baseType.size() / 8);
    for (int i = 0; i < numCopies; i++)
    {
        const int ptrSrc = _emitter.emitPtr(srcType.id(), src, i * 8, cfg);
        const int loadVar = _emitter.emitLoad(TYPE_I64, ptrSrc, cfg);
        const int ptrDst = _emitter.emitPtr(dstType.id(), dst, i * 8, cfg);
        _emitter.emitStore(TYPE_I64, ptrDst, loadVar, cfg);
    }
}

void poCodeGenerator::emitConstructor(poFlowGraph& cfg)
{
    // This is called by emitFunction, we emit the constructor base operations here.

    poBasicBlock* bb = cfg.getLast();

    const int thisInstruction = emitLoadThis(cfg);

    const int thisType = _emitter.getType(thisInstruction);
    poType& thisTypeData = _module.types()[thisType];
    const int baseType = thisTypeData.baseType();
    const poType& baseTypeData = _module.types()[baseType];

    for (int i = 0; i < int(baseTypeData.fields().size()); i++)
    {
        // Emit call to constructors for arrays
        const poField& field = baseTypeData.fields()[i];
        const poType& fieldTypeData = _module.types()[field.type()];
        if (!fieldTypeData.isArray())
        {
            continue;
        }

        const poType& arrayBaseTypeData = _module.types()[fieldTypeData.baseType()];
        if (arrayBaseTypeData.constructors().size() == 0)
        {
            continue;
        }

        int constructorId = -1;
        for (const poConstructor& c : arrayBaseTypeData.constructors())
        {
            if (c.arguments().size() == 0)
            {
                constructorId = c.id();
            }
        }
        if (constructorId != -1)
        {
            const int memberId = _emitter.emitPtr(fieldTypeData.id(), _thisInstruction, field.offset(), cfg);
            emitLoopPreHeader(cfg, memberId, field.numElements());
            emitLoopHeader(cfg, memberId);

            const poType& baseTypeData = _module.types()[fieldTypeData.baseType()];
            const int memberPtr = getPointerType(fieldTypeData.baseType());
            const int symbolId = _module.addSymbol(baseTypeData.fullname() + "::" + baseTypeData.name());
            _emitter.emitCall(TYPE_VOID, 1, symbolId, cfg);
            _emitter.emitArg(memberPtr, _elementPtr, cfg);

            emitLoopEnd(cfg);
        }
        else
        {
            setError("No constructor with zero arguments found.");
            break;
        }
    }

    for (int i = 0; i < int(baseTypeData.fields().size()); i++)
    {
        const poField& field = baseTypeData.fields()[i];
        const poType& fieldTypeData = _module.types()[field.type()];
        if (fieldTypeData.constructors().size() == 0)
        {
            continue;
        }

        // Invoke the constructor for the field.
        int constructorId = -1;
        for (const poConstructor& c : fieldTypeData.constructors())
        {
            if (c.arguments().size() == 0)
            {
                constructorId = c.id();
            }
        }
        if (constructorId != -1)
        {
            const int memberPtr = getPointerType(field.type());
            const int symbolId = _module.addSymbol(fieldTypeData.fullname() + "::" + fieldTypeData.name());
            const int memberId = _emitter.emitPtr(memberPtr, _thisInstruction, field.offset(), cfg);
            _emitter.emitCall(TYPE_VOID, 1, symbolId, cfg);
            _emitter.emitArg(memberPtr, memberId, cfg);
        }
        else
        {
            setError("No constructor with zero arguments found.");
            break;
        }
    }
}

void poCodeGenerator::emitDefaultConstructor(poFunction& function, const int type)
{
    // Reset the variable emitter.
    _variables.clear();
    _emitter.reset();

    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    _returnInstruction = -1;
    _returnType = TYPE_VOID;

    const poType& typeData = _module.types()[type];
    const int ptrType = getPointerType(getPointerType(type));
    const int thisName = addVariable("this", ptrType, QUALIFIER_CONST, 8);
    _thisInstruction = thisName;

    // Emit the 'this' parameter
    emitParameter(ptrType, cfg, 0, _thisInstruction);

    emitConstructor(cfg);

    /* remove empty blocks before checking if for terminators */
    cfg.optimize();

    _emitter.emitReturn(cfg);
}

void poCodeGenerator::emitFunction(poNode* node, poFunction& function)
{
    // Reset the variable emitter.
    _variables.clear();
    _emitter.reset();

    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    assert(node->type() == poNodeType::FUNCTION ||
        node->type() == poNodeType::CONSTRUCTOR);
    int retType = TYPE_VOID;
    poListNode* list = static_cast<poListNode*>(node);
    poNode* body = nullptr;
    poNode* args = nullptr;
    poResolverNode* resolver = nullptr;
    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            // args
            args = child;
        }
        else if (child->type() == poNodeType::BODY)
        {
            // body
            body = child;
        }
        else if (child->type() == poNodeType::RETURN_TYPE)
        {
            // return type
            retType = getType(child->token());

            poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(child);
            if (returnTypeNode->child()->type() == poNodeType::POINTER)
            {
                retType = getPointerType(retType);
            }
        }
        else if (child->type() == poNodeType::RESOLVER)
        {
            assert(resolver == nullptr);
            resolver = static_cast<poResolverNode*>(child);
        }
    }

    _returnInstruction = -1;
    _thisInstruction = -1;
    _returnType = retType;

    if (resolver)
    {
        const std::vector<std::string>& path = resolver->path();
        if (path.size() > 0)
        {
            // Extract the class type and add it as the first argument
            const std::string& className = path[0];
            const int type = _module.getTypeFromName(className);
            if (type != -1)
            {
                const poType& typeData = _module.types()[type];
                const int ptrType = getPointerType(getPointerType(type));
                const int thisName = addVariable("this", ptrType, QUALIFIER_CONST, 8);
                if (thisName == EMIT_ERROR)
                {
                    setError("Redefinition of variable 'this'.", resolver->token());
                }
                else
                {
                    _thisInstruction = thisName;
                }
            }
            else
            {
                setError("Undefined type for 'this'.", resolver->token());
            }
        }
    }

    poType& type = _module.types()[retType];
    if (type.size() > 8)
    {
        // X64 calling convention the return value
        // should be passed in the first parameter
        _returnInstruction = _emitter.addVariable(getPointerType(retType), QUALIFIER_NONE);
    }

    if (args)
    {
        emitArgs(args, cfg);
    }

    if (node->type() == poNodeType::CONSTRUCTOR)
    {
        emitConstructor(cfg);
    }

    if (body)
    {
        emitBody(body, cfg, nullptr, nullptr);
    }

    /* remove empty blocks before checking if for terminators */
    cfg.optimize();

    // Insert a terminator instruction if required.
    bool hasTerminator = false;
    poBasicBlock* last = cfg.getLast();
    if (last->numInstructions() > 0)
    {
        const int lastCode = last->instructions()[last->numInstructions() - 1].code();
        hasTerminator = (lastCode == IR_RETURN);
    }

    if (!hasTerminator)
    {
        if (retType == TYPE_VOID)
        {
            _emitter.emitReturn(cfg);
        }
        else
        {
            // error: there isn't a return defined and the function doesn't return void
            // so we can't insert one
            setError("Function expects a return value, but function doesn't return one.", node->token());
        }
    }

    for (auto& variable : _variables)
    {
        function.addVariable(variable.second.id());
    }
}

void poCodeGenerator::emitBody(poNode* node, poFlowGraph& cfg, poBasicBlock* loopHeader, poBasicBlock* loopEnd)
{
    poBasicBlock* bb = cfg.getLast();
    poListNode* list = static_cast<poListNode*>(node);
    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::STATEMENT)
        {
            emitStatement(child, cfg);
        }
        else if (child->type() == poNodeType::WHILE)
        {
            poBasicBlock* endBB = new poBasicBlock(); // Create a basic block for *after* the while statement
            emitWhileStatement(child, cfg, endBB);
            cfg.addBasicBlock(endBB);
        }
        else if (child->type() == poNodeType::IF)
        {
            poBasicBlock* endBB = new poBasicBlock(); // Create a basic block for *after* the if statement
            emitIfStatement(child, cfg, endBB, loopHeader, loopEnd);
            cfg.addBasicBlock(endBB);
        }
        else if (child->type() == poNodeType::BREAK)
        {
            assert(loopEnd);

            _emitter.emitBranch(IR_JUMP_UNCONDITIONAL, cfg);
            cfg.getLast()->setBranch(loopEnd, true);
            loopEnd->addIncoming(cfg.getLast());
            cfg.addBasicBlock(new poBasicBlock());
        }
        else if (child->type() == poNodeType::CONTINUE)
        {
            assert(loopHeader);

            _emitter.emitBranch(IR_JUMP_UNCONDITIONAL, cfg);
            cfg.getLast()->setBranch(loopHeader, true);
            loopHeader->addIncoming(cfg.getLast());
            cfg.addBasicBlock(new poBasicBlock());
        }
        else
        {
            setError("Unexpected statement in body.", child->token());
        }
    }
}

void poCodeGenerator::emitParameter(const int type, poFlowGraph& cfg, const int paramIndex, const int varName)
{
    /*
     * type = the type of varName
     * bb = the basic block to emit the parameter into
     * paramIndex = the index of the parameter in the function call
     * varName = the variable name to store the parameter into
     */

    const poType& typeData = _module.types()[type];
    const int size = typeData.size();
    if (typeData.isPointer() &&
        _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
    {
        const poType& baseType = _module.types()[typeData.baseType()];

        if (baseType.size() <= 8)
        {
            // X86_64 calling convention is to pass this in via a register

            const int param = _emitter.emitParam(type, paramIndex, cfg);
            _emitter.emitAlloca(type, varName, cfg.getLast());
            const int pointerType = _emitter.getType(varName);
            const int ptr = _emitter.emitPtr(pointerType, varName, 0, cfg);
            _emitter.emitStore(TYPE_I64, ptr, param, cfg);
        }
        else
        {
            // X86_64 calling convention this will be passed in as a pointer

            cfg.getLast()->addInstruction(poInstruction(varName, type, paramIndex, -1, IR_PARAM));
        }
    }
    else
    {
        /* We need to store this on the stack, in case it is referenced */
        /* TODO: the emitAlloca isn't working properly as varName is created with 'type' rather than the pointer version */

        const int baseType = typeData.baseType();
        const int param = _emitter.emitParam(baseType, paramIndex, cfg);
        _emitter.emitAlloca(baseType, varName, cfg.getLast());
        const int pointerType = _emitter.getType(varName);
        const int ptr = _emitter.emitPtr(pointerType, varName, 0, cfg);
        _emitter.emitStore(baseType, ptr, param, cfg);
    }
}

void poCodeGenerator::emitArgs(poNode* node, poFlowGraph& cfg)
{
    poBasicBlock* bb = cfg.getLast();
    poListNode* list = static_cast<poListNode*>(node);
    int16_t paramCount = 0;

    if (_returnInstruction > -1)
    {
        emitParameter(_emitter.getType(_returnInstruction), cfg, paramCount, _returnInstruction);
        paramCount++;
    }

    if (_thisInstruction > -1)
    {
        const int type = _emitter.getType(_thisInstruction);
        emitParameter(type, cfg, paramCount, _thisInstruction);
        paramCount++;
    }

    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::PARAMETER)
        {
            poUnaryNode* param = static_cast<poUnaryNode*>(child);
            std::string name = param->token().string();
            poNode* type = param->child();

            int code = getType(type->token());

            if (type->type() == poNodeType::POINTER)
            {
                poPointerNode* pointerNode = static_cast<poPointerNode*>(type);
                code = getPointerType(code);
            }

            const poType& baseType = _module.types()[code];

            const int pointerType = getPointerType(baseType.id());
            const int var = getOrAddVariable(name, pointerType, QUALIFIER_NONE, 1);
            emitParameter(pointerType, cfg, paramCount, var);

            paramCount++;
        }
    }
}

void poCodeGenerator::emitStatement(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* statement = static_cast<poUnaryNode*>(node);
    poNode* child = statement->child();
    if (child->type() == poNodeType::RETURN)
    {
        emitReturn(child, cfg);
    }
    else if (child->type() == poNodeType::DECL)
    {
        emitDecl(child, cfg);
    }
    else if (child->type() == poNodeType::ASSIGNMENT)
    {
        emitAssignment(child, cfg);
    }
    else if (child->type() == poNodeType::CALL)
    {
        if (emitCall(child, cfg, -1) == EMIT_ERROR)
        {
            setError("Error emitting call.", child->token());
        }
    }
    else if (child->type() == poNodeType::MEMBER_CALL)
    {
        if (emitMemberCall(child, cfg) == EMIT_ERROR)
        {
            setError("Error emitting call.", child->token());
        }
    }
    else if (child->type() == poNodeType::BODY)
    {
        emitBody(child, cfg, nullptr, nullptr);
    }
}

void poCodeGenerator::emitWhileStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB)
{
    poListNode* whileLoop = static_cast<poListNode*>(node);
    poNode* body = nullptr;
    poNode* expr = nullptr;
    poNode* stride = nullptr;
    for (poNode* child : whileLoop->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            body = child;
        }
        else if (child->type() == poNodeType::STRIDE)
        {
            stride = child;
        }
        else
        {
            expr = child;
        }
    }

    assert(body && expr);
    poBasicBlock* loopHeaderBB = new poBasicBlock();
    poBasicBlock* bodyBB = new poBasicBlock();
    cfg.addBasicBlock(loopHeaderBB);
    
    poBasicBlock* strideBB = nullptr;
    if (stride)
    {
        strideBB = new poBasicBlock();
    }

    _graph.buildGraph(expr);
    auto& root = _graph.root();
    emitIfExpression(root, cfg, bodyBB, endBB);
    cfg.addBasicBlock(bodyBB);
    if (strideBB)
    {
        emitBody(body, cfg, strideBB, endBB);
        cfg.addBasicBlock(strideBB);

        poUnaryNode* strideNode = static_cast<poUnaryNode*>(stride);
        assert(strideNode->child()->type() == poNodeType::STATEMENT);
        emitStatement(strideNode->child(), cfg);
    }
    else
    {
        emitBody(body, cfg, loopHeaderBB, endBB);
    }
    _emitter.emitBranch(IR_JUMP_UNCONDITIONAL, cfg);
    cfg.getLast()->setBranch(loopHeaderBB, true);
    loopHeaderBB->addIncoming(cfg.getLast());
}

void poCodeGenerator::emitIfStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB, poBasicBlock* loopHeader, poBasicBlock* loopEnd)
{
    poListNode* ifStatement = static_cast<poListNode*>(node);
    poListNode* body = nullptr;
    poUnaryNode* statement = nullptr;
    poUnaryNode* elseNode = nullptr;
    for (poNode* child : ifStatement->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            body = static_cast<poListNode*>(child);
        }
        else if (child->type() == poNodeType::STATEMENT)
        {
            statement = static_cast<poUnaryNode*>(child);
        }
        else if (child->type() == poNodeType::ELSE)
        {
            elseNode = static_cast<poUnaryNode*>(child);
        }
    }

    _graph.buildGraph(statement->child());

    auto& root = _graph.root();

    poBasicBlock* curBB = cfg.getLast(); // The current basic block we are working with.
    poBasicBlock* bodyBB = new poBasicBlock(); // Basic block for the body of the if statement

    if (elseNode)
    {
        poBasicBlock* elseBB = new poBasicBlock();
        emitIfExpression(root, cfg, bodyBB, elseBB);
        cfg.addBasicBlock(bodyBB);
        emitBody(body, cfg, loopHeader, loopEnd);
        _emitter.emitBranch(IR_JUMP_UNCONDITIONAL, cfg); /*unconditional branch*/
        cfg.getLast()->setBranch(endBB, true);
        endBB->addIncoming(cfg.getLast());
        cfg.addBasicBlock(elseBB);

        poNode* child = elseNode->child();
        if (child->type() == poNodeType::IF)
        {
            emitIfStatement(child, cfg, endBB, loopHeader, loopEnd);
        }
        else if (child->type() == poNodeType::BODY)
        {
            // Emit the body and fall through to the end bb.
            emitBody(child, cfg, loopHeader, loopEnd);
        }
    }
    else
    {
        emitIfExpression(root, cfg, bodyBB, endBB);
        cfg.addBasicBlock(bodyBB);
        emitBody(body, cfg, loopHeader, loopEnd);
    }
}

void poCodeGenerator::emitIfExpression(poConditionGraphNode& cgn, poFlowGraph& cfg, poBasicBlock* successBB, poBasicBlock* failBB)
{
    _graph.computePredecessors();

    for (int i = 0; i < _graph.numNodes(); i++)
    {
        auto& node = _graph.at(i);
        if (node.id() == _graph.fail())
        {
            node.setBasicBlock(failBB);
        }
        else if (node.id() == _graph.success())
        {
            node.setBasicBlock(successBB);
        }
        else if (node.id() != _graph.root().id())
        {
            node.setBasicBlock(new poBasicBlock());
        }
    }

    int id = _graph.root().id();
    while (id != -1)
    {
        auto& node = _graph.at(id);
        auto& fail = _graph.at(node.fail());
        auto& success = _graph.at(node.success());
        const int expr = emitExpr(node.node(), cfg);
        node.setEmitted(true);
        id = -1;
        const int type = cfg.getLast()->instructions()[cfg.getLast()->numInstructions() - 1].type();
        if (!fail.isEmitted() && checkPredecessors(_graph, fail) && fail.node())
        {
            id = fail.id();
            _emitter.emitBranch(type, getCompare(node.node()), cfg);
            cfg.getLast()->setBranch(success.getBasicBlock(), false);
            success.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(fail.getBasicBlock());
        }
        else if (!success.isEmitted() && checkPredecessors(_graph, success) && success.node())
        {
            id = success.id();
            _emitter.emitBranch(type, flipJump(getCompare(node.node())), cfg);
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(success.getBasicBlock());
        }
        else
        {
            /*we are the last node? branch to the fail state */
            _emitter.emitBranch(type, flipJump(getCompare(node.node())), cfg);
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
        }
    }
}

int poCodeGenerator::emitPassByValue(const int expr, poFlowGraph& cfg)
{
    const int type = _emitter.getType(expr);
    const poType& typeData = _module.types()[type];

    const int ptr = _emitter.emitPtr(typeData.id(), expr, 0, cfg);
    const int load = _emitter.emitLoad(TYPE_I64, ptr, cfg);

    return load; /* return the value loaded from the pointer */
}

int poCodeGenerator::emitPassByReference(const int expr, poFlowGraph& cfg)
{
    const int type = _emitter.getType(expr);
    const poType& typeData = _module.types()[type];
    const int varName = _emitter.emitAlloca(typeData.baseType(), cfg.getLast()); /* get a pointer to the stack */
    emitCopy(expr, varName, cfg);

    return varName; /* return the variable name that is a pointer to the stack */
}

int poCodeGenerator::emitMemberCall(poNode* node, poFlowGraph& cfg)
{
    poBinaryNode* memberCall = static_cast<poBinaryNode*>(node);
    assert(memberCall->type() == poNodeType::MEMBER_CALL);

    int expr = emitExpr(memberCall->left(), cfg);
    if (expr == EMIT_ERROR)
    {
        return EMIT_ERROR;
    }

    const int typeId = _emitter.getType(expr);
    const poType& typeData = _module.types()[typeId];
    const poType& baseTypeData = _module.types()[typeData.baseType()];
    if (baseTypeData.isPointer())
    {
        expr = _emitter.emitLoad(typeData.baseType(), expr, cfg);
    }

    return emitCall(memberCall->right(), cfg, expr);
}

int poCodeGenerator::emitNewArray(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* newNode = static_cast<poUnaryNode*>(node);
    poArrayNode* arrayNode = static_cast<poArrayNode*>(newNode->child());

    const int baseType = getType(arrayNode->token());

    poType& typeData = _module.types()[baseType];
    const int symbolMallocId = _module.addSymbol("std::malloc3");
    int symbolConstructorId = -1;
    bool isDefaultConstructor = false;
    for (const poConstructor& constructor : typeData.constructors())
    {
        if (constructor.arguments().size() == 0)
        {
            std::string fullName = typeData.fullname() + "::" + typeData.name();
            symbolConstructorId = _module.addSymbol(fullName);
            isDefaultConstructor = constructor.isDefault();
            break;
        }
    }

    const int sizeConstantInstruction = _emitter.emitConstant(uint64_t(typeData.size()), cfg);
    const int typeConstantInstruction = _emitter.emitConstant(uint64_t(baseType), cfg);
    const int flagsConstantInstruction = _emitter.emitConstant(uint64_t(0), cfg);
    const int numElements = emitExpr(arrayNode->child(), cfg);

    const int ptrType = getPointerType(baseType);
    const int newId = _emitter.emitCall(ptrType, 4, symbolMallocId, cfg);
    _emitter.emitArg(TYPE_U64, sizeConstantInstruction, cfg);
    _emitter.emitArg(TYPE_U64, numElements, cfg);
    _emitter.emitArg(TYPE_U64, typeConstantInstruction, cfg);
    _emitter.emitArg(TYPE_U64, flagsConstantInstruction, cfg);

    if (symbolConstructorId != -1)
    {
        emitLoopPreHeader(cfg, newId);
        emitLoopHeader(cfg, newId, numElements);

        _emitter.emitCall(TYPE_VOID, 1, symbolConstructorId, cfg);
        _emitter.emitArg(ptrType, _elementPtr, cfg);

        emitLoopEnd(cfg);
    }

    return newId;
}

int poCodeGenerator::emitNew(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* newNode = static_cast<poUnaryNode*>(node);
    if (newNode->child()->type() == poNodeType::DYNAMIC_ARRAY)
    {
        return emitNewArray(newNode, cfg);
    }

    poListNode* constructor = static_cast<poListNode*>(newNode->child());
    poListNode* call = nullptr;
    for (poNode* child : constructor->list())
    {
        if (child->type() == poNodeType::CALL)
        {
            call = static_cast<poListNode*>(child);
            break;
        }
    }

    if (call == nullptr)
    {
        setError("Constructor missing call node.", node->token());
        return EMIT_ERROR;
    }


    const std::string& name = constructor->token().string();
    const int type = _module.getTypeFromName(name);
    if (type == -1)
    {
        setError("Undefined type '" + name + "' for 'new' operator.", node->token());
        return EMIT_ERROR;
    }

    poType& typeData = _module.types()[type];
    const int symbolMallocId = _module.addSymbol("std::malloc2");
    int symbolConstructorId = -1;
    bool isDefaultConstructor = false;
    const int numArgs = int(call->list().size());
    for (const poConstructor& constructor : typeData.constructors())
    {
        if (constructor.arguments().size() == numArgs)
        {
            std::string fullName = typeData.fullname() + "::" + typeData.name();
            symbolConstructorId = _module.addSymbol(fullName);
            isDefaultConstructor = constructor.isDefault();
            break;
        }
    }

    const int sizeConstantInstruction = _emitter.emitConstant(uint64_t(typeData.size()), cfg);
    const int typeConstantInstruction = _emitter.emitConstant(uint64_t(type), cfg);
    const int flagsConstantInstruction = _emitter.emitConstant(uint64_t(0), cfg);

    const int ptrType = getPointerType(type);
    const int newId = _emitter.emitCall(ptrType, 3, symbolMallocId, cfg);
    _emitter.emitArg(TYPE_U64, sizeConstantInstruction, cfg);
    _emitter.emitArg(TYPE_U64, typeConstantInstruction, cfg);
    _emitter.emitArg(TYPE_U64, flagsConstantInstruction, cfg);

    if (symbolConstructorId != -1)
    {
        // Call the constructor
        if (isDefaultConstructor)
        {
            _emitter.emitCall(TYPE_VOID, 1, symbolConstructorId, cfg);
            _emitter.emitArg(ptrType, newId, cfg);
        }
        else
        {
            emitCall(call, cfg, newId);
        }
    }

    return newId;
}

int poCodeGenerator::emitCall(poNode* node, poFlowGraph& cfg, const int instanceExpr)
{
    poListNode* call = static_cast<poListNode*>(node);

    std::vector<int> args;
    std::vector<int> argTypes;
    int returnType = 0;
    poListNode* argsNode = nullptr;
    
    int instance = instanceExpr;
    std::string name = call->token().string();
    if (instanceExpr != -1)
    {
        poType& typeData = _module.types()[_emitter.getType(instanceExpr)];
        assert(typeData.isPointer());
        poType& baseType = _module.types()[typeData.baseType()];
        name = baseType.name() + "::" + name;
    }
    else if (_thisInstruction != -1)
    {
        const int type = _emitter.getType(_thisInstruction);
        poType& typeData = _module.types()[type];
        poType& ptrTypeData = _module.types()[typeData.baseType()];
        poType& classType = _module.types()[ptrTypeData.baseType()];
        for (const poMemberFunction& method : classType.functions())
        {
            if (method.name() != name)
            {
                continue;
            }

            name = classType.name() + "::" + name;
            instance = emitLoadThis(cfg);
            break;
        }
    } 

    std::string fullName = name;
    poListNode* functionNode = nullptr;
    for (const std::string& import : _imports)
    {
        const auto& it = _functions.find(import + "::" + name);
        if (it != _functions.end())
        {
            if (it->second->type() == poNodeType::EXTERN)
            {
                poUnaryNode* externNode = static_cast<poUnaryNode*>(it->second);
                functionNode = static_cast<poListNode*>(externNode->child());
            }
            else
            {
                functionNode = static_cast<poListNode*>(it->second);
            }

            fullName = import + "::" + name;
            break;
        }
    }

    if (functionNode == nullptr)
    {
        setError("Undefined function '" + name + "'.", node->token());
        return EMIT_ERROR;
    }

    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            argsNode = static_cast<poListNode*>(child);
        }
        else if (child->type() == poNodeType::RETURN_TYPE)
        {
            returnType = getType(child->token());

            poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(child);
            if (returnTypeNode->child()->type() == poNodeType::POINTER)
            {
                poPointerNode* pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
                returnType = getPointerType(returnType);
            }
        }
    }

    assert(argsNode);
    assert(returnType != -1);

    /* Extract the expected types from the AST */

    for (int i = 0; i < int(argsNode->list().size()); i++)
    {
        poNode* node = argsNode->list()[i];
        poUnaryNode* param = static_cast<poUnaryNode*>(node);
        int type = getType(param->child()->token());
        if (param->child()->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(param->child());
            type = getPointerType(type);
        }

        argTypes.push_back(type);
    }

    /* Determine what should be passed into the function */

    for (int i = 0; i < int(call->list().size()); i++)
    {
        poNode* child = call->list()[i];
        const int expr = emitExpr(child, cfg);
        if (expr == EMIT_ERROR)
        {
            return EMIT_ERROR;
        }

        const int targetType = argTypes[i];
        const poType& targetTypeData = _module.types()[targetType];

        const int type = _emitter.getType(expr);
        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() &&
            _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT &&
            !targetTypeData.isPointer())
        {
            // X86_64 calling convention: structs smaller/equal to 8 bytes can be passed by value
            // Greater than that pass in via a pointer to a IR_ALLOCA

            assert(typeData.isPointer());

            const poType& baseType = _module.types()[typeData.baseType()];
            if (baseType.size() <= 8)
            {
                args.push_back(emitPassByValue(expr, cfg));
            }
            else
            {
                args.push_back(emitPassByReference(expr, cfg));
            }
        }
        else
        {
            args.push_back(expr);
        }
    }

    int numArgs = int(call->list().size());

    /* Insert instance param */
    if (instance != -1)
    {
        args.insert(args.begin(), instance);
        argTypes.insert(argTypes.begin(), _emitter.getType(instance));
        numArgs++;
    }

    /* Generate the call instruction also handle the return variable */

    int retVariable = -1;
    const poType& typeData = _module.types()[returnType];
    if (typeData.baseType() == TYPE_OBJECT)
    {
        if (typeData.size() > 8)
        {
            // X86_64 calling convention: the return value is passed in as the first arg
            numArgs++;

            const int returnPtr = getPointerType(returnType);
            retVariable = _emitter.emitAlloca(returnType, cfg.getLast());

            const int symbol = _module.addSymbol(fullName);
            _emitter.emitCall(TYPE_VOID, numArgs, symbol, cfg);

            _emitter.emitArg(returnPtr, retVariable, cfg);
        }
        else
        {
            const int symbol = _module.addSymbol(fullName);
            retVariable = _emitter.emitCall(TYPE_I64, numArgs, symbol, cfg);
        }
    }
    else
    {
        const int symbol = _module.addSymbol(fullName);
        retVariable = _emitter.emitCall(returnType, numArgs, symbol, cfg);
    }

    /* Generate the param instructions */

    for (int i = 0; i < int(args.size()); i++)
    {
        const int type = argTypes[i];
        const poType& typeData = _module.types()[type];
        if (typeData.baseType() == TYPE_OBJECT)
        {
            if (typeData.size() > 8)
            {
                // X86_64 pass in via a pointer

                const int pointerType = getPointerType(type);
                _emitter.emitArg(pointerType, args[i], cfg);
            }
            else
            {
                // X86_64 - it fits in a register (packed into a I64)

                _emitter.emitArg(TYPE_I64, args[i], cfg);
            }
        }
        else
        {
            _emitter.emitArg(type, args[i], cfg);
        }
    }

    /* Complete any further handling of the return value */

    if (typeData.baseType() == TYPE_OBJECT && typeData.size() <= 8)
    {
        // X86_64 - we need to load the value into a struct

        const int stackAlloc = _emitter.emitAlloca(typeData.id(), cfg.getLast());
        const int pointerType = getPointerType(typeData.id());

        const int ptr = _emitter.emitPtr(pointerType, stackAlloc, 0, cfg);
        _emitter.emitStore(TYPE_I64, ptr, retVariable, cfg);

        return stackAlloc;
    }

    return retVariable;
}

void poCodeGenerator::emitLoopPreHeader(poFlowGraph& cfg, const int instanceExpr)
{
    // Generate a loop to call a function for each element in the array (generally the constructor)
    const int type = _emitter.getType(instanceExpr);
    const poType& typeData = _module.types()[type];
    assert(typeData.isArray() || typeData.isPointer());
    _pointerType = getPointerType(typeData.baseType());

    _preHeaderBB = cfg.getLast();
    _headerBB = new poBasicBlock();
    _loopBodyBB = new poBasicBlock();
    _loopEndBB = new poBasicBlock();
    _headerBB->setBranch(_loopEndBB, false);
    _loopEndBB->addIncoming(_headerBB);
    _loopBodyBB->setBranch(_headerBB, true);
    _headerBB->addIncoming(_loopBodyBB);
    _arrayConstant = -1;

    // Get the increment value
    _incrementConstant = _module.constants().getConstant(int64_t(1));
    if (_incrementConstant == -1)
    {
        _incrementConstant = _module.constants().addConstant(int64_t(1));
    }

    // Get the loop index default value
    const int zeroConstantId = _emitter.emitConstant(int64_t(0), cfg);

    // Emit pre-header
    _loopIndexVar = _emitter.emitAlloca(TYPE_I64, cfg.getLast());
    const int loopIndexPtrPreHeader = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    _emitter.emitStore(TYPE_I64, loopIndexPtrPreHeader, zeroConstantId, cfg);
}

void poCodeGenerator::emitLoopHeader(poFlowGraph& cfg, const int instanceExpr, const int loopSizeExpr)
{
    // Emit header
    cfg.addBasicBlock(_headerBB);

    const int loopIndexPtr = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    const int loopIndexLoad = _emitter.emitLoad(TYPE_I64, loopIndexPtr, cfg);
    const int cmpVar = _emitter.emitCmp(TYPE_I64, loopIndexLoad, loopSizeExpr, cfg);
    _emitter.emitBranch(TYPE_I64, IR_JUMP_GREATER_EQUALS, cfg);

    // Emit loop body
    cfg.addBasicBlock(_loopBodyBB);
    _loopIndexPtrBody = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    _loopIndexLoadBody = _emitter.emitLoad(TYPE_I64, _loopIndexPtrBody, cfg);

    _elementPtr = _emitter.emitElementPtr(_pointerType, instanceExpr, _loopIndexLoadBody, cfg);

}

void poCodeGenerator::emitLoopPreHeader(poFlowGraph& cfg, const int instanceExpr, const int64_t arraySize)
{

    // Generate a loop to call a function for each element in the array (generally the constructor)
    const int type = _emitter.getType(instanceExpr);
    const poType& typeData = _module.types()[type];
    assert(typeData.isArray());
    _pointerType = getPointerType(typeData.baseType());

    _preHeaderBB = cfg.getLast();
    _headerBB = new poBasicBlock();
    _loopBodyBB = new poBasicBlock();
    _loopEndBB = new poBasicBlock();
    _headerBB->setBranch(_loopEndBB, false);
    _loopEndBB->addIncoming(_headerBB);
    _loopBodyBB->setBranch(_headerBB, true);
    _headerBB->addIncoming(_loopBodyBB);

    // Get the size of the array
    _arrayConstant = _module.constants().getConstant(arraySize);
    if (_arrayConstant == -1)
    {
        _arrayConstant = _module.constants().addConstant(arraySize);
    }

    // Get the increment value
    _incrementConstant = _module.constants().getConstant(int64_t(1));
    if (_incrementConstant == -1)
    {
        _incrementConstant = _module.constants().addConstant(int64_t(1));
    }

    // Get the loop index default value
    const int zeroConstantId = _emitter.emitConstant(int64_t(0), cfg);

    // Emit pre-header
    _loopIndexVar = _emitter.emitAlloca(TYPE_I64, cfg.getLast());
    const int loopIndexPtrPreHeader = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    _emitter.emitStore(TYPE_I64, loopIndexPtrPreHeader, zeroConstantId, cfg);
}
void poCodeGenerator::emitLoopHeader(poFlowGraph& cfg, const int instanceExpr)
{
    // Emit header
    cfg.addBasicBlock(_headerBB);
    const int arraySizeVar = _emitter.emitConstant(TYPE_I64, _arrayConstant, cfg);

    const int loopIndexPtr = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    const int loopIndexLoad = _emitter.emitLoad(TYPE_I64, loopIndexPtr, cfg);
    const int cmpVar = _emitter.emitCmp(TYPE_I64, loopIndexLoad, arraySizeVar, cfg);
    _emitter.emitBranch(TYPE_I64, IR_JUMP_GREATER_EQUALS, cfg);

    // Emit loop body
    cfg.addBasicBlock(_loopBodyBB);
    _loopIndexPtrBody = _emitter.emitPtr(TYPE_I64, _loopIndexVar, 0, cfg);
    _loopIndexLoadBody = _emitter.emitLoad(TYPE_I64, _loopIndexPtrBody, cfg);

    _elementPtr = _emitter.emitElementPtr(_pointerType, instanceExpr, _loopIndexLoadBody, cfg);

}
void poCodeGenerator::emitLoopEnd(poFlowGraph& cfg)
{
    // Increment loop index
    const int loopIndexInc = _emitter.emitConstant(TYPE_I64, _incrementConstant, cfg);

    const int loopIndexNew = _emitter.emitAdd(TYPE_I64, _loopIndexLoadBody, loopIndexInc, cfg);
    _emitter.emitStore(TYPE_I64, _loopIndexPtrBody, loopIndexNew, cfg);
    _emitter.emitBranch(IR_JUMP_UNCONDITIONAL, cfg);

    // Emit loop end
    cfg.addBasicBlock(_loopEndBB);
}

void poCodeGenerator::emitCall(poNode* node, poFlowGraph& cfg, const int instanceExpr, const int64_t arraySize)
{
    emitLoopPreHeader(cfg, instanceExpr, arraySize);
    emitLoopHeader(cfg, instanceExpr);

    emitCall(node, cfg, _elementPtr);

    emitLoopEnd(cfg);
}

void poCodeGenerator::emitAssignment(poNode* node, poFlowGraph& cfg)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(node);
    const int right = emitExpr(assignment->right(), cfg);
    if (right == EMIT_ERROR)
    {
        // Error: expression could not be evaluated
        setError("Assignment expression could not be evaluated.", node->token());
        return;
    }

    const int type = _emitter.getType(right);
    assert(type != -1);

    if (assignment->left()->type() == poNodeType::VARIABLE)
    {
        const std::string& name = assignment->left()->token().string();
        const int variable = getVariable(name);

        if (variable == EMIT_ERROR)
        {
            for (int id = 0; id < int(_module.staticVariables().size()); id++)
            {
                const poStaticVariable& variable = _module.staticVariables()[id];
                if (variable.name() == name)
                {
                    _emitter.emitStoreGlobal(variable.type(), right, id, cfg);
                    return;
                }
            }
        }

        if (variable == EMIT_ERROR)
        {
            // Check if this is a member variable
            if (_thisInstruction != -1)
            {
                const int thisType = _emitter.getType(_thisInstruction);
                const poType& thisTypeData = _module.types()[thisType];
                if (thisTypeData.isPointer())
                {
                    const int ptrType = thisTypeData.baseType();
                    const int classType = _module.types()[ptrType].baseType();
                    const poType& classTypeData = _module.types()[classType];
                    int fieldOffset = 0;
                    int fieldType = -1;
                    for (const auto& member : classTypeData.fields())
                    {
                        if (member.name() == name)
                        {
                            fieldType = member.type();
                            fieldOffset = member.offset();
                            break;
                        }
                    }

                    if (fieldType != -1)
                    {
                        const int thisVar = emitLoadThis(cfg);
                        const int pointerType = getPointerType(fieldType);
                        const int ptr = _emitter.emitPtr(pointerType, thisVar, fieldOffset, cfg);
                        _emitter.emitStore(fieldType, ptr, right, cfg);
                        return;
                    }
                    else
                    {
                        setError("Undefined member variable '" + name + "'.", node->token());
                        return;
                    }
                }
            }
        }

        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() && _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT &&
            _emitter.getType(variable) == type)
        {
            emitCopy(right, variable, cfg);
        }
        else if (typeData.isArray())
        {
            const auto& it = _variables.find(assignment->left()->token().string());
            const int size = it->second.size();
            emitArrayCopy(right, variable, cfg, size);
        }
        else
        {
            const int pointerType = getPointerType(type);
            const int ptr = _emitter.emitPtr(pointerType, variable, 0, cfg);
            _emitter.emitStore(type, ptr, right, cfg);
        }
    }
    else if (assignment->left()->type() == poNodeType::ARRAY_ACCESSOR)
    {
        emitStoreArray(assignment->left(), cfg, right);
    }
    else if (assignment->left()->type() == poNodeType::POINTER)
    {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(assignment->left());

        const int variable = getVariable(pointerNode->token().string());

        const int pointerType = getPointerType(type);
        const int ptr = _emitter.emitPtr(pointerType, variable, 0, cfg);
        _emitter.emitStore(type, ptr, right, cfg);
    }
    else if (assignment->left()->type() == poNodeType::DEREFERENCE)
    {
        poUnaryNode* pointerNode = static_cast<poUnaryNode*>(assignment->left());

        const int variable = emitLoadVariable(pointerNode, cfg);
        
        auto& typeData = _module.types()[type];
        if (typeData.isPointer() && _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
        {
            emitCopy(right, variable, cfg);
        }
        else
        {
            const int pointerType = getPointerType(type);
            assert(_emitter.getType(variable) == pointerType);

            const int ptr = _emitter.emitPtr(pointerType, variable, 0, cfg);
            _emitter.emitStore(type, ptr, right, cfg);
        }
    }
    else if (assignment->left()->type() == poNodeType::DYNAMIC_ARRAY)
    {
        poUnaryNode* dynamicArrayNode = static_cast<poUnaryNode*>(assignment->left());
        assert(dynamicArrayNode->type() == poNodeType::DYNAMIC_ARRAY);

        poNode* variableNode = dynamicArrayNode->child();
        assert(variableNode->type() == poNodeType::VARIABLE);

        const int variable = getVariable(variableNode->token().string());
        const int pointerType = getPointerType(_emitter.getType(variable));

        const int ptr = _emitter.emitPtr(pointerType, variable, 0, cfg);
        _emitter.emitStore(type, ptr, right, cfg);
    }
    else
    {
        emitStoreMember(assignment->left(), cfg, right);
    }
}

void poCodeGenerator::emitReturn(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* returnNode = static_cast<poUnaryNode*>(node);
    if (returnNode->child())
    {
        const int expr = emitExpr(returnNode->child(), cfg);
        if (expr == EMIT_ERROR)
        {
            // Error: expression could not be evaluated
            setError("Return expression could not be evaluated.", node->token());
            return;
        }

        const int type = _emitter.getType(expr);
        const poType& typeData = _module.types()[type];
        if (_returnInstruction != -1)
        {
            const poType& baseType = _module.types()[typeData.baseType()];

            if (baseType.size() > 8)
            {
                // We need to return the value through the return variable instead of the return statement.
                // This is done by copying the data in 64-bit chunks.
                //
                emitCopy(expr, _returnInstruction, cfg);

                _emitter.emitReturn(cfg);
            }
        }
        else
        {
            bool isObject = false;
            poType& returnTypeData = _module.types()[_returnType];
            if (typeData.isPointer() && !returnTypeData.isPointer())
            {
                const poType& baseType = _module.types()[typeData.baseType()];
                isObject = (baseType.baseType() == TYPE_OBJECT);
            }

            if (isObject)
            {
                // We need to pack the struct data into a I64 and return this.

                const int ptr = _emitter.emitPtr(type, expr, 0, cfg);
                const int load = _emitter.emitLoad(TYPE_I64, ptr, cfg);
                _emitter.emitReturn(TYPE_I64, load, cfg);
            }
            else
            {
                // Return the value in the default manner.

                _emitter.emitReturn(type, expr, cfg);
            }
        }
    }
    else
    {
        _emitter.emitReturn(cfg);
    }
    cfg.addBasicBlock(new poBasicBlock());
}

void poCodeGenerator::emitDecl(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* declNode = static_cast<poUnaryNode*>(node);
    const int type = getType(declNode->token());
    if (declNode->child()->type() == poNodeType::ASSIGNMENT)
    {
        poBinaryNode* assignment = static_cast<poBinaryNode*>(declNode->child());
        if (assignment->left()->type() == poNodeType::POINTER)
        {
            /* We need to account for pointers */
            poPointerNode* pointerNode = static_cast<poPointerNode*>(assignment->left());
            poNode* variableNode = pointerNode->child();
            assert(variableNode->type() == poNodeType::VARIABLE);

            /* Emit an ALLOCA for the variable we are defining */
            const int baseType = getPointerType(type);
            const int ptrType = getPointerType(baseType);
            const int variable = getOrAddVariable(pointerNode->token().string(), ptrType, QUALIFIER_NONE, 1);
            _emitter.emitAlloca(baseType, variable, cfg.getLast());
        }
        else if (assignment->left()->type() == poNodeType::DYNAMIC_ARRAY)
        {
            poUnaryNode* dynamicArrayNode = static_cast<poUnaryNode*>(assignment->left());
            assert(dynamicArrayNode->type() == poNodeType::DYNAMIC_ARRAY);
            
            poNode* variableNode = dynamicArrayNode->child();
            assert(variableNode->type() == poNodeType::VARIABLE);

            const int pointerType = getPointerType(type);
            const int pointerType2 = getPointerType(pointerType);
            const int variable = getOrAddVariable(dynamicArrayNode->token().string(), pointerType2, QUALIFIER_NONE, 1);
            _emitter.emitAlloca(pointerType, variable, cfg.getLast());
        }
        else
        {
            assert(assignment->left()->type() == poNodeType::VARIABLE);

            /* Emit an ALLOCA for the variable we are defining */
            const int ptrType = getPointerType(type);
            const int variable = getOrAddVariable(assignment->left()->token().string(), ptrType, QUALIFIER_NONE, 1);
            _emitter.emitAlloca(type, variable, cfg.getLast());
        }

        emitAssignment(declNode->child(), cfg);
    }
    else if (declNode->child()->type() == poNodeType::POINTER)
    {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(declNode->child());
        assert(pointerNode->type() == poNodeType::POINTER);

        poNode* variable = pointerNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const std::string name = variable->token().string();
        const int pointerType = getPointerType(type); /* tODO: we need to get the pointer type with respect to the pointer count.. */
        const int storeType = getPointerType(pointerType);
        const int var = addVariable(name, storeType, QUALIFIER_NONE, 1);
        _emitter.emitAlloca(pointerType, var, cfg.getLast());
    }
    else if (declNode->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(declNode->child());
        assert(arrayNode->type() == poNodeType::ARRAY);

        poNode* variable = arrayNode->child();
        int pointerCount = 0;
        if (variable->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(variable);
            variable = pointerNode->child();
            pointerCount = pointerNode->count();
        }

        assert(variable->type() == poNodeType::VARIABLE);

        const int arrayType = getArrayType(getPointerType(type), 1);
        const std::string name = variable->token().string();
        const int var = addVariable(name, arrayType, QUALIFIER_NONE, int(arrayNode->arraySize()));

        cfg.getLast()->addInstruction(poInstruction(var, arrayType, int16_t(arrayNode->arraySize()) /* num elements */, -1, IR_ALLOCA));
    }
    else if (declNode->child()->type() == poNodeType::CONSTRUCTOR)
    {
        // Make a call to the constructor
        
        int var = -1;
        poNode* call = nullptr;
        poNode* variableNode = nullptr;
        poArrayNode* arrayNode = nullptr;
        poListNode* constructor = static_cast<poListNode*>(declNode->child());
        for (poNode* childNode : constructor->list())
        {
            if (childNode->type() == poNodeType::VARIABLE)
            {
                variableNode = static_cast<poUnaryNode*>(childNode);
            }
            else if (childNode->type() == poNodeType::ARRAY)
            {
                arrayNode = static_cast<poArrayNode*>(childNode);
                variableNode = arrayNode->child();
            }
            else if (childNode->type() == poNodeType::CALL)
            {
                call = childNode;
            }
        }

        if (variableNode == nullptr)
        {
            setError("Constructor missing variable name.", constructor->token());
            return;
        }

        // Declare the variable
        const std::string name = variableNode->token().string();

        int declType = type;
        if (variableNode->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(variableNode);
            variableNode = pointerNode->child();
            declType = getPointerType(type);
        }

        if (arrayNode)
        {
            const int arrayType = getArrayType(declType, 1);
            var = addVariable(name, arrayType, QUALIFIER_NONE, int(arrayNode->arraySize()));
            cfg.getLast()->addInstruction(poInstruction(var, arrayType, int16_t(arrayNode->arraySize()) /* num elements */, -1, IR_ALLOCA));
        }
        else
        {
            const int ptrType = getPointerType(declType);
            var = addVariable(name, ptrType, QUALIFIER_NONE, 1);
            _emitter.emitAlloca(declType, var, cfg.getLast());
        }

        assert(var != -1);
        const int type = _emitter.getType(var);
        poType& typeData = _module.types()[type];
        poType& baseType = _module.types()[typeData.baseType()];
        if (baseType.constructors().size() > 0)
        {
            if (arrayNode)
            {
                emitCall(call, cfg, var, arrayNode->arraySize());
            }
            else
            {
                emitCall(call, cfg, var);
            }
        }
        else if (!arrayNode)
        {
            emitDefaultValue(baseType.id(), cfg, type, var);
        }
    }
    else
    {
        poNode* variable = declNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const std::string name = variable->token().string();

        const int ptrType = getPointerType(type);
        const int var = addVariable(name, ptrType, QUALIFIER_NONE, 1);
        _emitter.emitAlloca(type, var, cfg.getLast());

        emitDefaultValue(type, cfg, ptrType, var);
    }
}

void poCodeGenerator::emitDefaultValue(const int type, poFlowGraph& cfg, const int ptrType, const int var)
{
    // Assign a default value
    poConstantPool& pool = _module.constants();
    int constant = -1;
    switch (type)
    {
    case TYPE_BOOLEAN:
    case TYPE_U8:
        constant = pool.getConstant((uint8_t)0);
        if (constant == -1) constant = pool.addConstant((uint8_t)0);
        break;
    case TYPE_U16:
        constant = pool.getConstant((uint16_t)0);
        if (constant == -1) constant = pool.addConstant((uint16_t)0);
        break;
    case TYPE_U32:
        constant = pool.getConstant((uint32_t)0);
        if (constant == -1) constant = pool.addConstant((uint32_t)0);
        break;
    case TYPE_U64:
        constant = pool.getConstant((uint64_t)0);
        if (constant == -1) constant = pool.addConstant((uint64_t)0);
        break;
    case TYPE_I64:
        constant = pool.getConstant((int64_t)0);
        if (constant == -1) constant = pool.addConstant((int64_t)0);
        break;
    case TYPE_I32:
        constant = pool.getConstant((int32_t)0);
        if (constant == -1) constant = pool.addConstant((int32_t)0);
        break;
    case TYPE_I16:
        constant = pool.getConstant((int16_t)0);
        if (constant == -1) constant = pool.addConstant((int16_t)0);
        break;
    case TYPE_I8:
        constant = pool.getConstant((int8_t)0);
        if (constant == -1) constant = pool.addConstant((int8_t)0);
        break;
    case TYPE_F32:
        constant = pool.getConstant(0.0f);
        if (constant == -1) constant = pool.addConstant(0.0f);
        break;
    case TYPE_F64:
        constant = pool.getConstant(0.0);
        if (constant == -1) constant = pool.addConstant(0.0);
        break;
    }

    if (constant != -1)
    {
        const int constantVar = _emitter.emitConstant(type, constant, cfg);
        const int ptr = _emitter.emitPtr(ptrType, var, 0, cfg);
        _emitter.emitStore(type, ptr, constantVar, cfg);
    }
}

int poCodeGenerator::emitExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = EMIT_ERROR;
    switch (node->type())
    {
    case poNodeType::ADD:
    case poNodeType::SUB:
    case poNodeType::MUL:
    case poNodeType::DIV:
    case poNodeType::MODULO:
    case poNodeType::LEFT_SHIFT:
    case poNodeType::RIGHT_SHIFT:
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
        instructionId = emitBinaryExpr(node, cfg);
        break;
    case poNodeType::CONSTANT:
        instructionId = emitConstantExpr(node, cfg);
        break;
    case poNodeType::NEW:
        instructionId = emitNew(node, cfg);
        break;
    case poNodeType::VARIABLE:
        instructionId = emitLoadVariable(node, cfg);
        break;
    case poNodeType::UNARY_SUB:
        instructionId = emitUnaryMinus(node, cfg);
        break;
    case poNodeType::CALL:
        instructionId = emitCall(node, cfg, -1);
        break;
    case poNodeType::MEMBER:
        instructionId = emitLoadMember(node, cfg);
        break;
    case poNodeType::MEMBER_CALL:
        instructionId = emitMemberCall(node, cfg);
        break;
    case poNodeType::ARRAY_ACCESSOR:
        instructionId = emitLoadArray(node, cfg);
        break;
    case poNodeType::DEREFERENCE:
        instructionId = emitDereference(node, cfg);
        break;
    case poNodeType::REFERENCE:
        instructionId = emitReference(node, cfg);
        break;
    case poNodeType::CAST:
        instructionId = emitCast(node, cfg);
        break;
    case poNodeType::NULLPTR:
        instructionId = emitNullptr(node, cfg);
        break;
    case poNodeType::SIZEOF:
        instructionId = emitSizeof(node, cfg);
        break;
    case poNodeType::RESOLVER:
        instructionId = emitResolver(node, cfg);
        break;
    }
    return instructionId;
}

int poCodeGenerator::emitNullptr(poNode* node, poFlowGraph& cfg)
{
    return _emitter.emitConstant(int64_t(0), cfg);
}

int poCodeGenerator::emitCast(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* castNode = static_cast<poUnaryNode*>(node);
    assert(castNode->child()->type() == poNodeType::TYPE);

    poUnaryNode* typeNode = static_cast<poUnaryNode*>(castNode->child());

    int dstType = getType(typeNode->token());
    if (typeNode->child()->type() == poNodeType::POINTER)
    {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(typeNode->child());
        dstType = getPointerType(dstType);
        typeNode = pointerNode;
    }
    else if (typeNode->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(typeNode->child());
        dstType = getArrayType(dstType, 1);
        typeNode = arrayNode;
    }

    const int expr = emitExpr(typeNode->child(), cfg);
    if (expr == EMIT_ERROR)
    {
        setError("Cast expression could not be evaluated.", typeNode->token());
        return EMIT_ERROR;
    }

    const int srcType = _emitter.getType(expr);

    if (srcType == dstType)
    {
        // No cast needed, return the expression as is.
        return expr;
    }

    const poType& dstTypeData = _module.types()[dstType];
    if (dstTypeData.isPointer())
    {
        poType& srcTypeData = _module.types()[srcType];
        if (srcTypeData.isArray())
        {
            const int ptr = _emitter.emitPtr(dstType, expr, 0, cfg);
            return _emitter.emitBitwiseCast(dstType, srcType, ptr, cfg);
        }

        return _emitter.emitBitwiseCast(dstType, srcType, expr, cfg);
    }
    else if (dstTypeData.isArray())
    {
        poType& srcTypeData = _module.types()[srcType];
        if (srcTypeData.isPointer())
        {
            const int pointerType = getPointerType(dstTypeData.baseType());
            return _emitter.emitBitwiseCast(pointerType, srcType, expr, cfg);
        }
    }

    std::vector<poOperator> operators;
    getOperators(dstType, operators);

    int type = srcType;
    while (type != -1)
    {
        for (const poOperator& op : operators)
        {
            if (op.getOperator() == poOperatorType::EXPLICIT_CAST && op.otherType() == type)
            {
                // We have a cast operator, use it.
                switch (op.flags())
                {
                case OPERATOR_FLAG_SIGN_EXTEND:
                    return _emitter.emitSignExtend(dstType, type, expr, cfg);
                case OPERATOR_FLAG_ZERO_EXTEND:
                    return _emitter.emitZeroExtend(dstType, type, expr, cfg);
                case OPERATOR_FLAG_CONVERT_TO_INTEGER:
                case OPERATOR_FLAG_CONVERT_TO_REAL:
                    return _emitter.emitConvert(dstType, type, expr, cfg);
                default:
                    break;
                }

                return _emitter.emitBitwiseCast(dstType, type, expr, cfg);
            }
        }

        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() || typeData.isArray())
        {
            type = -1;
            continue;
        }
        type = typeData.baseType();
    }

    setError("Invalid cast from type '" + _module.types()[srcType].name() + "' to type '" +
             _module.types()[dstType].name() + "'.",
        typeNode->token());
    return EMIT_ERROR;
}

void poCodeGenerator::getOperators(int type, std::vector<poOperator>& operators)
{
    int currentType = type;
    while (currentType != -1)
    {
        poType& typeData = _module.types()[currentType];
        for (const poOperator& op : typeData.operators())
        {
            operators.push_back(op);
        }
        if (!typeData.isPointer())
        {
            currentType = typeData.baseType();
        }
        else
        {
            currentType = -1;
        }
    }
}

int poCodeGenerator::emitReference(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* refNode = static_cast<poUnaryNode*>(node);
    poNode* child = static_cast<poNode*>(refNode->child());

    int expr = -1;
    if (child->type() == poNodeType::VARIABLE)
    {
        expr = getVariable(child->token().string());
    }
    else if (child->type() == poNodeType::ARRAY_ACCESSOR)
    {
        expr = emitLoadArray(child, cfg);
    }
    else if (child->type() == poNodeType::MEMBER)
    {
        expr = emitLoadMember(child, cfg);
    }

    if (expr == EMIT_ERROR)
    {
        setError("Reference failed, variable not found.", child->token());
        return EMIT_ERROR;
    }

    const int type = _emitter.getType(expr);
    const poType& typeData = _module.types()[type];
    if (!typeData.isPointer())
    {
        return EMIT_ERROR;
    }

    return expr;
}

int poCodeGenerator::emitNullCheck(poFlowGraph& cfg, const int expr)
{
    poBasicBlock* bb = cfg.getLast();

    // Check if the pointer is null

    const int id = _emitter.emitConstant(0, cfg);
    _emitter.emitCmp(TYPE_I64, expr, id, cfg);
    _emitter.emitBranch(TYPE_I64, IR_JUMP_NOT_EQUALS, cfg);

    poBasicBlock* panicBB = new poBasicBlock();
    cfg.addBasicBlock(panicBB);
    emitPanic(cfg);

    poBasicBlock* newBB = new poBasicBlock();
    cfg.addBasicBlock(newBB);

    bb->setBranch(newBB, false);
    newBB->addIncoming(bb);

    return 0;
}

int poCodeGenerator::emitDereference(poNode* node, poFlowGraph& cfg)
{
    int id = EMIT_ERROR;
    poUnaryNode* derefNode = static_cast<poUnaryNode*>(node);
    poNode* child = static_cast<poNode*>(derefNode->child());
    if (child->type() != poNodeType::VARIABLE)
    {
        setError("Dereference can only be applied to a variable.", child->token());
        return EMIT_ERROR;
    }

    const int expr = emitLoadVariable(child, cfg);
    if (expr == EMIT_ERROR)
    {
        setError("Dereference failed, variable not found.", child->token());
        return EMIT_ERROR;
    }

    emitNullCheck(cfg, expr);

    const int type = _emitter.getType(expr);
    const poType& typeData = _module.types()[type];
    if (typeData.isPointer())
    {
        // Dereference a pointer
        const int ptr = _emitter.emitPtr(type, expr, 0, cfg);
        id = _emitter.emitLoad(typeData.baseType(), ptr, cfg);
    }
    else
    {
        setError("Dereferencing a non-pointer type.", child->token());
    }
    return id;
}

int poCodeGenerator::emitSizeof(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* sizeofNode = static_cast<poUnaryNode*>(node);
    poNode* child = sizeofNode->child();
    int pointerCount = 0;
    if (child->type() == poNodeType::POINTER)
    {
        poPointerNode* ptr = static_cast<poPointerNode*>(child);
        child = ptr->child();
        pointerCount = ptr->count();
    }

    if (child->type() != poNodeType::TYPE)
    {
        setError("Sizeof can only be applied to a type.", node->token());
        return EMIT_ERROR;
    }

    int type = getType(child->token());
    if (pointerCount > 0)
    {
        type = getPointerType(type);
    }
    const poType& typeData = _module.types()[type];

    const uint64_t size = typeData.size();
    return _emitter.emitConstant(size, cfg);
}

int poCodeGenerator::emitLoadThis(poFlowGraph& cfg)
{
    const int thisType = _emitter.getType(_thisInstruction);
    const poType& thisTypeData = _module.types()[thisType];
    const poType& ptrType = _module.types()[thisTypeData.baseType()];

    const int ptr = _emitter.emitPtr(ptrType.id(), _thisInstruction, 0, cfg); /* result of IR_PTR */
    const int instructionId = _emitter.emitLoad(ptrType.id(), ptr, cfg);
    return instructionId;
}

int poCodeGenerator::emitLoadVariable(poNode* node, poFlowGraph& cfg)
{
    const std::string& name = node->token().string();
    const int varName = getVariable(name);
    if (varName == EMIT_ERROR)
    {
        if (_thisInstruction != EMIT_ERROR)
        {
            const int thisType = _emitter.getType(_thisInstruction);
            const poType& thisTypeData = _module.types()[thisType];
            if (thisTypeData.isPointer())
            {
                const poType& ptrType = _module.types()[thisTypeData.baseType()];
                const poType& baseType = _module.types()[ptrType.baseType()];

                int offset = 0;
                int fieldType = -1;
                for (const auto& field : baseType.fields())
                {
                    if (field.name() == name)
                    {
                        offset = field.offset();
                        fieldType = field.type();
                        break;
                    }
                }

                if (fieldType == -1)
                {
                    setError("Variable '" + name + "' not found.", node->token());
                    return EMIT_ERROR;
                }

                auto& fieldTypeData = _module.types()[fieldType];
                if (fieldTypeData.baseType() == TYPE_OBJECT)
                {
                    return _emitter.emitPtr(getPointerType(fieldType), _thisInstruction, offset, cfg);
                }

                if (fieldTypeData.isArray())
                {
                    return _emitter.emitPtr(getPointerType(fieldTypeData.baseType()), _thisInstruction, offset, cfg);
                }

                const int thisVar = emitLoadThis(cfg);
                const int ptr = _emitter.emitPtr(getPointerType(fieldType), thisVar, offset, cfg);
                return _emitter.emitLoad(fieldType, ptr, cfg);
            }
        }
    }

    if (varName == EMIT_ERROR)
    {
        for (int id = 0; id < int(_module.staticVariables().size()); id++)
        {
            const poStaticVariable& variable = _module.staticVariables()[id];
            if (variable.name() == name)
            {
                return _emitter.emitLoadGlobal(variable.type(), id, cfg);
            }
        }
    }

    const int type = _emitter.getType(varName);

    auto& typeData = _module.types()[type];
    if (typeData.isArray())
    {
        return varName; // If it's an array, we return the variable directly.
    }

    assert(typeData.isPointer());
    auto& baseType = _module.types()[typeData.baseType()];
    if (baseType.baseType() == TYPE_OBJECT)
    {
        return varName;
    }

    const int ptr = _emitter.emitPtr(type, varName, 0, cfg);
    return _emitter.emitLoad(typeData.baseType(), ptr, cfg);
}

void poCodeGenerator::emitStoreArray(poNode* node, poFlowGraph& cfg, const int id)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _emitter.getType(expr);
    const poType type = _module.types()[arrayType];

    if (type.isArray())
    {
        const int accessor = emitExpr(array->accessor(), cfg);
        const int pointerType = getPointerType(type.baseType());

        emitBoundsCheck(cfg, expr, accessor);

        const int ptr = _emitter.emitElementPtr(pointerType, expr, accessor, cfg);
        _emitter.emitStore(type.baseType(), ptr, id, cfg);
    }
    else
    {
        const int accessor = emitExpr(array->accessor(), cfg);

        const int ptr = _emitter.emitElementPtr(type.id(), expr, accessor, cfg);
        _emitter.emitStore(type.baseType(), ptr, id, cfg);
    }
}

int poCodeGenerator::emitPanic(poFlowGraph& cfg)
{
    const int symbolId = _module.addSymbol("std::panic");
    _emitter.emitCall(TYPE_VOID, 0, symbolId, cfg);

    return 0;    
}

int poCodeGenerator::emitBoundsCheck(poFlowGraph& cfg, const int var, const int accessor)
{
    const int arrayBounds = _emitter.getArraySize(var);
    poBasicBlock* bb = cfg.getLast();

    // Check upper bounds

    const int id = _emitter.emitConstant(int64_t(arrayBounds), cfg);
    _emitter.emitCmp(TYPE_I64, accessor, id, cfg);
    _emitter.emitBranch(TYPE_I64, IR_JUMP_LESS, cfg);

    poBasicBlock* panicBB = new poBasicBlock();
    cfg.addBasicBlock(panicBB);
    emitPanic(cfg);

    poBasicBlock* lowerBB = new poBasicBlock();
    cfg.addBasicBlock(lowerBB);

    bb->setBranch(lowerBB, false);
    lowerBB->addIncoming(bb);

    // Check lower bounds

    const int zeroId = _emitter.emitConstant(int64_t(0), cfg);
    _emitter.emitCmp(TYPE_I64, accessor, zeroId, cfg);
    _emitter.emitBranch(TYPE_I64, IR_JUMP_GREATER_EQUALS, cfg);

    poBasicBlock* panicBB2 = new poBasicBlock();
    cfg.addBasicBlock(panicBB2);
    emitPanic(cfg);

    poBasicBlock* newBB = new poBasicBlock();
    cfg.addBasicBlock(newBB);

    lowerBB->setBranch(newBB, false);
    newBB->addIncoming(lowerBB);

    return 0;
}

int poCodeGenerator::emitLoadArray(poNode* node, poFlowGraph& cfg)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _emitter.getType(expr);
    poType type = _module.types()[arrayType];

    if (type.isArray())
    {
        const int accessor = emitExpr(array->accessor(), cfg);
        const int pointerType = getPointerType(type.baseType());
        poType baseType = _module.types()[type.baseType()];

        emitBoundsCheck(cfg, expr, accessor);

        const int ptr = _emitter.emitElementPtr(pointerType, expr, accessor, cfg);

        if (!array->dereference() || baseType.baseType() == TYPE_OBJECT)
        {
            return ptr; // Return the pointer to the object
        }

        return _emitter.emitLoad(type.baseType(), ptr, cfg);
    }
    else if (type.isPointer())
    {
        const int accessor = emitExpr(array->accessor(), cfg);

        const int ptr = _emitter.emitElementPtr(type.id(), expr, accessor, cfg);

        poType baseType = _module.types()[type.baseType()];
        if (!array->dereference() || baseType.baseType() == TYPE_OBJECT)
        {
            return ptr; // Return the pointer to the object
        }

        return _emitter.emitLoad(type.baseType(), ptr, cfg);
    }

    return EMIT_ERROR;
}

void poCodeGenerator::emitStoreMember(poNode* node, poFlowGraph& cfg, const int id)
{
    int expr = EMIT_ERROR;
    std::vector<poNode*> stack = { node };

    poNode* child = static_cast<poUnaryNode*>(node)->child();
    while (child)
    {
        if (child->type() == poNodeType::MEMBER)
        {
            stack.push_back(child);
            child = static_cast<poUnaryNode*>(child)->child();
        }
        else if (child->type() == poNodeType::ARRAY_ACCESSOR)
        {
            stack.push_back(child);
            poArrayAccessor* accessor = static_cast<poArrayAccessor*>(child);
            child = accessor->child();
        }
        else
        {
            expr = emitExpr(child, cfg);
            child = nullptr;
        }
    }

    const poType& pointerType = _module.types()[_emitter.getType(expr)];
    assert(pointerType.isPointer() || pointerType.isArray());

    int fieldType = pointerType.baseType();
    int offset = 0;
    int dynamicOffsetIns = -1;

    for (int i = int(stack.size() - 1); i >= 0; i--)
    {
        poNode* member = stack[i];

        if (member->type() == poNodeType::ARRAY_ACCESSOR)
        {
            poArrayAccessor* accessor = static_cast<poArrayAccessor*>(member);
            const int accessorExpr = emitExpr(accessor->accessor(), cfg);
            if (accessorExpr == EMIT_ERROR)
            {
                setError("Array accessor expression could not be evaluated.", member->token());
                return;
            }

            poType& accessType = _module.types()[fieldType];
            const int accessorSize = _emitter.emitConstant(int64_t(accessType.size()), cfg);
            const int result = _emitter.emitMul(TYPE_I64, accessorExpr, accessorSize, cfg);
            if (dynamicOffsetIns == -1)
            {
                dynamicOffsetIns = result;
            }
            else
            {
                const int temp = _emitter.emitAdd(TYPE_I64, dynamicOffsetIns, result, cfg);
                dynamicOffsetIns = temp;
            }

            if (accessType.isPointer())
            {
                fieldType = accessType.baseType();
            }

            continue;
        }

        if (_module.types()[fieldType].isPointer() && i > 0)
        {
            /* We need to dereference the pointer first and reset the offset */

            const poType& type = _module.types()[fieldType];

            const int ptr = _emitter.emitPtr(type.id(), expr, offset, cfg); /* result of IR_PTR */
            expr = _emitter.emitLoad(type.baseType(), ptr, cfg); /* result of IR_LOAD */

            offset = 0;
            fieldType = type.baseType(); /* Fix the type used when dereferencing a pointer */
        }

        const int typeId = fieldType;
        const poType& type = _module.types()[typeId];
        int fieldOffset = 0;
        for (const poField field : type.fields())
        {
            if (field.name() == member->token().string())
            {
                fieldType = field.type();
                fieldOffset = field.offset();
                break;
            }
        }

        offset += fieldOffset;
    }

    const int storePtr = getPointerType(fieldType);

    if (dynamicOffsetIns == -1)
    {
        const int ptr = _emitter.emitPtr(storePtr, expr, offset, cfg);
        _emitter.emitStore(fieldType, ptr, id, cfg);
    }
    else
    {
        const int finalPtr = _emitter.emitPtr(storePtr, expr, dynamicOffsetIns, offset, cfg);
        _emitter.emitStore(fieldType, finalPtr, id, cfg);
    }
}

int poCodeGenerator::emitLoadMember(poNode* node, poFlowGraph& cfg)
{
    int expr = EMIT_ERROR;
    std::vector<poNode*> stack = { node };

    poNode* child = static_cast<poUnaryNode*>(node)->child();
    while (child)
    {
        if (child->type() == poNodeType::MEMBER)
        {
            stack.push_back(child);
            child = static_cast<poUnaryNode*>(child)->child();
        }
        else if (child->type() == poNodeType::ARRAY_ACCESSOR)
        {
            stack.push_back(child);
            poArrayAccessor* accessor = static_cast<poArrayAccessor*>(child);
            child = accessor->child();
        }
        else
        {
            expr = emitExpr(child, cfg);
            child = nullptr;
        }
    }

    const poType& pointerType = _module.types()[_emitter.getType(expr)];
    assert(pointerType.isPointer() || pointerType.isArray());

    int offset = 0;
    int fieldType = pointerType.baseType();
    int dynamicOffsetIns = -1;

    for (int i = int(stack.size() - 1); i >= 0; i--)
    {
        poNode* member = stack[i];

        if (member->type() == poNodeType::ARRAY_ACCESSOR)
        {
            poArrayAccessor* accessor = static_cast<poArrayAccessor*>(member);
            const int accessorExpr = emitExpr(accessor->accessor(), cfg);
            if (accessorExpr == EMIT_ERROR)
            {
                setError("Array accessor expression could not be evaluated.", accessor->token());
                return EMIT_ERROR;
            }
            poType& accessType = _module.types()[fieldType];

            const int accessorSize = _emitter.emitConstant(int64_t(accessType.size()), cfg);
            const int result = _emitter.emitMul(TYPE_I64, accessorExpr, accessorSize, cfg);
            if (dynamicOffsetIns == -1)
            {
                dynamicOffsetIns = result;
            }
            else
            {
                const int temp = _emitter.emitAdd(TYPE_I64, dynamicOffsetIns, result, cfg);
                dynamicOffsetIns = temp;
            }

            if (accessType.isPointer())
            {
                fieldType = accessType.baseType();
            }

            continue;
        }

        if (_module.types()[fieldType].isPointer() && i > 0)
        {
            /* We need to dereference the pointer first and reset the offset */

            const poType& type = _module.types()[fieldType];

            const int ptr = _emitter.emitPtr(type.id(), expr, offset, cfg);
            expr = _emitter.emitLoad(type.baseType(), ptr, cfg);

            offset = 0;
            fieldType = type.baseType(); /* Fix the type used when dereferencing a pointer */
        }

        const int typeId = fieldType;
        const poType& type = _module.types()[typeId];

        int fieldOffset = -1;
        assert(!type.isPointer());

        std::string fieldName = member->token().string();
        for (const poMemberProperty& property : type.properties())
        {
            if (property.name() == fieldName)
            {
                fieldName = property.backingFieldName();
                break;
            }
        }

        for (const poField& field : type.fields())
        {
            if (field.name() == fieldName)
            {
                fieldType = field.type();
                fieldOffset = field.offset();
                break;
            }
        }

        assert(fieldOffset != -1);

        offset += fieldOffset;
    }

    if (_module.types()[fieldType].isArray())
    {
        if (dynamicOffsetIns == -1)
        {
            const int baseType = _module.types()[fieldType].baseType();
            const int pointerType = getPointerType(baseType);
            return _emitter.emitPtr(pointerType, expr, offset, cfg);
        }
        else
        {
            const int baseType = _module.types()[fieldType].baseType();
            const int pointerType = getPointerType(baseType);
            return _emitter.emitPtr(pointerType, expr, dynamicOffsetIns, offset, cfg);
        }
    }

    if (dynamicOffsetIns == -1)
    {
        const int loadPtr = getPointerType(fieldType);
        const int ptr = _emitter.emitPtr(loadPtr, expr, offset, cfg);
        if (_module.types()[fieldType].baseType() == TYPE_OBJECT)
        {
            return ptr;
        }
        return _emitter.emitLoad(fieldType, ptr, cfg);
    }
    else
    {
        const int loadPtr = getPointerType(fieldType);
        const int finalPtr = _emitter.emitPtr(loadPtr, expr, dynamicOffsetIns, offset, cfg);
        if (_module.types()[fieldType].baseType() == TYPE_OBJECT)
        {
            return finalPtr;
        }
        return _emitter.emitLoad(fieldType, finalPtr, cfg);
    }
}

int poCodeGenerator::emitConstantExpr(poNode* node, poFlowGraph& cfg)
{
    poConstantNode* constant = static_cast<poConstantNode*>(node);
    poConstantPool& constants = _module.constants();
    int instructionId = EMIT_ERROR;
    switch (constant->type())
    {
    case TYPE_I64:
        instructionId = _emitter.emitConstant(constant->i64(), cfg);
        break;
    case TYPE_BOOLEAN:
    case TYPE_U8:
        instructionId = _emitter.emitConstant(constant->u8(), cfg);
        break;
    case TYPE_F64:
        instructionId = _emitter.emitConstant(constant->f64(), cfg);
        break;
    case TYPE_F32:
        instructionId = _emitter.emitConstant(constant->f32(), cfg);
        break;
    case TYPE_U64:
        instructionId = _emitter.emitConstant(constant->u64(), cfg);
        break;
    case TYPE_STRING:
        instructionId = _emitter.emitConstant(constant->token().string(), cfg);
        break;
    }
    return instructionId;
}

int poCodeGenerator::emitResolver(poNode* node, poFlowGraph& cfg)
{
    poResolverNode* resolver = static_cast<poResolverNode*>(node);
    int instructionId = EMIT_ERROR;
    int type = -1;
    for (const std::string& pathPart : resolver->path())
    {
        if (type == -1)
        {
            type = _module.getTypeFromName(pathPart);
        }
        else
        {
            poType& typeData = _module.types()[type];
            bool found = false;
            for (const poField& field : typeData.fields())
            {
                if (field.hasAttribute(poAttributes::STATIC) &&
                    typeData.baseType() == TYPE_ENUM &&
                    field.name() == pathPart)
                {
                    instructionId = _emitter.emitConstant(type, field.constantValue(), cfg);
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                setError("Resolver path part '" + pathPart + "' not found.", resolver->token());
                return EMIT_ERROR;
            }
        }
    }

    return instructionId;
}

int poCodeGenerator::emitUnaryMinus(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* unary = static_cast<poUnaryNode*>(node);
    const int expr = emitExpr(unary->child(), cfg);
    const int type = _emitter.getType(expr);
    return _emitter.emitUnaryMinus(type, expr, cfg);
}

int poCodeGenerator::emitBinaryExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = EMIT_ERROR;
    poBinaryNode* binary = static_cast<poBinaryNode*>(node);
    const int left = emitExpr(binary->left(), cfg);
    const int right = emitExpr(binary->right(), cfg);
    if (left == -1 || right == -1)
    {
        return EMIT_ERROR;
    }

    const int type = _emitter.getType(left);

    switch (node->type())
    {
    case poNodeType::ADD:
        instructionId = _emitter.emitAdd(type, left, right, cfg);
        break;
    case poNodeType::SUB:
        instructionId = _emitter.emitSub(type, left, right, cfg);
        break;
    case poNodeType::MUL:
        instructionId = _emitter.emitMul(type, left, right, cfg);
        break;
    case poNodeType::DIV:
        instructionId = _emitter.emitDiv(type, left, right, cfg);
        break;
    case poNodeType::LEFT_SHIFT:
        instructionId = _emitter.emitLeftShift(type, left, right, cfg);
        break;
    case poNodeType::RIGHT_SHIFT:
        instructionId = _emitter.emitRightShift(type, left, right, cfg);
        break;
    case poNodeType::MODULO:
        instructionId = _emitter.emitModulo(type, left, right, cfg);
        break;
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
        instructionId = _emitter.emitCmp(type, left, right, cfg);
        break;
    }
    return instructionId;
}

void poCodeGenerator::getModules(poNode* node)
{
    poListNode* module = static_cast<poListNode*>(node);
    assert(module->type() == poNodeType::MODULE);
    
    std::vector<poNode*> importNodes;
    for (poNode* child : module->list())
    {
        if (child->type() == poNodeType::IMPORT)
        {
            importNodes.push_back(child);
        }
    }

    for (poNode* child : module->list())
    {
        if (child->type() == poNodeType::NAMESPACE)
        {
            getNamespaces(child, importNodes);
        }
    }
}

void poCodeGenerator::getNamespaces(poNode* node, const std::vector<poNode*>& importNodes)
{
    poListNode* namespaceNode = static_cast<poListNode*>(node);
    assert(namespaceNode->type() == poNodeType::NAMESPACE);
    const std::string namespaceName = namespaceNode->token().string();
    for (poNode* child : namespaceNode->list())
    {
        poListNode* functionNode = static_cast<poListNode*>(child);
        if (child->type() == poNodeType::EXTERN)
        {
            functionNode = static_cast<poListNode*>(static_cast<poUnaryNode*>(child)->child());
        }

        if (functionNode->type() != poNodeType::FUNCTION &&
            functionNode->type() != poNodeType::CONSTRUCTOR)
        {
            continue;
        }

        poResolverNode* resolver = nullptr;
        for (poNode* funcChild : functionNode->list())
        {
            if (funcChild->type() == poNodeType::RESOLVER)
            {
                resolver = static_cast<poResolverNode*>(funcChild);
                break;
            }
        }

        std::string fullName;
        if (resolver && resolver->path().size() > 0)
        {
            fullName = namespaceName + "::" + resolver->path()[0] + "::" + child->token().string();
        }
        else
        {
            fullName = namespaceName + "::" + child->token().string();
        }

        if (child->type() == poNodeType::FUNCTION ||
            child->type() == poNodeType::CONSTRUCTOR)
        {
            _functions.insert(std::pair<std::string, poNode*>(fullName, child));
            _functionImports.insert(std::pair<std::string, std::vector<poNode*>>(fullName, importNodes));
        }
        else if (child->type() == poNodeType::EXTERN)
        {
            _functions.insert(std::pair<std::string, poNode*>(fullName, child));
        }
    }
}


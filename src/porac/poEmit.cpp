#include "poEmit.h"
#include "poAST.h"
#include "poModule.h"
#include "poType.h"
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
// CodeGenerator
//===================

poCodeGenerator::poCodeGenerator(poModule& module)
    :
    _module(module),
    _instructionCount(0),
    _returnInstruction(-1),
    _isError(false)
{
}

void poCodeGenerator::setError(const std::string& errorText)
{
    if (!_isError)
    {
        _errorText = errorText;
        _isError = true;
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
        for (auto& staticFunction : ns.functions())
        {
            const auto& node = _functions.find(staticFunction.name());
            if (node != _functions.end() &&
                node->second->type() == poNodeType::FUNCTION)
            {
                emitFunction(node->second, staticFunction);
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
        type = TYPE_U8; // Boolean is represented as a single byte
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
        _types.push_back(type);
        _qualifiers.push_back(qualifiers);
        var = _instructionCount++;
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
        _types.push_back(type);
        _qualifiers.push_back(qualifiers);
        var = _instructionCount++;
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type, size)));
    }
    return var;
}

void poCodeGenerator::emitInstruction(const poInstruction& instruction, poBasicBlock* bb)
{
    bb->addInstruction(instruction);
    _types.push_back(instruction.type());
    _qualifiers.push_back(QUALIFIER_NONE);
}

int poCodeGenerator::emitAlloca(const int type, poBasicBlock* bb)
{
    const int id = _instructionCount;
    const int ptrType = getPointerType(type);
    emitInstruction(poInstruction(_instructionCount++, ptrType, 1 /* num elements */, -1, IR_ALLOCA), bb);
    return id;
}

int poCodeGenerator::emitAlloca(const int type, const int varName, poBasicBlock* bb)
{
    const int ptrType = getPointerType(type);
    bb->addInstruction(poInstruction(varName, ptrType, 1 /* num elements */, -1, IR_ALLOCA));
    return varName;
}

void poCodeGenerator::emitArrayCopy(const int src, const int dst, poBasicBlock* bb, const int size)
{
    const poType& srcType = _module.types()[_types[src]];
    const poType& dstType = _module.types()[_types[dst]];
    assert(srcType.id() == dstType.id());
    assert(srcType.isArray());

    const poType& baseType = _module.types()[srcType.baseType()];
    const int pointerType = getPointerType(srcType.id());

    for (int i = 0; i < size; i++)
    {
        const int ptr = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, pointerType, src, -1, i * baseType.size(), IR_PTR), bb);

        const int load = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, baseType.id(), ptr, -1, IR_LOAD), bb);

        const int dstPtr = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, pointerType, dst, -1, i * baseType.size(), IR_PTR), bb);

        emitInstruction(poInstruction(_instructionCount++, baseType.id(), dstPtr, load, IR_STORE), bb);
    }
}

void poCodeGenerator::emitCopy(const int src, const int dst, poBasicBlock* bb)
{
    /* copy an object */

    const poType& srcType = _module.types()[_types[src]];
    const poType& dstType = _module.types()[_types[dst]];
    assert(srcType.id() == dstType.id());
    assert(srcType.isPointer());

    const poType& baseType = _module.types()[srcType.baseType()];
    const int numCopies = baseType.size() / 8;
    for (int i = 0; i < numCopies; i++)
    {
        const int ptrSrc = _instructionCount++;
        emitInstruction(poInstruction(ptrSrc, srcType.id(), src, -1, i * 8, IR_PTR), bb);

        const int loadVar = _instructionCount++;
        emitInstruction(poInstruction(loadVar, TYPE_I64, ptrSrc, -1, IR_LOAD), bb);

        const int ptrDst = _instructionCount++;
        emitInstruction(poInstruction(ptrDst, dstType.id(), dst, -1, i * 8, IR_PTR), bb);

        const int store = _instructionCount++;
        emitInstruction(poInstruction(store, TYPE_I64, ptrDst, loadVar, IR_STORE), bb);
    }
}

void poCodeGenerator::emitFunction(poNode* node, poFunction& function)
{
    // Reset the variable emitter.
    _variables.clear();
    _types.clear();
    _instructionCount = 0;

    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    assert(node->type() == poNodeType::FUNCTION);
    int retType = TYPE_VOID;
    poListNode* list = static_cast<poListNode*>(node);
    poNode* body = nullptr;
    poNode* args = nullptr;
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
    }

    _returnInstruction = -1;

    poType& type = _module.types()[retType];
    if (type.size() > 8)
    {
        // X64 calling convention the return value
        // should be passed in the first parameter
        _returnInstruction = _instructionCount++;
        _types.push_back(getPointerType(retType));
        _qualifiers.push_back(QUALIFIER_NONE);
    }

    if (args)
    {
        emitArgs(args, cfg);
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
            emitInstruction(poInstruction(_instructionCount++, TYPE_VOID, -1, -1, IR_RETURN), last);
        }
        else
        {
            // error: there isn't a return defined and the function doesn't return void
            // so we can't insert one
            setError("Function expects a return value, but function doesn't return one.");
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

            emitInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR), cfg.getLast());
            cfg.getLast()->setBranch(loopEnd, true);
            loopEnd->addIncoming(cfg.getLast());
            cfg.addBasicBlock(new poBasicBlock());
        }
        else if (child->type() == poNodeType::CONTINUE)
        {
            assert(loopHeader);

            emitInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR), cfg.getLast());
            cfg.getLast()->setBranch(loopHeader, true);
            loopHeader->addIncoming(cfg.getLast());
            cfg.addBasicBlock(new poBasicBlock());
        }
        else
        {
            setError("Unexpected statement in body.");
        }
    }
}

void poCodeGenerator::emitParameter(const int type, poBasicBlock* bb, const int paramIndex, const int varName)
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
            // X86_64 calling convection is to pass this in via a register

            const int param = _instructionCount++;
            emitInstruction(poInstruction(param, type, paramIndex, -1, IR_PARAM), bb);

            emitAlloca(type, varName, bb);

            const int pointerType = _types[varName];
            const int ptr = _instructionCount++;
            emitInstruction(poInstruction(ptr, pointerType, varName, -1, 0, IR_PTR), bb);

            emitInstruction(poInstruction(_instructionCount++, TYPE_I64, ptr, param, IR_STORE), bb);
        }
        else
        {
            // X86_64 calling convention this will be passed in as a pointer

            bb->addInstruction(poInstruction(varName, type, paramIndex, -1, IR_PARAM));
        }
    }
    else
    {
        /* We need to store this on the stack, in case it is referenced */
        /* TODO: the emitAlloca isn't working properly as varName is created with 'type' rather than the pointer version */

        const int baseType = typeData.baseType();
        const int param = _instructionCount++;
        emitInstruction(poInstruction(param, baseType, paramIndex, -1, IR_PARAM), bb);

        emitAlloca(baseType, varName, bb);

        const int pointerType = _types[varName];
        const int ptr = _instructionCount++;
        emitInstruction(poInstruction(ptr, pointerType, varName, -1, 0, IR_PTR), bb);

        emitInstruction(poInstruction(_instructionCount++, baseType, ptr, param, IR_STORE), bb);
    }
}

void poCodeGenerator::emitArgs(poNode* node, poFlowGraph& cfg)
{
    poBasicBlock* bb = cfg.getLast();
    poListNode* list = static_cast<poListNode*>(node);
    int16_t paramCount = 0;

    if (_returnInstruction > -1)
    {
        emitParameter(_types[_returnInstruction], bb, paramCount, _returnInstruction);
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
            emitParameter(pointerType, bb, paramCount, var);

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
        emitCall(child, cfg);
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
    emitInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR), cfg.getLast());
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
        emitInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR), cfg.getLast()); /*unconditional branch*/
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
        int expr = emitExpr(node.node(), cfg);
        node.setEmitted(true);
        id = -1;
        const int type = cfg.getLast()->instructions()[cfg.getLast()->numInstructions() - 1].type();
        if (!fail.isEmitted() && checkPredecessors(_graph, fail) && fail.node())
        {
            id = fail.id();
            emitInstruction(poInstruction(_instructionCount++, type, getCompare(node.node()), -1, IR_BR), cfg.getLast());
            cfg.getLast()->setBranch(success.getBasicBlock(), false);
            success.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(fail.getBasicBlock());
        }
        else if (!success.isEmitted() && checkPredecessors(_graph, success) && success.node())
        {
            id = success.id();
            emitInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR), cfg.getLast());
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(success.getBasicBlock());
        }
        else
        {
            /*we are the last node? branch to the fail state */
            emitInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR), cfg.getLast());
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
        }
    }
}

int poCodeGenerator::emitPassByValue(const int expr, poFlowGraph& cfg)
{
    const int type = _types[expr];
    const poType& typeData = _module.types()[type];

    const int ptr = _instructionCount++;
    emitInstruction(poInstruction(ptr, typeData.id(), expr, -1, IR_PTR), cfg.getLast());

    const int load = _instructionCount++;
    emitInstruction(poInstruction(load, TYPE_I64, ptr, -1, IR_LOAD), cfg.getLast());

    return load; /* return the value loaded from the pointer */
}

int poCodeGenerator::emitPassByReference(const int expr, poFlowGraph& cfg)
{
    const int type = _types[expr];
    const poType& typeData = _module.types()[type];
    const int varName = emitAlloca(typeData.baseType(), cfg.getLast()); /* get a pointer to the stack */
    emitCopy(expr, varName, cfg.getLast());

    return varName; /* return the variable name that is a pointer to the stack */
}

int poCodeGenerator::emitCall(poNode* node, poFlowGraph& cfg)
{
    poListNode* call = static_cast<poListNode*>(node);

    std::vector<int> args;
    std::vector<int> argTypes;
    int returnType = 0;
    const std::string& name = call->token().string();
    poListNode* argsNode = nullptr;
    poListNode* functionNode = static_cast<poListNode*>(_functions[name]);
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

        const int type = _types[expr];
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

    /* Generate the call instruction also handle the return variable */

    const int retVariable = _instructionCount;
    const poType& typeData = _module.types()[returnType];
    if (typeData.baseType() == TYPE_OBJECT)
    {
        if (typeData.size() > 8)
        {
            // X86_64 calling convention: the return value is passed in as the first arg
            numArgs++;

            const int returnPtr = getPointerType(returnType);
            emitAlloca(returnType, retVariable, cfg.getLast());
            _types.push_back(returnPtr);
            _instructionCount++;

            const int symbol = _module.addSymbol(name);
            emitInstruction(poInstruction(_instructionCount++, TYPE_VOID, numArgs, symbol, IR_CALL), cfg.getLast());

            emitInstruction(poInstruction(_instructionCount++, returnPtr, retVariable, -1, IR_ARG), cfg.getLast());
        }
        else
        {
            const int symbol = _module.addSymbol(name);
            emitInstruction(poInstruction(_instructionCount++, TYPE_I64, numArgs, symbol, IR_CALL), cfg.getLast());
        }
    }
    else
    {
        const int symbol = _module.addSymbol(name);
        emitInstruction(poInstruction(_instructionCount++, returnType, numArgs, symbol, IR_CALL), cfg.getLast());
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
                emitInstruction(poInstruction(_instructionCount++, pointerType, args[i], -1, IR_ARG), cfg.getLast());
            }
            else
            {
                // X86_64 - it fits in a register (packed into a I64)

                emitInstruction(poInstruction(_instructionCount++, TYPE_I64, args[i], -1, IR_ARG), cfg.getLast());
            }
        }
        else
        {
            emitInstruction(poInstruction(_instructionCount++, type, args[i], -1, IR_ARG), cfg.getLast());
        }
    }

    /* Complete any further handling of the return value */

    if (typeData.baseType() == TYPE_OBJECT && typeData.size() <= 8)
    {
        // X86_64 - we need to load the value into a struct

        const int stackAlloc = emitAlloca(typeData.id(), cfg.getLast());
        const int pointerType = getPointerType(typeData.id());

        const int ptr = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, pointerType, stackAlloc, -1, IR_PTR), cfg.getLast());

        emitInstruction(poInstruction(_instructionCount++, TYPE_I64, ptr, retVariable, IR_STORE), cfg.getLast());

        return stackAlloc;
    }

    return retVariable;
}

void poCodeGenerator::emitAssignment(poNode* node, poFlowGraph& cfg)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(node);
    const int right = emitExpr(assignment->right(), cfg);
    if (right == EMIT_ERROR)
    {
        // Error: expression could not be evaluated
        setError("Assignment expression could not be evaluated.");
        return;
    }

    const int type = _types[right];
    assert(type != -1);

    if (assignment->left()->type() == poNodeType::VARIABLE)
    {
        const int variable = getVariable(assignment->left()->token().string());

        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() && _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
        {
            emitCopy(right, variable, cfg.getLast());
        }
        else if (typeData.isArray())
        {
            const auto& it = _variables.find(assignment->left()->token().string());
            const int size = it->second.size();
            emitArrayCopy(right, variable, cfg.getLast(), size);
        }
        else
        {
            const int pointerType = getPointerType(type);
            const int ptr = _instructionCount++;
            emitInstruction(poInstruction(ptr, pointerType, variable, -1, 0, IR_PTR), cfg.getLast());
            emitInstruction(poInstruction(_instructionCount++, type, ptr, right, IR_STORE), cfg.getLast());
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
        const int ptr = _instructionCount++;
        emitInstruction(poInstruction(ptr, pointerType, variable, -1, 0, IR_PTR), cfg.getLast());
        emitInstruction(poInstruction(_instructionCount++, type, ptr, right, IR_STORE), cfg.getLast());
    }
    else if (assignment->left()->type() == poNodeType::DEREFERENCE)
    {
        poUnaryNode* pointerNode = static_cast<poUnaryNode*>(assignment->left());

        const int variable = emitLoadVariable(pointerNode, cfg);
        
        auto& typeData = _module.types()[type];
        if (typeData.isPointer() && _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
        {
            emitCopy(right, variable, cfg.getLast());
        }
        else
        {
            const int pointerType = getPointerType(type);
            assert(_types[variable] == pointerType);

            const int ptr = _instructionCount++;
            emitInstruction(poInstruction(ptr, pointerType, variable, -1, 0, IR_PTR), cfg.getLast());
            emitInstruction(poInstruction(_instructionCount++, type, ptr, right, IR_STORE), cfg.getLast());
        }
    }
    else if (assignment->left()->type() == poNodeType::DYNAMIC_ARRAY)
    {
        poUnaryNode* dynamicArrayNode = static_cast<poUnaryNode*>(assignment->left());
        assert(dynamicArrayNode->type() == poNodeType::DYNAMIC_ARRAY);

        poNode* variableNode = dynamicArrayNode->child();
        assert(variableNode->type() == poNodeType::VARIABLE);

        const int variable = getVariable(variableNode->token().string());
        const int pointerType = getPointerType(_types[variable]);
        const int ptr = _instructionCount++;

        emitInstruction(poInstruction(ptr, pointerType, variable, -1, 0, IR_PTR), cfg.getLast());
        emitInstruction(poInstruction(_instructionCount++, type, ptr, right, IR_STORE), cfg.getLast());
    }
    else
    {
        emitStoreMember(assignment, cfg, right);
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
            setError("Return expression could not be evaluated.");
            return;
        }

        poBasicBlock* bb = cfg.getLast();
        const int type = _types[expr];
        const poType& typeData = _module.types()[type];
        if (_returnInstruction != -1)
        {
            const poType& baseType = _module.types()[typeData.baseType()];

            if (baseType.size() > 8)
            {
                // We need to return the value through the return variable instead of the return statement.
                // This is done by copying the data in 64-bit chunks.
                //
                emitCopy(expr, _returnInstruction, bb);

                emitInstruction(poInstruction(_instructionCount++, 0, -1, -1, IR_RETURN), bb);
            }
        }
        else
        {
            if (typeData.isPointer())
            {
                // We need to pack the struct data into a I64 and return this.

                const int ptr = _instructionCount++;
                emitInstruction(poInstruction(ptr, type, expr, -1, IR_PTR), bb);

                const int load = _instructionCount++;
                emitInstruction(poInstruction(load, TYPE_I64, ptr, -1, IR_LOAD), bb);

                emitInstruction(poInstruction(_instructionCount++, TYPE_I64, load, -1, IR_RETURN), bb);
            }
            else
            {
                // Return the value in the default manner.

                emitInstruction(poInstruction(_instructionCount++, type, expr, -1, IR_RETURN), bb);
            }
        }
    }
    else
    {
        emitInstruction(poInstruction(_instructionCount++, 0, -1, -1, IR_RETURN), cfg.getLast());
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
            emitAlloca(baseType, variable, cfg.getLast());
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
            emitAlloca(pointerType, variable, cfg.getLast());
        }
        else
        {
            assert(assignment->left()->type() == poNodeType::VARIABLE);

            /* Emit an ALLOCA for the variable we are defining */
            const int ptrType = getPointerType(type);
            const int variable = getOrAddVariable(assignment->left()->token().string(), ptrType, QUALIFIER_NONE, 1);
            emitAlloca(type, variable, cfg.getLast());
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
        addVariable(name, pointerType, QUALIFIER_NONE, 1);
    }
    else if (declNode->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(declNode->child());
        assert(arrayNode->type() == poNodeType::ARRAY);

        poNode* variable = arrayNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const int arrayType = getArrayType(type, 1);
        const std::string name = variable->token().string();
        const int var = addVariable(name, arrayType, QUALIFIER_NONE, int(arrayNode->arraySize()));

        cfg.getLast()->addInstruction(poInstruction(var, arrayType, int16_t(arrayNode->arraySize()) /* num elements */, -1, IR_ALLOCA));
    }
    else
    {
        poNode* variable = declNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const std::string name = variable->token().string();

        const int ptrType = getPointerType(type);
        const int var = addVariable(name, ptrType, QUALIFIER_NONE, 1);
        emitAlloca(type, var, cfg.getLast());

        // Assign a default value
        poConstantPool& pool = _module.constants();
        int constant = -1;
        switch (type)
        {
        case TYPE_BOOLEAN:
        case TYPE_U8:
            constant = pool.getConstant((uint8_t)0);
            break;
        case TYPE_U16:
            constant = pool.getConstant((uint16_t)0);
            break;
        case TYPE_U32:
            constant = pool.getConstant((uint32_t)0);
            break;
        case TYPE_U64:
            constant = pool.getConstant((uint64_t)0);
            break;
        case TYPE_I64:
            constant = pool.getConstant((int64_t)0);
            break;
        case TYPE_I32:
            constant = pool.getConstant((int32_t)0);
            break;
        case TYPE_I16:
            constant = pool.getConstant((int16_t)0);
            break;
        case TYPE_I8:
            constant = pool.getConstant((int8_t)0);
            break;
        case TYPE_F32:
            constant = pool.getConstant(0.0f);
            break;
        case TYPE_F64:
            constant = pool.getConstant(0.0);
            break;
        }

        if (constant != -1)
        {
            const int constantVar = _instructionCount++;
            emitInstruction(poInstruction(constantVar, type, constant, IR_CONSTANT), cfg.getLast());

            const int ptr = _instructionCount++;
            emitInstruction(poInstruction(ptr, ptrType, var, -1, 0, IR_PTR), cfg.getLast());

            emitInstruction(poInstruction(_instructionCount++, type, ptr, constantVar, IR_STORE), cfg.getLast());
        }
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
    case poNodeType::VARIABLE:
        instructionId = emitLoadVariable(node, cfg);
        break;
    case poNodeType::UNARY_SUB:
        instructionId = emitUnaryMinus(node, cfg);
        break;
    case poNodeType::CALL:
        instructionId = emitCall(node, cfg);
        break;
    case poNodeType::MEMBER:
        instructionId = emitLoadMember(node, cfg);
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
    }
    return instructionId;
}

int poCodeGenerator::emitNullptr(poNode* node, poFlowGraph& cfg)
{
    emitInstruction(poInstruction(_instructionCount++, TYPE_I64, _module.constants().getConstant((int64_t)0), IR_CONSTANT), cfg.getLast());
    return _instructionCount - 1;
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
        setError("Cast expression could not be evaluated.");
        return EMIT_ERROR;
    }

    const int srcType = _types[expr];

    if (srcType == dstType)
    {
        // No cast needed, return the expression as is.
        return expr;
    }

    const poType& dstTypeData = _module.types()[dstType];
    if (dstTypeData.isPointer())
    {
        const int castId = _instructionCount++;
        emitInstruction(poInstruction(castId, dstType, expr, -1, srcType, IR_BITWISE_CAST), cfg.getLast());
        return castId;
    }
    else if (dstTypeData.isArray())
    {
        poType& srcTypeData = _module.types()[srcType];
        if (srcTypeData.isPointer())
        {
            const int pointerType = getPointerType(dstTypeData.baseType());
            const int castId = _instructionCount++;
            emitInstruction(poInstruction(castId, pointerType, expr, -1, srcType, IR_BITWISE_CAST), cfg.getLast());
            return castId;
        }
    }

    for (const poOperator& op : dstTypeData.operators())
    {
        if (op.getOperator() == poOperatorType::EXPLICIT_CAST && op.otherType() == srcType)
        {
            // We have a cast operator, use it.
            int operand = IR_BITWISE_CAST;
            switch (op.flags())
            {
            case OPERATOR_FLAG_SIGN_EXTEND:
                operand = IR_SIGN_EXTEND;
                break;
            case OPERATOR_FLAG_ZERO_EXTEND:
                operand = IR_ZERO_EXTEND;
                break;
            case OPERATOR_FLAG_CONVERT_TO_INTEGER:
            case OPERATOR_FLAG_CONVERT_TO_REAL:
                operand = IR_CONVERT;
                break;
            default:
                break;
            }

            const int castId = _instructionCount++;
            emitInstruction(poInstruction(castId, dstType, expr, -1, srcType, operand), cfg.getLast());
            return castId;
        }
    }

    return EMIT_ERROR;
}

int poCodeGenerator::emitReference(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* refNode = static_cast<poUnaryNode*>(node);
    poNode* child = static_cast<poNode*>(refNode->child());
    if (child->type() != poNodeType::VARIABLE)
    {
        setError("Reference can only be applied to a variable.");
        return EMIT_ERROR;
    }

    const int expr = getVariable(child->token().string());
    if (expr == EMIT_ERROR)
    {
        setError("Reference failed, variable not found.");
        return EMIT_ERROR;
    }

    const int type = _types[expr];
    const poType& typeData = _module.types()[type];
    if (!typeData.isPointer())
    {
        return EMIT_ERROR;
    }

    return expr;
}

int poCodeGenerator::emitDereference(poNode* node, poFlowGraph& cfg)
{
    int id = EMIT_ERROR;
    poUnaryNode* derefNode = static_cast<poUnaryNode*>(node);
    poNode* child = static_cast<poNode*>(derefNode->child());
    if (child->type() != poNodeType::VARIABLE)
    {
        setError("Dereference can only be applied to a variable.");
        return EMIT_ERROR;
    }

    const int expr = emitLoadVariable(child, cfg);
    if (expr == EMIT_ERROR)
    {
        setError("Dereference failed, variable not found.");
        return EMIT_ERROR;
    }

    const int type = _types[expr];
    const poType& typeData = _module.types()[type];
    if (typeData.isPointer())
    {
        // Dereference a pointer
        const int ptr = _instructionCount++;
        emitInstruction(poInstruction(ptr, type, expr, -1, IR_PTR), cfg.getLast());
        id = _instructionCount++;
        emitInstruction(poInstruction(id, typeData.baseType(), ptr, -1, IR_LOAD), cfg.getLast());
    }
    else
    {
        setError("Dereferencing a non-pointer type.");
    }
    return id;
}

int poCodeGenerator::emitSizeof(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* sizeofNode = static_cast<poUnaryNode*>(node);
    poNode* child = sizeofNode->child();
    if (child->type() != poNodeType::TYPE)
    {
        setError("Sizeof can only be applied to a type.");
        return EMIT_ERROR;
    }

    const int type = getType(child->token());
    const poType& typeData = _module.types()[type];

    const uint64_t size = typeData.size();
    poConstantPool& pool = _module.constants();
    int constantId = pool.getConstant(size);
    if (constantId == -1)
    {
        constantId = pool.addConstant(size);
    }

    const int id = _instructionCount++;
    emitInstruction(poInstruction(id, TYPE_U64, int16_t(constantId), IR_CONSTANT), cfg.getLast());
    return id;
}

int poCodeGenerator::emitLoadVariable(poNode* node, poFlowGraph& cfg)
{
    const std::string& name = node->token().string();
    const int varName = getVariable(name);
    const int type = _types[varName];

    auto& typeData = _module.types()[type];
    if (typeData.isArray())
    {
        return varName; // If it's an array, we return the variable directly.
    }

    assert(typeData.isPointer());
    if (typeData.isPointer())
    {
        auto& baseType = _module.types()[typeData.baseType()];
        if (baseType.baseType() == TYPE_OBJECT)
        {
            return varName;
        }
    }

    poBasicBlock* bb = cfg.getLast();

    const int ptr = _instructionCount++;
    emitInstruction(poInstruction(ptr, type, varName, -1, IR_PTR), bb);
    const int load = _instructionCount++;
    emitInstruction(poInstruction(load, typeData.baseType(), ptr, -1, IR_LOAD), bb);
    return load;
}

void poCodeGenerator::emitStoreArray(poNode* node, poFlowGraph& cfg, const int id)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _types[expr];
    auto& type = _module.types()[arrayType];

    if (type.isArray())
    {
        const int accessor = emitExpr(array->accessor(), cfg);
        const int pointerType = getPointerType(arrayType);

        const int ptr = _instructionCount; /* result of IR_PTR */
        emitInstruction(poInstruction(_instructionCount++, pointerType, expr, accessor, IR_ELEMENT_PTR), cfg.getLast());

        emitInstruction(poInstruction(_instructionCount++, type.baseType(), ptr, id, IR_STORE), cfg.getLast());
    }
    else
    {
        const int accessor = emitExpr(array->accessor(), cfg);

        const int ptr = _instructionCount; /* result of IR_PTR */
        emitInstruction(poInstruction(_instructionCount++, type.baseType(), expr, accessor, IR_ELEMENT_PTR), cfg.getLast());

        emitInstruction(poInstruction(_instructionCount++, type.baseType(), ptr, id, IR_STORE), cfg.getLast());
    }
}

int poCodeGenerator::emitLoadArray(poNode* node, poFlowGraph& cfg)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _types[expr];
    auto& type = _module.types()[arrayType];

    if (type.isArray())
    {
        const int accessor = emitExpr(array->accessor(), cfg);
        const int pointerType = getPointerType(arrayType);

        const int ptr = _instructionCount; /* result of IR_ELEMENT_PTR */
        emitInstruction(poInstruction(_instructionCount++, pointerType, expr, accessor, IR_ELEMENT_PTR), cfg.getLast());

        const int load = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, arrayType, ptr, -1, IR_LOAD), cfg.getLast());

        return load;
    }
    else if (type.isPointer())
    {
        const int accessor = emitExpr(array->accessor(), cfg);

        const int ptr = _instructionCount; /* result of IR_ELEMENT_PTR */
        emitInstruction(poInstruction(_instructionCount++, type.baseType(), expr, accessor, IR_ELEMENT_PTR), cfg.getLast());

        const int load = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type.baseType(), ptr, -1, IR_LOAD), cfg.getLast());

        return load;
    }

    return EMIT_ERROR;
}

void poCodeGenerator::emitStoreMember(poNode* node, poFlowGraph& cfg, const int id)
{
    poUnaryNode* member = static_cast<poUnaryNode*>(node);

    int expr = EMIT_ERROR;
    std::vector<poUnaryNode*> stack = { member };
    while (member)
    {
        poNode* child = member->child();
        if (child->type() == poNodeType::MEMBER)
        {
            member = static_cast<poUnaryNode*>(child);
            stack.push_back(member);
        }
        else
        {
            expr = emitExpr(child, cfg);
            member = nullptr;
        }
    }

    const poType& pointerType = _module.types()[_types[expr]];
    assert(pointerType.isPointer());

    int fieldType = pointerType.baseType();
    int offset = 0;

    for (int i = int(stack.size() - 1); i >= 0; i--)
    {
        poNode* member = stack[i];

        if (_module.types()[fieldType].isPointer())
        {
            /* We need to dereference the pointer first and reset the offset */

            const poType& type = _module.types()[fieldType];
            const int ptr = _instructionCount; /* result of IR_PTR */
            emitInstruction(poInstruction(_instructionCount++, type.id(), expr, -1, offset, IR_PTR), cfg.getLast());

            expr = _instructionCount; /* result of IR_LOAD */
            emitInstruction(poInstruction(_instructionCount++, type.baseType(), ptr, -1, IR_LOAD), cfg.getLast());
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
    const int ptr = _instructionCount; /* result of IR_PTR */
    emitInstruction(poInstruction(_instructionCount++, storePtr, expr, -1, offset, IR_PTR), cfg.getLast());

    emitInstruction(poInstruction(_instructionCount++, fieldType, ptr, id, IR_STORE), cfg.getLast());
}

int poCodeGenerator::emitLoadMember(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* member = static_cast<poUnaryNode*>(node);

    int expr = EMIT_ERROR;
    std::vector<poUnaryNode*> stack = { member };
    while (member)
    {
        poNode* child = member->child();
        if (child->type() == poNodeType::MEMBER)
        {
            member = static_cast<poUnaryNode*>(child);
            stack.push_back(member);
        }
        else
        {
            expr = emitExpr(child, cfg);
            member = nullptr;
        }
    }
    
    const poType& pointerType = _module.types()[_types[expr]];
    assert(pointerType.isPointer());

    int offset = 0;
    int fieldType = pointerType.baseType();

    for (int i = int(stack.size() - 1); i >= 0; i--)
    {
        poNode* member = stack[i];

        if (_module.types()[fieldType].isPointer())
        {
            /* We need to dereference the pointer first and reset the offset */

            const poType& type = _module.types()[fieldType];
            const int ptr = _instructionCount; /* result of IR_PTR */
            emitInstruction(poInstruction(_instructionCount++, type.id(), expr, -1, offset, IR_PTR), cfg.getLast());

            expr = _instructionCount; /* result of IR_LOAD */
            emitInstruction(poInstruction(_instructionCount++, type.baseType(), ptr, -1, IR_LOAD), cfg.getLast());
            offset = 0;
            fieldType = type.baseType(); /* Fix the type used when dereferencing a pointer */
        }

        const int typeId = fieldType;
        const poType& type = _module.types()[typeId];
      
        int fieldOffset = 0;
        assert(!type.isPointer());

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
    const int ptr = _instructionCount; /* result of IR_PTR */
    emitInstruction(poInstruction(_instructionCount++, storePtr, expr, -1, offset, IR_PTR), cfg.getLast());

    const int instructionId = _instructionCount;
    emitInstruction(poInstruction(_instructionCount++, fieldType, ptr, -1, IR_LOAD), cfg.getLast());
    return instructionId;
}

int poCodeGenerator::emitConstantExpr(poNode* node, poFlowGraph& cfg)
{
    poConstantNode* constant = static_cast<poConstantNode*>(node);
    poConstantPool& constants = _module.constants();
    int id = -1;
    int instructionId = EMIT_ERROR;
    switch (constant->type())
    {
    case TYPE_I64:
        id = constants.getConstant(constant->i64());
        if (id == -1) { id = constants.addConstant(constant->i64()); }
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, TYPE_I64, id, IR_CONSTANT), cfg.getLast());
        break;
    case TYPE_BOOLEAN:
    case TYPE_U8:
        id = constants.getConstant(constant->u8());
        if (id == -1) { id = constants.addConstant(constant->u8()); }
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, TYPE_U8, id, IR_CONSTANT), cfg.getLast());
        break;
    case TYPE_F64:
        id = constants.getConstant(constant->f64());
        if (id == -1) { id = constants.addConstant(constant->f64()); }
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, TYPE_F64, id, IR_CONSTANT), cfg.getLast());
        break;
    case TYPE_F32:
        id = constants.getConstant(constant->f32());
        if (id == -1) { id = constants.addConstant(constant->f32()); }
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, TYPE_F32, id, IR_CONSTANT), cfg.getLast());
        break;
    case TYPE_U64:
        id = constants.getConstant(constant->u64());
        if (id == -1) { id = constants.addConstant(constant->u64()); }
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, TYPE_U64, id, IR_CONSTANT), cfg.getLast());
        break;
    }
    return instructionId;
}

int poCodeGenerator::emitUnaryMinus(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* unary = static_cast<poUnaryNode*>(node);
    const int expr = emitExpr(unary->child(), cfg);
    const int instruction = _instructionCount++;
    const int type = _types[expr];
    emitInstruction(poInstruction(instruction, type, expr, -1, IR_UNARY_MINUS), cfg.getLast());
    return instruction;
}

int poCodeGenerator::emitBinaryExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = EMIT_ERROR;
    poBinaryNode* binary = static_cast<poBinaryNode*>(node);
    const int left = emitExpr(binary->left(), cfg);
    const int right = emitExpr(binary->right(), cfg);
    const int type = _types[left];

    switch (node->type())
    {
    case poNodeType::ADD:
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type, left, right, IR_ADD), cfg.getLast());
        break;
    case poNodeType::SUB:
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type, left, right, IR_SUB), cfg.getLast());
        break;
    case poNodeType::MUL:
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type, left, right, IR_MUL), cfg.getLast());
        break;
    case poNodeType::DIV:
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type, left, right, IR_DIV), cfg.getLast());
        break;
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
        instructionId = _instructionCount;
        emitInstruction(poInstruction(_instructionCount++, type, left, right, IR_CMP), cfg.getLast());
        break;
    }
    return instructionId;
}

void poCodeGenerator::getModules(poNode* node)
{
    poListNode* module = static_cast<poListNode*>(node);
    assert(module->type() == poNodeType::MODULE);
    for (poNode* child : module->list())
    {
        if (child->type() == poNodeType::NAMESPACE)
        {
            getNamespaces(child);
        }
    }
}

void poCodeGenerator::getNamespaces(poNode* node)
{
    poListNode* namespaceNode = static_cast<poListNode*>(node);
    assert(namespaceNode->type() == poNodeType::NAMESPACE);
    for (poNode* child : namespaceNode->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            getFunction(child);
        }
        else if (child->type() == poNodeType::STRUCT)
        {
            getStruct(child);
        }
        else if (child->type() == poNodeType::EXTERN)
        {
            _functions.insert(std::pair<std::string, poNode*>(child->token().string(), child));
        }
    }
}

void poCodeGenerator::getStruct(poNode* node)
{
    assert(node->type() == poNodeType::STRUCT);
}

void poCodeGenerator::getFunction(poNode* node)
{
    assert(node->type() == poNodeType::FUNCTION);
    _functions.insert(std::pair<std::string, poNode*>(node->token().string(), node));
}

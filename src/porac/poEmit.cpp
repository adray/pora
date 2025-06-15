#include "poEmit.h"
#include "poAST.h"
#include "poModule.h"
#include "poType.h"
#include <assert.h>
#include <algorithm>
#include <iostream>

using namespace po;

//=====================
// Variable
//=====================

poVariable::poVariable(const int id, const int type)
    :
    _id(id),
    _type(type)
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
    _errorText = errorText;
    _isError = true;
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
            if (node != _functions.end())
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
    const int ptr = _module.getPointerType(baseType);
    if (ptr == -1)
    {
        const poType& type = _module.types()[baseType];

        const int id = int(_module.types().size());
        poType pointerType(id, baseType, type.name() + "*");
        pointerType.setPointer(true);
        pointerType.setSize(8);
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
    case poTokenType::I8_TYPE:
        type = TYPE_I8;
        break;
    case poTokenType::U64_TYPE:
        type = TYPE_U64;
        break;
    case poTokenType::U32_TYPE:
        type = TYPE_U32;
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
    default:
        type = _module.getTypeFromName(token.string());
        break;
    }
    return type;
}

int poCodeGenerator::getOrAddVariable(const std::string& name, const int type)
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
        var = _instructionCount++;
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type)));
    }
    return var;
}

int poCodeGenerator::addVariable(const std::string& name, const int type)
{
    /* TODO: shadowing variables is broken? */

    int var = -1;
    const auto& it = _variables.find(name);
    if (it == _variables.end())
    {
        _types.push_back(type);
        var = _instructionCount++;
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type)));
    }
    return var;
}

int poCodeGenerator::emitAlloca(const int type, poBasicBlock* bb)
{
    const int id = _instructionCount;
    const int ptrType = getPointerType(type);
    bb->addInstruction(poInstruction(_instructionCount++, ptrType, 1 /* num elements */, -1, IR_ALLOCA));
    _types.push_back(ptrType);
    return id;
}

int poCodeGenerator::emitAlloca(const int type, const int varName, poBasicBlock* bb)
{
    const int ptrType = getPointerType(type);
    bb->addInstruction(poInstruction(varName, ptrType, 1 /* num elements */, -1, IR_ALLOCA));
    return varName;
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
        bb->addInstruction(poInstruction(ptrSrc, srcType.id(), src, -1, i * 8, IR_PTR));
        _types.push_back(srcType.id());

        const int loadVar = _instructionCount++;
        bb->addInstruction(poInstruction(loadVar, TYPE_I64, ptrSrc, -1, IR_LOAD));
        _types.push_back(TYPE_I64);

        const int ptrDst = _instructionCount++;
        bb->addInstruction(poInstruction(ptrDst, dstType.id(), dst, -1, i * 8, IR_PTR));
        _types.push_back(dstType.id());

        const int store = _instructionCount++;
        bb->addInstruction(poInstruction(store, TYPE_I64, ptrDst, loadVar, IR_STORE));
        _types.push_back(TYPE_I64);
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
        else if (child->type() == poNodeType::TYPE)
        {
            // return type
            retType = getType(child->token());
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
    }

    if (args)
    {
        emitArgs(args, cfg);
    }

    if (body)
    {
        emitBody(body, cfg, nullptr, nullptr);
    }

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
            last->addInstruction(poInstruction(_instructionCount++, TYPE_VOID, -1, -1, IR_RETURN));
            _types.push_back(TYPE_VOID);
        }
        else
        {
            // error: there isn't a return defined and the function doesn't return void
            // so we can't insert one
            setError("Function expects a return value, but function doesn't return one.");
        }
    }

    cfg.optimize();

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
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR));
            _types.push_back(0);
            cfg.getLast()->setBranch(loopEnd, true);
            loopEnd->addIncoming(cfg.getLast());
            cfg.addBasicBlock(new poBasicBlock());
        }
        else if (child->type() == poNodeType::CONTINUE)
        {
            assert(loopHeader);
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR));
            _types.push_back(0);
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
    const poType& typeData = _module.types()[type];
    const int size = typeData.size();
    if (typeData.isPointer() &&
        _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
    {
        if (size <= 8)
        {
            // X86_64 calling convection is to pass this in via a register

            const int param = _instructionCount++;
            bb->addInstruction(poInstruction(param, type, paramIndex, -1, IR_PARAM));
            _types.push_back(type);

            emitAlloca(type, varName, bb);

            const int pointerType = _types[varName];
            const int ptr = _instructionCount++;
            bb->addInstruction(poInstruction(ptr, pointerType, varName, -1, 0, IR_PTR));
            _types.push_back(pointerType);

            bb->addInstruction(poInstruction(_instructionCount++, TYPE_I64, ptr, param, IR_STORE));
            _types.push_back(TYPE_I64);
        }
        else
        {
            // X86_64 calling convention this will be passed in as a pointer

            bb->addInstruction(poInstruction(varName, type, paramIndex, -1, IR_PARAM));
        }
    }
    else
    {
        bb->addInstruction(poInstruction(varName, type, paramIndex, -1, IR_PARAM));
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
            const int code = getType(type->token());

            const poType& baseType = _module.types()[code];
            if (baseType.baseType() == TYPE_OBJECT)
            {
                // X86_64 calling convention is this is passed in via a pointer.

                const int pointerType = getPointerType(baseType.id());
                const int var = getOrAddVariable(name, pointerType);
                emitParameter(pointerType, bb, paramCount, var);
            }
            else
            {
                const int var = getOrAddVariable(name, code);
                emitParameter(code, bb, paramCount, var);
            }
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
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR));
    _types.push_back(0);
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
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR)); /*unconditional branch*/
        _types.push_back(0);
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
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, getCompare(node.node()), -1, IR_BR));
            _types.push_back(type);
            cfg.getLast()->setBranch(success.getBasicBlock(), false);
            success.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(fail.getBasicBlock());
        }
        else if (!success.isEmitted() && checkPredecessors(_graph, success) && success.node())
        {
            id = success.id();
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR));
            _types.push_back(type);
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(success.getBasicBlock());
        }
        else
        {
            /*we are the last node? branch to the fail state */
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR));
            _types.push_back(type);
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
        }
    }
}

int poCodeGenerator::emitCall(poNode* node, poFlowGraph& cfg)
{
    std::vector<int> args;
    poListNode* call = static_cast<poListNode*>(node);
    for (poNode* child : call->list())
    {
        const int expr = emitExpr(child, cfg);

        const int type = _types[expr];
        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() &&
            _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
        {
            // X86_64 calling convention: structs smaller/equal to 8 bytes can be passed by value
            // Greater than that pass in via a pointer to a IR_ALLOCA

            assert(typeData.isPointer());

            if (typeData.size() <= 8)
            {
                const int ptr = _instructionCount++;
                cfg.getLast()->addInstruction(poInstruction(ptr, typeData.id(), expr, -1, IR_PTR));
                _types.push_back(typeData.id());

                const int load = _instructionCount++;
                cfg.getLast()->addInstruction(poInstruction(load, TYPE_I64, ptr, -1, IR_LOAD));
                _types.push_back(TYPE_I64);

                args.push_back(load);
            }
            else
            {
                const int varName = emitAlloca(type, cfg.getLast()); /* get a pointer to the stack */
                emitCopy(expr, varName, cfg.getLast());
                args.push_back(varName);
            }
        }
        else
        {
            args.push_back(expr);
        }
    }

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
        else if (child->type() == poNodeType::TYPE)
        {
            returnType = getType(child->token());
        }
    }

    int numArgs = int(call->list().size());

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
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_VOID, numArgs, symbol, IR_CALL));
            _types.push_back(TYPE_VOID);

            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, returnPtr, retVariable, -1, IR_ARG));
            _types.push_back(returnPtr);
        }
        else
        {
            const int symbol = _module.addSymbol(name);
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_I64, numArgs, symbol, IR_CALL));
            _types.push_back(TYPE_I64);
        }
    }
    else
    {
        const int symbol = _module.addSymbol(name);
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, returnType, numArgs, symbol, IR_CALL));
        _types.push_back(returnType);
    }

    for (int i = 0; i < int(args.size()); i++)
    {
        poUnaryNode* param = static_cast<poUnaryNode*>(argsNode->list()[i]);
        const int type = getType(param->child()->token());
        const poType& typeData = _module.types()[type];
        if (typeData.baseType() == TYPE_OBJECT)
        {
            if (typeData.size() > 8)
            {
                // X86_64 pass in via a pointer

                const int pointerType = getPointerType(type);
                cfg.getLast()->addInstruction(poInstruction(_instructionCount++, pointerType, args[i], -1, IR_ARG));
                _types.push_back(pointerType);
            }
            else
            {
                // X86_64 - it fits in a register (packed into a I64)

                cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_I64, args[i], -1, IR_ARG));
                _types.push_back(TYPE_I64);
            }
        }
        else
        {
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, args[i], -1, IR_ARG));
            _types.push_back(type);
        }
    }

    if (typeData.baseType() == TYPE_OBJECT && typeData.size() <= 8)
    {
        // X86_64 - we need to load the value into a struct

        const int stackAlloc = emitAlloca(typeData.id(), cfg.getLast());
        const int pointerType = getPointerType(typeData.id());

        const int ptr = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, pointerType, stackAlloc, -1, IR_PTR));
        _types.push_back(pointerType);

        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_I64, ptr, retVariable, IR_STORE));
        _types.push_back(TYPE_I64);

        return stackAlloc;
    }

    return retVariable;
}

void poCodeGenerator::emitAssignment(poNode* node, poFlowGraph& cfg)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(node);
    const int right = emitExpr(assignment->right(), cfg);
    const int type = _types[right];

    if (assignment->left()->type() == poNodeType::VARIABLE)
    {
        const int variable = getOrAddVariable(assignment->left()->token().string(), type);

        const poType& typeData = _module.types()[type];
        if (typeData.isPointer() && _module.types()[typeData.baseType()].baseType() == TYPE_OBJECT)
        {
            emitCopy(right, variable, cfg.getLast());
        }
        else
        {
            cfg.getLast()->addInstruction(poInstruction(variable, type, right, -1, IR_COPY));
        }
    }
    else if (assignment->left()->type() == poNodeType::ARRAY_ACCESSOR)
    {
        emitStoreArray(assignment->left(), cfg, right);
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

                bb->addInstruction(poInstruction(_instructionCount++, 0, -1, -1, IR_RETURN));
                _types.push_back(0);
            }
        }
        else
        {
            if (typeData.isPointer())
            {
                // We need to pack the struct data into a I64 and return this.

                const int ptr = _instructionCount++;
                bb->addInstruction(poInstruction(ptr, type, expr, -1, IR_PTR));
                _types.push_back(type);

                const int load = _instructionCount++;
                bb->addInstruction(poInstruction(load, TYPE_I64, ptr, -1, IR_LOAD));
                _types.push_back(TYPE_I64);

                bb->addInstruction(poInstruction(_instructionCount++, TYPE_I64, load, -1, IR_RETURN));
                _types.push_back(TYPE_I64);
            }
            else
            {
                // Return the value in the default manner.

                bb->addInstruction(poInstruction(_instructionCount++, type, expr, -1, IR_RETURN));
                _types.push_back(type);
            }
        }
    }
    else
    {
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, -1, -1, IR_RETURN));
        _types.push_back(0);
    }
    cfg.addBasicBlock(new poBasicBlock());
}

void poCodeGenerator::emitDecl(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* declNode = static_cast<poUnaryNode*>(node);
    const int type = getType(declNode->token());
    if (declNode->child()->type() == poNodeType::ASSIGNMENT)
    {
        emitAssignment(declNode->child(), cfg);
    }
    else if (declNode->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(declNode->child());
        assert(arrayNode->type() == poNodeType::ARRAY);

        poNode* variable = arrayNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const int arrayType = getArrayType(type, 1);
        const std::string name = variable->token().string();
        const int var = addVariable(name, type);

        cfg.getLast()->addInstruction(poInstruction(var, arrayType, int16_t(arrayNode->arraySize()) /* num elements */, -1, IR_ALLOCA));
    }
    else
    {
        poNode* variable = declNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const std::string name = variable->token().string();

        // Assign a default value
        poConstantPool& pool = _module.constants();
        int constant = -1;
        switch (type)
        {
        case TYPE_BOOLEAN:
        case TYPE_U8:
            constant = pool.getConstant((uint8_t)0);
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
            const int var = addVariable(name, type);
            cfg.getLast()->addInstruction(poInstruction(var, type, constant, IR_CONSTANT));
        }
        else
        {
            const int var = addVariable(name, getPointerType(type));
            emitAlloca(type, var, cfg.getLast());
        }
    }
}

int poCodeGenerator::emitExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = -1;
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
        instructionId = getOrAddVariable(node->token().string(), TYPE_VOID);
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
    }
    return instructionId;
}

void poCodeGenerator::emitStoreArray(poNode* node, poFlowGraph& cfg, const int id)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _types[expr];

    const int accessor = emitExpr(array->accessor(), cfg);
    const int pointerType = getPointerType(arrayType);

    const int ptr = _instructionCount; /* result of IR_PTR */
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, pointerType, expr, accessor, IR_ELEMENT_PTR));
    _types.push_back(pointerType);

    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, arrayType, ptr, id, IR_STORE));
    _types.push_back(arrayType);
}

int poCodeGenerator::emitLoadArray(poNode* node, poFlowGraph& cfg)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);

    poNode* child = array->child();
    const int expr = emitExpr(child, cfg);
    const int arrayType = _types[expr];
    auto& type = _module.types()[arrayType];

    const int accessor = emitExpr(array->accessor(), cfg);
    const int pointerType = getPointerType(arrayType);

    const int ptr = _instructionCount; /* result of IR_ELEMENT_PTR */
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, pointerType, expr, accessor, IR_ELEMENT_PTR));
    _types.push_back(pointerType);

    const int load = _instructionCount;
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, arrayType, ptr, -1, IR_LOAD));
    _types.push_back(arrayType);

    return load;
}

void poCodeGenerator::emitStoreMember(poNode* node, poFlowGraph& cfg, const int id)
{
    poUnaryNode* member = static_cast<poUnaryNode*>(node);

    int expr = -1;
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
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, storePtr, expr, -1, offset, IR_PTR));
    _types.push_back(storePtr);

    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, fieldType, ptr, id, IR_STORE));
    _types.push_back(fieldType);
}

int poCodeGenerator::emitLoadMember(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* member = static_cast<poUnaryNode*>(node);

    int expr = -1;
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
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, storePtr, expr, -1, offset, IR_PTR));
    _types.push_back(storePtr);

    const int instructionId = _instructionCount;
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, fieldType, ptr, -1, IR_LOAD));
    _types.push_back(fieldType);
    return instructionId;
}

int poCodeGenerator::emitConstantExpr(poNode* node, poFlowGraph& cfg)
{
    poConstantNode* constant = static_cast<poConstantNode*>(node);
    poConstantPool& constants = _module.constants();
    int id = -1;
    int instructionId = -1;
    switch (constant->type())
    {
    case TYPE_I64:
        id = constants.getConstant(constant->i64());
        if (id == -1) { id = constants.addConstant(constant->i64()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_I64, id, IR_CONSTANT));
        _types.push_back(TYPE_I64);
        break;
    case TYPE_BOOLEAN:
    case TYPE_U8:
        id = constants.getConstant(constant->u8());
        if (id == -1) { id = constants.addConstant(constant->u8()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_U8, id, IR_CONSTANT));
        _types.push_back(TYPE_U8);
        break;
    case TYPE_F64:
        id = constants.getConstant(constant->f64());
        if (id == -1) { id = constants.addConstant(constant->f64()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_F64, id, IR_CONSTANT));
        _types.push_back(TYPE_F64);
        break;
    case TYPE_F32:
        id = constants.getConstant(constant->f32());
        if (id == -1) { id = constants.addConstant(constant->f32()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_F32, id, IR_CONSTANT));
        _types.push_back(TYPE_F32);
        break;
    case TYPE_U64:
        id = constants.getConstant(constant->u64());
        if (id == -1) { id = constants.addConstant(constant->u64()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_U64, id, IR_CONSTANT));
        _types.push_back(TYPE_U64);
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
    cfg.getLast()->addInstruction(poInstruction(instruction, type, expr, -1, IR_UNARY_MINUS));
    _types.push_back(type);
    return instruction;
}

int poCodeGenerator::emitBinaryExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = -1;
    poBinaryNode* binary = static_cast<poBinaryNode*>(node);
    const int left = emitExpr(binary->left(), cfg);
    const int right = emitExpr(binary->right(), cfg);
    const int type = _types[left];

    switch (node->type())
    {
    case poNodeType::ADD:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, left, right, IR_ADD));
        _types.push_back(type);
        break;
    case poNodeType::SUB:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, left, right, IR_SUB));
        _types.push_back(type);
        break;
    case poNodeType::MUL:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, left, right, IR_MUL));
        _types.push_back(type);
        break;
    case poNodeType::DIV:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, left, right, IR_DIV));
        _types.push_back(type);
        break;
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, left, right, IR_CMP));
        _types.push_back(type);
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

#include "poEmit.h"
#include "poAST.h"
#include "poModule.h"
#include "poTypeChecker.h"
#include <assert.h>
#include <algorithm>

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
    //poBinaryNode* expr = static_cast<poBinaryNode*>(ast);
    //const int left = build(expr->left(), success, fail);
    //const int right = build(expr->right(), left, fail);

    //return right;

    poBinaryNode* expr = static_cast<poBinaryNode*>(ast);
    const int right = build(expr->right(), success, fail);
    const int left = build(expr->left(), right, fail);

    return left;
}

int poConditionGraph::buildOr(poNode* ast, int success, int fail)
{
    //poBinaryNode* expr = static_cast<poBinaryNode*>(ast);
    //const int left = build(expr->left(), success, fail);
    //const int right = build(expr->right(), success, left);

    //return right;

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
    _instructionCount(0)
{
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
        type = -1;
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
        var = _instructionCount++;
        _variables.insert(std::pair<std::string, poVariable>(name, poVariable(var, type)));
    }
    return var;
}

void poCodeGenerator::emitFunction(poNode* node, poFunction& function)
{
    // Reset the variable emitter.
    _variables.clear();
    _instructionCount = 0;

    poFlowGraph& cfg = function.cfg();

    poBasicBlock* bb = new poBasicBlock();
    cfg.addBasicBlock(bb);

    assert(node->type() == poNodeType::FUNCTION);
    int retType = TYPE_VOID;
    poListNode* list = static_cast<poListNode*>(node);
    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            // args
            emitArgs(child, cfg);
        }
        else if (child->type() == poNodeType::BODY)
        {
            // body
            emitBody(child, cfg);
        }
        else if (child->type() == poNodeType::TYPE)
        {
            // return type
            retType = getType(child->token());
        }
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
        }
        else
        {
            // error: there isn't a return defined and the function doesn't return void
            // so we can't insert one
        }
    }

    cfg.optimize();

    for (auto& variable : _variables)
    {
        function.addVariable(variable.second.id());
    }
}

void poCodeGenerator::emitBody(poNode* node, poFlowGraph& cfg)
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
            emitIfStatement(child, cfg, endBB);
            cfg.addBasicBlock(endBB);
        }
    }
}

void poCodeGenerator::emitArgs(poNode* node, poFlowGraph& cfg)
{
    poBasicBlock* bb = cfg.getLast();
    poListNode* list = static_cast<poListNode*>(node);
    int16_t paramCount = 0;
    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::PARAMETER)
        {
            poUnaryNode* param = static_cast<poUnaryNode*>(child);
            std::string name = param->token().string();
            poNode* type = param->child();
            const int code = getType(type->token());
            bb->addInstruction(poInstruction(getOrAddVariable(name, code), code, paramCount++, -1, IR_PARAM));
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
        emitBody(child, cfg);
    }
}

void poCodeGenerator::emitWhileStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB)
{
    poListNode* whileLoop = static_cast<poListNode*>(node);
    poNode* body = nullptr;
    poNode* expr = nullptr;
    for (poNode* child : whileLoop->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            body = child;
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

    _graph.buildGraph(expr);
    auto& root = _graph.root();
    emitIfExpression(root, cfg, bodyBB, endBB);
    cfg.addBasicBlock(bodyBB);
    emitBody(body, cfg);
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR));
    cfg.getLast()->setBranch(loopHeaderBB, true);
    loopHeaderBB->addIncoming(cfg.getLast());
}

void poCodeGenerator::emitIfStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB)
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
        emitBody(body, cfg);
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, IR_JUMP_UNCONDITIONAL, -1, IR_BR)); /*unconditional branch*/
        cfg.getLast()->setBranch(endBB, true);
        endBB->addIncoming(cfg.getLast());
        cfg.addBasicBlock(elseBB);

        poNode* child = elseNode->child();
        if (child->type() == poNodeType::IF)
        {
            emitIfStatement(child, cfg, endBB);
        }
        else if (child->type() == poNodeType::BODY)
        {
            // Emit the body and fall through to the end bb.
            emitBody(child, cfg);
        }
    }
    else
    {
        emitIfExpression(root, cfg, bodyBB, endBB);
        cfg.addBasicBlock(bodyBB);
        emitBody(body, cfg);
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
            cfg.getLast()->setBranch(success.getBasicBlock(), false);
            success.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(fail.getBasicBlock());
        }
        else if (!success.isEmitted() && checkPredecessors(_graph, success) && success.node())
        {
            id = success.id();
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR));
            cfg.getLast()->setBranch(fail.getBasicBlock(), false);
            fail.getBasicBlock()->addIncoming(cfg.getLast());
            cfg.addBasicBlock(success.getBasicBlock());
        }
        else
        {
            /*we are the last node? branch to the fail state */
            cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, flipJump(getCompare(node.node())), -1, IR_BR));
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
        args.push_back(emitExpr(child, cfg));
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
    
    const int symbol = _module.addSymbol(name);
    const int retVariable = _instructionCount;
    cfg.getLast()->addInstruction(poInstruction(_instructionCount++, returnType, int(call->list().size()), symbol, IR_CALL));
    for (int i = 0; i < int(args.size()); i++)
    {
        poUnaryNode* param = static_cast<poUnaryNode*>(argsNode->list()[i]);
        const int type = getType(param->child()->token());
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, type, args[i], -1, IR_ARG));
    }
    return retVariable;
}

void poCodeGenerator::emitAssignment(poNode* node, poFlowGraph& cfg)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(node);
    const int right = emitExpr(assignment->right(), cfg);
    auto& ins = cfg.getLast()->getInstruction(int(cfg.getLast()->numInstructions()) - 1);
    const int variable = getOrAddVariable(assignment->left()->token().string(), ins.type());
    ins.setName(variable); /* update the final instruction to assign the variable */
}

void poCodeGenerator::emitReturn(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* returnNode = static_cast<poUnaryNode*>(node);
    if (returnNode->child())
    {
        const int expr = emitExpr(returnNode->child(), cfg);
        poBasicBlock* bb = cfg.getLast();
        const poInstruction& ins = bb->getInstruction(int(bb->numInstructions()) - 1);
        assert(ins.name() == expr);
        bb->addInstruction(poInstruction(_instructionCount++, ins.type(), expr, -1, IR_RETURN));
    }
    else
    {
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, 0, -1, -1, IR_RETURN));
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
    else
    {
        poNode* variable = declNode->child();
        assert(variable->type() == poNodeType::VARIABLE);

        const std::string name = variable->token().string();
        const int var = addVariable(name, type);

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

        cfg.getLast()->addInstruction(poInstruction(var, type, constant, IR_CONSTANT));
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
        instructionId = getOrAddVariable(node->token().string(), TYPE_I64);
        break;
    case poNodeType::UNARY_SUB:
        instructionId = emitUnaryMinus(node, cfg);
        break;
    case poNodeType::CALL:
        instructionId = emitCall(node, cfg);
        break;
    }
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
        break;
    case TYPE_BOOLEAN:
    case TYPE_U8:
        id = constants.getConstant(constant->u8());
        if (id == -1) { id = constants.addConstant(constant->u8()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_U8, id, IR_CONSTANT));
        break;
    case TYPE_F64:
        id = constants.getConstant(constant->f64());
        if (id == -1) { id = constants.addConstant(constant->f64()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_F64, id, IR_CONSTANT));
        break;
    case TYPE_F32:
        id = constants.getConstant(constant->f32());
        if (id == -1) { id = constants.addConstant(constant->f32()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_F32, id, IR_CONSTANT));
        break;
    case TYPE_U64:
        id = constants.getConstant(constant->u64());
        if (id == -1) { id = constants.addConstant(constant->u64()); }
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, TYPE_U64, id, IR_CONSTANT));
        break;
    }
    return instructionId;
}

int poCodeGenerator::emitUnaryMinus(poNode* node, poFlowGraph& cfg)
{
    poUnaryNode* unary = static_cast<poUnaryNode*>(node);
    const int expr = emitExpr(unary->child(), cfg);
    const int instruction = _instructionCount++;
    /*TODO: get left/right instruction type*/
    auto& ins = cfg.getLast()->instructions()[cfg.getLast()->numInstructions() - 1];
    cfg.getLast()->addInstruction(poInstruction(instruction, ins.type(), expr, -1, IR_UNARY_MINUS));
    return instruction;
}

int poCodeGenerator::emitBinaryExpr(poNode* node, poFlowGraph& cfg)
{
    int instructionId = -1;
    poBinaryNode* binary = static_cast<poBinaryNode*>(node);
    const int left = emitExpr(binary->left(), cfg);
    const int right = emitExpr(binary->right(), cfg);
    /*TODO: get left/right instruction type*/
    auto& ins = cfg.getLast()->instructions()[cfg.getLast()->numInstructions() - 1];

    switch (node->type())
    {
    case poNodeType::ADD:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, ins.type(), left, right, IR_ADD));
        break;
    case poNodeType::SUB:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, ins.type(), left, right, IR_SUB));
        break;
    case poNodeType::MUL:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, ins.type(), left, right, IR_MUL));
        break;
    case poNodeType::DIV:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, ins.type(), left, right, IR_DIV));
        break;
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
        instructionId = _instructionCount;
        cfg.getLast()->addInstruction(poInstruction(_instructionCount++, ins.type(), left, right, IR_CMP));
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
    poNamespace ns(namespaceNode->token().string());
    for (poNode* child : namespaceNode->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            getFunctions(child, ns);
        }
    }
    _module.addNamespace(ns);
}

void poCodeGenerator::getFunctions(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::FUNCTION);
    _functions.insert(std::pair<std::string, poNode*>(node->token().string(), node));
    poListNode* functionNode = static_cast<poListNode*>(node);
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            ns.addFunction(poFunction(
                node->token().string(),
                int(args->list().size()),
                poAttributes::PUBLIC));
        }
    }
}

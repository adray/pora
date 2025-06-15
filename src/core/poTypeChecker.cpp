#include "poTypeChecker.h"
#include "poAST.h"
#include "poType.h"
#include "poModule.h"

#include <assert.h>
#include <iostream>

using namespace po;


poTypeChecker::poTypeChecker(poModule& module)
    :
    _isError(false),
    _errorCol(0),
    _errorLine(0),
    _module(module)
{
}

void poTypeChecker::setError(const std::string& text, const poToken& token)
{
    if (!_isError)
    {
        _isError = true;
        _errorText = text;
        _errorLine = token.line();
        _errorCol = token.column();
    }
}

void poTypeChecker::setError(const std::string& text, const int line, const int col)
{
    if (!_isError)
    {
        _isError = true;
        _errorText = text;
        _errorLine = line;
        _errorCol = col;
    }
}

bool poTypeChecker::check(const std::vector<poNode*>& nodes)
{
    /*
    * TODO: gather all the classes and their defined member fuctions.
    */

    for (poNode* node : nodes)
    {
        getModules(node);
    }

    for (poNode* node : nodes)
    {
        checkModules(node);
        if (isError())
        {
            break;
        }
    }
    return !isError();
}

void poTypeChecker::getModules(poNode* node)
{
    if (node->type() == poNodeType::MODULE)
    {
        poListNode* ns = static_cast<poListNode*>(node);
        for (poNode* child : ns->list())
        {
            if (child->type() == poNodeType::NAMESPACE)
            {
                getNamespace(child);
            }
        }
    }
}

void poTypeChecker::getNamespace(poNode* node)
{
    poListNode* ns = static_cast<poListNode*>(node);
    for (poNode* child : ns->list())
    {
        if (child->type() == poNodeType::FUNCTION ||
            child->type() == poNodeType::EXTERN)
        {
            poListNode* func = static_cast<poListNode*>(child);
            const std::string& name = func->token().string();
            _functions.insert(std::pair<std::string, poListNode*>(name, func));
        }
        else if (child->type() == poNodeType::STRUCT)
        {
            poListNode* structure = static_cast<poListNode*>(child);
            _types.insert(std::pair<std::string, poListNode*>(structure->token().string(), structure));
        }
    }
}

int poTypeChecker::getVariable(const std::string& name)
{
    for (int i = int(_variables.size()) - 1; i >= 0; i--)
    {
        auto& vars = _variables[i];
        const auto& it = vars.find(name);
        if (it != vars.end())
        {
            return it->second;
        }
    }

    return -1;
}

bool poTypeChecker::addVariable(const std::string& name, const int type)
{
    bool ok = true;
    if (_variables.size() > 0)
    {
        for (int i = int(_variables.size()) - 1; i >= 0; i--)
        {
            auto& vars = _variables[i];
            const auto& it = vars.find(name);
            if (it != vars.end())
            {
                ok = false;
                break;
            }
        }

        if (ok)
        {
            auto& top = _variables[_variables.size() - 1];
            top.insert(std::pair<std::string, int>(name, type));
        }
    }

    return ok;
}

void poTypeChecker::pushScope()
{
    _variables.push_back(std::unordered_map<std::string, int>());
}

void poTypeChecker::popScope()
{
    assert(_variables.size() > 0);
    _variables.erase(_variables.begin() + _variables.size() - 1);
}

void poTypeChecker::checkModules(poNode* node)
{
    assert(node->type() == poNodeType::MODULE);
    poListNode* ns = static_cast<poListNode*>(node);
    for (poNode* child : ns->list())
    {
        if (child->type() == poNodeType::NAMESPACE)
        {
            checkNamespaces(child);
        }
        else if (child->type() == poNodeType::IMPORT)
        {
            // TODO: deal with imports...
        }
        else
        {
            break;
        }
    }
}

void poTypeChecker::checkNamespaces(poNode* node)
{
    assert(node->type() == poNodeType::NAMESPACE);
    poListNode* ns = static_cast<poListNode*>(node);
    for (poNode* child : ns->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            checkFunctions(child);
            if (isError())
            {
                break;
            }
        }
        else if (child->type() == poNodeType::STRUCT ||
            child->type() == poNodeType::EXTERN)
        {
            continue;
        }
        else
        {
            setError("Unexpected node type in namespace.", node->token());
            break;
        }
    }
}

void poTypeChecker::checkFunctions(poNode* node)
{
    assert(node->type() == poNodeType::FUNCTION);
    poListNode* func = static_cast<poListNode*>(node);

    poListNode* args = nullptr;
    poNode* body = nullptr;
    poNode* type = nullptr;

    for (poNode* child : func->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            assert(body == nullptr);
            body = child;
        }
        else if (child->type() == poNodeType::ARGS)
        {
            assert(args == nullptr);
            args = static_cast<poListNode*>(child);
        }
        else if (child->type() == poNodeType::TYPE)
        {
            assert(type == nullptr);
            type = child;
        }
        else
        {
            setError("Unexpected type in function AST.", child->token());
            break;
        }
    }

    if (!isError() && args)
    {
        pushScope();
        for (poNode* arg : args->list())
        {
            poUnaryNode* unary = static_cast<poUnaryNode*>(arg);
            assert(unary->type() == poNodeType::PARAMETER);
            poNode* type = unary->child();
            assert(type->type() == poNodeType::TYPE);
            const int variableType = getType(type->token());
            if (variableType != -1)
            {
                if (!addVariable(unary->token().string(), variableType))
                {
                    setError("Redefinition of variable.", arg->token());
                    break;
                }
            }
            else
            {
                setError("Argument type not valid.", arg->token());
                break;
            }
        }

        const int returnType = getType(type->token());
        if (returnType == -1)
        {
            setError("Undefined type.", type->token());
        }
        checkBody(body, returnType);
        popScope();
    }
}

void poTypeChecker::checkBody(poNode* node, const int returnType)
{
    assert(node->type() == poNodeType::BODY);
    poListNode* func = static_cast<poListNode*>(node);
    for (poNode* statement : func->list())
    {
        if (statement->type() == poNodeType::STATEMENT)
        {
            checkStatement(statement, returnType);
            if (isError())
            {
                break;
            }
        }
        else if (statement->type() == poNodeType::IF)
        {
            checkIfStatement(statement, returnType);
        }
        else if (statement->type() == poNodeType::WHILE)
        {
            checkWhileStatement(statement, returnType);
        }
        else
        {
            setError("Type checking failed, statement expected.", statement->token());
            break;
        }
    }
}

void poTypeChecker::checkStatement(poNode* node, const int returnType)
{
    poUnaryNode* statementNode = static_cast<poUnaryNode*>(node);
    switch (statementNode->child()->type())
    {
    case poNodeType::DECL:
        checkDecl(statementNode->child());
        break;
    case poNodeType::ASSIGNMENT:
        checkAssignment(statementNode->child());
        break;
    case poNodeType::RETURN:
        checkReturn(statementNode->child(), returnType);
        break;
    case poNodeType::CALL:
        /* As long as it returns any type and not an error it is OK */
        if (checkCall(statementNode->child()) == -1)
        {
            setError("Error checking call.", statementNode->token());
        }
        break;
    case poNodeType::BODY:
        pushScope();
        checkBody(statementNode->child(), returnType);
        popScope();
        break;
    default:
        setError("Error checking statement.", statementNode->token());
        break;
    }
}

void poTypeChecker::checkReturn(poNode* node, const int returnType)
{
    poUnaryNode* returnNode = static_cast<poUnaryNode*>(node);
    const int expr = returnNode->child() ? checkExpr(returnNode->child()) : TYPE_VOID;
    if (!isError() && expr != returnType)
    {
        setError("Invalid return type", returnNode->token());
    }
}

int poTypeChecker::getArrayType(const int baseType, const int arrayRank)
{
    const poType& type = _module.types()[baseType];
    const std::string name = type.name() + "[]";
    int arrayType = _module.getTypeFromName(name);
    if (arrayType == -1)
    {
        const int numTypes = int(_module.types().size());
        arrayType = numTypes;
        poType type(arrayType, baseType, name);
        type.setArray(true);
        _module.addType(type);
    }
    return arrayType;
}

int poTypeChecker::getType(const poToken& token)
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
    case poTokenType::BOOLEAN:
        type = TYPE_BOOLEAN;
        break;
    case poTokenType::IDENTIFIER:
        type = _module.getTypeFromName(token.string());
        break;
    default:
        type = -1;
        break;
    }
    return type;
}

void poTypeChecker::checkDecl(poNode* node)
{
    poUnaryNode* decl = static_cast<poUnaryNode*>(node);
    if (decl->child()->type() == poNodeType::ASSIGNMENT)
    {
        const int type = getType(decl->token());
        if (type == -1)
        {
            setError("Undefined type.", decl->token());
        }

        poBinaryNode* assignment = static_cast<poBinaryNode*>(decl->child());
        const int rhs = checkExpr(assignment->right());
        if (rhs != type)
        {
            setError("Error checking types in declaration.", decl->token());
        }

        if (!isError() && assignment->left()->type() == poNodeType::VARIABLE)
        {
            const auto& token = assignment->left()->token();
            const std::string& name = token.string();
            if (!addVariable(name, type))
            {
                setError("Redefinition of variable.", decl->token());
            }
        }
    }
    else if (decl->child()->type() == poNodeType::VARIABLE)
    {
        const int type = getType(decl->token());
        if (type == -1)
        {
            setError("Undefined type.", decl->token());
        }

        const auto& token = decl->child()->token();
        const std::string& name = token.string();
        if (!isError() && !addVariable(name, type))
        {
            setError("Redefinition of variable.", decl->token());
        }
    }
    else if (decl->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());

        const int type = getType(decl->token());
        if (type == -1)
        {
            setError("Undefined type.", decl->token());
        }

        const int arrayType = getArrayType(type, 1);
        if (arrayType == -1)
        {
            setError("Undefined type.", decl->token());
        }

        const auto& token = decl->child()->token();
        const std::string& name = token.string();
        if (!isError() && !addVariable(name, arrayType))
        {
            setError("Redefinition of variable.", decl->token());
        }
    }
    else
    {
        setError("Malformed declaration.", decl->token());
    }
}

void poTypeChecker::checkAssignment(poNode* node)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(node);
    assert(assignment->type() == poNodeType::ASSIGNMENT);

    int rhs = checkExpr(assignment->right());
    if (rhs == -1)
    {
        setError("Error checking types in assignment.", assignment->token());
    }

    if (!isError())
    {
        if (assignment->left()->type() == poNodeType::VARIABLE)
        {
            const auto& token = assignment->left()->token();
            const std::string& name = token.string();
            if (rhs != getVariable(name))
            {
                setError("Error checking types in assignment.", token);
            }
        }
        else if (assignment->left()->type() == poNodeType::MEMBER)
        {
            const int lhs = checkMember(assignment->left());
            if (lhs != rhs)
            {
                setError("Error checking types in assignment.", assignment->token());
            } 
        }
        else if (assignment->left()->type() == poNodeType::ARRAY_ACCESSOR)
        {
            const int lhs = checkArray(assignment->left());
            if (lhs != rhs)
            {
                setError("Error checking types in assignment.", assignment->token());
            }
        }
        else
        {
            setError("Error checking types in assignment.", assignment->token());
        }
    }
}

int poTypeChecker::checkExpr(poNode* node)
{
    int type = -1;
    switch (node->type())
    {
    case poNodeType::CONSTANT:
        switch (node->token().token())
        {
        case poTokenType::I64:
            type = TYPE_I64;
            break;
        case poTokenType::I32:
            type = TYPE_I32;
            break;
        case poTokenType::I8:
            type = TYPE_I8;
            break;
        case poTokenType::U64:
            type = TYPE_U64;
            break;
        case poTokenType::U32:
            type = TYPE_U32;
            break;
        case poTokenType::U8:
            type = TYPE_U8;
            break;
        case poTokenType::F64:
            type = TYPE_F64;
            break;
        case poTokenType::F32:
            type = TYPE_F32;
            break;
        case poTokenType::TRUE:
        case poTokenType::FALSE:
            type = TYPE_BOOLEAN;
            break;
        case poTokenType::CHAR:
            type = TYPE_U8;
            break;
        }
        break;
    case poNodeType::CALL:
        type = checkCall(node);
        break;
    case poNodeType::MEMBER:
        type = checkMember(node);
        break;
    case poNodeType::ARRAY_ACCESSOR:
        type = checkArray(node);
        break;
    case poNodeType::VARIABLE:
    {
        type = getVariable(node->token().string());
        if (type == -1)
        {
            setError("Variable is not defined.", node->token());
        }
    }
    break;
    case poNodeType::UNARY_SUB:
    {
        poUnaryNode* unary = static_cast<poUnaryNode*>(node);
        type = checkExpr(unary->child());
        break;
    }
    case poNodeType::ADD:
    case poNodeType::SUB:
    case poNodeType::MUL:
    case poNodeType::DIV:
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = checkExpr(binary->left());
        if (type != checkExpr(binary->right()))
        {
            type = -1;
        }
        break;
    }
    case poNodeType::OR:
    case poNodeType::AND:
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = (checkExpr(binary->left()) == TYPE_BOOLEAN &&
            checkExpr(binary->right()) == TYPE_BOOLEAN) ? TYPE_BOOLEAN : -1;
    }
        break;
    case poNodeType::CMP_EQUALS:
    case poNodeType::CMP_NOT_EQUALS:
    case poNodeType::CMP_LESS_EQUALS:
    case poNodeType::CMP_GREATER_EQUALS:
    case poNodeType::CMP_GREATER:
    case poNodeType::CMP_LESS:
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = (checkExpr(binary->left()) == checkExpr(binary->right())) ? TYPE_BOOLEAN : -1;
    }
        break;
    }
    return type;
}

int poTypeChecker::checkArray(poNode* node)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);
    assert(array->type() == poNodeType::ARRAY_ACCESSOR);

    const int expr = checkExpr(array->child());
    if (expr != -1)
    {
        auto& type = _module.types()[expr];
        if (type.isArray())
        {
            return type.baseType();
        }
    }

    return -1;
}

int poTypeChecker::checkMember(poNode* node)
{
    poUnaryNode* member = static_cast<poUnaryNode*>(node);
    assert(member->type() == poNodeType::MEMBER);

    const int expr = checkExpr(member->child());
    if (expr > TYPE_OBJECT)
    {
        const std::string& memberName = member->token().string();
        const poType& type = _module.types()[expr];
        for (const poField& field : type.fields())
        {
            if (field.name() == memberName)
            {
                return field.type();
            }
        }
    }

    return -1;
}

int poTypeChecker::checkCall(poNode* node)
{
    poListNode* function = nullptr;

    const auto& it = _functions.find(node->token().string());
    if (it != _functions.end())
    {
        function = it->second;
    }

    int type = -1;
    if (function)
    {
        poListNode* args = nullptr;
        for (poNode* child : function->list())
        {
            if (child->type() == poNodeType::TYPE)
            {
                type = getType(child->token());
                if (type == -1)
                {
                    setError("Undefined type.", child->token());
                }
            }
            else if (child->type() == poNodeType::ARGS)
            {
                args = static_cast<poListNode*>(child);
            }
        }

        poListNode* exprArgs = static_cast<poListNode*>(node);
        if (args && exprArgs->list().size() == args->list().size())
        {
            for (size_t i = 0; i < args->list().size(); i++)
            {
                poNode* childExpr = exprArgs->list()[i];
                poUnaryNode* arg = static_cast<poUnaryNode*>(args->list()[i]);
                poNode* paramType = arg->child();
                if (checkExpr(childExpr) != getType(paramType->token()))
                {
                    setError("Mismatch in parameters.", node->token());
                    type = -1;
                    break;
                }
            }
        }
        else
        {
            setError("Incorrect number of arguments.", node->token());
        }
    }

    return type;
}

void po::poTypeChecker::checkIfStatement(po::poNode* node, const int returnType)
{
    assert(node->type() == poNodeType::IF);
    poListNode* ifStatement = static_cast<poListNode*>(node);
    for (poNode* child : ifStatement->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            pushScope();
            checkBody(child, returnType);
            popScope();
        }
        else if (child->type() == poNodeType::ELSE)
        {
            poUnaryNode* body = static_cast<poUnaryNode*>(child);
            if (body->child()->type() == poNodeType::BODY)
            {
                pushScope();
                checkBody(body->child(), returnType);
                popScope();
            }
            else if (body->child()->type() == poNodeType::IF)
            {
                checkIfStatement(body->child(), returnType);
            }
        }
        else if (child->type() == poNodeType::STATEMENT /* expression */)
        {
            poUnaryNode* expression = static_cast<poUnaryNode*>(child);
            const int type = checkExpr(expression->child());
            if (type != TYPE_BOOLEAN)
            {
                setError("Error, condition doesn't evaluate to a boolean.", expression->token());
            }
        }
    }
}

void po::poTypeChecker::checkWhileStatement(po::poNode* node, const int returnType)
{
    assert(node->type() == poNodeType::WHILE);
    poListNode* whileStatement = static_cast<poListNode*>(node);
    for (poNode* child : whileStatement->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            poUnaryNode* body = static_cast<poUnaryNode*>(child);

            pushScope();
            checkBody(body, returnType);
            popScope();
        }
        else
        {
            const int type = checkExpr(child);
            if (type != TYPE_BOOLEAN)
            {
                setError("While expression does not evaluate to boolean.", whileStatement->token());
            }
        }
    }
}

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
    * TODO: gather all the classes and their defined member functions.
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
            std::string fullname = ns->token().string() + "::" + name;
            for (poNode* node : func->list())
            {
                if (node->type() == poNodeType::RESOLVER)
                {
                    poResolverNode* resolver = reinterpret_cast<poResolverNode*>(node);
                    const std::vector<std::string>& path = resolver->path();
                    if (path.size() > 0)
                    {
                        fullname = ns->token().string() + "::" + path[0] + "::" + name;
                    }
                    break;
                }
            }

            _functions.insert(std::pair<std::string, poListNode*>(fullname, func));
        }
        else if (child->type() == poNodeType::STRUCT)
        {
            poListNode* structure = static_cast<poListNode*>(child);
            _types.insert(std::pair<std::string, poListNode*>(structure->token().string(), structure));
        }
        else if (child->type() == poNodeType::CLASS)
        {
            poListNode* classNode = static_cast<poListNode*>(child);
            _types.insert(std::pair<std::string, poListNode*>(classNode->token().string(), classNode));
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

    for (int i = int(_variables.size()) - 1; i >= 0; i--)
    {
        auto& vars = _variables[i];
        const auto& it = vars.find("this");
        if (it != vars.end())
        {
            const int classType = _module.types()[it->second].baseType();
            if (classType == -1)
            {
                continue;
            }

            const poType& typeData = _module.types()[classType];
            for (const auto& field : typeData.fields())
            {
                if (field.name() == name)
                {
                    return field.type();
                }
            }
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

bool poTypeChecker::checkEquivalence(const int lhs, const int rhs)
{
    if (lhs == -1 || rhs == -1)
    {
        return false;
    }

    if (lhs == rhs)
    {
        return true;
    }

    const poType& lhsType = _module.types()[lhs];
    const poType& rhsType = _module.types()[rhs];
    
    if ((lhsType.isPointer() && rhsType.id() == TYPE_NULLPTR) ||
        (lhsType.id() == TYPE_NULLPTR && rhsType.isPointer()))
    {
        return true;
    }

    return false;
}

void poTypeChecker::checkModules(poNode* node)
{
    assert(node->type() == poNodeType::MODULE);
    poListNode* ns = static_cast<poListNode*>(node);
    std::vector<poNode*> importNodes;
    for (poNode* child : ns->list())
    {
        if (child->type() == poNodeType::NAMESPACE)
        {
            checkNamespaces(child, importNodes);
        }
        else if (child->type() == poNodeType::IMPORT)
        {
            importNodes.push_back(child);
        }
        else
        {
            break;
        }
    }
}

void poTypeChecker::checkNamespaces(poNode* node, const std::vector<poNode*> importNodes)
{
    assert(node->type() == poNodeType::NAMESPACE);
    poListNode* ns = static_cast<poListNode*>(node);

    _imports.clear();
    _imports.push_back(ns->token().string());
    for (poNode* import : importNodes)
    {
        assert(import->type() == poNodeType::IMPORT);
        _imports.push_back(import->token().string());
    }
    

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
            child->type() == poNodeType::CLASS ||
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
    poUnaryNode* type = nullptr;
    poResolverNode* resolver = nullptr;

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
        else if (child->type() == poNodeType::RETURN_TYPE)
        {
            assert(type == nullptr);
            type = static_cast<poUnaryNode*>(child);
        }
        else if (child->type() == poNodeType::RESOLVER)
        {
            assert(resolver == nullptr);
            resolver = static_cast<poResolverNode*>(child);
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

        if (resolver)
        {
            const std::vector<std::string>& path = resolver->path();
            if (path.size() > 0) {
                // Extract the class type and add it as the first argument
                const std::string& className = path[0];
                const int type = _module.getTypeFromName(className, _imports);
                if (type != -1)
                {
                    const int ptrType = getPointerType(type, 1);
                    if (!addVariable("this", ptrType))
                    {
                        setError("Redefinition of variable 'this'.", resolver->token());
                    }
                }
                else
                {
                    setError("Undefined type for 'this'.", resolver->token());
                }
            }
        }

        for (poNode* arg : args->list())
        {
            poUnaryNode* unary = static_cast<poUnaryNode*>(arg);
            assert(unary->type() == poNodeType::PARAMETER);
            poNode* type = unary->child();
            
            int pointerCount = 0;
            if (type->type() == poNodeType::POINTER)
            {
                poPointerNode* pointerNode = static_cast<poPointerNode*>(type);
                pointerCount = pointerNode->count();
                type = pointerNode->child();
            }

            assert(type->type() == poNodeType::TYPE);
            const int variableType = getPointerType(getType(type->token()), pointerCount);
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

        int returnType = getType(type->token());
        if (type->child()->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(type->child());
            returnType = getPointerType(returnType, pointerNode->count());
        }

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
        else if (statement->type() == poNodeType::BREAK ||
            statement->type() == poNodeType::CONTINUE)
        {
            continue;
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
    case poNodeType::MEMBER_CALL:
        /* As long as it returns any type and not an error it is OK */
        if (checkMemberCall(statementNode->child()) == -1)
        {
            setError("Error checking member call.", statementNode->token());
        }
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

int poTypeChecker::getPointerType(const int baseType, const int count)
{
    int pointerType = baseType;
    if (pointerType != -1)
    {
        for (int i = 0; i < count; i++)
        {
            int type = _module.getPointerType(pointerType);
            if (type == -1)
            {
                const poType& typeData = _module.types()[pointerType];

                type = int(_module.types().size());
                poType newType(type, pointerType, typeData.name() + "*");
                newType.setPointer(true);
                newType.setSize(8);
                newType.setAlignment(8);
                _module.addType(newType);
            }
            pointerType = type;
        }
    }
    return pointerType;
}

int poTypeChecker::getArrayType(const int baseType, const int arrayRank)
{
    const poType& type = _module.types()[baseType];
    const std::string name = type.name() + "[]";
    int arrayType = _module.getTypeFromName(name, _imports);
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
    case poTokenType::IDENTIFIER:
        type = _module.getTypeFromName(token.string(), _imports);
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
        poBinaryNode* assignment = static_cast<poBinaryNode*>(decl->child());
        if (assignment->left()->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(assignment->left());
            if (pointerNode->child()->type() == poNodeType::VARIABLE)
            {
                const int type = getType(decl->token());
                if (type == -1)
                {
                    setError("Undefined type.", decl->token());
                }

                const int lhs = getPointerType(type, pointerNode->count());
                if (lhs == -1)
                {
                    setError("Undefined type.", decl->token());
                }

                const int rhs = checkExpr(assignment->right());
                if (!checkEquivalence(lhs, rhs))
                {
                    setError("Error checking types in declaration.", decl->token());
                }

                if (!isError() && pointerNode->child()->type() == poNodeType::VARIABLE)
                {
                    const auto& token = pointerNode->child()->token();
                    const std::string& name = token.string();
                    if (!addVariable(name, lhs))
                    {
                        setError("Redefinition of variable.", decl->token());
                    }
                }
            }
            else
            {
                setError("Malformed pointer node.", pointerNode->token());
            }
        }
        else if (assignment->left()->type() == poNodeType::DYNAMIC_ARRAY)
        {
            poUnaryNode* dynamicArray = static_cast<poUnaryNode*>(assignment->left());
            if (dynamicArray->child()->type() == poNodeType::VARIABLE)
            {
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
                const int rhs = checkExpr(assignment->right());
                if (!checkEquivalence(arrayType, rhs))
                {
                    setError("Error checking types in declaration.", decl->token());
                }
                if (!isError() && dynamicArray->child()->type() == poNodeType::VARIABLE)
                {
                    const auto& token = dynamicArray->child()->token();
                    const std::string& name = token.string();
                    if (!addVariable(name, arrayType))
                    {
                        setError("Redefinition of variable.", decl->token());
                    }
                }
            }
            else
            {
                setError("Malformed dynamic array node.", dynamicArray->token());
            }
        }
        else
        {
            const int type = getType(decl->token());
            if (type == -1)
            {
                setError("Undefined type.", decl->token());
            }

            const int rhs = checkExpr(assignment->right());
            if (!checkEquivalence(type, rhs))
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
    else if (decl->child()->type() == poNodeType::POINTER)
    {
        poPointerNode* pointer = static_cast<poPointerNode*>(decl->child());

        if (pointer->child()->type() == poNodeType::VARIABLE)
        {
            const int baseType = getType(node->token());
            if (baseType == -1)
            {
                setError("Undefined type.", node->token());
            }

            const int type = getPointerType(baseType, pointer->count());
            if (type == -1)
            {
                setError("Undefined type.", node->token());
            }

            const auto& token = pointer->child()->token();
            const std::string& name = token.string();
            if (!isError() && !addVariable(name, type))
            {
                setError("Redefinition of variable.", node->token());
            }
        }
        else
        {
            setError("Undefined type.", node->token());
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

    const int rhs = checkExpr(assignment->right());
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
            if (!checkEquivalence(getVariable(name), rhs))
            {
                setError("Error checking types in assignment.", token);
            }
        }
        else if (assignment->left()->type() == poNodeType::MEMBER)
        {
            const int lhs = checkMember(assignment->left());
            if (!checkEquivalence(lhs, rhs))
            {
                setError("Error checking types in assignment.", assignment->token());
            } 
        }
        else if (assignment->left()->type() == poNodeType::ARRAY_ACCESSOR)
        {
            const int lhs = checkArray(assignment->left());
            if (!checkEquivalence(lhs, rhs))
            {
                setError("Error checking types in assignment.", assignment->token());
            }
        }
        else if (assignment->left()->type() == poNodeType::DEREFERENCE)
        {
            poUnaryNode* ptr = static_cast<poUnaryNode*>(assignment->left());
            poNode* child = ptr->child();
            assert(child->type() == poNodeType::VARIABLE);
            
            const int variable = getVariable(child->token().string());
            if (variable == -1)
            {
                setError("Variable is not defined.", child->token());
            }
            else
            {
                poType& type = _module.types()[variable];
                if (!type.isPointer())
                {
                    setError("Cannot perform assignment of non-pointer type.", ptr->token());
                }

                if (!checkEquivalence(type.baseType(), rhs))
                {
                    setError("Error checking types in assignment.", assignment->token());
                }
            }
        }
        else if (assignment->left()->type() == poNodeType::DYNAMIC_ARRAY)
        {
            poUnaryNode* dynamicArray = static_cast<poUnaryNode*>(assignment->left());
            poNode* child = dynamicArray->child();
            assert(child->type() == poNodeType::VARIABLE);
            const int variable = getVariable(child->token().string());
            if (variable == -1)
            {
                setError("Variable is not defined.", child->token());
            }
            else
            {
                poType& type = _module.types()[variable];
                if (!checkEquivalence(type.id(), rhs))
                {
                    setError("Error checking types in assignment.", assignment->token());
                }
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
        case poTokenType::I16:
            type = TYPE_I16;
            break;
        case poTokenType::I8:
            type = TYPE_I8;
            break;
        case poTokenType::U64:
            type = TYPE_U64;
            break;
        case poTokenType::U16:
            type = TYPE_U16;
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
        case poTokenType::STRING:
            type = getPointerType(TYPE_U8, 1);
            break;
        }
        break;
    case poNodeType::CALL:
        type = checkCall(node);
        break;
    case poNodeType::MEMBER_CALL:
        type = checkMemberCall(node);
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
        switch (type)
        {
        case TYPE_U8:
        case TYPE_I8:
        case TYPE_U16:
        case TYPE_I16:
        case TYPE_U32:
        case TYPE_I32:
        case TYPE_U64:
        case TYPE_I64:
        case TYPE_F64:
        case TYPE_F32:
            break;
        default:
            type = -1; // unary minus operation not allowed
            break;
        }
        break;
    }
    case poNodeType::REFERENCE:
    {
        poUnaryNode* unary = static_cast<poUnaryNode*>(node);
        type = checkExpr(unary->child());
        if (type != -1)
        {
            type = getPointerType(type, 1);
        }
        else
        {
            type = -1;
        }
        break;
    }
    case poNodeType::DEREFERENCE:
    {
        poUnaryNode* unary = static_cast<poUnaryNode*>(node);
        type = checkExpr(unary->child());
        if (type != -1 && _module.types()[type].isPointer())
        {
            type = _module.types()[type].baseType();
        }
        else
        {
            type = -1;
        }
        break;
    }
    case poNodeType::LEFT_SHIFT:
    case poNodeType::RIGHT_SHIFT:
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = checkExpr(binary->left());
        switch (type)
        {
        case TYPE_U8:
        case TYPE_I8:
        case TYPE_U16:
        case TYPE_I16:
        case TYPE_U32:
        case TYPE_I32:
        case TYPE_U64:
        case TYPE_I64:
            switch (checkExpr(binary->right()))
            {
            case TYPE_U8:
            case TYPE_I8:
            case TYPE_U16:
            case TYPE_I16:
            case TYPE_U32:
            case TYPE_I32:
            case TYPE_U64:
            case TYPE_I64:
                break;
            default:
                type = -1;
                break;
            }
            break;
        default:
            type = -1; // shift not allowed
            break;
        }
        break;
    }
        break;
    case poNodeType::MODULO: {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = checkExpr(binary->left());
        switch (type)
        {
        case TYPE_U8:
        case TYPE_I8:
        case TYPE_U16:
        case TYPE_I16:
        case TYPE_U32:
        case TYPE_I32:
        case TYPE_U64:
        case TYPE_I64:
            if (!checkEquivalence(type, checkExpr(binary->right())))
            {
                type = -1;
            }
            break;
        default:
            type = -1; // binary arithmetic not allowed
            break;
        }
        break;
    }
    case poNodeType::ADD:
    case poNodeType::SUB:
    case poNodeType::MUL:
    case poNodeType::DIV:
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        type = checkExpr(binary->left());
        switch (type)
        {
        case TYPE_U8:
        case TYPE_I8:
        case TYPE_U16:
        case TYPE_I16:
        case TYPE_U32:
        case TYPE_I32:
        case TYPE_U64:
        case TYPE_I64:
        case TYPE_F64:
        case TYPE_F32:
            if (!checkEquivalence(type, checkExpr(binary->right())))
            {
                type = -1;
            }
            break;
        default:
            type = -1; // binary arithmetic not allowed
            break;
        }
        break;
    }
    case poNodeType::CAST:
        type = checkCast(node);
        break;
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
        type = checkEquivalence(checkExpr(binary->left()), checkExpr(binary->right())) ? TYPE_BOOLEAN : -1;
    }
        break;
    case poNodeType::NULLPTR:
        type = TYPE_NULLPTR;
        break;
    case poNodeType::SIZEOF:
    {
        poUnaryNode* sizeofNode = static_cast<poUnaryNode*>(node);
        poNode* typeNode = sizeofNode->child();
        if (getType(typeNode->token()) != -1)
        {
            type = TYPE_U64; // sizeof returns size
        }
        break;
    }
    }
    return type;
}

int poTypeChecker::checkCast(poNode* node)
{
    poUnaryNode* castNode = static_cast<poUnaryNode*>(node);
    assert(castNode->child()->type() == poNodeType::TYPE);
    poUnaryNode* typeNode = static_cast<poUnaryNode*>(castNode->child());
    int dstType = getType(typeNode->token());

    if (typeNode->child()->type() == poNodeType::POINTER)
    {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(typeNode->child());
        dstType = getPointerType(dstType, pointerNode->count());
        typeNode = pointerNode;
    }
    else if (typeNode->child()->type() == poNodeType::ARRAY)
    {
        poArrayNode* arrayNode = static_cast<poArrayNode*>(typeNode->child());
        dstType = getArrayType(dstType, 1);
        typeNode = arrayNode;
    }

    const int srcType = checkExpr(typeNode->child());

    if (dstType == srcType)
    {
        return dstType; // no cast needed
    }

    const auto& dstTypeData = _module.types()[dstType];
    if (dstTypeData.isPointer())
    {
        return dstType; // pointer type, bitwise cast
    }

    if (dstTypeData.isArray())
    {
        poType& srcTypeData = _module.types()[srcType];
        if (srcTypeData.isPointer() && srcTypeData.baseType() == dstTypeData.baseType())
        {
            return dstType; // pointer to array cast
        }
    }
    
    for (poOperator op : dstTypeData.operators())
    {
        if (op.getOperator() == poOperatorType::EXPLICIT_CAST &&
            op.otherType() == srcType)
        {
            return dstType;
        }
    }

    return -1;
}

int poTypeChecker::checkArray(poNode* node)
{
    poArrayAccessor* array = static_cast<poArrayAccessor*>(node);
    assert(array->type() == poNodeType::ARRAY_ACCESSOR);

    const int accessor = checkExpr(array->accessor());
    if (accessor != TYPE_I64)
    {
        setError("Array accessor must be of type i64.", array->accessor()->token());
        return -1;
    }

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

int poTypeChecker::checkMemberCall(poNode* node)
{
    poBinaryNode* memberCall = static_cast<poBinaryNode*>(node);
    assert(memberCall->type() == poNodeType::MEMBER_CALL);

    const int expr = checkExpr(memberCall->left());
    if (expr == -1)
    {
        return -1;
    }

    int objectType = expr;
    const poType& objectTypeData = _module.types()[objectType];
    if (objectTypeData.isPointer())
    {
        objectType = objectTypeData.baseType();
    }

    poType& type = _module.types()[objectType];
    for (const poMemberFunction& method : type.functions())
    {
        if (method.name() != memberCall->token().string())
        {
            continue;
        }

        if (((int)method.attributes() & (int)poAttributes::PRIVATE) == (int)poAttributes::PRIVATE)
        {
            return -1; // private method, cannot access
        }
    }

    if (memberCall->right()->type() != poNodeType::CALL)
    {
        setError("Expected function call after member access.", memberCall->token());
        return -1;
    }

    poListNode* function = nullptr;
    poListNode* call = static_cast<poListNode*>(memberCall->right());
    for (const std::string& import : _imports)
    {
        const auto& it = _functions.find(import + "::" + type.name() + "::" + call->token().string());
        if (it == _functions.end())
        {
            continue;
        }

        function = it->second;
        break;
    }

    if (!function)
    {
        setError("Function not defined.", call->token());
        return -1;
    }

    for (poNode* child : function->list())
    {
        if (child->type() != poNodeType::RETURN_TYPE)
        {
            continue;
        }

        poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(child);
        int type = getType(child->token());
        if (returnTypeNode->child()->type() == poNodeType::POINTER)
        {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
            type = getPointerType(type, pointerNode->count());
        }
        if (type == -1)
        {
            setError("Undefined type.", child->token());
            return -1;
        }

        return type;
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
        int objectType = expr;
        if (type.isPointer())
        {
            objectType = type.baseType();
        }

        const poType& baseType = _module.types()[objectType];
        for (const poField& field : baseType.fields())
        {
            if (field.name() == memberName &&
                ((int)field.attributes() & (int)poAttributes::PUBLIC) == (int)poAttributes::PUBLIC)
            {
                return field.type();
            }
        }
    }

    return -1;
}

poListNode* poTypeChecker::getFunction(const std::string& name)
{
    for (int i = int(_variables.size()) - 1; i >= 0; i--)
    {
        auto& vars = _variables[i];
        const auto& it = vars.find("this");
        if (it != vars.end())
        {
            const int classType = _module.types()[it->second].baseType();
            if (classType == -1)
            {
                continue;
            }

            const poType& typeData = _module.types()[classType];
            for (const poMemberFunction& method : typeData.functions())
            {
                if (method.name() != name)
                {
                    continue;
                }

                const int id = method.id();
                const poFunction& function = _module.functions()[id];
                const auto& it = _functions.find(function.fullname());
                if (it != _functions.end())
                {
                    return it->second;
                }
            }
        }
    }

    poListNode* function = nullptr;
    for (poNamespace& ns : _module.namespaces())
    {
        std::string fullname = ns.name() + "::" + name;
        const auto& it = _functions.find(fullname);
        if (it != _functions.end())
        {
            function = it->second;
        }
    }

    return function;
}

int poTypeChecker::checkCall(poNode* node)
{
    const std::string& name = node->token().string();

    poListNode* function = getFunction(name);
    if (function == nullptr)
    {
        return -1;
    }

    int type = -1;
    if (function)
    {
        poListNode* args = nullptr;
        for (poNode* child : function->list())
        {
            if (child->type() == poNodeType::RETURN_TYPE)
            {
                poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(child);
                type = getType(child->token());
                if (returnTypeNode->child()->type() == poNodeType::POINTER)
                {
                    poPointerNode* pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
                    type = getPointerType(type, pointerNode->count());
                }

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

                int pointerCount = 0;
                if (paramType->type() == poNodeType::POINTER)
                {
                    poPointerNode* pointerNode = static_cast<poPointerNode*>(paramType);
                    pointerCount = pointerNode->count();
                    paramType = pointerNode->child();
                }

                if (!checkEquivalence(checkExpr(childExpr), getPointerType(getType(paramType->token()), pointerCount)))
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

void poTypeChecker::checkIfStatement(po::poNode* node, const int returnType)
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

void poTypeChecker::checkWhileStatement(po::poNode* node, const int returnType)
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
        else if (child->type() == poNodeType::STRIDE)
        {
            poUnaryNode* stride = static_cast<poUnaryNode*>(child);

            if (stride->child()->type() == poNodeType::STATEMENT)
            {
                poUnaryNode* statement = static_cast<poUnaryNode*>(stride->child());
                checkStatement(statement, returnType);
            }
            else
            {
                setError("Malformed stride syntax tree node.", stride->token());
            }
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

#include "poOptFold.h"
#include "poAST.h"
#include "poTypeChecker.h"

#include <assert.h>

using namespace po;

void poOptFold::fold(std::vector<poNode*>& ast)
{
    for (poNode* node : ast)
    {
        fold(node);
    }
}

void poOptFold::fold(poNode* ast)
{
    poListNode* module = static_cast<poListNode*>(ast);
    assert(module->type() == poNodeType::MODULE);
    for (poNode* child : module->list())
    {
        if (child->type() == poNodeType::NAMESPACE)
        {
            foldNamespace(child);
        }
    }
}

void poOptFold::foldNamespace(poNode* ast)
{
    poListNode* ns = static_cast<poListNode*>(ast);
    assert(ns->type() == poNodeType::NAMESPACE);
    for (poNode* child : ns->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            foldFunctions(child);
        }
    }
}

void poOptFold::foldFunctions(poNode* ast)
{
    poListNode* function = static_cast<poListNode*>(ast);
    assert(function->type() == poNodeType::FUNCTION);
    for (poNode* child : function->list())
    {
        if (child->type() == poNodeType::BODY)
        {
            foldBody(child);
        }
    }
}

void poOptFold::foldBody(poNode* ast)
{
    poListNode* body = static_cast<poListNode*>(ast);
    assert(body->type() == poNodeType::BODY);
    for (poNode* child : body->list())
    {
        if (child->type() == poNodeType::STATEMENT)
        {
            foldStatement(child);
        }
    }
}

void poOptFold::foldStatement(poNode* ast)
{
    poUnaryNode* statement = static_cast<poUnaryNode*>(ast);
    assert(statement->type() == poNodeType::STATEMENT);
    
    poNode* child = statement->child();
    switch (child->type())
    {
    case poNodeType::DECL:
        foldDecl(child);
        break;
    case poNodeType::ASSIGNMENT:
        foldAssignment(child);
        break;
    }
}


void poOptFold::foldDecl(poNode* ast)
{
    poUnaryNode* decl = static_cast<poUnaryNode*>(ast);
    assert(decl->type() == poNodeType::DECL);
    if (decl->child()->type() == poNodeType::ASSIGNMENT)
    {
        foldAssignment(decl->child());
    }
}

void poOptFold::foldAssignment(poNode* ast)
{
    poBinaryNode* assignment = static_cast<poBinaryNode*>(ast);
    assert(assignment->type() == poNodeType::ASSIGNMENT);
    poNode* fold = nullptr;
    foldExpr(assignment->right(), &fold);
    if (fold)
    {
        delete assignment->right(); /*TODO: recursively...*/
        assignment->setRight(fold);
    }
}

void poOptFold::foldExpr(poNode* ast, poNode** fold)
{
    switch (ast->type())
    {
    case poNodeType::ADD:
    case poNodeType::SUB:
    case poNodeType::MUL:
    case poNodeType::DIV:
        foldBinaryExpr(ast, fold);
        break;
    }
}

void poOptFold::foldBinaryExpr(poNode* ast, poNode** fold)
{
    poBinaryNode* binary = static_cast<poBinaryNode*>(ast);

    poNode* left = nullptr;
    poNode* right = nullptr;
    foldExpr(binary->left(), &left);
    foldExpr(binary->right(), &right);

    if (left)
    {
        delete binary->left();
        binary->setLeft(left);
    }
    if (right)
    {
        delete binary->right();
        binary->setRight(right);
    }

    if (binary->left()->type() == poNodeType::CONSTANT &&
        binary->right()->type() == poNodeType::CONSTANT)
    {
        poConstantNode* leftConstant = static_cast<poConstantNode*>(binary->left());
        poConstantNode* rightConstant = static_cast<poConstantNode*>(binary->right());

        if (leftConstant->type() == TYPE_I64 &&
            rightConstant->type() == TYPE_I64)
        {
            int64_t left = 0;
            int64_t right = 0;
            int64_t result = 0;

            left = leftConstant->i64();
            right = rightConstant->i64();

            switch (ast->type())
            {
            case poNodeType::ADD:
                result = left + right;
                break;
            case poNodeType::MUL:
                result = left * right;
                break;
            case poNodeType::SUB:
                result = left - right;
                break;
            case poNodeType::DIV:
                result = left / right;
                break;
            }

            *fold = new poConstantNode(poNodeType::CONSTANT, binary->token(), result);
        }
    }
}

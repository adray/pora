#include "poEval.h"
#include "poAST.h"

using namespace po;

poEvaluator::poEvaluator()
    :
    _isError(false)
{
}

void poEvaluator::reset()
{
    _isError = false;
}

int64_t poEvaluator:: evaluateI64(poNode* node)
{
    if (node->type() == poNodeType::CONSTANT)
    {
        poConstantNode* constant = static_cast<poConstantNode*>(node);
        if (constant->type() != TYPE_I64)
        {
            // Error: not an i64 constant
            _isError = true;
            return 0;
        }
        return constant->i64();
    }

    switch (node->type())
    {
    case poNodeType::ADD:
    case poNodeType::SUB:
    case poNodeType::MUL:
    case poNodeType::DIV:
    case poNodeType::LEFT_SHIFT:
    case poNodeType::RIGHT_SHIFT:
        return evaluateBinaryI64(node);
    }

    // Error: unsupported node type
    _isError = true;
    return 0;
}

int64_t poEvaluator::evaluateBinaryI64(poNode* node)
{
    poBinaryNode* binary = static_cast<poBinaryNode*>(node);
    int64_t left = evaluateI64(binary->left());
    if (_isError) return 0;
    int64_t right = evaluateI64(binary->right());
    if (_isError) return 0;

    int64_t result = 0;
    switch (node->type())
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
    case poNodeType::LEFT_SHIFT:
        result = left << right;
        break;
    case poNodeType::RIGHT_SHIFT:
        result = left >> right;
        break;
    }

    return result;
}

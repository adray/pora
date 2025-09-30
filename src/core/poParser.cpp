#include "poParser.h"
#include "poAST.h"

#include <assert.h>

using namespace po;

poParser::poParser(const std::vector<poToken>& tokens)
    :
    _tokens(tokens),
    _pos(0),
    _error(false),
    _line(0),
    _col(0)
{
}

bool poParser::match(poTokenType type) const
{
    return !eof() && _tokens[_pos].token() == type;
}

bool poParser::matchPrimitiveType() const
{
    if (eof())
    {
        return false;
    }

    poTokenType type = _tokens[_pos].token();

    return type == poTokenType::BOOLEAN || type == poTokenType::U16_TYPE ||
        type == poTokenType::I16_TYPE || type == poTokenType::I32_TYPE ||
        type == poTokenType::I64_TYPE || type == poTokenType::I8_TYPE ||
        type == poTokenType::U32_TYPE || type == poTokenType::U64_TYPE ||
        type == poTokenType::U8_TYPE || type == poTokenType::F32_TYPE ||
        type == poTokenType::F64_TYPE;
}

bool poParser::lookahead(const poTokenType type, const int amount)
{
    return !(_pos + amount + 1 >= int(_tokens.size())) && _tokens[_pos + amount + 1].token() == type;
}

void poParser::advance()
{
    _pos++;
}

const poToken& poParser::peek()
{
    return _tokens[_pos];
}

void poParser::setError(const std::string& text)
{
    if (!_error)
    {
        _error = true;
        _errorText = text;

        if (eof())
        {
            _line = _tokens[_tokens.size() - 1].line();
            _col = _tokens[_tokens.size() - 1].column();
        }
        else
        {
            _line = _tokens[_pos].line();
            _col = _tokens[_pos].column();
        }
    }
}

bool poParser::eof() const
{
    return _pos >= int(_tokens.size());
}


//
// poFunctionParser
//

poFunctionParser::poFunctionParser(poParser& parser)
    :
    _parser(parser)
{
}

poNode* poFunctionParser::parseCall(const poToken& name)
{
    std::vector<poNode*> nodes;

    if (!_parser.match(poTokenType::CLOSE_PARAN))
    {
        poNode* node = parseTerm();
        if (node)
        {
            nodes.push_back(node);
            while (_parser.match(poTokenType::COMMA))
            {
                _parser.advance();
                nodes.push_back(parseTerm());
            }
        }
    }

    return new poListNode(poNodeType::CALL,
        nodes,
        name);
}

poNode* poFunctionParser::parseMember(poNode* variable, const poToken& token)
{
    poNode* node = variable;
    while (_parser.match(poTokenType::DOT))
    {
        _parser.advance();
        const poToken& next = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            node = new poBinaryNode(poNodeType::MEMBER_CALL, node, parseCall(next), next);
        }
        else if (_parser.match(poTokenType::OPEN_BRACKET))
        {
            _parser.advance();

            poNode* accessor = parseExpression();

            if (_parser.match(poTokenType::CLOSE_BRACKET))
            {
                _parser.advance();

                node = new poUnaryNode(poNodeType::MEMBER, node, next);
                node = new poArrayAccessor(accessor, node, poNodeType::ARRAY_ACCESSOR, token);
            }
            else
            {
                _parser.setError("Unexpected token.");
            }
        }
        else
        {
            node = new poUnaryNode(poNodeType::MEMBER, node, next);
        }
    }
    return node;
}

poNode* poFunctionParser::parsePrimary()
{
    poNode* node = nullptr;
    if (_parser.match(poTokenType::TRUE) ||
        _parser.match(poTokenType::FALSE) ||
        _parser.match(poTokenType::F32) ||
        _parser.match(poTokenType::F64) ||
        _parser.match(poTokenType::I32) ||
        _parser.match(poTokenType::I64) ||
        _parser.match(poTokenType::I8) ||
        _parser.match(poTokenType::I16) ||
        _parser.match(poTokenType::U32) ||
        _parser.match(poTokenType::U64) ||
        _parser.match(poTokenType::U8) ||
        _parser.match(poTokenType::U16) ||
        _parser.match(poTokenType::STRING) ||
        _parser.match(poTokenType::CHAR))
    {
        auto& token = _parser.peek();
        _parser.advance();
        node = new poConstantNode(poNodeType::CONSTANT, token);
    }
    else if (_parser.match(poTokenType::SIZEOF))
    {
        auto& token = _parser.peek();
        _parser.advance();

        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();
            if (_parser.match(poTokenType::IDENTIFIER) || _parser.matchPrimitiveType())
            {
                auto& typeToken = _parser.peek();
                _parser.advance();
                if (_parser.match(poTokenType::CLOSE_PARAN))
                {
                    _parser.advance();
                    node = new poUnaryNode(poNodeType::SIZEOF, new poNode(poNodeType::TYPE, typeToken), token);
                }
                else
                {
                    _parser.setError("Expected closing parenthesis.");
                }
            }
            else
            {
                _parser.setError("Expected a type or identifier after 'sizeof'.");
            }
        }
        else
        {
            _parser.setError("Expected opening parenthesis after 'sizeof'.");
        }
    }
    else if (_parser.match(poTokenType::NULLPTR))
    {
        auto& token = _parser.peek();
        _parser.advance();
        node = new poNode(poNodeType::NULLPTR, token);
    }
    else if (_parser.match(poTokenType::IDENTIFIER))
    {
        const poToken token = _parser.peek();
        _parser.advance();

        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();
            node = parseCall(token);

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
            }
            else
            {
                _parser.setError("Unexpected token.");
            }
        }
        else if (_parser.match(poTokenType::OPEN_BRACKET))
        {
            _parser.advance();

            poNode* accessor = parseExpression();

            if (_parser.match(poTokenType::CLOSE_BRACKET))
            {
                _parser.advance();

                node = new poArrayAccessor(accessor, new poNode(poNodeType::VARIABLE, token), poNodeType::ARRAY_ACCESSOR, token);
            }
            else
            {
                _parser.setError("Unexpected token.");
            }
        }
        else if (_parser.match(poTokenType::DOT))
        {
            node = parseMember(new poNode(poNodeType::VARIABLE, token), token);
        }
        else
        {
            node = new poNode(poNodeType::VARIABLE, token);
        }
    }
    else if (_parser.match(poTokenType::OPEN_PARAN))
    {
        _parser.advance();

        bool isCast = false;
        if (_parser.match(poTokenType::IDENTIFIER) ||
            _parser.matchPrimitiveType())
        {
            /* We are potentially a cast operator */

            if (_parser.lookahead(poTokenType::CLOSE_PARAN, 0))
            {
                isCast = true;

                auto& token = _parser.peek();
                _parser.advance();

                if (_parser.match(poTokenType::CLOSE_PARAN))
                {
                    _parser.advance();

                    poNode* castExpr = nullptr;
                    if (_parser.match(poTokenType::OPEN_PARAN))
                    {
                        castExpr = parseExpression();
                    }
                    else
                    {
                        castExpr = parsePrimary();
                    }
                    
                    node = new poUnaryNode(poNodeType::CAST,
                            new poUnaryNode(poNodeType::TYPE, castExpr, token),
                            token);
                }
            }
            else if (_parser.lookahead(poTokenType::STAR, 0) &&
                (_parser.lookahead(poTokenType::CLOSE_PARAN, 1) || _parser.lookahead(poTokenType::STAR, 1)))
            {
                isCast = true; /* Cast to a pointer */

                auto& token = _parser.peek();
                _parser.advance();

                const int pointerCount = parsePointer();

                if (_parser.match(poTokenType::CLOSE_PARAN))
                {
                    _parser.advance();

                    poNode* castExpr = nullptr;
                    if (_parser.match(poTokenType::OPEN_PARAN))
                    {
                        castExpr = parseExpression();
                    }
                    else
                    {
                        castExpr = parsePrimary();
                    }

                    node = new poUnaryNode(poNodeType::CAST,
                        new poUnaryNode(poNodeType::TYPE,
                            new poPointerNode(poNodeType::POINTER, castExpr, token, pointerCount),
                            token),
                        token);
                }
            }
            else if (_parser.lookahead(poTokenType::OPEN_BRACKET, 0))
            {
                isCast = true; /* Cast to an array */

                auto& token = _parser.peek();
                _parser.advance();

                if (_parser.match(poTokenType::OPEN_BRACKET))
                {
                    _parser.advance();
                }

                if (!_parser.isError() && _parser.match(poTokenType::I64))
                {
                    auto& size = _parser.peek();
                    _parser.advance();

                    if (_parser.match(poTokenType::CLOSE_BRACKET))
                    {
                        _parser.advance();
                    }

                    if (!_parser.isError() && _parser.match(poTokenType::CLOSE_PARAN))
                    {
                        _parser.advance();

                        node = new poUnaryNode(poNodeType::CAST,
                            new poUnaryNode(poNodeType::TYPE,  new poArrayNode(size.i64(), parseExpression(), poNodeType::ARRAY, token),
                                token),
                            token);
                    }
                }
            }
        }

        if (!isCast)
        {
            node = parseExpression();

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
            }
        }
    }
    else
    {
        _parser.setError("Unexpected token.");
    }
    return node;
}

poNode* poFunctionParser::parseUnary()
{
    if (_parser.match(poTokenType::MINUS))
    {
        poToken token = _parser.peek();
        _parser.advance();
        return new poUnaryNode(poNodeType::UNARY_SUB, parsePrimary(), token);
    }
    else if (_parser.match(poTokenType::AMPERSAND))
    {
        poToken token = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::IDENTIFIER))
        {
            poToken variable = _parser.peek();
            _parser.advance();

            return new poUnaryNode(poNodeType::REFERENCE, 
                new poNode(poNodeType::VARIABLE, variable),
                token);
        }
        else
        {
            _parser.setError("Expected an identifier.");
        }
    }
    else if (_parser.match(poTokenType::STAR))
    {
        poToken token = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::IDENTIFIER))
        {
            poToken variable = _parser.peek();
            _parser.advance();

            return new poUnaryNode(poNodeType::DEREFERENCE,
                new poNode(poNodeType::VARIABLE, variable),
                token);
        }
        else
        {
            _parser.setError("Expected an identifier.");
        }
    }

    return parsePrimary();
}

poNode* poFunctionParser::parseFactor()
{
    poNode* node = parseUnary();
    while (_parser.match(poTokenType::STAR) || _parser.match(poTokenType::SLASH))
    {
        const poToken token = _parser.peek();
        _parser.advance();
        node = new poBinaryNode(
            token.token() == poTokenType::STAR ? poNodeType::MUL : poNodeType::DIV,
            node,
            parseUnary(),
            token);
    }

    return node;
}

poNode* poFunctionParser::parseTerm()
{
    poNode* node = parseFactor();
    while (_parser.match(poTokenType::PLUS) || _parser.match(poTokenType::MINUS))
    {
        const poToken token = _parser.peek();
        _parser.advance();
        node = new poBinaryNode(
            token.token() == poTokenType::PLUS ? poNodeType::ADD : poNodeType::SUB,
            node,
            parseFactor(),
            token);
    }

    return node;
}

poNode* poFunctionParser::parseEquality()
{
    poNode* node = parseTerm();
    while (_parser.match(poTokenType::EQUALS_EQUALS) ||
        _parser.match(poTokenType::NOT_EQUALS) ||
        _parser.match(poTokenType::LESS) ||
        _parser.match(poTokenType::GREATER) ||
        _parser.match(poTokenType::LESS_EQUALS) ||
        _parser.match(poTokenType::GREATER_EQUALS))
    {
        auto& token = _parser.peek();
        _parser.advance();

        poNodeType type = poNodeType::CMP_EQUALS;
        switch (token.token())
        {
        case poTokenType::EQUALS_EQUALS:
            type = poNodeType::CMP_EQUALS;
            break;
        case poTokenType::NOT_EQUALS:
            type = poNodeType::CMP_NOT_EQUALS;
            break;
        case poTokenType::LESS:
            type = poNodeType::CMP_LESS;
            break;
        case poTokenType::GREATER:
            type = poNodeType::CMP_GREATER;
            break;
        case poTokenType::LESS_EQUALS:
            type = poNodeType::CMP_LESS_EQUALS;
            break;
        case poTokenType::GREATER_EQUALS:
            type = poNodeType::CMP_GREATER_EQUALS;
            break;
        }

        node = new poBinaryNode(type,
            node,
            parseTerm(),
            token);
    }

    return node;
}

poNode* poFunctionParser::parseAnd()
{
    poNode* node = parseEquality();
    while (_parser.match(poTokenType::AND))
    {
        auto& token = _parser.peek();
        _parser.advance();

        node = new poBinaryNode(poNodeType::AND, node, parseEquality(), token);
    }

    return node;
}

poNode* poFunctionParser::parseOr()
{
    poNode* node = parseAnd();
    while (_parser.match(poTokenType::OR))
    {
        auto& token = _parser.peek();
        _parser.advance();

        node = new poBinaryNode(poNodeType::OR, node, parseAnd(), token);
    }

    return node;
}

poNode* poFunctionParser::parseExpression()
{
    return parseOr();
}

poNode* poFunctionParser::parseRH(poNode* lhs)
{
    poNode* node = nullptr;
    if (_parser.match(poTokenType::EQUALS))
    {
        auto& assign = _parser.peek();
        _parser.advance();

        node = new poBinaryNode(poNodeType::ASSIGNMENT,
            lhs,
            parseTerm(),
            assign);
    }
    else
    {
        _parser.setError("Unexpected token.");
    }
    return node;
}

poNode* poFunctionParser::parseWhile()
{
    std::vector<poNode*> children;

    auto& whileStatement = _parser.peek();
    if (_parser.match(poTokenType::WHILE))
    {
        _parser.advance();
        if (!_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.setError("Expected open parenthesis.");
            return nullptr;
        }

        _parser.advance();
        poNode* equality = parseExpression();

        if (!_parser.match(poTokenType::CLOSE_PARAN))
        {
            _parser.setError("Expected close parenthesis.");
            return nullptr;
        }
        _parser.advance();

        children.push_back(equality);

        if (!_parser.match(poTokenType::OPEN_BRACE))
        {
            _parser.setError("Expected open brace.");
            return nullptr;
        }

        poNode* body = parseBody(true);
        children.push_back(body);

        if (!_parser.match(poTokenType::CLOSE_BRACE))
        {
            _parser.setError("Expected close brace.");
            return nullptr;
        }
        _parser.advance();
    }

    return new poListNode(poNodeType::WHILE, children, whileStatement);
}

poNode* poFunctionParser::parseFor()
{
    std::vector<poNode*> children;

    auto& forStatement = _parser.peek();
    if (_parser.match(poTokenType::FOR))
    {
        _parser.advance();
        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();

            if (_parser.match(poTokenType::SEMICOLON))
            {
                _parser.advance();
            }
            else
            {
                children.push_back(parseStatement());
            }

            poNode* equality = parseExpression();
            if (!_parser.match(poTokenType::SEMICOLON))
            {
                _parser.setError("Expected semicolon.");
                return nullptr;
            }
            _parser.advance();

            poNode* stride = new poUnaryNode(poNodeType::STRIDE, parseExpressionStatement(), forStatement);
            if (!_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.setError("Expected close parenthesis.");
                return nullptr;
            }
            _parser.advance();

            if (!_parser.match(poTokenType::OPEN_BRACE))
            {
                _parser.setError("Expected open brace.");
                return nullptr;
            }
            poNode* body = parseBody(true);

            if (!_parser.match(poTokenType::CLOSE_BRACE))
            {
                _parser.setError("Expected close brace.");
                return nullptr;
            }
            _parser.advance();

            std::vector<poNode*> loopChildren = {
                body,
                equality,
                stride
            };

            poListNode* loopNode = new poListNode(poNodeType::WHILE, loopChildren, forStatement);
            children.push_back(loopNode);
        }
    }

    return new poUnaryNode(poNodeType::STATEMENT, new poListNode(poNodeType::BODY, children, forStatement), forStatement);
}

poNode* poFunctionParser::insertCompare(poNode* node)
{
    poToken expr = node->token();
    poNode* compare = new poBinaryNode(poNodeType::CMP_EQUALS,
        node,
        new poConstantNode(poNodeType::CONSTANT, poToken(poTokenType::TRUE, expr.string(), expr.line(), expr.column())),
        expr);
    return compare;
}

void poFunctionParser::fixupExpression(poNode* node)
{
    if (node->type() == poNodeType::AND ||
        node->type() == poNodeType::OR)
    {
        poBinaryNode* binary = static_cast<poBinaryNode*>(node);
        if (binary->left()->type() == poNodeType::VARIABLE ||
            binary->left()->type() == poNodeType::CONSTANT)
        {
            poNode* compare = insertCompare(binary->left());
            binary->setLeft(compare);
        }
        if (binary->right()->type() == poNodeType::VARIABLE ||
            binary->left()->type() == poNodeType::CONSTANT)
        {
            poNode* compare = insertCompare(binary->right());
            binary->setRight(compare);
        }

        fixupExpression(binary->left());
        fixupExpression(binary->right());
    }
}

poNode* poFunctionParser::parseIfStatement(const bool isLoop)
{
    std::vector<poNode*> children;

    auto& ifStatement = _parser.peek();
    if (_parser.match(poTokenType::IF))
    {
        auto& expr = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();

            if ((_parser.match(poTokenType::IDENTIFIER) ||
                _parser.match(poTokenType::TRUE) ||
                _parser.match(poTokenType::FALSE)) &&
                _parser.lookahead(poTokenType::CLOSE_PARAN, 0))
            {
                poNode* expression = parseExpression();
                poNode* compare = new poBinaryNode(poNodeType::CMP_EQUALS,
                    expression,
                    new poConstantNode(poNodeType::CONSTANT, poToken(poTokenType::TRUE, expr.string(), expr.line(), expr.column())),
                    expr);

                children.push_back(new poUnaryNode(poNodeType::STATEMENT, compare, expr));
            }
            else
            {
                poNode* expression = parseExpression();
                fixupExpression(expression);

                children.push_back(new poUnaryNode(poNodeType::STATEMENT, expression, expr));
            }

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
            }
        }
        else
        {
            _parser.setError("Expected parenthesis.");
        }

        if (_parser.match(poTokenType::OPEN_BRACE))
        {
            children.push_back(parseBody(isLoop));
        }
        else
        {
            _parser.setError("Expected brace.");
        }

        if (_parser.match(poTokenType::CLOSE_BRACE))
        {
            _parser.advance();
            if (_parser.match(poTokenType::ELSE))
            {
                _parser.advance();
                if (_parser.match(poTokenType::IF))
                {
                    children.push_back(new poUnaryNode(poNodeType::ELSE,
                        parseIfStatement(isLoop),
                        expr));
                }
                else if (_parser.match(poTokenType::OPEN_BRACE))
                {
                    children.push_back(new poUnaryNode(poNodeType::ELSE,
                        parseBody(isLoop),
                        expr));
                    if (_parser.match(poTokenType::CLOSE_BRACE))
                    {
                        _parser.advance();
                    }
                    else
                    {
                        _parser.setError("Expected brace.");
                    }
                }
            }
        }
        else
        {
            _parser.setError("Expected parenthesis.");
        }
    }

    return new poListNode(poNodeType::IF, children, ifStatement);
}

const int poFunctionParser::parsePointer()
{
    int pointerCount = 0;
    while (_parser.match(poTokenType::STAR))
    {
        // pointer
        _parser.advance();
        pointerCount++;
    }
    return pointerCount;
}

poNode* poFunctionParser::parseDecl(const poToken& type)
{
    poNode* statement = nullptr;

    const int pointerCount = parsePointer();

    int64_t arraySize = 0;
    bool isArray = false;
    if (_parser.match(poTokenType::OPEN_BRACKET))
    {
        _parser.advance();
        if (_parser.match(poTokenType::I64))
        {
            arraySize = _parser.peek().i64();
            _parser.advance();

            if (_parser.match(poTokenType::CLOSE_BRACKET))
            {
                isArray = true;

                _parser.advance();
            }
            else
            {
                _parser.setError("Expected close bracket.");
            }
        }
        else
        {
            _parser.setError("Expected array size.");
        }
    }

    // variable decl
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        const auto& id = _parser.peek();
        _parser.advance();

        poNode* variable = new poNode(poNodeType::VARIABLE, id);
        if (pointerCount > 0)
        {
            variable = new poPointerNode(poNodeType::POINTER, variable, id, pointerCount);
        }
        if (isArray)
        {
            variable = new poArrayNode(arraySize, 
                    variable,
                    poNodeType::ARRAY, id);
        }

        if (_parser.match(poTokenType::SEMICOLON))
        {
            // end
            poNode* decl = new poUnaryNode(poNodeType::DECL,
                variable,
                type);
            statement = new poUnaryNode(poNodeType::STATEMENT, decl, type);
        }
        else
        {
            poNode* lhs = variable;
            poNode* assign = parseRH(lhs);
            if (assign)
            {
                poNode* decl = new poUnaryNode(poNodeType::DECL,
                    assign,
                    type);
                statement = new poUnaryNode(poNodeType::STATEMENT, decl, type);
            }
            else
            {
                _parser.setError("Assignment node error.");
            }
        }
    }
    else
    {
        _parser.setError("Expected identifier.");
    }

    return statement;
}

poNode* poFunctionParser::parseExpressionStatement()
{
    auto& id = _parser.peek();
    _parser.advance();

    if (_parser.match(poTokenType::DOT))
    {
        poNode* member = parseMember(new poNode(poNodeType::VARIABLE, id), id);
        if (_parser.match(poTokenType::EQUALS))
        {
            // assignment
            poNode* assign = parseRH(member);
            return new poUnaryNode(poNodeType::STATEMENT, assign, id);
        }
        else
        {
            _parser.setError("Unexpected token.");
        }
    }
    else if (_parser.match(poTokenType::EQUALS))
    {
        // assignment
        poNode* assign = parseRH(new poNode(poNodeType::VARIABLE, id));
        return new poUnaryNode(poNodeType::STATEMENT, assign, id);
    }
    else if (_parser.match(poTokenType::OPEN_PARAN))
    {
        // call
        _parser.advance();
        poNode* node = new poUnaryNode(poNodeType::STATEMENT, parseCall(id), id);

        if (_parser.match(poTokenType::CLOSE_PARAN))
        {
            _parser.advance();
        }
        else
        {
            _parser.setError("Unexpected token.");
        }

        return node;
    }
    else if (_parser.match(poTokenType::OPEN_BRACKET))
    {
        _parser.advance();

        poNode* accessor = parseExpression();

        if (_parser.match(poTokenType::CLOSE_BRACKET))
        {
            _parser.advance();

            poNode* lhs = new poArrayAccessor(accessor, new poNode(poNodeType::VARIABLE, id), poNodeType::ARRAY_ACCESSOR, id);

            if (_parser.match(poTokenType::EQUALS))
            {
                // assignment
                poNode* assign = parseRH(lhs);
                return new poUnaryNode(poNodeType::STATEMENT, assign, id);
            }
            else
            {
                _parser.setError("Unexpected token.");
            }
        }
        else
        {
            _parser.setError("Unexpected token.");
        }
    }
    else
    {
        _parser.setError("Unexpected token.");
    }

    return nullptr;
}

poNode* poFunctionParser::parseStatement()
{
    poNode* statement = nullptr;

    if (_parser.match(poTokenType::STAR))
    {
        // Pointer assignment
        _parser.advance();

        if (_parser.match(poTokenType::IDENTIFIER))
        {
            const poToken& id = _parser.peek();
            _parser.advance();

            if (_parser.match(poTokenType::EQUALS))
            {
                poUnaryNode* lhs = new poUnaryNode(poNodeType::DEREFERENCE,
                    new poNode(poNodeType::VARIABLE, id),
                    id);

                statement = new poUnaryNode(poNodeType::STATEMENT, parseRH(lhs), id);

                if (_parser.match(poTokenType::SEMICOLON))
                {
                    _parser.advance();
                    return statement;
                }
            }
        }

        _parser.setError("Unexpected token");
        return nullptr;
    }

    auto& id = _parser.peek();
    _parser.advance();
    if (_parser.match(poTokenType::DOT))
    {
        // Expression statement

        poNode* member = parseMember(new poNode(poNodeType::VARIABLE, id), id);
        if (_parser.match(poTokenType::EQUALS))
        {
            // assignment
            poNode* assign = parseRH(member);
            statement = new poUnaryNode(poNodeType::STATEMENT, assign, id);
        }
        else
        {
            _parser.setError("Unexpected token.");
        }
    }
    else if (_parser.match(poTokenType::EQUALS))
    {
        // assignment
        poNode* assign = parseRH(new poNode(poNodeType::VARIABLE, id));
        statement = new poUnaryNode(poNodeType::STATEMENT, assign, id);
    }
    else if (_parser.match(poTokenType::OPEN_PARAN))
    {
        // call
        _parser.advance();
        poNode* node = new poUnaryNode(poNodeType::STATEMENT, parseCall(id), id);

        if (_parser.match(poTokenType::CLOSE_PARAN))
        {
            _parser.advance();
        }
        else
        {
            _parser.setError("Unexpected token.");
        }

        statement = node;
    }
    else if (_parser.match(poTokenType::OPEN_BRACKET))
    {
        _parser.advance();

        poNode* accessor = nullptr;
        if (!_parser.match(poTokenType::CLOSE_BRACKET))
        {
            accessor = parseExpression(); /* parse the expression in the array accessor */
        }

        if (_parser.match(poTokenType::CLOSE_BRACKET))
        {
            _parser.advance();

            if (_parser.match(poTokenType::IDENTIFIER))
            {
                // Decl

                const poToken& name = _parser.peek();
                _parser.advance();

                if (accessor != nullptr)
                {
                    /* Fixed array decl */

                    if (accessor->type() == poNodeType::CONSTANT)
                    {
                        poNode* variable = new poArrayNode(accessor->token().i64(),
                            new poNode(poNodeType::VARIABLE, name),
                            poNodeType::ARRAY, name);

                        poNode* decl = new poUnaryNode(poNodeType::DECL,
                            variable,
                            id);
                        statement = new poUnaryNode(poNodeType::STATEMENT, decl, id);
                    }
                    else
                    {
                        _parser.setError("Array size must be constant.");
                    }

                    delete accessor;
                }
                else
                {
                    /* Dynamic array decl */

                    poNode* variable = new poUnaryNode(
                        poNodeType::DYNAMIC_ARRAY, 
                        new poNode(poNodeType::VARIABLE, name),
                        name);

                    poNode* child = nullptr;

                    if (_parser.match(poTokenType::EQUALS))
                    {
                        // Array assignment
                        poNode* rhs = parseRH(variable);
                        child = new poUnaryNode(poNodeType::DECL,
                            rhs,
                            id);
                    }
                    else
                    {
                        child = new poUnaryNode(poNodeType::DECL,
                            variable,
                            id);
                    }

                    statement = new poUnaryNode(poNodeType::STATEMENT, child, id);
                }
            }
            else if (_parser.match(poTokenType::EQUALS))
            {
                poNode* lhs = new poArrayAccessor(accessor, new poNode(poNodeType::VARIABLE, id), poNodeType::ARRAY_ACCESSOR, id);
                
                // assignment
                poNode* assign = parseRH(lhs);
                statement = new poUnaryNode(poNodeType::STATEMENT, assign, id);
            }
            else if (_parser.match(poTokenType::DOT))
            {
                // assignment

                poNode* member = parseMember(new poArrayAccessor(accessor,
                    new poNode(poNodeType::VARIABLE, id),
                    poNodeType::ARRAY_ACCESSOR, id),
                    id);

                if (_parser.match(poTokenType::EQUALS))
                {
                    poNode* assign = parseRH(member);
                    statement = new poUnaryNode(poNodeType::STATEMENT, assign, id);
                }
                else
                {
                    _parser.setError("Unexpected token.");
                }
            }
            else
            {
                _parser.setError("Unexpected token.");
            }
        }
        else
        {
            _parser.setError("Unexpected token.");
        }
    }
    else if (_parser.match(poTokenType::IDENTIFIER) ||
        _parser.match(poTokenType::STAR))
    {
        statement = parseDecl(id);
    }
    else
    {
        _parser.setError("Unexpected token.");
    }

    if (_parser.match(poTokenType::SEMICOLON))
    {
        _parser.advance();
    }
    else
    {
        _parser.setError("Unexpected token.");
    }

    return statement;
}

poNode* poFunctionParser::parseBody(const bool isLoop)
{
    auto& function = _parser.peek();
    _parser.advance();

    std::vector<poNode*> children;

    while (!_parser.match(poTokenType::CLOSE_BRACE))
    {
        if (_parser.match(poTokenType::BOOLEAN) ||
            _parser.match(poTokenType::I32_TYPE) ||
            _parser.match(poTokenType::I64_TYPE) ||
            _parser.match(poTokenType::I16_TYPE) ||
            _parser.match(poTokenType::I8_TYPE) ||
            _parser.match(poTokenType::U32_TYPE) ||
            _parser.match(poTokenType::U64_TYPE) ||
            _parser.match(poTokenType::U16_TYPE) ||
            _parser.match(poTokenType::U8_TYPE) ||
            _parser.match(poTokenType::F32_TYPE) ||
            _parser.match(poTokenType::F64_TYPE) ||
            _parser.match(poTokenType::OBJECT) ||
            _parser.match(poTokenType::IDENTIFIER) ||
            _parser.match(poTokenType::STAR))
        {
            children.push_back(parseStatement());
        }
        else if (_parser.match(poTokenType::RETURN))
        {
            auto& returnToken = _parser.peek();
            _parser.advance();

            if (_parser.match(poTokenType::SEMICOLON))
            {
                _parser.advance();
                children.push_back(new poUnaryNode(poNodeType::STATEMENT,
                    new poUnaryNode(poNodeType::RETURN, nullptr, returnToken),
                    returnToken));
            }
            else
            {
                poNode* term = parseTerm();
                if (_parser.match(poTokenType::SEMICOLON))
                {
                    _parser.advance();
                    children.push_back(new poUnaryNode(poNodeType::STATEMENT,
                        new poUnaryNode(poNodeType::RETURN, term, returnToken),
                        returnToken));
                }
                else
                {
                    _parser.setError("Expected semicolon.");
                }
            }
        }
        else if (_parser.match(poTokenType::IF))
        {
            children.push_back(parseIfStatement(isLoop));
        }
        else if (_parser.match(poTokenType::FOR))
        {
            children.push_back(parseFor());
        }
        else if (_parser.match(poTokenType::WHILE))
        {
            children.push_back(parseWhile());
        }
        else if (_parser.match(poTokenType::CONTINUE))
        {
            if (isLoop)
            {
                const poToken& token = _parser.peek();
                children.push_back(new poNode(poNodeType::CONTINUE, token));
                _parser.advance();

                if (_parser.match(poTokenType::SEMICOLON))
                {
                    _parser.advance();
                }
                else
                {
                    _parser.setError("Expected semicolon.");
                }
            }
            else
            {
                _parser.setError("Unexpected continue.");
            }
        }
        else if (_parser.match(poTokenType::BREAK))
        {
            if (isLoop)
            {
                const poToken& token = _parser.peek();
                children.push_back(new poNode(poNodeType::BREAK, token));
                _parser.advance();

                if (_parser.match(poTokenType::SEMICOLON))
                {
                    _parser.advance();
                }
                else
                {
                    _parser.setError("Expected semicolon.");
                }
            }
            else
            {
                _parser.setError("Unexpected break.");
            }
        }
        else
        {
            _parser.setError("Unexpected token.");
        }

        if (_parser.isError())
        {
            return nullptr;
        }
    }

    return new poListNode(poNodeType::BODY, children, function);
}

poNode* poFunctionParser::parseArg()
{
    poNode* node = nullptr;
    if (_parser.match(poTokenType::BOOLEAN) ||
        _parser.match(poTokenType::I64_TYPE) ||
        _parser.match(poTokenType::I32_TYPE) ||
        _parser.match(poTokenType::I8_TYPE) ||
        _parser.match(poTokenType::U64_TYPE) ||
        _parser.match(poTokenType::U32_TYPE) ||
        _parser.match(poTokenType::U8_TYPE) ||
        _parser.match(poTokenType::F64_TYPE) ||
        _parser.match(poTokenType::F32_TYPE) ||
        _parser.match(poTokenType::OBJECT) ||
        _parser.match(poTokenType::IDENTIFIER))
    {
        auto& type = _parser.peek();
        _parser.advance();
        
        const int pointerCount = parsePointer();
        if (_parser.match(poTokenType::IDENTIFIER))
        {
            auto& id = _parser.peek();
            _parser.advance();

            node = new poNode(poNodeType::TYPE, type);
            if (pointerCount > 0)
            {
                node = new poPointerNode(poNodeType::POINTER, node, type, pointerCount);
            }
            node = new poUnaryNode(poNodeType::PARAMETER, node, id);
        }
    }

    return node;
}

poNode* poFunctionParser::parse(const poToken& ret)
{
    poNode* node = nullptr;

    _parser.advance();

    const int pointerCount = parsePointer();
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        const poToken& id = _parser.peek();
        _parser.advance();

        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();
            std::vector<poNode*> nodes;
            // parse args...
            std::vector<poNode*> args;
            const auto& token = _parser.peek();

            if (!_parser.match(poTokenType::CLOSE_PARAN))
            {
                args.push_back(parseArg());
                while (_parser.match(poTokenType::COMMA))
                {
                    _parser.advance();
                    args.push_back(parseArg());
                }
            }

            nodes.push_back(new poListNode(poNodeType::ARGS, args, token));

            if (pointerCount > 0)
            {
                nodes.push_back(new poUnaryNode(poNodeType::RETURN_TYPE, 
                    new poPointerNode(
                        poNodeType::POINTER,
                        new poNode(poNodeType::TYPE, ret),
                        ret,
                        pointerCount),
                    ret));
            }
            else
            {
                nodes.push_back(new poUnaryNode(poNodeType::RETURN_TYPE, new poNode(poNodeType::TYPE, ret), ret));
            }

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
                if (_parser.match(poTokenType::OPEN_BRACE))
                {
                    nodes.push_back(parseBody(false));
                    node = new poListNode(poNodeType::FUNCTION, nodes, id);
                }
                else
                {
                    _parser.setError("Expected open brace.");
                    return nullptr;
                }

                if (_parser.match(poTokenType::CLOSE_BRACE))
                {
                    _parser.advance();
                }
                else
                {
                    _parser.setError("Expected close brace.");
                }
            }
            else
            {
                _parser.setError("Expected close bracket.");
            }
        }
        else
        {
            _parser.setError("Expected open bracket.");
        }
    }
    else
    {
        _parser.setError("Expected identifier");
    }

    return node;
}

poNode* poFunctionParser::parseExtern(const poToken& ret)
{
    poNode* node = nullptr;
    _parser.advance();

    const int pointerCount = parsePointer();
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        const poToken& id = _parser.peek();
        _parser.advance();

        if (_parser.match(poTokenType::OPEN_PARAN))
        {
            _parser.advance();

            std::vector<poNode*> nodes;
            // parse args...
            std::vector<poNode*> args;
            const auto& token = _parser.peek();

            if (!_parser.match(poTokenType::CLOSE_PARAN))
            {
                args.push_back(parseArg());
                while (_parser.match(poTokenType::COMMA))
                {
                    _parser.advance();
                    args.push_back(parseArg());
                }
            }

            nodes.push_back(new poListNode(poNodeType::ARGS, args, token));

            if (pointerCount > 0)
            {
                nodes.push_back(new poUnaryNode(poNodeType::RETURN_TYPE,
                    new poPointerNode(
                        poNodeType::POINTER,
                        new poNode(poNodeType::TYPE, ret),
                        ret,
                        pointerCount),
                    ret));
            }
            else
            {
                nodes.push_back(new poUnaryNode(poNodeType::RETURN_TYPE, new poNode(poNodeType::TYPE, ret), ret));
            }

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
            }
            else
            {
                _parser.setError("Expected close parenthesis.");
            }

            if (_parser.match(poTokenType::SEMICOLON))
            {
                _parser.advance();

                node = new poListNode(poNodeType::EXTERN, nodes, id);
            }
            else
            {
                _parser.setError("Expected close parenthesis.");
            }
        }
        else
        {
            _parser.setError("Expected open parenthesis.");
        }
    }
    else
    {
        _parser.setError("Expected identifier");
    }

    return node;
}

//
// poClassParser
//

poClassParser::poClassParser(poParser& parser)
    :
    _parser(parser)
{
}

poNode* poClassParser::parse()
{
    return nullptr;
}

//
// poStructParser
//

poStructParser::poStructParser(poParser& parser)
    :
    _parser(parser)
{
}

const int poStructParser::parsePointer()
{
    int pointerCount = 0;
    while (_parser.match(poTokenType::STAR))
    {
        // pointer
        _parser.advance();
        pointerCount++;
    }
    return pointerCount;
}

poNode* poStructParser::parse()
{
    _parser.advance();

    if (!_parser.match(poTokenType::IDENTIFIER))
    {
        _parser.setError("Expected identifier.");
        return nullptr;
    }

    poToken name = _parser.peek();
    _parser.advance();

    if (!_parser.match(poTokenType::OPEN_BRACE))
    {
        _parser.setError("Expected open brace");
        return nullptr;
    }

    _parser.advance();

    std::vector<poNode*> children;
    while (!_parser.match(poTokenType::CLOSE_BRACE) && !_parser.isError())
    {
        if (_parser.match(poTokenType::IDENTIFIER) ||
            _parser.matchPrimitiveType())
        {
            poToken type = _parser.peek();
            _parser.advance();

            const int pointerCount = parsePointer();

            if (_parser.match(poTokenType::IDENTIFIER))
            {
                poToken name = _parser.peek();
                _parser.advance();

                poNode* typeNode = new poNode(poNodeType::TYPE, type);
                if (pointerCount > 0)
                {
                    typeNode = new poPointerNode(poNodeType::POINTER, typeNode, name, pointerCount);
                }

                children.push_back(new poUnaryNode(poNodeType::DECL, typeNode, name));

                if (_parser.match(poTokenType::SEMICOLON))
                {
                    _parser.advance();
                }
                else
                {
                    _parser.setError("Expected semicolon.");
                }
            }
            else if (_parser.match(poTokenType::OPEN_BRACKET))
            {
                _parser.advance();
                if (_parser.match(poTokenType::I64))
                {
                    const int64_t size = _parser.peek().i64();

                    _parser.advance();
                    if (_parser.match(poTokenType::CLOSE_BRACKET))
                    {
                        _parser.advance();

                        poToken name = _parser.peek();
                        _parser.advance();

                        children.push_back(new poUnaryNode(poNodeType::DECL,
                            new poArrayNode(size, 
                                new poNode(poNodeType::TYPE, type),
                            poNodeType::ARRAY, name),
                            name));
                    }
                    else
                    {
                        _parser.setError("Expected array size.");
                    }

                    if (!_parser.isError() && _parser.match(poTokenType::SEMICOLON))
                    {
                        _parser.advance();
                    }
                    else
                    {
                        _parser.setError("Expected semicolon.");
                    }
                }
                else
                {
                    _parser.setError("Expected array size.");
                }
            }
            else
            {
                _parser.setError("Expected field name.");
            }
        }
        else
        {
            _parser.setError("Expected field type.");
        }
    }

    _parser.advance();

    return new poListNode(poNodeType::STRUCT, children, name);
}

//
// poNamespaceParser
//

poNamespaceParser::poNamespaceParser(poParser& parser)
    :
    _parser(parser)
{
}

poNode* poNamespaceParser::parseExternalFunction(const poToken& ret)
{
    poFunctionParser parser(_parser);
    return parser.parseExtern(ret);
}

poNode* poNamespaceParser::parseFunction(const poToken& ret)
{
    poFunctionParser parser(_parser);
    return parser.parse(ret);
}

poNode* poNamespaceParser::parseStruct()
{
    poStructParser parser(_parser);
    return parser.parse();
}

poNode* poNamespaceParser::parse()
{
    std::vector<poNode*> children;
    auto& token = _parser.peek();
    _parser.advance();
    if (_parser.match(poTokenType::OPEN_BRACE))
    {
        _parser.advance();

        while (!_parser.match(poTokenType::CLOSE_BRACE))
        {
            if (_parser.match(poTokenType::CLASS))
            {
                // Classes are not supported yet!
                _parser.setError("Classes not supported!");
            }
            else if (_parser.match(poTokenType::STRUCT))
            {
                children.push_back(parseStruct());
            }
            /*else if (_parser.match(poTokenType::VOID) ||
                _parser.match(poTokenType::I32_TYPE) ||
                _parser.match(poTokenType::I64_TYPE) ||
                _parser.match(poTokenType::F32_TYPE) ||
                _parser.match(poTokenType::F64_TYPE))
            {
                parseFunction(token);
            }*/
            else if (_parser.match(poTokenType::EXTERN))
            {
                _parser.advance();
                auto& token = _parser.peek();

                if (_parser.match(poTokenType::VOID) ||
                    _parser.matchPrimitiveType() ||
                    _parser.match(poTokenType::IDENTIFIER))
                {
                    children.push_back(parseExternalFunction(token));
                }
                else
                {
                    _parser.setError("Expected return type.");
                }
            }
            else if (_parser.match(poTokenType::STATIC))
            {
                _parser.advance();
                auto& token = _parser.peek();

                if (_parser.match(poTokenType::VOID) ||
                    _parser.matchPrimitiveType() ||
                    _parser.match(poTokenType::IDENTIFIER))
                {
                    children.push_back(parseFunction(token));
                }
                else
                {
                    _parser.setError("Expected return type.");
                }
            }
            else
            {
                _parser.setError("Unexpected token");
            }

            if (_parser.isError())
            {
                return nullptr;
            }
        }

        _parser.advance();
    }
    else
    {
        _parser.setError("Expected open brace.");
    }

    return new poListNode(poNodeType::NAMESPACE, children, token);
}

//
// poModuleParser
//

poModuleParser::poModuleParser(poParser& parser)
    :
    _parser(parser)
{
}

poNode* poModuleParser::parseNamespace()
{
    poNode* node = nullptr;
    _parser.advance();
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        auto& token = _parser.peek();
        
        poNamespaceParser ns(_parser);
        node = ns.parse();
    }
    else
    {
        _parser.setError("Expected identifier.");
    }

    return node;
}

poNode* poModuleParser::parseImport()
{
    poNode* node = nullptr;
    _parser.advance();
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        auto& id = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::SEMICOLON))
        {
            _parser.advance();

            node = new poNode(poNodeType::IMPORT, id);
        }
        else
        {
            _parser.setError("Expected semicolon.");
        }
    }
    else
    {
        _parser.setError("Expected identifier.");
    }

    return node;
}

poNode* poModuleParser::parseTopLevel()
{
    std::vector<poNode*> nodes;
    auto& token = _parser.peek();
    while (!_parser.isError() && !_parser.eof())
    {
        if (_parser.match(poTokenType::IMPORT))
        {
            nodes.push_back(parseImport());
        }
        else if (_parser.match(poTokenType::NAMESPACE))
        {
            nodes.push_back(parseNamespace());
        }
        else
        {
            _parser.setError("Unexpected token");
        }
    }

    return new poListNode(poNodeType::MODULE, nodes, token);
}

poNode* poModuleParser::parse()
{
    return parseTopLevel();
}


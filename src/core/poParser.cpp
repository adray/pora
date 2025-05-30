#include "poParser.h"
#include "poAST.h"

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

bool poParser::match(poTokenType type)
{
    return !eof() && _tokens[_pos].token() == type;
}

bool poParser::lookahead(poTokenType type)
{
    return !(_pos + 1 >= int(_tokens.size())) && _tokens[_pos + 1].token() == type;
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

bool poParser::eof()
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

    return new poListNode(poNodeType::CALL,
        nodes,
        name);
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
        _parser.match(poTokenType::U32) ||
        _parser.match(poTokenType::U64) ||
        _parser.match(poTokenType::U8) ||
        _parser.match(poTokenType::STRING) ||
        _parser.match(poTokenType::CHAR))
    {
        auto& token = _parser.peek();
        _parser.advance();
        node = new poConstantNode(poNodeType::CONSTANT, token);
    }
    else if (_parser.match(poTokenType::IDENTIFIER))
    {
        const poToken token = _parser.peek();
        _parser.advance();

        //while (_parser.match(poTokenType::DOT) || _parser.match(poTokenType::OPEN_PARAN))
        //{
        //    if (_parser.match(poTokenType::DOT))
        //    {
        //        _parser.advance();

        //        if (node == nullptr)
        //        {
        //            poNode* varNode = new poNode(poNodeType::VARIABLE, token);
        //            node = new poUnaryNode(poNodeType::MEMBER, varNode, token);
        //        }
        //        else
        //        {
        //            node = new poUnaryNode(poNodeType::MEMBER, node, token);
        //        }
        //    }
        //    else if (_parser.match(poTokenType::OPEN_PARAN))
        //    {
        //        _parser.advance();
        //        node = parseCall(token);

        //        if (_parser.match(poTokenType::CLOSE_PARAN))
        //        {
        //            _parser.advance();
        //        }
        //        else
        //        {
        //            _parser.setError("Unexpected token.");
        //        }
        //    }
        //}

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
        else
        {
            node = new poNode(poNodeType::VARIABLE, token);
        }
    }
    else if (_parser.match(poTokenType::OPEN_PARAN))
    {
        auto& token = _parser.peek();
        _parser.advance();

        node = parseExpression();
        if (_parser.match(poTokenType::CLOSE_PARAN))
        {
            _parser.advance();
        }
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

        poNode* body = parseBody();
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

            poNode* stride = parseExpressionStatement();
            if (!_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.setError("Expected close parenethesis.");
                return nullptr;
            }
            _parser.advance();

            if (!_parser.match(poTokenType::OPEN_BRACE))
            {
                _parser.setError("Expected open brace.");
                return nullptr;
            }
            poNode* body = parseBody();

            if (!_parser.match(poTokenType::CLOSE_BRACE))
            {
                _parser.setError("Expected close brace.");
                return nullptr;
            }
            _parser.advance();

            poListNode* bodyList = static_cast<poListNode*>(body);
            bodyList->list().push_back(stride);

            std::vector<poNode*> loopChildren = {
                bodyList,
                equality
            };

            poListNode* loopNode = new poListNode(poNodeType::WHILE, loopChildren, forStatement);
            children.push_back(loopNode);
        }
    }

    return new poUnaryNode(poNodeType::STATEMENT, new poListNode(poNodeType::BODY, children, forStatement), forStatement);
}

poNode* poFunctionParser::parseIfStatement()
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

            children.push_back(new poUnaryNode(poNodeType::STATEMENT, parseExpression(), expr));

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
            children.push_back(parseBody());
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
                        parseIfStatement(),
                        expr));
                }
                else if (_parser.match(poTokenType::OPEN_BRACE))
                {
                    children.push_back(new poUnaryNode(poNodeType::ELSE,
                        parseBody(),
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

poNode* poFunctionParser::parseDecl()
{
    poNode* statement = nullptr;

    auto& type = _parser.peek();
    _parser.advance();

    while (_parser.match(poTokenType::STAR))
    {
        // pointer
        _parser.advance();
    }

    // variable decl
    if (_parser.match(poTokenType::IDENTIFIER))
    {
        const auto& id = _parser.peek();
        _parser.advance();

        if (_parser.match(poTokenType::SEMICOLON))
        {
            // end
            poNode* decl = new poUnaryNode(poNodeType::DECL,
                new poNode(poNodeType::VARIABLE, id),
                type);
            statement = new poUnaryNode(poNodeType::STATEMENT, decl, type);
        }
        else
        {
            poNode* lhs = new poNode(poNodeType::VARIABLE, id);
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
    if (_parser.match(poTokenType::EQUALS))
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
    else
    {
        _parser.setError("Unexpected token.");
    }

    return nullptr;
}

poNode* poFunctionParser::parseStatement()
{
    poNode* statement = nullptr;
    if (_parser.match(poTokenType::IDENTIFIER)  && !_parser.lookahead(poTokenType::IDENTIFIER))
    {
        statement = parseExpressionStatement();
    }
    else
    {
        statement = parseDecl();
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

poNode* poFunctionParser::parseBody()
{
    auto& function = _parser.peek();
    _parser.advance();

    std::vector<poNode*> children;

    while (!_parser.match(poTokenType::CLOSE_BRACE))
    {
        if (_parser.match(poTokenType::BOOLEAN) ||
            _parser.match(poTokenType::I32_TYPE) ||
            _parser.match(poTokenType::I64_TYPE) ||
            _parser.match(poTokenType::I8_TYPE) ||
            _parser.match(poTokenType::U32_TYPE) ||
            _parser.match(poTokenType::U64_TYPE) ||
            _parser.match(poTokenType::U8_TYPE) ||
            _parser.match(poTokenType::F32_TYPE) ||
            _parser.match(poTokenType::F64_TYPE) ||
            _parser.match(poTokenType::IDENTIFIER))
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
            children.push_back(parseIfStatement());
        }
        else if (_parser.match(poTokenType::FOR))
        {
            children.push_back(parseFor());
        }
        else if (_parser.match(poTokenType::WHILE))
        {
            children.push_back(parseWhile());
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
        _parser.match(poTokenType::IDENTIFIER))
    {
        auto& type = _parser.peek();
        _parser.advance();
        if (_parser.match(poTokenType::IDENTIFIER))
        {
            auto& id = _parser.peek();
            _parser.advance();

            node = new poUnaryNode(poNodeType::PARAMETER, new poNode(poNodeType::TYPE, type), id);
        }
    }

    return node;
}

poNode* poFunctionParser::parse(const poToken& ret)
{
    poNode* node = nullptr;

    _parser.advance();
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
            nodes.push_back(new poNode(poNodeType::TYPE, ret));

            if (_parser.match(poTokenType::CLOSE_PARAN))
            {
                _parser.advance();
                if (_parser.match(poTokenType::OPEN_BRACE))
                {
                    nodes.push_back(parseBody());
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
            _parser.match(poTokenType::I64_TYPE) ||
            _parser.match(poTokenType::I32_TYPE) ||
            _parser.match(poTokenType::I8_TYPE) ||
            _parser.match(poTokenType::U64_TYPE) ||
            _parser.match(poTokenType::U32_TYPE) ||
            _parser.match(poTokenType::U8_TYPE) ||
            _parser.match(poTokenType::F64_TYPE) ||
            _parser.match(poTokenType::F32_TYPE) ||
            _parser.match(poTokenType::BOOLEAN))
        {
            poToken type = _parser.peek();
            _parser.advance();

            if (_parser.match(poTokenType::IDENTIFIER))
            {
                poToken name = _parser.peek();
                _parser.advance();

                children.push_back(new poUnaryNode(poNodeType::DECL, new poNode(poNodeType::TYPE, type), name));

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
            else if (_parser.match(poTokenType::STATIC))
            {
                _parser.advance();
                auto& token = _parser.peek();

                if (_parser.match(poTokenType::VOID) ||
                    _parser.match(poTokenType::BOOLEAN) ||
                    _parser.match(poTokenType::I32_TYPE) ||
                    _parser.match(poTokenType::I64_TYPE) ||
                    _parser.match(poTokenType::I8_TYPE) ||
                    _parser.match(poTokenType::U32_TYPE) ||
                    _parser.match(poTokenType::U64_TYPE) ||
                    _parser.match(poTokenType::U8_TYPE) ||
                    _parser.match(poTokenType::F32_TYPE) ||
                    _parser.match(poTokenType::F64_TYPE) ||
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


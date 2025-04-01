#pragma once
#include "poLex.h"

namespace po
{
    class poNode;

    class poParser
    {
    public:
        poParser(const std::vector<poToken>& tokens);
        inline bool isError() const { return _error; }
        void setError(const std::string& text);
        void advance();
        const poToken& peek();
        bool match(poTokenType type);
        bool eof();
        inline const std::string& error() const { return _errorText; }
        inline int errorLine() const { return _line; }
        inline int errorColumn() const { return _col; }

    private:
        const std::vector<poToken>& _tokens;

        int _pos;
        bool _error;
        std::string _errorText;
        int _line;
        int _col;
    };

    class poFunctionParser
    {
    public:
        poFunctionParser(poParser& parser);
        poNode* parse(const poToken& ret);
    private:
        poNode* parseRH(poNode* lhs);
        poNode* parsePrimary();
        poNode* parseUnary();
        poNode* parseCall(const poToken& token);
        poNode* parseFactor();
        poNode* parseTerm();
        poNode* parseEquality();
        poNode* parseAnd();
        poNode* parseOr();
        poNode* parseExpression();
        poNode* parseBody();
        poNode* parseArg();
        poNode* parseIfStatement();
        poNode* parseWhile();
        poNode* parseFor();
        poNode* parseDecl();
        poNode* parseStatement();
        poNode* parseExpressionStatement();

        poParser& _parser;
    };

    class poClassParser
    {
    public:
        poClassParser(poParser& parser);
        poNode* parse();
    private:
        poParser& _parser;
    };

    class poNamespaceParser
    {
    public:
        poNamespaceParser(poParser& parser);
        poNode* parse();
    private:
        poNode* parseFunction(const poToken& ret);

        poParser& _parser;
    };

    class poModuleParser
    {
    public:
        poModuleParser(poParser& parser);
        poNode* parse();
    private:
        poNode* parseTopLevel();
        poNode* parseImport();
        poNode* parseNamespace();

        poParser& _parser;
    };

}

#pragma once
#include "poLex.h"

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace po
{
    class poNode;
    class poListNode;

    class poTypeChecker
    {
    public:
        poTypeChecker();
        bool check(const std::vector<poNode*>& nodes);
        inline const std::string& errorText() const { return _errorText; }
        inline int errorLine() const { return _errorLine; }
        inline int errorCol() const { return _errorCol; }
    private:
        inline bool isError() const { return _isError; }
        void setError(const std::string& text, const int line, const int col);
        void setError(const std::string& text, const poToken& token);

        void getModules(poNode* node);
        void getNamespace(poNode* node);

        void checkModules(poNode* node);
        void checkNamespaces(poNode* node);
        void checkFunctions(poNode* node);
        void checkBody(poNode* node, const int returnType);
        void checkStatement(poNode* node, const int returnType);
        void checkDecl(poNode* node);
        void checkAssignment(poNode* node);
        void checkReturn(poNode* node, const int returnType);
        int checkExpr(poNode* node);
        int checkCall(po::poNode* node);
        int getType(const poToken& token);
        void checkIfStatement(po::poNode* node, const int returnType);
        void checkWhileStatement(po::poNode* node, const int returnType);

        int getVariable(const std::string& name);
        bool addVariable(const std::string& name, const int type);
        void pushScope();
        void popScope();

        std::unordered_map<std::string, poListNode*> _types;
        std::vector<std::unordered_map<std::string, int>> _variables;
        std::unordered_map<std::string, poListNode*> _functions;
        std::string _errorText;
        int _errorLine;
        int _errorCol;
        bool _isError;
    };
}

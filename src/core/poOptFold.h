#pragma once
#include <vector>

namespace po
{
    class poNode;

    class poOptFold
    {
    public:
        void fold(std::vector<poNode*>& ast);
        void fold(poNode* ast);

    private:
        void foldNamespace(poNode* ast);
        void foldFunctions(poNode* ast);
        void foldBody(poNode* ast);
        void foldStatement(poNode* ast);
        void foldDecl(poNode* ast);
        void foldAssignment(poNode* ast);
        void foldExpr(poNode* ast, poNode** fold);
        void foldBinaryExpr(poNode* ast, poNode** fold);
    };
}

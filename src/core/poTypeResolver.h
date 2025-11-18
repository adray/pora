#pragma once
#include <vector>
#include <unordered_map>
#include <string>

namespace po
{
    class poModule;
    class poNode;
    class poListNode;
    class poNamespace;
    class poToken;

    class poTypeResolver
    {
    public:
        poTypeResolver(poModule& module);
        void resolve(const std::vector<poNode*>& nodes);

        inline const std::string& errorText() const { return _errorText; }
        inline const bool isError() const { return _isError; }

    private:
        void addType(poNode* node, poNamespace& ns, const int nsId);
        void resolveTypes();
        void resolveType(const std::string& name, poNamespace& ns, poListNode* typeNode);
        void resolveStatics(poNode* namespaceNode);
        void getPrototypeArgs(poListNode* prototypeNode, std::vector<int>& argTypes);
        void getNamespaces(poNode* node);
        void getStruct(poNode* node, poNamespace& ns, const int nsId);
        void getClass(poNode* node, poNamespace& ns, const int nsId);
        void getExtern(poNode* node, poNamespace& ns);
        void getFunction(poNode* node, poNamespace& ns);
        void getConstructor(poNode* node, poNamespace& ns);

        int getReturnType(poListNode* prototypeNode);
        int getTypeSize(const int type);
        int getType(const poToken& token);
        int getPointerType(const int baseType, const int count);
        int align(const int size, const int alignment) const;

        void updateArgs(poNode* node);
        bool isTypeResolved(poNode* node, bool isPointer);

        void setError(const std::string& errorText);

        std::string _errorText;
        bool _isError;

        poModule& _module;
        std::vector<poNode*> _userTypes;
        std::vector<poNode*> _unresolvedTypes;
        std::unordered_map<poNode*, int> _namespaceForTypes;
        std::unordered_map<std::string, int> _resolvedTypes;
        std::unordered_map<poNode*, int> _namespaces;
        std::unordered_map<std::string, int> _memberFunctions;
    };
}

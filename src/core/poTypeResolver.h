#pragma once
#include <vector>
#include <unordered_map>
#include <string>

namespace po
{
    class poModule;
    class poNode;
    class poNamespace;
    class poToken;

    class poTypeResolver
    {
    public:
        poTypeResolver(poModule& module);
        void resolve(const std::vector<poNode*>& nodes);

    private:
        void resolveTypes(poNamespace& ns);
        void getNamespaces(poNode* node);
        void getStruct(poNode* node, poNamespace& ns);
        void getExtern(poNode* node, poNamespace& ns);
        void getFunction(poNode* node, poNamespace& ns);

        int getTypeSize(const int type);
        int getType(const poToken& token);
        int getPointerType(const int baseType, const int count);
        int align(const int size, const int alignment) const;

        void updateArgs(poNode* node, poNamespace& ns);
        
        poModule& _module;
        std::vector<poNode*> _userTypes;
        std::unordered_map<std::string, int> _resolvedTypes;
    };
}

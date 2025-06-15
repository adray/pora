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
        void addPrimitives();
        void resolveTypes(poNamespace& ns);
        void getNamespaces(poNode* node);
        void getStruct(poNode* node, poNamespace& ns);
        void getExtern(poNode* node, poNamespace& ns);
        void getFunction(poNode* node, poNamespace& ns);

        int getTypeSize(const int type);
        int getType(const poToken& token);

        poModule& _module;
        std::vector<poNode*> _userTypes;
        std::unordered_map<std::string, int> _resolvedTypes;
    };
}

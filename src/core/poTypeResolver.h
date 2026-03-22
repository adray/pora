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
    class poMorph;
    class poType;
    class poMorphNode;
    class poGenericNode;

    class poTypeResolver
    {
    public:
        poTypeResolver(poModule& module);
        void resolve(const std::vector<poNode*>& nodes);

        inline const std::string& errorText() const { return _errorText; }
        inline const bool isError() const { return _isError; }
        inline const int errorFile() const { return _errorFile; }
        inline const int errorLine() const { return _errorLine; }

    private:
        void addType(poNode* node, poNamespace& ns, const int nsId, const int baseType, const int kind);
        void resolveTypes(poMorph& morph);
        bool processWorklist(poMorph& morph, std::vector<poNode*>& work, std::vector<poNode*>& worklist);
        bool processMorphWorklist(poMorph& morph, std::vector<poMorphNode*>& work, std::vector<poMorphNode*>& worklist);
        bool generateSpecializations(poMorphNode* node, poListNode* typeNode, const std::string& name, const int typeId);
        void resolveType(const std::string& name, poNamespace& ns, poListNode* typeNode);
        void resolveStatics(poNode* namespaceNode);
        void getPrototypeArgs(poListNode* prototypeNode, poListNode* parametricArgs, std::vector<int>& argTypes);
        void getNamespaces(poNode* node);
        void getStruct(poNode* node, poNamespace& ns, const int nsId);
        void getClass(poNode* node, poNamespace& ns, const int nsId);
        void getEnum(poNode* node, poNamespace& ns, const int nsId);
        void getExtern(poNode* node, poNamespace& ns);
        void getFunction(poNode* node, poNamespace& ns);
        void getConstructor(poNode* node, poNamespace& ns);

        bool isGenericResolved(poGenericNode* genericNode);
        void getGenericTypeName(std::string& newName, poMorphNode* node);
        void getGenericTypeName(std::string& typeName, poGenericNode* genericArgs);
        void getGenericTypeName(std::string& genericName, poGenericNode* genericNode, poListNode* genericArgs, poMorphNode* node);

        int getArrayType(int fieldType);
        int getParametricType(const std::string& name, poListNode* parametricArgs);
        int getReturnType(poListNode* prototypeNode, poListNode* parametricArg);
        int getTypeSize(const int type);
        int getPointerType(const int baseType, const int count);
        int align(const int size, const int alignment) const;

        void updateArgs(poNode* nodes);
        bool isTypeResolved(poNode* node, bool isPointer, poListNode* parametricArgs);

        void setError(const std::string& errorText, const poToken& token);

        std::string _errorText;
        bool _isError;
        int _errorFile;
        int _errorLine;

        poModule& _module;
        std::vector<poNode*> _userTypes;
        std::vector<poNode*> _unresolvedTypes;
        std::unordered_map<poNode*, int> _namespaceForTypes;
        std::unordered_map<std::string, int> _resolvedTypes;
        std::unordered_map<poNode*, int> _namespaces;
        std::unordered_map<std::string, int> _memberFunctions;
    };
}

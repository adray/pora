#pragma once
#include "poAST.h"

#include <vector>
#include <unordered_map>
#include <string>

// A file for applying monomorphization.
// This is the process of generating the required generic derivations.
// In order to determine which ones are required we build
// a graph to represent the constraints between types.
// If the graph contains cycles (i.e. unbounded recursion) then monomorphization cannot be applied.

namespace po {

    class poModule;
    class poNode;
    class poMorphNode;

    class poMorphCache {
    public:
        void add(poMorphNode* node);
        poMorphNode* find(const int type, const std::vector<int>& parameters);

    private:
        std::unordered_map<int, std::vector<poMorphNode*>> _items;
    };

    // A class for a generic type
    class poMorphType {
    public:
        poMorphType(const int type, const int numParameters, poNode* node) :
            _type(type),
            _numParameters(numParameters),
            _node(node)
        {
        }

        std::vector<poMorphNode*>& nodes() { return _nodes; }
        const int type() const { return _type; }
        const int numParameters() const { return _numParameters; }
        poNode* node() const { return _node; }

    private:
        int _type;
        int _numParameters;
        poNode* _node;
        std::vector<poMorphNode*> _nodes;
    };

    class poMorphParameter {
    public:
        poMorphParameter(const int type, poNode* node) :
            _isTypeName(false),
            _type(type),
            _index(0),
            _node(node)
        { }

        poMorphParameter(const int type, const int index) :
            _isTypeName(true),
            _type(type),
            _index(index),
            _node(nullptr)
        { }

        bool isTypeName() const { return _isTypeName; }
        int type() const { return _type; }
        int index() const { return _index; }
        poNode* node() const { return _node; }

    private:
        bool _isTypeName;
        int _type;
        int _index;
        poNode* _node;
    };

    class poMorphNode {
    public:
        poMorphNode(poMorphType* type) : _type(type) {}

        poMorphType* type() { return _type; }
        std::vector<poMorphParameter>& parameters() { return _parameters; }

    private:
        poMorphType* _type;
        std::vector<poMorphParameter> _parameters;
    };

    class poMorph
    {
    public:
        poMorph(poModule& module)
            :
            _module(module),
            _isError(false),
            _errorLine(0),
            _errorCol(0),
            _errorFileId(0)
        {
        }

        void morph(const std::vector<poNode*>& nodes);
        void scan(const std::vector<poNode*>& nodes);

        const std::vector<poMorphNode*> nodes() const { return _nodes; }
        poMorphType* getMorphType(const int type);

        bool isError() const { return _isError; }
        const std::string& errorText() const { return _errorText; }
        int errorLine() const { return _errorLine; }
        int errorCol() const { return _errorCol; }
        int errorFile() const { return _errorFileId; }

    private:
        void setError(const std::string& errorText, const poToken& token);
        void buildGraph(const std::vector<poNode*>& nodes,
            std::vector<poNode*>& userTypes);
        void walkNamespace(poNode* node, const std::vector<poNode*>& userTypes);
        void walkFunction(poNode* node, const std::vector<poNode*>& userTypes);
        void walkStatement(poNode* node, const std::vector<poNode*>& userTypes);
        void walkDecl(poMorphType* sourceType, poNode* node);
        void walkDecl(poNode* node, const std::vector<poNode*>& types);
        void checkConstraints();
        void generateMorphs();
        void morphASTs(const std::vector<poNode*>& nodes, std::vector<poNode*>& userTypes);
        void substituteFunctions(const std::string& namespaceName, std::vector<poNode*>& ns, poNode* node, std::vector<poNode*>& userTypes);
        void substituteConstructor(const std::string& namespaceName, std::vector<poNode*>& ns, poNode* node, std::vector<poNode*>& userTypes);
        void substituteBody(poListNode* body, poListNode* generic, poMorphNode* morph, std::vector<poNode*>& nodes);
        poNode* substituteStatement(poNode* statement, poListNode* generic, poMorphNode* morph);
        poNode* substituteAssignment(poNode* assignment, poListNode* generic, poMorphNode* morph);
        poNode* substituteDecl(poNode* decl, poListNode* generic, poMorphNode* morph);
        poNode* substituteCast(poNode* cast, poListNode* generic, poMorphNode* morph);
        poNode* substituteType(poNode* type, poListNode* generic, poMorphNode* morph);
        poNode* substituteReturnType(poNode* type, poListNode* generic, poMorphNode* morph);
        poNode* substituteCall(poNode* call, poListNode* generic, poMorphNode* morph);
        poNode* substituteArray(poNode* array, poListNode* generic, poMorphNode* morph);
        poNode* substituteUnary(poNode* unary, poListNode* generic, poMorphNode* morph);
        poNode* substituteDynamicArray(poNode* unary, poListNode* generic, poMorphNode* morph);
        poNode* substitutePointer(poNode* pointer, poListNode* generic, poMorphNode* morph);
        poNode* substitute(poNode* node, poListNode* generic, poMorphNode* morph);
        void substituteArgs(poListNode* args, poListNode* generic, poMorphNode* morph, std::vector<poNode*>& argList);
        poNode* match(const poToken& token, poListNode* generic, poMorphNode* morph);
        void getGenericType(std::string& name, poMorphNode* morph);
        void gatherTypesFromNamespace(poNode* child, std::vector<poNode*>& userTypes);
        void gatherTypes(const std::vector<poNode*>& nodes, std::vector<poNode*>& userTypes);

        void dump();
        void dumpType(poMorphType* type);
        void dumpNode(poMorphNode* node);

        std::vector<poMorphType*> _types;               // Type by ID
        std::vector<poMorphNode*> _nodes;               // Nodes
        std::unordered_map<int, poMorphType*> _typeMap; // TYPE -> morph type
        std::unordered_map<std::string, int> _map;      // TYPE NAME -> AST node index
        poMorphCache _cache;
        poModule& _module;

        std::string _errorText;
        bool _isError;
        int _errorLine;
        int _errorCol;
        int _errorFileId;
    };
}

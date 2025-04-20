#pragma once
#include <vector>
#include <string>
#include <unordered_map>

namespace po
{
    class poModule;
    class poNode;
    class poNamespace;
    class poFunction;
    class poFlowGraph;
    class poToken;
    class poBasicBlock;

    class poVariable
    {
    public:
        poVariable(const int id, const int type);
        const int type() const { return _type; }
        const int id() const { return _id; }

    private:
        int _type;
        int _id;
    };

    class poConditionGraphNode
    {
    public:
        poConditionGraphNode(int id, poNode* ast);
        poConditionGraphNode(int id, int success, int fail, poNode* ast, const bool terminator);
        inline int id() const { return _id; }
        inline int fail() const { return _fail; }
        inline int success() const { return _success; }
        inline poNode* node() const { return _ast; }
        inline bool terminator() const { return _terminator; }
        inline void setBasicBlock(poBasicBlock* bb) { _bb = bb; }
        inline poBasicBlock* getBasicBlock() const { return _bb; }
        inline void setEmitted(const bool isEmitted) { _emitted = isEmitted; }
        inline bool isEmitted() const { return _emitted; }
        inline std::vector<int>& predecessors() { return _predecessors; }

    private:
        int _success;
        int _fail;
        int _id;
        bool _terminator;
        bool _emitted;
        poBasicBlock* _bb;
        poNode* _ast;
        std::vector<int> _predecessors;
    };

    class poConditionGraph
    {
    public:
        poConditionGraph();
        void buildGraph(poNode* ast);
        inline poConditionGraphNode& root() { return _nodes[_root]; }
        inline poConditionGraphNode& at(const int id) { return _nodes[id]; }
        inline const int success() const { return _success; }
        inline const int fail() const { return _fail; }
        inline const int numNodes() const { return int(_nodes.size()); }

        void computePredecessors();
    private:
        int build(poNode* ast, int success, int fail);
        int createNode(poNode* ast);
        int createNode(poNode* ast, int success, int fail);
        int buildAnd(poNode* ast, int success, int fail);
        int buildOr(poNode* ast, int success, int fail);

        int _root;
        int _success;
        int _fail;
        std::vector<poConditionGraphNode> _nodes;
    };

    class poCodeGenerator
    {
    public:
        poCodeGenerator(poModule& module);
        void generate(const std::vector<poNode*>& ast);

    private:
        int getType(const poToken& token);
        int getOrAddVariable(const std::string& name, const int type);
        int addVariable(const std::string& name, const int type);
        void getModules(poNode* node);
        void getNamespaces(poNode* node);
        void getFunctions(poNode* node, poNamespace& ns);
        void emitFunction(poNode* node, poFunction& function);
        void emitBody(poNode* node, poFlowGraph& cfg);
        void emitArgs(poNode* node, poFlowGraph& cfg);
        void emitStatement(poNode* node, poFlowGraph& cfg);
        int emitCall(poNode* node, poFlowGraph& cfg);
        void emitReturn(poNode* node, poFlowGraph& cfg);
        void emitDecl(poNode* node, poFlowGraph& cfg);
        void emitAssignment(poNode* node, poFlowGraph& cfg);
        void emitIfStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB);
        void emitWhileStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB);
        void emitIfExpression(poConditionGraphNode& cgn, poFlowGraph& cfg, poBasicBlock* successBB, poBasicBlock* failBB);
        int emitExpr(poNode* node, poFlowGraph& cfg);
        int emitConstantExpr(poNode* node, poFlowGraph& cfg);
        int emitBinaryExpr(poNode* node, poFlowGraph& cfg);
        int emitUnaryMinus(poNode* node, poFlowGraph& cfg);

        poModule& _module;
        poConditionGraph _graph;
        std::unordered_map<std::string, poNode*> _functions;
        std::unordered_map<std::string, poVariable> _variables;
        int _instructionCount;
    };
}

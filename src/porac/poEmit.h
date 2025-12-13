#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

#include "poType.h"

namespace po
{
    class poModule;
    class poNode;
    class poNamespace;
    class poFunction;
    class poFlowGraph;
    class poToken;
    class poBasicBlock;
    class poInstruction;

    class poVariable
    {
    public:
        poVariable(const int id, const int type, const int size);
        const int type() const { return _type; }
        const int id() const { return _id; }
        const int size() const { return _size; }

    private:
        int _type;
        int _id;
        int _size; // array size
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

    class poEmitter
    {
    public:
        poEmitter(poModule& module);
        void emitInstruction(const poInstruction& instruction, poBasicBlock* bb);
        int emitConstant(const int64_t i64, poFlowGraph& cfg);
        int emitConstant(const uint64_t u64, poFlowGraph& cfg);
        int emitConstant(const int32_t i32, poFlowGraph& cfg);
        int emitConstant(const uint32_t u32, poFlowGraph& cfg);
        int emitConstant(const int16_t i16, poFlowGraph& cfg);
        int emitConstant(const uint16_t u16, poFlowGraph& cfg);
        int emitConstant(const int8_t i8, poFlowGraph& cfg);
        int emitConstant(const uint8_t u8, poFlowGraph& cfg);
        int emitConstant(const float f32, poFlowGraph& cfg);
        int emitConstant(const double f64, poFlowGraph& cfg);
        int emitConstant(const std::string& str, poFlowGraph& cfg);
        int emitConstant(const int type, const int constantId, poFlowGraph& cfg);
        int emitAdd(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitSub(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitMul(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitDiv(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitLeftShift(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitRightShift(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitModulo(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitAnd(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitOr(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitUnaryMinus(const int type, const int left, poFlowGraph& cfg);
        int emitCmp(const int type, const int left, const int right, poFlowGraph& cfg);
        int emitCall(const int returnType, const int numArgs, const int symbolId, poFlowGraph& cfg);
        int emitArg(const int type, const int arg, poFlowGraph& cfg);
        int emitReturn(const int type, const int value, poFlowGraph& cfg);
        int emitReturn(poFlowGraph& cfg);
        int emitSignExtend(const int dstType, const int srcType, const int value, poFlowGraph& cfg);
        int emitZeroExtend(const int dstType, const int srcType, const int value, poFlowGraph& cfg);
        int emitBitwiseCast(const int dstType, const int srcType, const int value, poFlowGraph& cfg);
        int emitConvert(const int dstType, const int srcType, const int value, poFlowGraph& cfg);
        int emitAlloca(const int type, poBasicBlock* bb);
        int emitAlloca(const int type, const int varName, poBasicBlock* bb);
        int emitPtr(const int type, const int value, const int offset, poFlowGraph& cfg);
        int emitPtr(const int type, const int value, const int dynamicOffset, const int offset, poFlowGraph& cfg);
        int emitElementPtr(const int type, const int value, const int element, poFlowGraph& cfg);
        int emitLoad(const int type, const int ptr, poFlowGraph& cfg);
        int emitStore(const int type, const int ptr, const int value, poFlowGraph& cfg);
        int emitBranch(const int branchType, poFlowGraph& cfg);
        int emitBranch(const int type, const int branchType, poFlowGraph& cfg);
        int emitParam(const int type, const int paramIndex, poFlowGraph& cfg);
        int emitStoreGlobal(const int type, const int value, const int globalId, poFlowGraph& cfg);
        int emitLoadGlobal(const int type, const int globalId, poFlowGraph& cfg);

        void reset();
        int addVariable(const int type, const int qualifier);

        int getArrayType(const int baseType, const int arrayRank);
        int getPointerType(const int baseType);

        inline const int getType(const int id) const { return _types[id]; }
    private:
        poModule& _module;
        std::vector<int> _types;
        std::vector<int> _qualifiers;
        int _instructionCount;
    };

    class poCodeGenerator
    {
    public:
        poCodeGenerator(poModule& module);
        void generate(const std::vector<poNode*>& ast);

        inline const bool isError() const { return _isError; }
        inline const std::string& errorText() const { return _errorText; }
        inline const int errorFile() const { return _errorFile; }
        inline const int errorLine() const { return _errorLine; }
        inline const int errorColumn() const { return _errorCol; }

    private:
        void setError(const std::string& errorText);
        void setError(const std::string& errorText, const poToken& token);

        int getType(const poToken& token);
        int getTypeSize(const int type);
        int getVariable(const std::string& name);
        int getOrAddVariable(const std::string& name, const int type, const int qualifiers, const int size);
        int addVariable(const std::string& name, const int type, const int qualifiers, const int size);
        int getArrayType(const int baseType, const int arrayRank);
        int getPointerType(const int baseType);
        void getModules(poNode* node);
        void getNamespaces(poNode* node, const std::vector<poNode*>& importNodes);
        void emitArrayCopy(const int src, const int dst, poFlowGraph& cfg, const int size);
        void emitCopy(const int src, const int dst, poFlowGraph& cfg);
        void emitFunction(poNode* node, poFunction& function);
        void emitConstructor(poFlowGraph& cfg);
        void emitDefaultConstructor(poFunction& function, const int type);
        void emitBody(poNode* node, poFlowGraph& cfg, poBasicBlock* loopHeader, poBasicBlock* loopEnd);
        void emitArgs(poNode* node, poFlowGraph& cfg);
        void emitParameter(const int type, poFlowGraph& cfg, const int paramIndex, const int varName);
        void emitStatement(poNode* node, poFlowGraph& cfg);
        int emitPassByValue(const int expr, poFlowGraph& cfg);
        int emitPassByReference(const int expr, poFlowGraph& cfg);
        int emitNew(poNode* node, poFlowGraph& cfg);
        int emitNewArray(poNode* node, poFlowGraph& cfg);
        int emitCall(poNode* node, poFlowGraph& cfg, const int instanceExpr);
        void emitCall(poNode* node, poFlowGraph& cfg, const int instanceExpr, const int64_t arraySize);
        int emitMemberCall(poNode* node, poFlowGraph& cfg);
        void emitReturn(poNode* node, poFlowGraph& cfg);
        void emitDecl(poNode* node, poFlowGraph& cfg);
        void emitDefaultValue(const int type, poFlowGraph& cfg, const int ptrType, const int var);
        void emitAssignment(poNode* node, poFlowGraph& cfg);
        void emitIfStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB, poBasicBlock* loopHeader, poBasicBlock* loopEnd);
        void emitWhileStatement(poNode* node, poFlowGraph& cfg, poBasicBlock* endBB);
        void emitIfExpression(poConditionGraphNode& cgn, poFlowGraph& cfg, poBasicBlock* successBB, poBasicBlock* failBB);
        int emitExpr(poNode* node, poFlowGraph& cfg);
        int emitNullptr(poNode* node, poFlowGraph& cfg);
        int emitCast(poNode* node, poFlowGraph& cfg);
        int emitDereference(poNode* node, poFlowGraph& cfg);
        int emitReference(poNode* node, poFlowGraph& cfg);
        int emitConstantExpr(poNode* node, poFlowGraph& cfg);
        int emitBinaryExpr(poNode* node, poFlowGraph& cfg);
        int emitResolver(poNode* node, poFlowGraph& cfg);
        int emitUnaryMinus(poNode* node, poFlowGraph& cfg);
        int emitLoadArray(poNode* node, poFlowGraph& cfg);
        void emitStoreArray(poNode* node, poFlowGraph& cfg, const int id);
        int emitLoadMember(poNode* node, poFlowGraph& cfg);
        void emitStoreMember(poNode* node, poFlowGraph& cfg, const int id);
        int emitLoadThis(poFlowGraph& cfg);
        int emitLoadVariable(poNode* node, poFlowGraph& cfg);
        int emitSizeof(poNode* node, poFlowGraph& cfg);

        void getOperators(int type, std::vector<poOperator>& operators);

        void emitLoopPreHeader(poFlowGraph& cfg, const int instanceExpr, const int64_t arraySize);
        void emitLoopPreHeader(poFlowGraph& cfg, const int instanceExpr);
        void emitLoopHeader(poFlowGraph& cfg, const int instanceExpr);
        void emitLoopHeader(poFlowGraph& cfg, const int instanceExpr, const int loopSizeExpr);
        void emitLoopEnd(poFlowGraph& cfg);

        // Loop related members

        poBasicBlock* _preHeaderBB;
        poBasicBlock* _headerBB;
        poBasicBlock* _loopBodyBB;
        poBasicBlock* _loopEndBB;
        int _arrayConstant;
        int _loopIndexVar;
        int _loopIndexLoadBody;
        int _loopIndexPtrBody;
        int _incrementConstant;
        int _pointerType;
        int _elementPtr;

        // End loop related members

        poModule& _module;
        poConditionGraph _graph;
        poEmitter _emitter;
        std::unordered_map<std::string, poNode*> _functions;
        std::unordered_map<std::string, poVariable> _variables;
        std::unordered_map<std::string, std::vector<poNode*>> _functionImports;
        std::vector<std::string> _imports;
        std::string _namespace;
        int _returnInstruction;
        int _thisInstruction;
        int _returnType;
        bool _isError;
        int _errorLine;
        int _errorCol;
        int _errorFile;
        std::string _errorText;
    };
}

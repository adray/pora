#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace po
{
    class poFlowGraph;
    class poDom;
    class poNLF;

    class poLiveNode
    {
    public:
        void addEdge(const int node, const bool isBackEdge);
        void addLiveIn(const int variable);
        void addLiveOut(const int variable);
        void removeLiveIn(const int variable);
        void removeLiveOut(const int variable);
        inline const std::vector<int>& forwardEdges() const { return _forwardEdges; }
        inline const std::vector<int>& backEdges() const { return _backEdges; }
        inline const std::unordered_set<int>& liveIn() const { return _liveIn; }
        inline const std::unordered_set<int>& liveOut() const { return _liveOut; }

    private:
        std::vector<int> _forwardEdges;
        std::vector<int> _backEdges;
        std::unordered_set<int> _liveIn;
        std::unordered_set<int> _liveOut;
    };

    class poLive
    {
    public:
        poLive();
        void compute(poFlowGraph& cfg, poDom& dom); /* compute liveness for a CFG in SSA form */
        inline const std::vector<poLiveNode>& nodes() const { return _nodes; }
        inline const bool isError() const { return _error; }
        inline const std::string errorText() const { return _errorText; }
    private:
        void initializeLive(poDom& dom);
        void dagDfs(poDom& dom, const int id);
        void phiUses(poDom& dom, const int id);
        void loopTreeDfs(poDom& dom, poNLF& nlf, const int id);

        bool _error;
        std::string _errorText;
        std::vector<poLiveNode> _nodes;
        std::unordered_set<int> _visited;
    };

    class poLiveRange
    {
    public:
        void compute(poFlowGraph& cfg);
        int getLiveRange(const int index);

    private:

        void extendLiveRange(const int variable, const int pos);
        void setLiveRange(const int variable, const int range);
        void addLiveRange(const int variable);

        std::vector<int> _range;
        std::unordered_map<int, std::vector<int>> _variableMapping;
    };
}

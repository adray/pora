#pragma once
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace po
{
    class poFlowGraph;
    class poDom;

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
        void compute(poFlowGraph& cfg, poDom& dom);
    private:
        bool isReducible(poFlowGraph& cfg, poDom& dom);
        void dagDfs(poDom& dom, const int id);

        std::vector<poLiveNode> _nodes;
        std::unordered_set<int> _visited;
    };

    class poLiveRange
    {
    public:
        void compute(poFlowGraph& cfg);
        //int getLiveRange(const int variable);
        int getLiveRange(const int index);

    private:

        void extendLiveRange(const int variable, const int pos);
        void setLiveRange(const int variable, const int range);
        void addLiveRange(const int variable);

        std::vector<int> _range;
        std::unordered_map<int, std::vector<int>> _variableMapping;
    };
}

#pragma once
#include <vector>
#include <stack>

// This is a file to determine if a directed graph
// contains cycles or not.

namespace po
{
    class poCycle
    {
    public:
        poCycle() :
            _hasCycle(false),
            _numVertices(0)
        {
        }

        void init(const int numVertices);
        void addEdge(const int v1, const int v2);
        void compute();

        inline bool hasCycle() const { return _hasCycle; }

    private:
        bool _hasCycle;
        int _numVertices;
        std::vector<std::vector<int>> _edges;
        std::vector<int> _colors;
        std::stack<int> _stack;
    };
}

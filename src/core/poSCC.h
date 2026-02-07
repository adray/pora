#pragma once
#include <vector>
#include <stack>

// This is a file for computing the strongly connected components of a directed graph
// based on the paper "Depth First Search and Linear Algorithms" by Tarjan.

namespace po
{
    class poSCC
    {
    public:
        poSCC() : _index(0) {}
        void init(int numVertices);
        void addEdge(int from, int to);
        void compute();
        void reset();

        inline int header(int vertex) const { return _components[vertex]; }

    private:
        void strongconnect(int v);

        int _index = 0;
        std::stack<int> _stack;
        std::vector<int> _lowlink;
        std::vector<int> _number;
        std::vector<bool> _onStack;
        std::vector<std::vector<int>> _edges;
        std::vector<int> _components;
    };
}

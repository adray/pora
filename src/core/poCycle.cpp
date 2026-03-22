#include "poCycle.h"

using namespace po;

constexpr int COLOR_WHITE = 0;
constexpr int COLOR_GRAY = 1;
constexpr int COLOR_BLACK = 2;

void poCycle::init(const int numVertices)
{
    _numVertices = numVertices;
    _edges.resize(numVertices);
    _colors.resize(numVertices);
}

void poCycle::addEdge(const int v1, const int v2)
{
    _edges[v1].push_back(v2);
}

void poCycle::compute()
{
    for (int i = 0; i < _numVertices; i++) {
        if (_colors[i] != COLOR_WHITE) {
            continue;
        }

        _stack.push(i);
        while (!_stack.empty())
        {
            const int vertex = _stack.top();
            const int color = _colors[vertex];
            if (color == COLOR_WHITE)
            {
                _colors[vertex] = COLOR_GRAY;

                const std::vector<int>& edges = _edges[vertex];
                for (const int next : edges) {
                    if (_colors[next] == COLOR_GRAY) {
                        _hasCycle = true;
                        break;
                    } else if (_colors[next] == COLOR_WHITE) {
                        _stack.push(next);
                    }
                }
            }
            else if (color == COLOR_GRAY)
            {
                _colors[vertex] = COLOR_BLACK;
                _stack.pop();
            }
        }
    }
}


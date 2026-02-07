#include "poSCC.h"

using namespace po;

void poSCC:: init(int numVertices)
{
    _number.resize(numVertices, -1);
    _lowlink.resize(numVertices, -1);
    _onStack.resize(numVertices, false);
    _edges.resize(numVertices);
    _components.resize(numVertices, -1);
}

void poSCC::addEdge(int from, int to)
{
    _edges[from].push_back(to);
}

void poSCC::compute()
{
    for (size_t v = 0; v < _edges.size(); ++v)
    {
        if (_number[v] == -1)
        {
            strongconnect(int(v));
        }
    }
}

void poSCC:: reset()
{
    _edges.clear();
    _components.clear();
    _lowlink.clear();
    _number.clear();
    _onStack.clear();
}

void poSCC::strongconnect(int v)
{
    _index++;
    _number[v] = _index;
    _lowlink[v] = _index;

    _stack.push(v);
    _onStack[v] = true;

    for (const int w : _edges[v])
    {
        if (_number[w] == -1)
        {
            strongconnect(w);
            _lowlink[v] = std::min(_lowlink[v], _lowlink[w]);
        }
        else if (_number[w] < _number[v] && _onStack[w])
        {
            _lowlink[v] = std::min(_lowlink[v], _number[w]);
        }
    }

    if (_lowlink[v] == _number[v])
    {
        while (!_stack.empty() && _number[_stack.top()] >= _number[v])
        {
            const int w = _stack.top();
            _stack.pop();
            _onStack[w] = false;
            _components[w] = v;
        } 
    }
}

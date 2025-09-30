#pragma once
#include <unordered_map>
#include <vector>

namespace po
{
    class poFlowGraph;

    class poPhiWeb
    {
    public:
        poPhiWeb();
        int add(const int name);
        void add(const int name, const int id);
        int find(const int name) const;
        void update(const int name, const int id);

        void findPhiWebs(poFlowGraph& cfg);

    private:
        void merge(const int left, const int right);
      
        std::unordered_map<int, int> _webs;
        std::vector<std::vector<int>> _items;
        int _id;
    };
}

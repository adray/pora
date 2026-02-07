#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace po {

    class poModule;

    class poCallGraphNode
    {
    public:
        poCallGraphNode(const std::string& name, const int id);

        const int id() const { return _id; }
        const std::string& name() const { return _name; }

        inline const std::vector<poCallGraphNode*>& children() const { return _children; }
        inline void addChild(poCallGraphNode* child) { _children.push_back(child); }

        inline bool isLeafNode() const { return _children.size() == 0; }

        inline void setSCCId(const int sccId) { _sccId = sccId; }
        inline int sccId() const { return _sccId; }

    private:
        std::vector<poCallGraphNode*> _children;
        std::string _name;
        int _id;
        int _sccId; /* strongly connected component id, used for identifying recursive functions */
    };

    class poCallGraph
    {
    public:
        void analyze(poModule& module);

        poCallGraphNode* findNodeByName(const std::string& name) const;
        inline const std::vector<poCallGraphNode*>& nodes() const { return _nodes; }

    private:
        void analyzeFunction(poModule& module, poCallGraphNode* node);

        std::vector<poCallGraphNode*> _nodes;
        std::unordered_map<std::string, int> _functions;
    };
}

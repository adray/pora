#include "poCallGraph.h"
#include "poModule.h"
#include "poSCC.h"

using namespace po;

poCallGraphNode::poCallGraphNode(const std::string& name, const int id)
    :
    _name(name),
    _id(id),
    _sccId(-1)
{
}

poCallGraphNode* poCallGraph::findNodeByName(const std::string& name) const
{
    const auto& it = _functions.find(name);
    if (it != _functions.end())
    {
        return _nodes[it->second];
    }
    return nullptr;
}

void poCallGraph::analyze(poModule& module)
{
    for (int i = 0; i < int(module.functions().size()); i++)
    {
        poFunction& func = module.functions()[i];
        const std::string& name = func.fullname();

        poCallGraphNode* node = new poCallGraphNode(name, i);
        _nodes.push_back(node);

        _functions.insert(std::pair<std::string, int>(name, i));
    }
    
    for (poCallGraphNode* node : _nodes)
    {
        analyzeFunction(module, node);
    }

    // Compute strongly connected components to identify recursive functions

    poSCC scc;
    scc.init(int(_nodes.size()));
    for (poCallGraphNode* node : _nodes)
    {
        for (poCallGraphNode* child : node->children())
        {
            scc.addEdge(node->id(), child->id());
        }
    }
    scc.compute();
    for (poCallGraphNode* node : _nodes)
    {
        node->setSCCId(scc.header(node->id()));
    }
}

void poCallGraph::analyzeFunction(poModule& module, poCallGraphNode* node)
{
    poFunction& func = module.functions()[node->id()];
    for (int i = 0; i < int(func.cfg().numBlocks()); i++)
    {
        poBasicBlock* block = func.cfg().getBasicBlock(i);
        for (const poInstruction& instr : block->instructions())
        {
            if (instr.code() == IR_CALL)
            {
                const int symbolId = instr.right();
                std::string functionName;
                if (!module.getSymbol(symbolId, functionName))
                {
                    continue;
                }

                const int id = _functions.find(functionName)->second;
                poCallGraphNode* childNode = _nodes[id];

                bool alreadyAdded = false;
                for (poCallGraphNode* child : node->children())
                {
                    if (child->id() == childNode->id())
                    {
                        alreadyAdded = true;
                        break;
                    }
                }

                if (!alreadyAdded)
                {
                    node->addChild(childNode);
                }
            }
        }
    }
}


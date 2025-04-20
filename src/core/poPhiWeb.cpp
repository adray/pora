#include "poPhiWeb.h"
#include "poCFG.h"

#include <algorithm>

using namespace po;


//====================
// poPhiWeb
//====================


poPhiWeb::poPhiWeb()
    :
    _id(0)
{
}

int poPhiWeb::add(const int name)
{
    const int id = _id;
    _webs.insert(std::pair<int, int>(name, _id));
    std::vector<int> items = { name };
    _items.push_back(items);
    _id++;
    return id;
}

void poPhiWeb::add(const int name, const int id)
{
    _webs.insert(std::pair<int, int>(name, id));
    _items[id].push_back(name);
}

int poPhiWeb::find(const int name)
{
    const auto& it = _webs.find(name);
    if (it != _webs.end())
    {
        return it->second;
    }
    return -1;
}

void poPhiWeb::update(const int name, const int id)
{
    const auto& it = _webs.find(name);
    if (it != _webs.end())
    {
        // Remove from previous list

        const int prev = it->second;
        std::vector<int>& items = _items[prev];
        std::erase(items, name);

        // Insert into new list
        _items[id].push_back(name);

        it->second = id;
    }
}

void poPhiWeb::merge(const int left, const int right)
{
    const int leftId = find(left);
    const int rightId = find(right);

    if (leftId != rightId)
    {
        auto& leftItems = _items[leftId];
        auto& rightItems = _items[rightId];

        for (int item : rightItems)
        {
            leftItems.push_back(item);
            update(item, leftId);
        }
        rightItems.clear();
    }
}

void poPhiWeb::findPhiWebs(poFlowGraph& cfg)
{
    // Reset

    _id = 0;
    _webs.clear();

    // Find webs

    poBasicBlock* bb = cfg.getFirst();
    while (bb)
    {
        auto& instructions = bb->instructions();
        for (int i = 0; i < int(instructions.size()); i++)
        {
            auto& ins = instructions[i];
            if (ins.code() == IR_PHI)
            {
                const int left = ins.left();
                const int right = ins.right();
                const int name = ins.name();

                // 1) check if left or right exist in the phi web
                // 2a) if one does: then add this and the other one to the web
                // 2b) if neither do: add a new phi web, with all three
                // 2c) if both do: merge the phi webs and add this to it

                const int leftId = find(left);
                const int rightId = find(right);
                if (leftId == -1 && rightId == -1)
                {
                    const int id = add(name);
                    add(left, id);
                    add(right, id);
                }
                else if (leftId == -1 && rightId != -1)
                {
                    add(name, rightId);
                    add(left, rightId);
                }
                else if (leftId != -1 && rightId == -1)
                {
                    add(name, leftId);
                    add(right, leftId);
                }
                else
                {
                    merge(left, right);
                    add(name, leftId);
                }
            }
        }

        bb = bb->getNext();
    }
}

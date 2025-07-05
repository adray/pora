#pragma once

#include <unordered_set>

/* File for performing dead code elimination (DCE) on 
* the program in SSA form. */

namespace po
{
    class poModule;
    class poFunction;
    class poDom;
    class poDomNode;

    class poOptDCE
    {
    public:
        void optimize(poModule& module);

    private:
        void optimize(poFunction& function);
        void optimize(poDom& dom, const int id);

        std::unordered_set<int> _usedNames;
        std::unordered_set<int> _visitedNodes;
    };
}

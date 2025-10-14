#pragma once

// File to perform copy propagation optimization on a program's intermediate representation.
//

namespace po
{
    class poModule;
    class poFlowGraph;
    class poUses;

    class poOptCopy
    {
    public:

        void optimize(poModule& module);
        void optimize(poModule& module, poFlowGraph& cfg);

        void copyPropagation(poUses& uses, const int name, const int source);
    };
}


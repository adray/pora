#pragma once

namespace po
{
    class po_x86_64_flow_graph;
    class poModule;

    class poAnalyzer
    {
        public:
            void checkCallSites(poModule& module, po_x86_64_flow_graph& cfg);
        private:
            int checkFunction(poModule& module, const int id);
            void fixFunctionCalls(poModule& module, po_x86_64_flow_graph& cfg);

    };
}

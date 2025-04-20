#pragma once
#include <vector>
#include <unordered_map>

namespace po
{
    //
    // Linear register allocator.
    // 
    // A linear register allocator is fairly quick due to treating the flow graph
    // as a "hyperblock" and therefore able to allocate the registers using only few
    // simple loops. However, the does not produce optimal register allocation
    // or spilling.
    //

    class poLive;
    class poFlowGraph;

    enum class poRegType
    {
        General,
        SSE
    };

    class poRegLinear
    {
    public:
        void setNumRegisters(const int numRegisters);
        void setVolatile(const int reg, const bool isVolatile);
        void setType(const int reg, const poRegType type);
        void allocateRegisters(poFlowGraph& cfg);
        inline int getRegister(const int index) { return _registers[index]; }
        int getRegisterByVariable(const int variable);

    private:
        void allocateRegister(const int pos, const poRegType type, const int live, const int variable);

        int _numRegisters;
        std::vector<bool> _volatile;
        std::vector<poRegType> _type;
        std::vector<int> _registers;
        std::vector<bool> _registerUsed;
        std::vector<int> _registerExpiry;
        std::unordered_map<int, int> _registerMap;
    };
}

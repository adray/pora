#pragma once
#include <cstdint>

namespace po
{
    class poNode;

    class poEvaluator
    {
    public:
        poEvaluator();
        void reset();
        int64_t evaluateI64(poNode* node);

        inline bool isError() const { return _isError; }
    private:
        int64_t evaluateBinaryI64(poNode* node);

        bool _isError;
    };
}

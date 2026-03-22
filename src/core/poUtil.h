#pragma once
#include <string>
#include <vector>

namespace po {
    class poToken;
    class poModule;
    class poNode;

    class poUtil {
    public:
        static int getType(poToken& token);
        static int getPointerType(poModule& module, const int baseType);
        static int unpackTypeNode(poModule& module, const std::vector<std::string>& parametricArgs, poNode* node);
        static int unpackTypeNode(poModule& module, poNode* node);
    };
}

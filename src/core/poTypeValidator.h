#pragma once
#include <string>

namespace po {
    class poType;
    class poModule;

    class poTypeValidator {
    public:
        bool validateModule(poModule& module);
        bool validateType(const poModule& module, const poType& type);

        inline const std::string& errorText() const { return _errorText; }

    private:
        std::string _errorText;
    };
}


#pragma once
#include <string>

namespace po {
    class poType;
    class poModule;

    class poTypeValidator {
    public:
        poTypeValidator();
        bool validateModule(poModule& module);
        bool validateType(const poModule& module, const poType& type);

        inline const std::string& errorText() const { return _errorText; }
        inline int errorCol() const { return _errorCol; }
        inline int errorLine() const { return _errorLine; }
        inline int errorFile() const { return _errorFile; }

    private:
        std::string _errorText;
        int _errorCol;
        int _errorLine;
        int _errorFile;
    };
}


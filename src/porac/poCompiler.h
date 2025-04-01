#pragma once
#include <vector>
#include <string>

namespace po
{
    class poCompiler
    {
    public:
        void addFile(const std::string& file);
        int compile();
        inline const std::vector<std::string>& errors() const { return _errors; }

    private:
        std::vector<std::string> _files;
        std::vector<std::string> _errors;
    };
}

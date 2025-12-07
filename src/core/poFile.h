#pragma once
#include <string>
#include <vector>

namespace po
{
    class poNode;
    class poLexer;

    class poFile {
    public:
        poFile(const std::string& filename, const int fileId);
        void load(poLexer& lexer);
        void getErrorLines(const int lineNum, const int context, std::vector<std::string>& lines) const;

        inline bool isError() const { return _isError; }
        inline const std::string& errorText() const { return _errorText; }
        inline poNode* ast() const { return _ast; }
        inline const std::string& filename() const { return _filename; }
        inline int lineNum() const { return _lineNum; }
        inline int colNum() const { return _colNum; }
        inline int fileId() const { return _fileId; }

    private:
        poNode* _ast;
        std::string _filename;
        int _fileId;

        bool _isError;
        std::string _errorText;

        int _lineNum;
        int _colNum;
        std::vector<int> _lineStartPositions;
    };
}

#include "poFile.h"
#include "poLex.h"
#include "poParser.h"
#include "poAST.h"

#include <fstream>

using namespace po;

poFile::poFile(const std::string& filename, const int fileId)
    :
    _filename(filename),
    _fileId(fileId),
    _ast(nullptr),
    _isError(false),
    _colNum(0),
    _lineNum(0)
{
}

void poFile::load(poLexer& lexer)
{
    // Lexer
    lexer.tokenizeFile(_filename, _fileId);
    if (lexer.isError())
    {
        _isError = true;
        _errorText = lexer.errorText();
        _lineNum = lexer.lineNum();
    }
    else
    {
        _lineStartPositions = lexer.lineStartPositions();

        // Parse tokens into Abstract syntax tree
        poParser parser(lexer.tokens());
        poModuleParser moduleParser(parser);
        _ast = moduleParser.parse();
        if (parser.isError())
        {
            _isError = true;
            _errorText = parser.error();
            _lineNum = parser.errorLine();
            _colNum = parser.errorColumn();
        }
    }
}

void poFile::getErrorLines(const int lineNum, const int context, std::vector<std::string>& lines) const
{
    if (lineNum < 1 || lineNum > int(_lineStartPositions.size()))
    {
        return;
    }

    const int startLine = std::max(1, lineNum - context);
    const int endLine = std::min(int(_lineStartPositions.size()), lineNum + context);

    std::ifstream stream(_filename);
    if (stream.is_open())
    {
        std::string line;
        int currentLine = startLine;
        while (!stream.eof() && currentLine <= endLine)
        {
            stream.seekg(_lineStartPositions[currentLine - 1]);
            std::getline(stream, line);
            lines.push_back(line);
            currentLine++;
        }
    }
}

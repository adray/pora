#include "poLex.h"
#include <fstream>
#include <sstream>
#include <limits>
#include <math.h>

using namespace po;

//===============
// Token
//===============

poToken::poToken(poTokenType token, const std::string& value, const int line, const int col, const int fileId)
    :
    _tok(token),
    _value(value),
    _line(line),
    _col(col),
    _fileId(fileId)
{
}

uint64_t poToken::u64() const
{
    std::stringstream ss(_value);
    uint64_t u64 = 0;
    ss >> u64;
    return u64;
}

//==============
// Lexer
//==============

poLexer::poLexer()
    :
    _pos(0),
    _lineNum(1),
    _colNum(1),
    _fileId(0),
    _filePos(0),
    _isError(false),
    _scanning(false)
{
    addKeyword("if", poTokenType::IF);
    addKeyword("else", poTokenType::ELSE);
    //addKeyword("function", poTokenType::FUNCTION);
    //addKeyword("var", poTokenType::VAR);
    //addKeyword("yield", poTokenType::YIELD);
    addKeyword("return", poTokenType::RETURN);
    addKeyword("while", poTokenType::WHILE);
    addKeyword("for", poTokenType::FOR);
    addKeyword("continue", poTokenType::CONTINUE);
    addKeyword("break", poTokenType::BREAK);
    addKeyword("class", poTokenType::CLASS);
    addKeyword("enum", poTokenType::ENUM);
    addKeyword("new", poTokenType::NEW);
    addKeyword("delete", poTokenType::DELETE);
    addKeyword("public", poTokenType::PUBLIC);
    addKeyword("private", poTokenType::PRIVATE);
    addKeyword("protected", poTokenType::PROTECTED);
    addKeyword("internal", poTokenType::INTERNAL);
    //addKeyword("self", poTokenType::SELF);
    addKeyword("this", poTokenType::THIS);
    addKeyword("base", poTokenType::BASE);
    addKeyword("throw", poTokenType::THROW);
    addKeyword("catch", poTokenType::CATCH);
    addKeyword("try", poTokenType::TRY);
    addKeyword("import", poTokenType::IMPORT);
    addKeyword("namespace", poTokenType::NAMESPACE);
    addKeyword("static", poTokenType::STATIC);
    addKeyword("i64", poTokenType::I64_TYPE);
    addKeyword("i32", poTokenType::I32_TYPE);
    addKeyword("i16", poTokenType::I16_TYPE);
    addKeyword("i8", poTokenType::I8_TYPE);
    addKeyword("f64", poTokenType::F64_TYPE);
    addKeyword("f32", poTokenType::F32_TYPE);
    addKeyword("void", poTokenType::VOID);
    addKeyword("u64", poTokenType::U64_TYPE);
    addKeyword("u32", poTokenType::U32_TYPE);
    addKeyword("u16", poTokenType::U16_TYPE);
    addKeyword("u8", poTokenType::U8_TYPE);
    addKeyword("true", poTokenType::TRUE);
    addKeyword("false", poTokenType::FALSE);
    addKeyword("boolean", poTokenType::BOOLEAN);
    addKeyword("object", poTokenType::OBJECT);
    addKeyword("struct", poTokenType::STRUCT);
    addKeyword("extern", poTokenType::EXTERN);
    addKeyword("null", poTokenType::NULLPTR);
    addKeyword("sizeof", poTokenType::SIZEOF);
}

void poLexer::addKeyword(const std::string& keyword, poTokenType token)
{
    _keywords.insert(std::pair<std::string, poTokenType>(keyword, token));
}

void poLexer::addToken(poTokenType type, const std::string& value)
{
    poToken token(type, value, _lineNum, _colNum, _fileId);
    _tokens.push_back(token);
}

void poLexer::addToken(poTokenType type)
{
    poToken token(type, "", _lineNum, _colNum, _fileId);
    _tokens.push_back(token);
}

bool poLexer::isLetter(char ch)
{
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

void poLexer::scanIdentifier()
{
    std::string identifier;
    bool scanning = true;
    while (scanning)
    {
        char ch = peek();
        if (isLetter(ch) || isDigit(ch) || ch == '_')
        {
            identifier += ch;
            advance();
        }
        else
        {
            scanning = false;
        }
    }

    const auto& it = _keywords.find(identifier);
    if (it != _keywords.end())
    {
        addToken(it->second);
    }
    else
    {
        addToken(poTokenType::IDENTIFIER, identifier);
    }
}

bool poLexer::isDigit(char ch)
{
    bool isDigit = false;
    switch (ch)
    {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        isDigit = true;
        break;
    }
    return isDigit;
}

void poLexer::setError(const std::string& error)
{
    if (!_isError)
    {
        _isError = true;
        _errorText = error;
        _scanning = false;
    }
}

char poLexer::peek()
{
    return _line[_pos];
}

char poLexer::peekAhead()
{
    if (_pos + 1 < _line.size()) return _line[_pos + 1];
    else return '\0';
}

void poLexer::advance()
{
    if (_scanning)
    {
        _pos++;
        _scanning = _pos < _line.size();
    }
}

void poLexer::scanWhitespace()
{
    bool scanning = true;
    while (scanning && _scanning)
    {
        char ch = peek();
        switch (ch)
        {
        case ' ':
        case '\t':
            advance();
            break;
        default:
            scanning = false;
            break;
        }
    }
}

void poLexer::scanCharacterLiteral()
{
    char ch[2] = { peek(), '\0' };
    advance();
    if (peek() == '\'')
    {
        advance();
        addToken(poTokenType::CHAR, std::string(ch));
    }
    else
    {
        setError("Ill formed character literal.");
    }
}

void poLexer::scanStringLiteral()
{
    std::string str;
    bool scanning = true;
    while (scanning)
    {
        char ch = peek();
        if (ch == '\"')
        {
            addToken(poTokenType::STRING, str);
            advance();
            scanning = false;
        }
        else
        {
            str += ch;
            advance();
            if (!_scanning)
            {
                setError("Ill formed string literal.");
                scanning = false;
            }
        }
    }
}

template<typename T>
static bool validateInteger(const std::string& str)
{
    constexpr T maxValue = std::numeric_limits<T>().max();
    T integer = 0;
    for (size_t i = 0; i < str.size(); i++)
    {
        const int val = str[i] - '0';
        if (integer <= (maxValue - val) / 10) // N <= (K - V)/10
        {
            integer *= 10;
            integer += val;
            continue;
        }
        return false;
    }

    return true;
}

static bool validateSingle(const std::string& str)
{
    const float f32 = float(std::atof(str.c_str()));
    return !isnan(f32);
}

static bool validateDouble(const std::string& str)
{
    const double f64 = std::atof(str.c_str());
    return !isnan(f64);
}

void poLexer::scanNumberLiteral()
{
    std::string str;
    bool scanning = true;
    bool hasDot = false;
    while (scanning)
    {
        char ch = peek();
        if (isDigit(ch))
        {
            str += ch;
            advance();
        }
        else if (ch == '.')
        {
            if (!hasDot) {
                hasDot = true;
                str += ch;
                advance();
            }
            else
            {
                setError("Invalid numeric literal.");
                scanning = false;
            }
        }
        else
        {
            if (hasDot && ch == 'f')
            {
                if (validateSingle(str))
                {
                    advance();
                    addToken(poTokenType::F32, str);
                }
                else
                {
                    setError("The specified f32 is not valid.");
                }
            }
            else if (hasDot && ch == 'd')
            {
                if (validateDouble(str))
                {
                    advance();
                    addToken(poTokenType::F64, str);
                }
                else
                {
                    setError("The specified f64 is not valid.");
                }
            }
            else if (!hasDot && ch == 'b')
            {
                if (validateInteger<uint8_t>(str))
                {
                    advance();
                    addToken(poTokenType::U8, str);
                }
                else
                {
                    setError("The specified u8 is not valid.");
                }
            }
            else if (!hasDot && ch == 'u')
            {
                if (validateInteger<uint64_t>(str))
                {
                    advance();
                    addToken(poTokenType::U64, str);
                }
                else
                {
                    setError("The specified u64 is not valid.");
                }
            }
            else if (hasDot)
            {
                if (validateDouble(str))
                {
                    addToken(poTokenType::F64, str);
                }
                else
                {
                    setError("The specified f64 is not valid.");
                }
            }
            else
            {
                if (validateInteger<int64_t>(str))
                {
                    addToken(poTokenType::I64, str);
                }
                else
                {
                    setError("The specified i64 is not valid.");
                }
            }
            scanning = false;
        }
    }
}

void poLexer::scanLine(const std::string& line)
{
    if (_isError) { return; }

    _pos = 0;
    _line = line;
    _scanning = true;
    _lineStartPositions.push_back(_filePos);

    while (_scanning)
    {
        scanWhitespace();

        char ch = peek();

        switch (ch)
        {
        case '=':
            advance();
            if (peek() == '=') { advance(); addToken(poTokenType::EQUALS_EQUALS); }
            else if (peek() == '>') { advance(); addToken(poTokenType::ARROW); }
            else { addToken(poTokenType::EQUALS); }
            break;
        case '!':
            advance();
            if (peek() == '=') { advance(); addToken(poTokenType::NOT_EQUALS); }
            else { addToken(poTokenType::NOT); }
            break;
        case '<':
            advance();
            if (peek() == '=') { advance(); addToken(poTokenType::LESS_EQUALS); }
            else if (peek() == '<') { advance(); addToken(poTokenType::LEFT_SHIFT); }
            else { addToken(poTokenType::LESS); }
            break;
        case '>':
            advance();
            if (peek() == '=') { advance(); addToken(poTokenType::GREATER_EQUALS); }
            else if (peek() == '>') { advance(); addToken(poTokenType::RIGHT_SHIFT); }
            else { addToken(poTokenType::GREATER); }
            break;
        case ':':
            advance();
            if (peek() == ':') { advance(); addToken(poTokenType::RESOLVER); }
            else { addToken(poTokenType::COLON); }
            break;
        case ';':
            addToken(poTokenType::SEMICOLON);
            advance();
            break;
        case ',':
            addToken(poTokenType::COMMA);
            advance();
            break;
        case '(':
            addToken(poTokenType::OPEN_PARAN);
            advance();
            break;
        case ')':
            addToken(poTokenType::CLOSE_PARAN);
            advance();
            break;
        case '{':
            addToken(poTokenType::OPEN_BRACE);
            advance();
            break;
        case '}':
            addToken(poTokenType::CLOSE_BRACE);
            advance();
            break;
        case '[':
            addToken(poTokenType::OPEN_BRACKET);
            advance();
            break;
        case ']':
            addToken(poTokenType::CLOSE_BRACKET);
            advance();
            break;
        case '+':
            advance();
            if (peek() == '+') { advance(); addToken(poTokenType::INCREMENT); }
            else if (peek() == '=') { advance(); addToken(poTokenType::PLUS_EQUALS); }
            else { addToken(poTokenType::PLUS); }
            break;
        case '-':
            advance();
            if (peek() == '-') { advance(); addToken(poTokenType::DECREMENT); }
            else if (peek() == '=') { advance(); addToken(poTokenType::MINUS_EQUALS); }
            else { addToken(poTokenType::MINUS); }
            break;
        case '*':
            advance();
            if (peek() == '=') { advance(); addToken(poTokenType::STAR_EQUALS); }
            else { addToken(poTokenType::STAR); }
            break;
        case '/':
            advance();
            if (peek() == '/') { _scanning = false; } // comment
            else if (peek() == '=') { advance(); addToken(poTokenType::SLASH_EQUALS); }
            else { addToken(poTokenType::SLASH); }
            break;
        case '\'':
            advance();
            scanCharacterLiteral();
            break;
        case '\"':
            advance();
            scanStringLiteral();
            break;
        case '&':
            advance();
            if (peek() == '&')
            {
                addToken(poTokenType::AND);
                advance();
            }
            else
            {
                addToken(poTokenType::AMPERSAND);
            }
            break;
        case '|':
            advance();
            if (peek() == '|')
            {
                addToken(poTokenType::OR);
                advance();
            }
            else
            {
                setError("Bitwise OR not supported.");
            }
            break;
        case '%':
            advance();
            addToken(poTokenType::MODULO);
            break;
        default:
            if (isDigit(ch))
            {
                scanNumberLiteral();
            }
            else if (isLetter(ch))
            {
                scanIdentifier();
            }
            else if (ch == '.')
            {
                if (isDigit(peekAhead()))
                {
                    scanNumberLiteral();
                }
                else
                {
                    addToken(poTokenType::DOT);
                    advance();
                }
            }
            else if (ch == '\0')
            {
                advance();
            }
            else
            {
                setError("Unexcepted character.");
            }
            break;
        }
    }

    if (!isError())
    {
        _lineNum++;
    }
}

void poLexer::tokenizeText(const std::string& text, const int fileId)
{
    _fileId = fileId;

    const int size = 512;
    char line[size];

    std::stringstream stream(text);
    while (!stream.eof())
    {
        _filePos = int(stream.tellg());
        stream.getline(line, size);
        scanLine(line);
    }
}

void poLexer::tokenizeFile(const std::string& filename, const int fileId)
{
    _fileId = fileId;

    std::ifstream stream(filename);
    if (stream.is_open())
    {
        const int size = 512;
        char line[size];

        while (!stream.eof())
        {
            _filePos = int(stream.tellg());
            stream.getline(line, size);
            scanLine(line);
        }
    }
    else
    {
        setError("Could not open file: " + filename);
    }
}

void poLexer::reset()
{
    _tokens.clear();

    _pos = 0;
    _filePos = 0;
    _lineNum = 1;
    _colNum = 1;
    _lineStartPositions.clear();
    _isError = false;
    _scanning = false;
}

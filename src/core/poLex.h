#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace po
{
    enum class poTokenType
    {
        IDENTIFIER,
        STRING,
        CHAR,
        I8,
        I16,
        I32,
        I64,
        F32,
        F64,
        U64,
        U32,
        U16,
        U8,
        TRUE,
        FALSE,
        OPEN_PARAN,
        CLOSE_PARAN,
        OPEN_BRACKET,
        CLOSE_BRACKET,
        EQUALS_EQUALS,
        NOT_EQUALS,
        LESS_EQUALS,
        GREATER_EQUALS,
        EQUALS,
        NOT,
        LESS,
        GREATER,
        COLON,
        SEMICOLON,
        COMMA,
        OPEN_BRACE,
        CLOSE_BRACE,
        INCREMENT,
        DECREMENT,
        LEFT_SHIFT,
        RIGHT_SHIFT,
        PLUS,
        MINUS,
        STAR,
        SLASH,
        PLUS_EQUALS,
        MINUS_EQUALS,
        STAR_EQUALS,
        SLASH_EQUALS,
        AND,
        OR,
        DOT,
        AMPERSAND,
        MODULO,
        RESOLVER,

        IF,
        ELSE,
        RETURN,
        WHILE,
        FOR,
        CONTINUE,
        BREAK,
        CLASS,
        STRUCT,
        NEW,
        PUBLIC,
        PRIVATE,
        PROTECTED,
        INTERNAL,
        THIS,
        BASE,
        THROW,
        CATCH,
        TRY,
        IMPORT,
        NAMESPACE,
        EXTERN,
        STATIC,
        I64_TYPE,
        I32_TYPE,
        I16_TYPE,
        I8_TYPE,
        F64_TYPE,
        F32_TYPE,
        U64_TYPE,
        U32_TYPE,
        U16_TYPE,
        U8_TYPE,
        BOOLEAN,
        VOID,
        OBJECT,
        NULLPTR,
        SIZEOF
    };

    class poToken
    {
    public:
        poToken(poTokenType token, const std::string& value, int line, int col);
        inline poTokenType token() const { return _tok; }
        inline long long i64() const { return std::atoll(_value.c_str()); }
        inline int i32() const { return std::atol(_value.c_str()); }
        inline short i16() const { return (short) std::atol(_value.c_str()); }
        inline char i8() const { return static_cast<char>(std::atoi(_value.c_str()));; }
        uint64_t u64() const;
        inline unsigned int u32() const { return std::atol(_value.c_str()); }
        inline unsigned short u16() const { return (unsigned short) std::atol(_value.c_str()); }
        inline unsigned char u8() const { return static_cast<unsigned char>(std::atoi(_value.c_str())); }
        inline unsigned char character() const { return _value[0]; }
        inline const std::string& string() const { return _value; }
        inline float f32() const { return float(std::atof(_value.c_str())); }
        inline double f64() const { return std::atof(_value.c_str()); }

        inline int line() const { return _line; }
        inline int column() const { return _col; }
    private:

        poTokenType _tok;
        std::string _value;
        int _line;
        int _col;
    };

    class poLexer
    {
    public:
        poLexer();
        void tokenizeFile(const std::string& filename);
        void tokenizeText(const std::string& text);
        void reset();
        
        inline bool isError() { return _isError; }
        inline const std::string& errorText() { return _errorText; }
        inline int lineNum() const { return _lineNum; }
        inline const std::vector<poToken>& tokens() const { return _tokens; }

    private:
        void scanLine(const std::string& line);
        char peek();
        char peekAhead();
        bool isDigit(char ch);
        void scanIdentifier();
        bool isLetter(char ch);
        void setError(const std::string& error);
        void advance();
        void scanWhitespace();
        void scanStringLiteral();
        void scanNumberLiteral();
        void scanCharacterLiteral();
        void addKeyword(const std::string& keyword, poTokenType token);
        void addToken(poTokenType type, const std::string& value);
        void addToken(poTokenType type);

        std::unordered_map<std::string, poTokenType> _keywords;
        std::vector<poToken> _tokens;
        std::string _errorText;
        std::string _line;
        bool _isError;
        bool _scanning;
        int _pos;
        int _lineNum;
        int _colNum;
    };
}

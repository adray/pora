#pragma once
#include <vector>
#include <string>

namespace po
{
    constexpr int TYPE_VOID = 0;
    constexpr int TYPE_I64 = 1;
    constexpr int TYPE_I32 = 2;
    constexpr int TYPE_I16 = 3;
    constexpr int TYPE_I8 = 4;
    constexpr int TYPE_F64 = 5;
    constexpr int TYPE_F32 = 6;
    constexpr int TYPE_U64 = 7;
    constexpr int TYPE_U32 = 8;
    constexpr int TYPE_U16 = 9;
    constexpr int TYPE_U8 = 10;
    constexpr int TYPE_BOOLEAN = 11;
    constexpr int TYPE_STRING = 12;
    constexpr int TYPE_NULLPTR = 13;
    constexpr int TYPE_OBJECT = 14; /* values above this are user defined (non primitive) */

    enum class poOperatorType
    {
        IMPLICIT_CAST,
        EXPLICIT_CAST,
        PLUS,
        MINUS,
        STAR,
        SLASH,
        MODULO
    };

    constexpr int OPERATOR_FLAG_NONE = 0x00;
    constexpr int OPERATOR_FLAG_SIGN_EXTEND = 0x01;
    constexpr int OPERATOR_FLAG_ZERO_EXTEND = 0x02;
    constexpr int OPERATOR_FLAG_CONVERT_TO_REAL = 0x03;
    constexpr int OPERATOR_FLAG_CONVERT_TO_INTEGER = 0x04;


    class poOperator
    {
    public:
        poOperator(const poOperatorType operator_, const int otherType, const int flags);

        inline const int otherType() const { return _otherType; }
        inline const poOperatorType getOperator() const { return _operator; }
        inline const int flags() const { return _flags; }

    private:
        poOperatorType _operator; // the operator type
        int _otherType; // The type of the other parameter in a binary operation
        int _flags;
    };

    class poField
    {
    public:
        poField(const int offset, const int type, const std::string& name);

        inline const int offset() const { return _offset; }
        inline const int type() const { return _type; }
        inline const std::string& name() const { return _name; }

    private:
        int _offset;
        int _type;
        std::string _name;
    };

    class poType
    {
    public:
        poType(const int id, const int baseType, const std::string& name);

        inline void addField(const poField& field) { _fields.push_back(field); }
        inline void addOperator(const poOperator& operator_) { _operators.push_back(operator_); }
        inline void setSize(const int size) { _size = size; }
        inline void setAlignment(const int alignment) { _alignment = alignment; }

        inline void setArray(const bool isArray) { _isArray = isArray; }
        inline void setPointer(const bool isPointer) { _isPointer = isPointer; }

        inline const std::vector<poField>& fields() const { return _fields; }
        inline const std::vector<poOperator>& operators() const { return _operators; }

        inline const int id() const { return _id; }
        inline const int baseType() const { return _baseType; }
        inline const int size() const { return _size; }
        inline const int alignment() const { return _alignment; }
        inline const bool isArray() const { return _isArray; }
        inline const bool isPointer() const { return _isPointer; }
        inline const bool isPrimitive() const { return _id < TYPE_OBJECT; }
        inline const bool isObject() const { return _id == TYPE_OBJECT; }
        inline const std::string& name() const { return _name; }

    private:
        int _id;
        int _baseType;
        int _size;
        int _alignment;
        bool _isArray;
        bool _isPointer;
        std::string _name;
        std::vector<poField> _fields;
        std::vector<poOperator> _operators;
    };
}

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

    enum class poAttributes
    {
        PUBLIC = 0x1,
        PRIVATE = 0x2,
        INTERNAL = 0x4,
        EXTERN = 0x8,
        PROTECTED = 0x10,
    };

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
        poField(const poAttributes attributes, const int offset, const int type, const int numElements, const std::string& name);

        inline const poAttributes attributes() const { return _attributes; }
        inline const int offset() const { return _offset; }
        inline const int type() const { return _type; }
        inline const int numElements() const { return _numElements; }
        inline const std::string& name() const { return _name; }

    private:
        poAttributes _attributes;
        int _offset;
        int _type;
        int _numElements;
        std::string _name;
    };

    class poConstructor
    {
    public:
        poConstructor(const poAttributes attributes);
        inline void addArgument(const int type) { _arguments.push_back(type); }
        inline void setId(const int id) { _id = id; }
        inline void setIsDefault(const bool isDefault) { _isDefault = isDefault; }

        inline const std::vector<int>& arguments() const { return _arguments; }
        inline const poAttributes attributes() const { return _attributes; }
        inline const int id() const { return _id; }
        inline const bool isDefault() const { return _isDefault; }
    private:
        int _id;
        bool _isDefault;
        poAttributes _attributes;
        std::vector<int> _arguments;
    };

    class poMemberFunction
    {
    public:
        poMemberFunction(const poAttributes attributes, const int returnType, const std::string& name);
        inline void addArgument(const int type) { _arguments.push_back(type); }
        inline void setId(const int id) { _id = id; }
        inline const std::vector<int>& arguments() const { return _arguments; }
        inline const poAttributes attributes() const { return _attributes; }
        inline const int returnType() const { return _returnType; }
        inline const std::string& name() const { return _name; }
        inline const int id() const { return _id; }

    private:
        poAttributes _attributes;
        int _returnType;
        int _id;
        std::string _name;
        std::vector<int> _arguments;
    };

    class poType
    {
    public:
        poType(const int id, const int baseType, const std::string& name);
        poType(const int id, const int baseType, const std::string& name, const std::string& fullname);

        inline void addField(const poField& field) { _fields.push_back(field); }
        inline void addMethod(const poMemberFunction& method) { _methods.push_back(method); }
        inline void addOperator(const poOperator& operator_) { _operators.push_back(operator_); }
        inline void addConstructor(const poConstructor& constructor) { _constructors.push_back(constructor); }
        inline void setSize(const int size) { _size = size; }
        inline void setAlignment(const int alignment) { _alignment = alignment; }

        inline void setArray(const bool isArray) { _isArray = isArray; }
        inline void setPointer(const bool isPointer) { _isPointer = isPointer; }

        inline const std::vector<poField>& fields() const { return _fields; }
        inline const std::vector<poMemberFunction>& functions() const { return _methods; }
        inline const std::vector<poConstructor>& constructors() const { return _constructors; }
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
        inline const std::string& fullname() const { return _fullname; }

    private:
        int _id;
        int _baseType;
        int _size;
        int _alignment;
        bool _isArray;
        bool _isPointer;
        std::string _name;
        std::string _fullname;
        std::vector<poField> _fields;
        std::vector<poMemberFunction> _methods;
        std::vector<poConstructor> _constructors;
        std::vector<poOperator> _operators;
    };
}

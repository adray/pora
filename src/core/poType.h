#pragma once
#include <vector>
#include <string>

namespace po
{
    constexpr int TYPE_VOID = 0;
    constexpr int TYPE_I64 = 1;
    constexpr int TYPE_I32 = 2;
    constexpr int TYPE_I8 = 3;
    constexpr int TYPE_F64 = 4;
    constexpr int TYPE_F32 = 5;
    constexpr int TYPE_U64 = 6;
    constexpr int TYPE_U32 = 7;
    constexpr int TYPE_U8 = 8;
    constexpr int TYPE_BOOLEAN = 9;
    constexpr int TYPE_OBJECT = 10; /* values above this are user defined (non primitive) */

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
        inline void setSize(const int size) { _size = size; }

        inline void setArray(const bool isArray) { _isArray = isArray; }
        inline void setPointer(const bool isPointer) { _isPointer = isPointer; }

        inline const std::vector<poField>& fields() const { return _fields; }
        inline const int id() const { return _id; }
        inline const int baseType() const { return _baseType; }
        inline const int size() const { return _size; }
        inline const bool isArray() const { return _isArray; }
        inline const bool isPointer() const { return _isPointer; }
        inline const std::string& name() const { return _name; }

    private:
        int _id;
        int _baseType;
        int _size;
        bool _isArray;
        bool _isPointer;
        std::string _name;
        std::vector<poField> _fields;
    };
}

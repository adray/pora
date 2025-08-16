#pragma once
#include "poCFG.h"
#include "poType.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace po
{
    enum class poAttributes
    {
        PUBLIC = 0x1,
        PRIVATE = 0x2,
        INTERNAL = 0x4,
        EXTERN = 0x8
    };

    enum class poCallConvention
    {
        X86_64
    };

    template<typename T>
    class poResult
    {
    public:
        poResult(const std::vector<T>& list, const int index)
            :
            _list(list),
            _index(index)
        { }

        inline bool HasValue() const { return _index != 0; }
        inline T& Value() { return _list[_index]; }

    private:
        int _index;
        const std::vector<T>& _list;
    };

    class poConstant
    {
    public:
        poConstant(uint64_t u64);
        poConstant(int64_t i64);
        poConstant(int32_t i32);
        poConstant(uint32_t u32);
        poConstant(int16_t i16);
        poConstant(uint16_t u16);
        poConstant(int8_t i8);
        poConstant(uint8_t u8);
        poConstant(const float f32);
        poConstant(const double f64);
        inline int type() const { return _type; }
        inline int64_t u64() const { return _u64; }
        inline int64_t i64() const { return _i64; }
        inline int32_t i32() const { return _i32; }
        inline uint32_t u32() const { return _u32; }
        inline int16_t i16() const { return _i16; }
        inline uint16_t u16() const { return _u16; }
        inline int8_t i8() const { return _i8; }
        inline uint8_t u8() const { return _u8; }
        inline float f32() const { return _f32; }
        inline double f64() const { return _f64; }

    private:
        int _type;
        union
        {
            uint64_t _u64;
            int64_t _i64;
            int32_t _i32;
            uint32_t _u32;
            int16_t _i16;
            uint16_t _u16;
            int8_t _i8;
            uint8_t _u8;
            double _f64;
            float _f32;
        };
    };

    class poFunction
    {
    public:
        poFunction(const std::string& name, int arity, poAttributes attribute, poCallConvention callingConvention);
        inline const std::string& name() const { return _name; }
        inline const poAttributes attribute() const { return _attribute; }
        inline const poCallConvention callConvention() const { return _callingConvention; }
        inline poFlowGraph& cfg() { return _cfg; }
        inline void addVariable(const int name) { _variables.push_back(name); }
        inline const std::vector<int> variables() const { return _variables; }
        inline const bool hasAttribute(poAttributes attribute) const { return (int(_attribute) & int(attribute)) == int(attribute); }
        inline void addArgument(const int type) { _arguments.push_back(type); } 
        inline const std::vector<int>& args() const { return _arguments; }

    private:
        int _arity;
        poAttributes _attribute;
        std::string _name;
        poFlowGraph _cfg;
        poCallConvention _callingConvention;
        std::vector<int> _variables;
        std::vector<int> _arguments;
    };

    class poNamespace
    {
    public:
        poNamespace(const std::string& name);
        void addFunction(const poFunction& function);
        inline const std::string& name() const { return _name; }
        inline std::vector<poFunction>& functions() { return _functions; }
        inline const std::vector<poFunction>& functions() const { return _functions; }

    private:
        std::string _name;
        std::vector<poFunction> _functions;
    };

    class poConstantPool
    {
    public:
        int addConstant(const uint64_t u64);
        int addConstant(const int64_t i64);
        int addConstant(const int32_t i32);
        int addConstant(const uint32_t u32);
        int addConstant(const int16_t i16);
        int addConstant(const uint16_t u16);
        int addConstant(const int8_t i8);
        int addConstant(const uint8_t u8);
        int addConstant(const double f64);
        int addConstant(const float f32);
        int getConstant(const uint64_t u64);
        int getConstant(const uint32_t u32);
        int getConstant(const uint8_t u8);
        int getConstant(const int64_t i64);
        int getConstant(const int32_t i32);
        int getConstant(const int8_t i8);
        int getConstant(const double f64);
        int getConstant(const float f32);
        int64_t getI64(const int id);
        int32_t getI32(const int id);
        int16_t getI16(const int id);
        int8_t getI8(const int id);
        uint64_t getU64(const int id);
        uint32_t getU32(const int id);
        uint16_t getU16(const int id);
        uint8_t getU8(const int id);
        float getF32(const int id);
        double getF64(const int id);

    private:
        std::unordered_map<uint64_t, int> _u64;
        std::unordered_map<int64_t, int> _i64;
        std::unordered_map<int32_t, int> _i32;
        std::unordered_map<uint32_t, int> _u32;
        std::unordered_map<int16_t, int> _i16;
        std::unordered_map<uint16_t, int> _u16;
        std::unordered_map<int8_t, int> _i8;
        std::unordered_map<uint8_t, int> _u8;
        std::unordered_map<double, int> _f64;
        std::unordered_map<float, int> _f32;
        std::vector<poConstant> _constants;
    };

    class poModule
    {
    public:
        poModule();
        void addNamespace(const poNamespace& ns);
        poResult<poNamespace> getNamespace(const std::string& name);
        inline std::vector<poNamespace>& namespaces() { return _namespaces; }
        inline poConstantPool& constants() { return _constants; }
        inline std::vector<poType>& types() { return _types; }
        int addSymbol(const std::string& symbol);
        bool getSymbol(const int id, std::string& symbol);
        void addType(const poType& type);
        int getTypeFromName(const std::string& name) const;
        int getArrayType(const int baseType) const;
        int getPointerType(const int baseType) const;
        void dump();
        void dumpTypes();

    private:
        void addPrimitives();
        void addExplicitCastOperators();

        std::vector<poType> _types;
        std::unordered_map<std::string, int> _typeMapping;
        std::unordered_map<int, int> _arrayTypes; /* mapping from base type -> array type */
        std::unordered_map<int, int> _pointerTypes; /* mapping from base type -> pointer type */
        std::unordered_map<int, std::string> _symbols;
        std::vector<poNamespace> _namespaces;
        poConstantPool _constants;
    };
}

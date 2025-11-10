#pragma once
#include "poCFG.h"
#include "poType.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace po
{
    enum class poCallConvention
    {
        X86_64
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
        poFunction(const std::string& name, const std::string& fullname, int arity, poAttributes attribute, poCallConvention callingConvention);
        inline const std::string& name() const { return _name; }
        inline const std::string& fullname() const { return _fullname; }
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
        std::string _fullname;
        poFlowGraph _cfg;
        poCallConvention _callingConvention;
        std::vector<int> _variables;
        std::vector<int> _arguments;
    };

    class poNamespace
    {
    public:
        poNamespace(const std::string& name);
        inline const std::string& name() const { return _name; }
        inline void addFunction(const int id) { _functions.push_back(id); }
        inline void addType(const int id) { _types.push_back(id); }
        inline void addStaticVariable(const int id) { _staticVariables.push_back(id); }
        inline const std::vector<int>& functions() const { return _functions; }
        inline const std::vector<int>& types() const { return _types; }
        inline const std::vector<int>& staticVariables() const { return _staticVariables; }

    private:
        std::string _name;
        std::vector<int> _functions;
        std::vector<int> _types;
        std::vector<int> _staticVariables;
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
        int addConstant(const std::string& str);
        int getConstant(const uint64_t u64);
        int getConstant(const uint32_t u32);
        int getConstant(const uint16_t u16);
        int getConstant(const uint8_t u8);
        int getConstant(const int64_t i64);
        int getConstant(const int32_t i32);
        int getConstant(const int16_t i16);
        int getConstant(const int8_t i8);
        int getConstant(const double f64);
        int getConstant(const float f32);
        int getConstant(const std::string& str);
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
        const std::string& getString(const int id);

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
        std::unordered_map<std::string, int> _strings;
        std::vector<poConstant> _constants;
        std::vector<std::string> _strConstants;
    };

    class poStaticVariable
    {
    public:
        poStaticVariable(const int type, const std::string& name)
            : _type(type), _name(name), _constantId(-1)
        {
        }

        poStaticVariable(const int type, const std::string& name, const int constantId)
            : _type(type), _name(name), _constantId(constantId)
        {
        }

        inline const int type() const { return _type; }
        inline const std::string& name() const { return _name; }
        inline const int constantId() const { return _constantId; }

    private:
        int _type;
        int _constantId;
        std::string _name;
    };

    class poModule
    {
    public:
        poModule();
        void addNamespace(const poNamespace& ns);
        inline std::vector<poNamespace>& namespaces() { return _namespaces; }
        inline poConstantPool& constants() { return _constants; }
        inline std::vector<poType>& types() { return _types; }
        inline std::vector<poFunction>& functions() { return _functions; }
        inline const std::vector<poFunction>& functions() const { return _functions; }
        inline std::vector<poStaticVariable>& staticVariables() { return _staticVariables; }
        int addSymbol(const std::string& symbol);
        bool getSymbol(const int id, std::string& symbol);
        void addFunction(const poFunction& function);
        void addType(const poType& type);
        void addStaticVariable(const poStaticVariable& variable);
        int getTypeFromName(const std::string& name) const;
        int getArrayType(const int baseType) const;
        int getPointerType(const int baseType) const;
        void dump();
        void dumpTypes();

    private:
        void addPrimitives();
        void addExplicitCastOperators();

        std::vector<poType> _types;
        std::vector<poFunction> _functions;
        std::vector<poStaticVariable> _staticVariables;
        std::unordered_map<std::string, int> _typeMapping;
        std::unordered_map<int, int> _arrayTypes; /* mapping from base type -> array type */
        std::unordered_map<int, int> _pointerTypes; /* mapping from base type -> pointer type */
        std::unordered_map<int, std::string> _symbols;
        std::vector<poNamespace> _namespaces;
        poConstantPool _constants;
    };
}

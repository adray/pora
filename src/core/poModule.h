#pragma once
#include "poCFG.h"

#include <vector>
#include <string>
#include <unordered_map>

namespace po
{
    enum class poAttributes
    {
        PUBLIC,
        PRIVATE,
        INTERNAL
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

        inline bool HasValue() const { _index != 0; }
        inline T& Value() { return _list[_index]; }

    private:
        int _index;
        const std::vector<T>& _list;
    };

    class poConstant
    {
    public:
        poConstant(int64_t i64);
        poConstant(int32_t i32);
        poConstant(int8_t i8);
        poConstant(const float f32);
        poConstant(const double f64);
        inline int type() const { return _type; }
        inline int64_t i64() const { return _i64; }
        inline int64_t i32() const { return _i32; }
        inline int64_t i8() const { return _i8; }

    private:
        int _type;
        int64_t _i64;
        int32_t _i32;
        int8_t _i8;
        double _f64;
        float _f32;
    };

    class poFunction
    {
    public:
        poFunction(const std::string& name, int arity, poAttributes attribute);
        inline const std::string& name() const { return _name; }
        inline const poAttributes attribute() const { return _attribute; }
        inline poFlowGraph& cfg() { return _cfg; }
        inline void addVariable(const int name) { _variables.push_back(name); }
        inline const std::vector<int> variables() const { return _variables; }

    private:
        int _arity;
        poAttributes _attribute;
        std::string _name;
        poFlowGraph _cfg;
        std::vector<int> _variables;
    };

    class poType
    {
    public:
        poType(const std::string& name, poAttributes attribute);
        void addFunction(const poFunction& function);
        inline const std::string& name() const { return _name; }
        inline const poAttributes attribute() const { return _attribute; }

    private:
        std::string _name;
        std::vector<poFunction> _functions;
        poAttributes _attribute;
    };

    class poNamespace
    {
    public:
        poNamespace(const std::string& name);
        void addType(const poType& type);
        void addFunction(const poFunction& function);
        inline const std::string& name() const { return _name; }
        inline std::vector<poFunction>& functions() { return _functions; }

    private:
        std::string _name;
        std::vector<poType> _types;
        std::vector<poFunction> _functions;
    };

    class poConstantPool
    {
    public:
        int addConstant(const int64_t i64);
        int addConstant(const int32_t i32);
        int addConstant(const int8_t i8);
        int addConstant(const double f64);
        int addConstant(const float f32);
        int getConstant(const int64_t i64);
        int getConstant(const int32_t i32);
        int getConstant(const int8_t i8);
        int getConstant(const double f64);
        int getConstant(const float f32);

    private:
        std::unordered_map<int64_t, int> _i64;
        std::unordered_map<int32_t, int> _i32;
        std::unordered_map<int8_t, int> _i8;
        std::unordered_map<double, int> _f64;
        std::unordered_map<float, int> _f32;
        std::vector<poConstant> _constants;
    };

    class poModule
    {
    public:
        void addNamespace(const poNamespace& ns);
        poResult<poNamespace> getNamespace(const std::string& name);
        inline std::vector<poNamespace>& namespaces() { return _namespaces; }
        inline poConstantPool& constants() { return _constants; }
        void dump();

    private:
        std::vector<poNamespace> _namespaces;
        poConstantPool _constants;
    };
}

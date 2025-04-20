#include "poModule.h"
#include "poTypeChecker.h"
#include <iostream>

using namespace po;

//=================
// poConstant
//================

poConstant::poConstant(int64_t i64)
    :
    _i64(i64),
    _i32(0),
    _i8(0),
    _f32(0.0f),
    _f64(0.0),
    _type(TYPE_I64)
{
}
poConstant::poConstant(int32_t i32)
    :
    _i64(0),
    _i32(i32),
    _i8(0),
    _f32(0.0f),
    _f64(0.0),
    _type(TYPE_I32)
{
}
poConstant::poConstant(int8_t i8)
    :
    _i64(0),
    _i32(0),
    _i8(i8),
    _f32(0.0f),
    _f64(0.0),
    _type(TYPE_I8)
{
}
poConstant::poConstant(const float f32)
    :
    _i64(0),
    _i32(0),
    _i8(0),
    _f32(f32),
    _f64(0.0),
    _type(TYPE_F32)
{
}
poConstant::poConstant(const double f64)
    :
    _i64(0),
    _i32(0),
    _i8(0),
    _f32(0.0f),
    _f64(f64),
    _type(TYPE_F64)
{
}

//=================
// ConstantPool
//=================

int poConstantPool::addConstant(const int64_t i64)
{
    const auto& it = _i64.find(i64);
    if (it == _i64.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(i64));
        _i64.insert(std::pair<int64_t, int>(i64, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const int32_t i32)
{
    const auto& it = _i32.find(i32);
    if (it == _i32.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(i32));
        _i32.insert(std::pair<int32_t, int>(i32, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const int8_t i8)
{
    const auto& it = _i8.find(i8);
    if (it == _i8.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(i8));
        _i8.insert(std::pair<int8_t, int>(i8, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const double f64)
{
    const auto& it = _f64.find(f64);
    if (it == _f64.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(f64));
        _f64.insert(std::pair<double, int>(f64, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const float f32)
{
    const auto& it = _f32.find(f32);
    if (it == _f32.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(f32));
        _f32.insert(std::pair<float, int>(f32, id));
        return id;
    }
    return -1;
}

int poConstantPool::getConstant(const int64_t i64)
{
    const auto& it = _i64.find(i64);
    if (it != _i64.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const int32_t i32)
{
    const auto& it = _i32.find(i32);
    if (it != _i32.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const int8_t i8)
{
    const auto& it = _i8.find(i8);
    if (it != _i8.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const double f64)
{
    const auto& it = _f64.find(f64);
    if (it != _f64.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const float f32)
{
    const auto& it = _f32.find(f32);
    if (it != _f32.end())
    {
        return it->second;
    }

    return -1;
}

int64_t poConstantPool::getI64(const int id)
{
    return _constants[id].i64();
}

int32_t poConstantPool::getI32(const int id)
{
    return _constants[id].i32();
}

//=================
// Function
//================

poFunction::poFunction(const std::string& name, int arity, poAttributes attribute)
    :
    _name(name),
    _arity(arity),
    _attribute(attribute)
{
}

//===============
// Type
//===============

poType::poType(const std::string& name, poAttributes attribute)
    :
    _name(name),
    _attribute(attribute)
{
}

void poType::addFunction(const poFunction& function)
{
    _functions.push_back(function);
}

//=================
// Namespace
//=================

poNamespace::poNamespace(const std::string& name)
    :
    _name(name)
{
}
void poNamespace::addType(const poType& type)
{
    _types.push_back(type);
}
void poNamespace::addFunction(const poFunction& function)
{
    _functions.push_back(function);
}

//================
// Module
//================

void poModule::addNamespace(const poNamespace& ns)
{
    _namespaces.push_back(ns);
}

poResult<poNamespace> poModule::getNamespace(const std::string& name)
{
    int index = -1;
    for (size_t i = 0; i < _namespaces.size(); i++)
    {
        if (_namespaces[i].name() == name)
        {
            index = int(i);
            break;
        }
    }

    return poResult<poNamespace>(_namespaces, index);
}

int poModule::addSymbol(const std::string& symbol)
{
    int id = int(_symbols.size());
    _symbols.insert(std::pair<int, std::string>(id, symbol));
    return id;
}

bool poModule::getSymbol(const int id, std::string& symbol)
{
    const auto& it = _symbols.find(id);
    if (it != _symbols.end())
    {
        symbol = it->second;
        return true;
    }
    return false;
}

void poModule::dump()
{
    for (auto& ns : _namespaces)
    {
        std::cout << "===================" << std::endl;
        for (auto& func : ns.functions())
        {
            std::cout << ns.name() << "::" << func.name() << std::endl;
            
            const int numBB = int(func.cfg().numBlocks());
            std::unordered_map<poBasicBlock*, int> blocks;
            for (int i = 0; i < numBB; i++)
            {
                blocks[func.cfg().getBasicBlock(i)] = i;
            }

            for (int i = 0; i < numBB; i++)
            {
                std::cout << "Basic Block " << i << ":" << std::endl;
                poBasicBlock* bb = func.cfg().getBasicBlock(i);
                for (auto& ins : bb->instructions())
                {
                    std::cout << ins.name();
                    switch (ins.code())
                    {
                    case IR_ADD:
                        std::cout << " IR_ADD " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_SUB:
                        std::cout << " IR_SUB " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_MUL:
                        std::cout << " IR_MUL " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_DIV:
                        std::cout << " IR_DIV " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_CMP:
                        std::cout << " IR_CMP " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_PHI:
                        std::cout << " IR_PHI " << int(ins.type()) << " " << ins.left() << " " << ins.right();
                        break;
                    case IR_UNARY_MINUS:
                        std::cout << " IR_UNARY_MINUS " << int(ins.type()) << " " << ins.left();
                        break;
                    case IR_CONSTANT:
                        std::cout << " IR_CONSTANT " << int(ins.type()) << " " << ins.constant();
                        break;
                    case IR_RETURN:
                        std::cout << " IR_RETURN " << int(ins.type()) << " " << ins.left();
                        break;
                    case IR_CALL:
                        std::cout << " IR_CALL " << int(ins.type()) << " " << int(ins.left());
                        break;
                    case IR_ARG:
                        std::cout << " IR_ARG " << int(ins.type()) << " " << int(ins.left());
                        break;
                    case IR_PARAM:
                        std::cout << " IR_PARAM " << int(ins.type());
                        break;
                    case IR_BR:
                        std::cout << " IR_BR " << int(ins.type()) << " " << ins.left();
                        if (bb->getBranch())
                        {
                            const int target = blocks[bb->getBranch()];
                            std::cout << " --> Basic Block " << target;
                        }
                        break;
                    }
                    std::cout << std::endl;
                }
            }

            std::cout << "===================" << std::endl;
        }
    }
}

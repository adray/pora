#include "poModule.h"
#include <iostream>

using namespace po;

//=================
// poConstant
//================

poConstant::poConstant(uint64_t u64)
    :
    _u64(u64),
    _type(TYPE_U64)
{
}

poConstant::poConstant(int64_t i64)
    :
    _i64(i64),
    _type(TYPE_I64)
{
}
poConstant::poConstant(uint32_t u32)
    :
    _u32(u32),
    _type(TYPE_I32)
{
}
poConstant::poConstant(int32_t i32)
    :
    _i32(i32),
    _type(TYPE_I32)
{
}
poConstant::poConstant(int16_t i16)
    :
    _i16(i16),
    _type(TYPE_I16)
{
}
poConstant::poConstant(uint16_t u16)
    :
    _u16(u16),
    _type(TYPE_U16)
{
}
poConstant::poConstant(int8_t i8)
    :
    _i8(i8),
    _type(TYPE_I8)
{
}
poConstant::poConstant(uint8_t u8)
    :
    _u8(u8),
    _type(TYPE_U8)
{
}
poConstant::poConstant(const float f32)
    :
    _f32(f32),
    _type(TYPE_F32)
{
}
poConstant::poConstant(const double f64)
    :
    _f64(f64),
    _type(TYPE_F64)
{
}

//=================
// ConstantPool
//=================

int poConstantPool::addConstant(const uint64_t u64)
{
    const auto& it = _u64.find(u64);
    if (it == _u64.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(u64));
        _u64.insert(std::pair<int64_t, int>(u64, id));
        return id;
    }
    return -1;
}

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

int poConstantPool::addConstant(const uint32_t u32)
{
    const auto& it = _u32.find(u32);
    if (it == _u32.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(u32));
        _u32.insert(std::pair<uint32_t, int>(u32, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const int16_t i16)
{
    const auto& it = _i16.find(i16);
    if (it == _i16.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(i16));
        _i16.insert(std::pair<int16_t, int>(i16, id));
        return id;
    }
    return -1;
}

int poConstantPool::addConstant(const uint16_t u16)
{
    const auto& it = _u16.find(u16);
    if (it == _u16.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(u16));
        _u16.insert(std::pair<uint16_t, int>(u16, id));
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

int poConstantPool::addConstant(const uint8_t u8)
{
    const auto& it = _u8.find(u8);
    if (it == _u8.end())
    {
        const int id = int(_constants.size());
        _constants.push_back(poConstant(u8));
        _u8.insert(std::pair<uint8_t, int>(u8, id));
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

int poConstantPool::getConstant(const uint64_t u64)
{
    const auto& it = _u64.find(u64);
    if (it != _u64.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const uint32_t u32)
{
    const auto& it = _u32.find(u32);
    if (it != _u32.end())
    {
        return it->second;
    }

    return -1;
}

int poConstantPool::getConstant(const uint8_t u8)
{
    const auto& it = _u8.find(u8);
    if (it != _u8.end())
    {
        return it->second;
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

int8_t poConstantPool::getI8(const int id)
{
    return _constants[id].i8();
}

int32_t poConstantPool::getI32(const int id)
{
    return _constants[id].i32();
}

int16_t poConstantPool::getI16(const int id)
{
    return _constants[id].i16();
}

uint64_t poConstantPool::getU64(const int id)
{
    return _constants[id].u64();
}

uint32_t poConstantPool::getU32(const int id)
{
    return _constants[id].u32();
}

uint16_t poConstantPool::getU16(const int id)
{
    return _constants[id].u16();
}

uint8_t poConstantPool::getU8(const int id)
{
    return _constants[id].u8();
}

float poConstantPool::getF32(const int id)
{
    return _constants[id].f32();
}

double poConstantPool::getF64(const int id)
{
    return _constants[id].f64();
}

//=================
// Function
//================

poFunction::poFunction(const std::string& name, int arity, poAttributes attribute, poCallConvention callingConvention)
    :
    _name(name),
    _arity(arity),
    _attribute(attribute),
    _callingConvention(callingConvention)
{
}

//=================
// Namespace
//=================

poNamespace::poNamespace(const std::string& name)
    :
    _name(name)
{
}
void poNamespace::addFunction(const poFunction& function)
{
    _functions.push_back(function);
}

//================
// Module
//================

poModule::poModule()
{
    addPrimitives();
}

void poModule::addPrimitives()
{
    addType(poType(TYPE_VOID, -1, "VOID"));
    addType(poType(TYPE_I64, -1, "I64"));
    addType(poType(TYPE_I32, -1, "I32"));
    addType(poType(TYPE_I16, -1, "I16"));
    addType(poType(TYPE_I8, -1, "I8"));
    addType(poType(TYPE_F64, -1, "F64"));
    addType(poType(TYPE_F32, -1, "F32"));
    addType(poType(TYPE_U64, -1, "U64"));
    addType(poType(TYPE_U32, -1, "U32"));
    addType(poType(TYPE_U16, -1, "U16"));
    addType(poType(TYPE_U8, -1, "U8"));
    addType(poType(TYPE_BOOLEAN, -1, "BOOLEAN"));
    addType(poType(TYPE_NULLPTR, -1, "NULLPTR"));
    addType(poType(TYPE_OBJECT, -1, "OBJECT"));

    auto& types = _types;
    types[TYPE_I64].setSize(8);
    types[TYPE_I32].setSize(4);
    types[TYPE_I16].setSize(2);
    types[TYPE_I8].setSize(1);
    types[TYPE_F64].setSize(8);
    types[TYPE_F32].setSize(4);
    types[TYPE_U64].setSize(8);
    types[TYPE_U32].setSize(4);
    types[TYPE_U16].setSize(2);
    types[TYPE_U8].setSize(1);
    types[TYPE_BOOLEAN].setSize(1);

    for (auto& type : types)
    {
        type.setAlignment(type.size());
    }

    addExplicitCastOperators();
}

void poModule::addExplicitCastOperators()
{
    /* Allow explicit casts for all numeric types */
    
    auto& types = _types;
    for (int i = TYPE_I64; i <= TYPE_I8; i++)
    {
        auto& dstType = types[i];
        /* Signed to Signed */
        for (int j = TYPE_I64; j <= TYPE_I8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                int flags = OPERATOR_FLAG_NONE;
                if (dstType.size() > srcType.size())
                {
                    flags = OPERATOR_FLAG_SIGN_EXTEND;
                }
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, flags));
            }
        }

        /* Signed to Unsigned */
        for (int j = TYPE_U64; j <= TYPE_U8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                int flags = OPERATOR_FLAG_SIGN_EXTEND;
                if (dstType.size() <= srcType.size())
                {
                    flags = OPERATOR_FLAG_NONE;
                }
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, flags));
            }
        }

        /* Signed to floating point */
        dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, TYPE_F32, OPERATOR_FLAG_CONVERT_TO_REAL));
        dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, TYPE_F64, OPERATOR_FLAG_CONVERT_TO_REAL));
    }

    for (int i = TYPE_U64; i <= TYPE_U8; i++)
    {
        auto& dstType = types[i];
        /* Unsigned to Signed */
        for (int j = TYPE_I64; j <= TYPE_I8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, OPERATOR_FLAG_NONE));
            }
        }

        /* Unsigned to Unsigned */
        for (int j = TYPE_U64; j <= TYPE_U8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                int flags = OPERATOR_FLAG_ZERO_EXTEND;
                if (dstType.size() < srcType.size())
                if (dstType.size() < srcType.size())
                {
                    flags = OPERATOR_FLAG_NONE;
                }
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, flags));
            }
        }

        /* Unsigned to floating point */
        //dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, TYPE_F32, OPERATOR_FLAG_CONVERT_TO_REAL));
        //dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, TYPE_F64, OPERATOR_FLAG_CONVERT_TO_REAL));
    }

    for (int i = TYPE_F64; i <= TYPE_F32; i++)
    {
        auto& dstType = types[i];
        /* Floating point to Signed */
        for (int j = TYPE_I64; j <= TYPE_I8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, OPERATOR_FLAG_CONVERT_TO_INTEGER));
            }
        }

        /* Floating point to Unsigned */
        for (int j = TYPE_U64; j <= TYPE_U8; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, OPERATOR_FLAG_CONVERT_TO_INTEGER));
            }
        }

        /* Floating point to Floating point */
        for (int j = TYPE_F64; j <= TYPE_F32; j++)
        {
            if (i != j)
            {
                auto& srcType = types[j];
                dstType.addOperator(poOperator(poOperatorType::EXPLICIT_CAST, j, OPERATOR_FLAG_NONE));
            }
        }
    }
}

void poModule::addNamespace(const poNamespace& ns)
{
    _namespaces.push_back(ns);
}

void poModule::addType(const poType& type)
{
    _types.push_back(type);
    _typeMapping.insert(std::pair<std::string, int>(type.name(), type.id()));

    if (type.isArray())
    {
        _arrayTypes.insert(std::pair<int, int>(type.baseType(), type.id()));
    }
    else if (type.isPointer())
    {
        _pointerTypes.insert(std::pair<int, int>(type.baseType(), type.id()));
    }
}

int poModule::getArrayType(const int baseType) const
{
    const auto& it = _arrayTypes.find(baseType);
    if (it != _arrayTypes.end())
    {
        return it->second;
    }
    return -1;
}

int poModule::getPointerType(const int baseType) const
{
    const auto& it = _pointerTypes.find(baseType);
    if (it != _pointerTypes.end())
    {
        return it->second;
    }
    return -1;
}

int poModule::getTypeFromName(const std::string& name) const
{
    const auto& it = _typeMapping.find(name);
    if (it != _typeMapping.end())
    {
        return it->second;
    }
    return -1;
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
            if (func.hasAttribute(poAttributes::EXTERN))
            {
                continue;
            }

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
                    case IR_COPY:
                        std::cout << " IR_COPY " << int(ins.type()) << " " << ins.left();
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
                    case IR_ALLOCA:
                        std::cout << " IR_ALLOCA " << int(ins.type()) << " " << int(ins.left());
                        break;
                    case IR_MALLOC:
                        std::cout << " IR_MALLOC " << int(ins.type()) << " " << int(ins.left());
                        break;
                    case IR_LOAD:
                        std::cout << " IR_LOAD " << int(ins.type()) << " " << int(ins.left());
                        break;
                    case IR_STORE:
                        std::cout << " IR_STORE " << int(ins.type()) << " " << int(ins.left()) << " " << int(ins.right());
                        break;
                    case IR_PTR:
                        std::cout << " IR_PTR " << int(ins.type()) << " " << int(ins.left()) << " " << int(ins.right()) << " #" << int(ins.memOffset());
                        break;
                    case IR_ELEMENT_PTR:
                        std::cout << " IR_ELEMENT_PTR " << int(ins.type()) << " " << int(ins.left()) << " " << int(ins.right()) << " #" << int(ins.memOffset());
                        break;
                    case IR_SIGN_EXTEND:
                        std::cout << " IR_SIGN_EXTEND " << int(ins.type()) << " " << int(ins.left()) << " /" << int(ins.memOffset());
                        break;
                    case IR_ZERO_EXTEND:
                        std::cout << " IR_ZERO_EXTEND " << int(ins.type()) << " " << int(ins.left()) << " /" << int(ins.memOffset());
                        break;
                    case IR_BITWISE_CAST:
                        std::cout << " IR_BITWISE_CAST " << int(ins.type()) << " " << int(ins.left()) << " /" << int(ins.memOffset());
                        break;
                    case IR_CONVERT:
                        std::cout << " IR_CONVERT " << int(ins.type()) << " " << int(ins.left()) << " /" << int(ins.memOffset());
                        break;
                    }
                    std::cout << std::endl;
                }
            }

            std::cout << "===================" << std::endl;
        }
    }
}

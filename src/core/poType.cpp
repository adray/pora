#include "poType.h"

using namespace po;

poOperator::poOperator(const poOperatorType operator_, const int otherType, const int flags)
    :
    _operator(operator_),
    _otherType(otherType),
    _flags(flags)
{
}

poField::poField(const int offset, const int type, const std::string& name)
    :
    _offset(offset),
    _type(type),
    _name(name)
{
}

poType::poType(const int id, const int baseType, const std::string& name)
    :
    _id(id),
    _baseType(baseType),
    _isArray(false),
    _isPointer(false),
    _name(name),
    _size(0),
    _alignment(0)
{
}

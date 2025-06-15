#include "poType.h"

using namespace po;

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
    _size(0)
{
}

#include "poType.h"

using namespace po;

poOperator::poOperator(const poOperatorType operator_, const int otherType, const int flags)
    :
    _operator(operator_),
    _otherType(otherType),
    _flags(flags)
{
}

poConstructor::poConstructor(const poAttributes attributes) :
    _attributes(attributes),
    _id(-1),
    _isDefault(false)
{
}

poMemberFunction::poMemberFunction(const poAttributes attributes, const int returnType, const std::string& name) :
   _attributes(attributes),
    _returnType(returnType),
    _name(name),
    _id(-1)
{
}

poMemberProperty::poMemberProperty(const poAttributes attributes, const int type, const std::string& name, const std::string& backingFieldName)
    :
    _attributes(attributes),
    _type(type),
    _name(name),
    _backingFieldName(backingFieldName)
{
}

poField::poField(const poAttributes attributes, const int offset, const int type, const int numElements, const std::string& name)
    :
    _attributes(attributes),
    _offset(offset),
    _type(type),
    _numElements(numElements),
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
    _fullname(name),
    _size(0),
    _alignment(0)
{
}

poType::poType(const int id, const int baseType, const std::string& name, const std::string& fullname)
    :
    _id(id),
    _baseType(baseType),
    _isArray(false),
    _isPointer(false),
    _name(name),
    _fullname(fullname),
    _size(0),
    _alignment(0)
{
}

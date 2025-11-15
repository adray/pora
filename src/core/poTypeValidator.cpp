#include "poTypeValidator.h"
#include "poType.h"
#include "poModule.h"
#include <vector>

using namespace po;

bool poTypeValidator::validateModule(poModule& module)
{
    bool ok = true;
    for (const poType& type : module.types())
    {
        if (!validateType(module, type))
        {
            ok = false;
            break;
        }
    }

    // Check the static variable are uniquely named
    std::vector<std::string> varNames;
    for (const poStaticVariable& variable : module.staticVariables())
    {
        const auto& it = std::lower_bound(varNames.begin(), varNames.end(), variable.name());
        if (it != varNames.end() && *it == variable.name())
        {
            ok = false;
            break;
        }
        varNames.insert(it, variable.name());
    }

    return ok;
}

bool poTypeValidator::validateType(const poModule& module, const poType& type)
{
    bool ok = true;

    // Check the member fields are uniquely named
    std::vector<std::string> fieldNames;
    for (const poField& field : type.fields())
    {
        const auto& it = std::lower_bound(fieldNames.begin(), fieldNames.end(), field.name());
        if (it != fieldNames.end() && *it == field.name())
        {
            ok = false;
            break;
        }

        fieldNames.insert(it, field.name());
    }

    // Check the member functions are uniquely named
    std::vector<std::string> methodNames;
    for (const poMemberFunction& method : type.functions())
    {
        const auto& it = std::lower_bound(methodNames.begin(), methodNames.end(), method.name());
        if (it != methodNames.end() && *it == method.name())
        {
            ok = false;
            break;
        }
        methodNames.insert(it, method.name());
    }

    // Check if the fields have constructors with zero arguments
    for (const poField& field : type.fields())
    {
        int typeId = field.type();
        const poType& fieldType = module.types()[field.type()];
        if (fieldType.isPrimitive() || fieldType.isPointer())
        {
            continue;
        }
        if (fieldType.isArray())
        {
            const poType& baseType = module.types()[fieldType.baseType()];
            if (baseType.isPrimitive() || baseType.isPointer())
            {
                continue;
            }
            typeId = baseType.id();
        }

        const poType& checkType = module.types()[typeId];
        bool hasConstructor = int(checkType.constructors().size()) == 0;
        for (const poConstructor& constructor : checkType.constructors())
        {
            if (int(constructor.arguments().size()) == 0)
            {
                hasConstructor = true;
                break;
            }
        }
        ok = ok && hasConstructor;
    }

    return ok;
}

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
        if (!validateType(type))
        {
            ok = false;
            break;
        }
    }

    return ok;
}

bool poTypeValidator::validateType(const poType& type)
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

    return ok;
}

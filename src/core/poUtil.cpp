#include "poUtil.h"
#include "poLex.h"
#include "poType.h"
#include "poModule.h"
#include "poAST.h"

#include <assert.h>

using namespace po;

int poUtil::getType(poToken& token) {
    int type = TYPE_VOID;
    switch (token.token())
    {
    case poTokenType::I64_TYPE:
        type = TYPE_I64;
        break;
    case poTokenType::I32_TYPE:
        type = TYPE_I32;
        break;
    case poTokenType::I16_TYPE:
        type = TYPE_I16;
        break;
    case poTokenType::I8_TYPE:
        type = TYPE_I8;
        break;
    case poTokenType::U64_TYPE:
        type = TYPE_U64;
        break;
    case poTokenType::U32_TYPE:
        type = TYPE_U32;
        break;
    case poTokenType::U16_TYPE:
        type = TYPE_U16;
        break;
    case poTokenType::U8_TYPE:
        type = TYPE_U8;
        break;
    case poTokenType::F64_TYPE:
        type = TYPE_F64;
        break;
    case poTokenType::F32_TYPE:
        type = TYPE_F32;
        break;
    case poTokenType::VOID:
        type = TYPE_VOID;
        break;
    case poTokenType::BOOLEAN:
        type = TYPE_BOOLEAN;
        break;
    default:
        type = -1;
        break;
    }
    return type;
}

int poUtil::getPointerType(poModule& module, const int baseType)
{
    if (baseType == -1)
    {
        return -1;
    }

    const int ptr = module.getPointerType(baseType);
    if (ptr == -1)
    {
        const poType& type = module.types()[baseType];

        const int id = int(module.types().size());
        poType pointerType(id, baseType, type.name() + "*");
        pointerType.setKind(poTypeKind::Pointer);
        pointerType.setSize(8);
        pointerType.setAlignment(8);
        module.addType(pointerType);

        return id;
    }

    return ptr;
}

int poUtil::unpackTypeNode(poModule& module, const std::vector<std::string>& parametricArgs, poNode* node) {
    int pointerCount = 0;
    if (node->type() == poNodeType::POINTER) {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(node);
        pointerCount = pointerNode->count();
        node = pointerNode->child();
    }

    assert(node->type() == poNodeType::TYPE);

    int type = -1;
    for (int i = 0; i < int(parametricArgs.size()); i++) {
        const std::string& param = parametricArgs[i];
        if (param == node->token().string()) {
            type = TYPE_PARAMETRIC_1 + i;
            break;
        }
    }

    if (type == -1) {
        type = poUtil::getType(node->token());
        if (type == -1) {
            type = module.getTypeFromName(node->token().string());
            if (type == -1) {
                return -1;
            }
        }
    }

    while (pointerCount > 0) {
        type = getPointerType(module, type);
        pointerCount--;
    }

    return type;
}

int poUtil::unpackTypeNode(poModule& module, poNode* node) {

    int pointerCount = 0;
    if (node->type() == poNodeType::POINTER) {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(node);
        pointerCount = pointerNode->count();
        node = pointerNode->child();
    }

    assert(node->type() == poNodeType::TYPE);

    int type = poUtil::getType(node->token());
    if (type == -1) {
        type = module.getTypeFromName(node->token().string());
        if (type == -1) {
            return -1;
        }
    }

    while (pointerCount > 0) {
        type = getPointerType(module, type);
        pointerCount--;
    }

    return type;
}

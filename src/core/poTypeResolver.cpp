#include "poTypeResolver.h"
#include "poModule.h"
#include "poType.h"
#include "poAST.h"

#include <assert.h>

using namespace po;

poTypeResolver::poTypeResolver(poModule& module)
    :
    _module(module)
{
}

int poTypeResolver::getTypeSize(const int type)
{
    return _module.types()[type].size();
}

int poTypeResolver::getType(const poToken& token)
{
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
    default:
        type = -1;
        break;
    }
    return type;
}

void poTypeResolver::resolve(const std::vector<poNode*>& nodes)
{
    for (poNode* node : nodes)
    {
        if (node->type() == poNodeType::MODULE)
        {
            poListNode* moduleNode = static_cast<poListNode*>(node);
            for (poNode* child : moduleNode->list())
            {
                if (child->type() == poNodeType::NAMESPACE)
                {
                    getNamespaces(child);
                }
            }
        }
    }
}

void poTypeResolver::getNamespaces(poNode* node)
{
    poListNode* namespaceNode = static_cast<poListNode*>(node);
    assert(namespaceNode->type() == poNodeType::NAMESPACE);
    poNamespace ns(namespaceNode->token().string());
    for (poNode* child : namespaceNode->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            getFunction(child, ns);
        }
        else if (child->type() == poNodeType::STRUCT)
        {
            getStruct(child, ns);
        }
        else if (child->type() == poNodeType::EXTERN)
        {
            getExtern(child, ns);
        }
    }
    resolveTypes(ns);
    _module.addNamespace(ns);
}

void poTypeResolver::getStruct(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::STRUCT);
    _userTypes.push_back(node);
}

void poTypeResolver::getExtern(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::EXTERN);

    poListNode* functionNode = static_cast<poListNode*>(node);
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            ns.addFunction(poFunction(
                node->token().string(),
                int(args->list().size()),
                (poAttributes) (int(poAttributes::PUBLIC) | int(poAttributes::EXTERN)),
                poCallConvention::X86_64));
        }
    }
}

void poTypeResolver::getFunction(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::FUNCTION);

    poListNode* functionNode = static_cast<poListNode*>(node);
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            ns.addFunction(poFunction(
                node->token().string(),
                int(args->list().size()),
                poAttributes::PUBLIC,
                poCallConvention::X86_64));
        }
    }
}

int poTypeResolver::align(const int offset, const int alignment) const
{
    if (alignment <= 0)
    {
        return 0; // No alignment needed
    }

    const int mod = offset % alignment;
    if (mod == 0)
    {
        return 0;
    }

    return alignment - mod;
}

int poTypeResolver::getPointerType(const int baseType, const int count)
{
    int pointerType = baseType;
    for (int i = 0; i < count; i++)
    {
        int type = _module.getPointerType(pointerType);
        if (type == -1)
        {
            const poType& typeData = _module.types()[pointerType];

            type = int(_module.types().size());
            poType newType(type, pointerType, typeData.name() + "*");
            newType.setPointer(true);
            newType.setSize(8);
            newType.setAlignment(8);
            _module.addType(newType);
        }
        pointerType = type;

    }
    return pointerType;
}

void poTypeResolver::resolveTypes(poNamespace& ns)
{
    //
    // Loop over the unresolved types attempting to resolve them.
    // Types can be resolved when all the constituent types are known.
    //

    std::vector<poNode*> worklist;
    for (poNode* item : _userTypes)
    {
        worklist.push_back(item);
    }

    bool changes = true;
    while (changes)
    {
        changes = false;

        std::vector<poNode*> work = worklist;
        worklist.clear();

        for (poNode* node : work)
        {
            bool resolved = true;
            poListNode* structNode = static_cast<poListNode*>(node);
            for (poNode* child : structNode->list())
            {
                if (child->type() == poNodeType::DECL)
                {
                    poUnaryNode* decl = static_cast<poUnaryNode*>(child);
                    if (decl->child()->type() == poNodeType::POINTER)
                    {
                        poPointerNode* pointerNode = static_cast<poPointerNode*>(decl->child());
                        decl = pointerNode;
                    }

                    if (decl->child()->type() == poNodeType::ARRAY)
                    {
                        // An array type is in a good state if the base type is resolved.

                        poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
                        poNode* typeNode = arrayNode->child();

                        const int type = getType(typeNode->token());
                        if (type == -1) /* is not a primitive */
                        {
                            const auto& entry = _resolvedTypes.find(typeNode->token().string());
                            if (entry == _resolvedTypes.end())
                            {
                                // unable to resolve yet..
                                resolved = false;
                                break;
                            }
                        }
                    }

                    else if (decl->child()->type() == poNodeType::TYPE)
                    {
                        poNode* typeNode = decl->child();
                        const int type = getType(typeNode->token());
                        if (type == -1) /* is not a primitive */
                        {
                            const auto& entry = _resolvedTypes.find(typeNode->token().string());
                            if (entry == _resolvedTypes.end())
                            {
                                // unable to resolve yet..
                                resolved = false;
                                break;
                            }
                        }
                    }
                }
            }

            if (resolved)
            {
                const std::string& name = structNode->token().string();
                const auto& entry = _resolvedTypes.find(name);
                if (entry != _resolvedTypes.end())
                {
                    continue;
                }

                const int id = int(_module.types().size());
                _module.addType(poType(id,
                    TYPE_OBJECT,
                    name));
                int offset = 0;
                int sizeOfLargestField = 0;
                for (poNode* child : structNode->list())
                {
                    if (child->type() == poNodeType::DECL)
                    {
                        poUnaryNode* decl = static_cast<poUnaryNode*>(child);

                        int pointerCount = 0;
                        if (decl->child()->type() == poNodeType::POINTER)
                        {
                            poPointerNode* pointerNode = static_cast<poPointerNode*>(decl->child());
                            decl = pointerNode;
                            pointerCount = pointerNode->count();
                        }

                        if (decl->child()->type() == poNodeType::TYPE)
                        {
                            poNode* typeNode = decl->child();
                            int fieldType = getPointerType(getType(typeNode->token()), pointerCount);
                            int size = 0;
                            if (fieldType == -1)
                            {
                                const auto& userType = _resolvedTypes.find(typeNode->token().string());
                                if (userType != _resolvedTypes.end())
                                {
                                    auto& typeData = _module.types()[userType->second];
                                    size = typeData.size();
                                    fieldType = typeData.id();
                                }
                            }
                            else
                            {
                                size = getTypeSize(fieldType);
                            }

                            // Align the offset to the size of the type
                            const int alignment = _module.types()[fieldType].alignment();
                            const int padding = align(offset, alignment);
                            offset += padding;

                            auto& type = _module.types()[id];
                            type.addField(poField(offset, fieldType, decl->token().string()));

                            offset += size;
                            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
                        }
                        else if (decl->child()->type() == poNodeType::ARRAY)
                        {
                            poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
                            poNode* typeNode = arrayNode->child();
                            int fieldType = getPointerType(getType(typeNode->token()), pointerCount);
                            int size = 0;
                            if (fieldType == -1)
                            {
                                const auto& userType = _resolvedTypes.find(typeNode->token().string());
                                if (userType != _resolvedTypes.end())
                                {
                                    auto& typeData = _module.types()[userType->second];
                                    size = typeData.size();
                                    fieldType = typeData.id();
                                }
                            }
                            else
                            {
                                size = getTypeSize(fieldType);
                            }

                            int arrayType = _module.getArrayType(fieldType);
                            if (arrayType == -1)
                            {
                                arrayType = int(_module.types().size());
                                std::string baseName = _module.types()[fieldType].name();
                                poType type(arrayType, fieldType, baseName + "[]");
                                type.setArray(true);
                                _module.addType(type);
                            }

                            const int alignment = size * _module.types()[fieldType].alignment();
                            const int totalSize = size * int(arrayNode->arraySize());
                            const int padding = align(offset, alignment);
                            offset += padding;

                            auto& type = _module.types()[id];
                            type.addField(poField(offset, arrayType, decl->token().string()));
                            offset += totalSize;
                            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
                        }
                    }
                }

                // Align the size of the type to the size of the largest field
                const int padding = align(offset, sizeOfLargestField);
                auto& type = _module.types()[id];
                type.setSize(offset + padding);
                type.setAlignment(sizeOfLargestField);
                _resolvedTypes.insert(std::pair<std::string, int>(name, id));

                changes = true;
            }
            else
            {
                worklist.push_back(node);
            }
        }
    }

    // TODO: If the worklist still contains items
    // then we need to a raise an error for each of the items that
    // cannot be resolved and terminate the build
}


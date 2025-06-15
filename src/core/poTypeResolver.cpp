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
    case poTokenType::I8_TYPE:
        type = TYPE_I8;
        break;
    case poTokenType::U64_TYPE:
        type = TYPE_U64;
        break;
    case poTokenType::U32_TYPE:
        type = TYPE_U32;
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

void poTypeResolver::addPrimitives()
{
    _module.addType(poType(TYPE_VOID, -1, "VOID"));
    _module.addType(poType(TYPE_I64, -1, "I64"));
    _module.addType(poType(TYPE_I32, -1, "I32"));
    _module.addType(poType(TYPE_I8, -1, "I8"));
    _module.addType(poType(TYPE_F64, -1, "F64"));
    _module.addType(poType(TYPE_F32, -1, "F32"));
    _module.addType(poType(TYPE_U64, -1, "U64"));
    _module.addType(poType(TYPE_U32, -1, "U32"));
    _module.addType(poType(TYPE_U8, -1, "U8"));
    _module.addType(poType(TYPE_BOOLEAN, -1, "BOOLEAN"));
    _module.addType(poType(TYPE_OBJECT, -1, "OBJECT"));

    auto& types = _module.types();
    types[TYPE_I64].setSize(sizeof(int64_t));
    types[TYPE_I32].setSize(sizeof(int32_t));
    types[TYPE_I8].setSize(sizeof(int8_t));
    types[TYPE_F64].setSize(sizeof(double));
    types[TYPE_F32].setSize(sizeof(float));
    types[TYPE_U64].setSize(sizeof(uint64_t));
    types[TYPE_U32].setSize(sizeof(uint32_t));
    types[TYPE_U8].setSize(sizeof(uint8_t));
    types[TYPE_BOOLEAN].setSize(sizeof(int8_t));
}

void poTypeResolver::resolve(const std::vector<poNode*>& nodes)
{
    addPrimitives();

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
                poAttributes::PUBLIC,
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
                const int id = int(_module.types().size());
                poType type(id,
                    TYPE_OBJECT,
                    name);
                int offset = 0;
                for (poNode* child : structNode->list())
                {
                    if (child->type() == poNodeType::DECL)
                    {
                        poUnaryNode* decl = static_cast<poUnaryNode*>(child);
                        if (decl->child()->type() == poNodeType::TYPE)
                        {
                            poNode* typeNode = decl->child();
                            int fieldType = getType(typeNode->token());
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

                            type.addField(poField(offset, fieldType, decl->token().string()));

                            offset += size;
                        }
                        else if (decl->child()->type() == poNodeType::ARRAY)
                        {
                            poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
                            poNode* typeNode = arrayNode->child();
                            int fieldType = getType(typeNode->token());
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

                            const int arrayType = _module.getArrayType(fieldType);
                            if (arrayType == -1)
                            {
                                const int id = int(_module.types().size());
                                std::string baseName = _module.types()[fieldType].name();
                                poType type(id, fieldType, baseName + "[]");
                                type.setArray(true);
                                _module.addType(type);
                            }

                            offset += size * int(arrayNode->arraySize());
                        }
                    }
                }
                type.setSize(offset);
                _module.addType(type);
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


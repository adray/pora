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
    int size = 0;
    switch (type)
    {
    case TYPE_BOOLEAN:
    case TYPE_U8:
    case TYPE_I8:
        size = 1;
        break;
    case TYPE_U32:
    case TYPE_I32:
    case TYPE_F32:
        size = 4;
        break;
    case TYPE_U64:
    case TYPE_I64:
    case TYPE_F64:
        size = 8;
        break;
    default:
        break;
    }
    return size;
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
    }
    resolveTypes(ns);
    _module.addNamespace(ns);
}

void poTypeResolver::getStruct(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::STRUCT);
    _userTypes.push_back(node);
}

void poTypeResolver::getFunction(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::FUNCTION);
    //_functions.insert(std::pair<std::string, poNode*>(node->token().string(), node));
    poListNode* functionNode = static_cast<poListNode*>(node);
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            ns.addFunction(poFunction(
                node->token().string(),
                int(args->list().size()),
                poAttributes::PUBLIC));
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
                    if (decl->child()->type() == poNodeType::TYPE)
                    {
                        poNode* typeNode = decl->child();
                        const int type = getType(typeNode->token());
                        if (type == -1)
                        {
                            const auto& entry = _types.find(typeNode->token().string());
                            if (entry == _types.end())
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
                const int id = TYPE_OBJECT + 1 + int(_module.types().size());
                poType type(id,
                    TYPE_OBJECT,
                    false,
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
                                const auto& userType = _types.find(typeNode->token().string());
                                if (userType != _types.end())
                                {
                                    auto& typeData = _module.types()[userType->second - TYPE_OBJECT - 1];
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
                    }
                }
                type.setSize(offset);
                _module.addType(type);
                _types.insert(std::pair<std::string, int>(name, id));

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


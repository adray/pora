#include "poTypeResolver.h"
#include "poModule.h"
#include "poType.h"
#include "poAST.h"

#include <assert.h>

using namespace po;

poTypeResolver::poTypeResolver(poModule& module)
    :
    _module(module),
    _isError(false)
{
}

void poTypeResolver::setError(const std::string& errorText)
{
    _isError = true;
    _errorText = errorText;
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
    case poTokenType::BOOLEAN:
        type = TYPE_BOOLEAN;
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

    resolveTypes();

    if (_unresolvedTypes.size() > 0)
    {
        // TODO: If the worklist still contains items
        // then we need to a raise an error for each of the items that
        // cannot be resolved and terminate the build
        setError("Could not resolve type '" + _unresolvedTypes[0]->token().string() + "'");
    }
    else
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
                        updateArgs(child);
                        resolveStatics(child);
                    }
                }
            }
        }
    }
}

void poTypeResolver::resolveStatics(poNode* namespaceNode)
{
    poListNode* list = static_cast<poListNode*>(namespaceNode);
    poNamespace& ns = _module.namespaces()[_namespaces[namespaceNode]];

    for (poNode* child : list->list())
    {
        if (child->type() == poNodeType::STATEMENT)
        {
            poUnaryNode* statementNode = static_cast<poUnaryNode*>(child);
            poNode* decl = statementNode->child();
            poNode* constant = nullptr;
            if (statementNode->child()->type() == poNodeType::ASSIGNMENT)
            {
                poBinaryNode* assignment = static_cast<poBinaryNode*>(statementNode->child());
                decl = assignment->left();
                constant = assignment->right();
            }

            if (decl->type() == poNodeType::DECL)
            {
                poUnaryNode* declNode = static_cast<poUnaryNode*>(decl);

                poNode* type = declNode->child();
                int pointerCount = 0;
                if (declNode->child()->type() == poNodeType::POINTER)
                {
                    poPointerNode* pointerNode = static_cast<poPointerNode*>(declNode->child());
                    pointerCount = pointerNode->count();
                    type = pointerNode->child();
                }

                poToken& token = type->token();
                int dataType = getType(token);
                if (dataType == -1) { dataType = _module.getTypeFromName(token.string());}

                const int resultType = getPointerType(dataType, pointerCount);
                poType& resultTypeInfo = _module.types()[resultType];
                int constantId = -1;
                if (constant == nullptr)
                {
                    poConstantPool& pool = _module.constants();
                    switch (token.token())
                    {
                    case poTokenType::I64_TYPE:
                        constantId = pool.getConstant((int64_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((int64_t)0); }
                        break;
                    case poTokenType::U64_TYPE:
                        constantId = pool.getConstant((uint64_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((uint64_t)0); }
                        break;
                    case poTokenType::I32_TYPE:
                        constantId = pool.getConstant((int32_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((int32_t)0); }
                        break;
                    case poTokenType::U32_TYPE:
                        constantId = pool.getConstant((uint32_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((uint32_t)0); }
                        break;
                    case poTokenType::I16_TYPE:
                        constantId = pool.getConstant((int16_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((int16_t)0); }
                        break;
                    case poTokenType::U16_TYPE:
                        constantId = pool.getConstant((uint16_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((uint16_t)0); }
                        break;
                    case poTokenType::I8_TYPE:
                        constantId = pool.getConstant((int8_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((int8_t)0); }
                        break;
                    case poTokenType::U8_TYPE:
                    case poTokenType::BOOLEAN:
                        constantId = pool.getConstant((uint8_t)0);
                        if (constantId == -1) { constantId = pool.addConstant((uint8_t)0); }
                        break;
                    default:
                        if (resultTypeInfo.isPointer())
                        {
                            constantId = pool.getConstant((int64_t)0);
                            if (constantId == -1) { constantId = pool.addConstant((int64_t)0); }
                        }
                        break;
                    }
                }
                else
                {
                    poConstantPool& pool = _module.constants();
                    poToken token = constant->token();
                    switch (token.token())
                    {
                    case poTokenType::I64:
                        constantId = pool.getConstant(token.i64());
                        if (constantId == -1) { constantId = pool.addConstant(token.i64()); }
                        break;
                    case poTokenType::U64:
                        constantId = pool.getConstant(token.u64());
                        if (constantId == -1) { constantId = pool.addConstant(token.u64()); }
                        break;
                    case poTokenType::I32:
                        constantId = pool.getConstant(token.i32());
                        if (constantId == -1) { constantId = pool.addConstant(token.i32()); }
                        break;
                    case poTokenType::U32:
                        constantId = pool.getConstant(token.u32());
                        if (constantId == -1) { constantId = pool.addConstant(token.u32()); }
                        break;
                    case poTokenType::I16:
                        constantId = pool.getConstant(token.i16());
                        if (constantId == -1) { constantId = pool.addConstant(token.i16()); }
                        break;
                    case poTokenType::U16:
                        constantId = pool.getConstant(token.u16());
                        if (constantId == -1) { constantId = pool.addConstant(token.u16()); }
                        break;
                    case poTokenType::I8:
                        constantId = pool.getConstant(token.i8());
                        if (constantId == -1) { constantId = pool.addConstant(token.i8()); }
                        break;
                    case poTokenType::BOOLEAN:
                    case poTokenType::U8:
                        constantId = pool.getConstant(token.u8());
                        if (constantId == -1) { constantId = pool.addConstant(token.u8()); }
                        break;
                    }
                }
                if (constantId != -1)
                {
                    _module.addStaticVariable(poStaticVariable(
                        resultType,
                        declNode->token().string(),
                        constantId
                    ));
                }
                else
                {
                    setError("Unable to resolve constant for static variable: " + declNode->token().string());
                }
                ns.addStaticVariable(int(_module.staticVariables().size()) - 1);
            }
        }
    }
}

void poTypeResolver::getNamespaces(poNode* node)
{
    poListNode* namespaceNode = static_cast<poListNode*>(node);
    assert(namespaceNode->type() == poNodeType::NAMESPACE);
    const int id = int(_module.namespaces().size());
    poNamespace ns(namespaceNode->token().string());
    for (poNode* child : namespaceNode->list())
    {
        if (child->type() == poNodeType::FUNCTION)
        {
            getFunction(child, ns);
        }
        else if (child->type() == poNodeType::CONSTRUCTOR)
        {
            getConstructor(child, ns);
        }
        else if (child->type() == poNodeType::STRUCT)
        {
            getStruct(child, ns, id);
        }
        else if (child->type() == poNodeType::CLASS)
        {
            getClass(child, ns, id);
        }
        else if (child->type() == poNodeType::EXTERN)
        {
            getExtern(child, ns);
        }
    }
    _module.addNamespace(ns);

    _namespaces.insert(std::pair<poNode*, int>(node, id));
}

void poTypeResolver::addType(poNode* node, poNamespace& ns, const int nsId)
{
    const std::string& name = node->token().string();
    const int id = int(_module.types().size());
    _module.addType(poType(id,
        TYPE_OBJECT,
        name,
        ns.name() + "::" + name));
    ns.addType(id);
    _namespaceForTypes.insert(std::pair<poNode*, int>(node, nsId));
}

void poTypeResolver::getClass(poNode* node, poNamespace& ns, const int nsId)
{
    assert(node->type() == poNodeType::CLASS);
    _userTypes.push_back(node);

    addType(node, ns, nsId);
}

void poTypeResolver::getStruct(poNode* node, poNamespace& ns, const int nsId)
{
    assert(node->type() == poNodeType::STRUCT);
    _userTypes.push_back(node);

    addType(node, ns, nsId);
}

void poTypeResolver::getExtern(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::EXTERN);

    poUnaryNode* externNode = static_cast<poUnaryNode*>(node);
    poListNode* functionNode = static_cast<poListNode*>(externNode->child());
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            _module.addFunction(poFunction(
                node->token().string(),
                ns.name() + "::" + node->token().string(),
                int(args->list().size()),
                (poAttributes) (int(poAttributes::PUBLIC) | int(poAttributes::EXTERN)),
                poCallConvention::X86_64));
            ns.addFunction(int(_module.functions().size()) - 1);
        }
    }
}

void poTypeResolver::getFunction(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::FUNCTION);

    poListNode* functionNode = static_cast<poListNode*>(node);
    poResolverNode* resolver = nullptr;
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            const int id = int(_module.functions().size());
            std::string fullname = ns.name() + "::" + node->token().string();
            if (resolver && resolver->path().size() > 0)
            {
                fullname = ns.name() + "::" + resolver->path()[0] + "::" + node->token().string();
                if (_memberFunctions.find(fullname) != _memberFunctions.end())
                {
                    setError("Duplicate member function: " + fullname);
                    return;
                }
                _memberFunctions.insert(std::pair<std::string, int>(fullname, id));
            }
            else
            {
                ns.addFunction(id);
            }

            _module.addFunction(poFunction(
                node->token().string(),
                fullname,
                int(args->list().size()),
                poAttributes::PUBLIC,
                poCallConvention::X86_64));
        }
        else if (child->type() == poNodeType::RESOLVER)
        {
            resolver = static_cast<poResolverNode*>(child);
        }
    }
}

void poTypeResolver::getConstructor(poNode* node, poNamespace& ns)
{
    assert(node->type() == poNodeType::CONSTRUCTOR);

    poListNode* constructorNode = static_cast<poListNode*>(node);
    poResolverNode* resolver = nullptr;
    for (poNode* child : constructorNode->list())
    {
        if (child->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(child);
            const int id = int(_module.functions().size());
            std::string fullname = ns.name() + "::" + node->token().string();
            if (resolver && resolver->path().size() > 0)
            {
                fullname = ns.name() + "::" + resolver->path()[0] + "::" + node->token().string();
                if (_memberFunctions.find(fullname) != _memberFunctions.end())
                {
                    setError("Duplicate constructor: " + fullname);
                    return;
                }
                _memberFunctions.insert(std::pair<std::string, int>(fullname, id));
            }
            else
            {
                ns.addFunction(id);
            }

            _module.addFunction(poFunction(
                node->token().string(),
                fullname,
                int(args->list().size()),
                poAttributes::PUBLIC,
                poCallConvention::X86_64));
        }
        else if (child->type() == poNodeType::RESOLVER)
        {
            resolver = static_cast<poResolverNode*>(child);
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

bool poTypeResolver::isTypeResolved(poNode* node, bool isPointer)
{
    bool resolved = true;
    const int type = getType(node->token());
    if (type == -1) /* is not a primitive */
    {
        const std::string& name = node->token().string();
        if (isPointer)
        {
            // A pointer is resolved if the base class is a known type
            resolved = _module.getTypeFromName(name);
        }
        else
        {
            // unable to resolve yet?
            const auto& entry = _resolvedTypes.find(name);
            resolved = entry != _resolvedTypes.end(); 
        }
    }
    return resolved;
}

void poTypeResolver::resolveTypes()
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
            poListNode* typeNode = static_cast<poListNode*>(node);
            const std::string& name = typeNode->token().string();
            for (poNode* child : typeNode->list())
            {
                if (child->type() != poNodeType::DECL)
                {
                    continue;
                }

                poUnaryNode* decl = static_cast<poUnaryNode*>(child);
                poAttributes attributes = poAttributes::PUBLIC;
                if (decl->child()->type() == poNodeType::ATTRIBUTES)
                {
                    poAttributeNode* attributeNode = static_cast<poAttributeNode*>(decl->child());
                    attributes = attributeNode->attributes();
                    decl = attributeNode;
                }

                bool isPointer = false;
                if (decl->child()->type() == poNodeType::POINTER)
                {
                    poPointerNode* pointerNode = static_cast<poPointerNode*>(decl->child());
                    decl = pointerNode;
                    isPointer = true;
                }

                if (decl->child()->type() == poNodeType::ARRAY)
                {
                    // An array type is in a good state if the base type is resolved.

                    poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
                    poNode* typeNode = arrayNode->child();

                    resolved &= isTypeResolved(typeNode, isPointer);
                }

                else if (decl->child()->type() == poNodeType::TYPE)
                {
                    resolved &= isTypeResolved(decl->child(), isPointer);
                }
                else if (decl->child()->type() == poNodeType::EXTERN)
                {
                    // Method or constructor
                    poUnaryNode* externNode = static_cast<poUnaryNode*>(decl->child());
                    poListNode* method = static_cast<poListNode*>(externNode->child());

                    for (poNode* node : method->list())
                    {
                        if (node->type() == poNodeType::ARGS)
                        {
                            poListNode* args = static_cast<poListNode*>(node);
                            for (poNode* argument : args->list())
                            {
                                poUnaryNode* argNode = static_cast<poUnaryNode*>(argument);
                                resolved &= isTypeResolved(argNode->child(), isPointer);
                            }
                        }
                    }
                }
            }

            if (resolved)
            {
                const auto& entry = _resolvedTypes.find(name);
                if (entry != _resolvedTypes.end())
                {
                    continue;
                }

                const auto& namespaceIt = _namespaceForTypes.find(node);
                if (namespaceIt == _namespaceForTypes.end())
                {
                    continue;
                }

                poNamespace& ns = _module.namespaces()[namespaceIt->second];
                resolveType(name, ns, typeNode);

                changes = true;
            }
            else
            {
                worklist.push_back(node);
            }
        }
    }

    _unresolvedTypes = worklist;
}

void poTypeResolver::resolveType(const std::string& name, poNamespace& ns, poListNode* typeNode)
{
    const int id = _module.getTypeFromName(name);
    int offset = 0;
    int sizeOfLargestField = 0;
    for (poNode* child : typeNode->list())
    {
        if (child->type() != poNodeType::DECL)
        {
            continue;
        }

        poAttributes attributes = poAttributes::PUBLIC;
        poUnaryNode* decl = static_cast<poUnaryNode*>(child);
        if (decl->child()->type() == poNodeType::ATTRIBUTES)
        {
            poAttributeNode* attributeNode = static_cast<poAttributeNode*>(decl->child());
            decl = attributeNode;
            attributes = attributeNode->attributes();
        }

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
            int fieldType = getType(typeNode->token());
            int size = 0;
            if (fieldType == -1)
            {
                const std::string& typeName = typeNode->token().string();
                
                auto& typeData = _module.types()[_module.getTypeFromName(typeName)];
                fieldType = getPointerType(typeData.id(), pointerCount);
                size = getTypeSize(fieldType);
            }
            else
            {
                fieldType = getPointerType(fieldType, pointerCount);
                size = getTypeSize(fieldType);
            }

            // Align the offset to the size of the type
            const int alignment = _module.types()[fieldType].alignment();
            const int padding = align(offset, alignment);
            offset += padding;

            auto& type = _module.types()[id];
            type.addField(poField(attributes, offset, fieldType, 1, decl->token().string()));

            offset += size;
            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
        }
        else if (decl->child()->type() == poNodeType::ARRAY)
        {
            poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
            poNode* typeNode = arrayNode->child();
            int fieldType = getType(typeNode->token());
            int size = 0;
            if (fieldType == -1)
            {
                auto& typeData = _module.types()[_module.getTypeFromName(typeNode->token().string())];
                fieldType = getPointerType(typeData.id(), pointerCount);
                size = getTypeSize(fieldType);
            }
            else
            {
                fieldType = getPointerType(fieldType, pointerCount);
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
            type.addField(poField(poAttributes::PUBLIC, offset, arrayType, int(arrayNode->arraySize()), decl->token().string()));
            offset += totalSize;
            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
        }
        else if (decl->child()->type() == poNodeType::EXTERN)
        {
            poUnaryNode* externNode = static_cast<poUnaryNode*>(decl->child());
            poListNode* prototypeNode = static_cast<poListNode*>(externNode->child());

            // TODO: handle when the return type is the same as the class

            if (prototypeNode->type() == poNodeType::FUNCTION)
            {
                const int returnType = getReturnType(prototypeNode);

                poType type = _module.types()[id];
                const std::string& name = prototypeNode->token().string();
                const std::string fullName = ns.name() + "::" + type.name() + "::" + name;
                poMemberFunction method(attributes, returnType, name);
                const auto& it = _memberFunctions.find(fullName);
                if (it != _memberFunctions.end())
                {
                    method.setId(it->second);
                }

                std::vector<int> argTypes;
                getPrototypeArgs(prototypeNode, argTypes);

                {
                    poType& type = _module.types()[id];
                    for (const int arg : argTypes)
                    {
                        method.addArgument(arg);
                    }

                    type.addMethod(method);
                }
            }
            else if (prototypeNode->type() == poNodeType::CONSTRUCTOR)
            {
                auto& type = _module.types()[id];
                const std::string fullName = ns.name() + "::" + type.name() + "::" + type.name();
                poConstructor constructor(attributes);
                const auto& it = _memberFunctions.find(fullName);
                if (it != _memberFunctions.end())
                {
                    constructor.setId(it->second);
                }

                std::vector<int> argTypes;
                getPrototypeArgs(prototypeNode, argTypes);

                for (const int arg : argTypes)
                {
                    constructor.addArgument(arg);
                }

                type.addConstructor(constructor);
            }
        }
    }

    // Align the size of the type to the size of the largest field
    const int padding = align(offset, sizeOfLargestField);
    auto& type = _module.types()[id];
    type.setSize(offset + padding);
    type.setAlignment(sizeOfLargestField);

    // Add a default constructor (if required)
    if (type.constructors().size() == 0)
    {
        bool needsDefaultConstructor = false;
        for (poField field : type.fields())
        {
            // A default constructor is needed if any field has a constructor
            const poType& fieldType = _module.types()[field.type()];
            if (fieldType.constructors().size() > 0)
            {
                needsDefaultConstructor = true;
                break;
            }
            if (fieldType.isArray())
            {
                const poType& arrayBaseType = _module.types()[fieldType.baseType()];
                if (arrayBaseType.constructors().size() > 0)
                {
                    needsDefaultConstructor = true;
                    break;
                }
            }
        }

        if (needsDefaultConstructor)
        {
            poConstructor constructor(poAttributes::PUBLIC);
            constructor.setIsDefault(true);
            constructor.setId(int(_module.functions().size()));
            type.addConstructor(constructor);

            _module.addFunction(poFunction(
                type.name(),
                type.fullname() + "::" + type.name(),
                0,
                poAttributes::PUBLIC,
                poCallConvention::X86_64));
        }
    }
    _resolvedTypes.insert(std::pair<std::string, int>(name, id));
}

void poTypeResolver::getPrototypeArgs(poListNode* prototypeNode, std::vector<int>& argTypes)
{
    for (poNode* node : prototypeNode->list())
    {
        if (node->type() == poNodeType::ARGS)
        {
            poListNode* args = static_cast<poListNode*>(node);
            for (poNode* arg : args->list())
            {
                poUnaryNode* unary = static_cast<poUnaryNode*>(arg);
                assert(unary->type() == poNodeType::PARAMETER);

                poNode* type = unary->child();
                int pointerCount = 0;
                if (type->type() == poNodeType::POINTER)
                {
                    poPointerNode* pointerNode = static_cast<poPointerNode*>(type);
                    pointerCount = pointerNode->count();
                    type = pointerNode->child();
                }

                assert(type->type() == poNodeType::TYPE);

                int typeId = getType(type->token());
                if (typeId == -1)
                {
                    const auto& it = _resolvedTypes.find(type->token().string());
                    if (it != _resolvedTypes.end())
                    {
                        typeId = it->second;
                    }
                }

                const int variableType = getPointerType(typeId, pointerCount);
                argTypes.push_back(variableType);
            }
        }
    }
}

int poTypeResolver::getReturnType(poListNode* prototypeNode)
{
    int returnType = -1;
    for (poNode* node : prototypeNode->list())
    {
        if (node->type() == poNodeType::RETURN_TYPE)
        {
            poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(node);
            returnType = getType(returnTypeNode->token());
            if (returnType == -1)
            {
                const auto& it = _resolvedTypes.find(returnTypeNode->token().string());
                if (it != _resolvedTypes.end())
                {
                    returnType = it->second;
                }
            }
            if (returnTypeNode->child()->type() == poNodeType::POINTER)
            {
                poPointerNode* pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
                returnType = getPointerType(returnType, pointerNode->count());
            }
        }
    }
    return returnType;
}

void poTypeResolver::updateArgs(poNode* node)
{
    // Update the function arguments now that all types are resolved

    poNamespace& ns = _module.namespaces()[_namespaces[node]];

    assert(node->type() == poNodeType::NAMESPACE);
    poListNode* namespaceNode = static_cast<poListNode*>(node);

    for (poNode* child : namespaceNode->list())
    {
        poNode* funcNode = child;
        if (funcNode->type() == poNodeType::EXTERN)
        {
            funcNode = static_cast<poUnaryNode*>(funcNode)->child();
        }

        if (funcNode->type() != poNodeType::FUNCTION)
        {
            continue;
        }

        const std::string& name = child->token().string();

        poListNode* funNode = static_cast<poListNode*>(funcNode);
        for (poNode* funChildNode : funNode->list())
        {
            if (funChildNode->type() != poNodeType::ARGS)
            {
                continue;
            }

            poListNode* args = static_cast<poListNode*>(funChildNode);
            for (const int& funId : ns.functions())
            {
                poFunction& fun = _module.functions()[funId];
                if (fun.name() != name) { continue; }

                for (poNode* arg : args->list())
                {
                    poUnaryNode* unary = static_cast<poUnaryNode*>(arg);
                    assert(unary->type() == poNodeType::PARAMETER);
                    poNode* type = unary->child();

                    int pointerCount = 0;
                    if (type->type() == poNodeType::POINTER)
                    {
                        poPointerNode* pointerNode = static_cast<poPointerNode*>(type);
                        pointerCount = pointerNode->count();
                        type = pointerNode->child();
                    }

                    assert(type->type() == poNodeType::TYPE);
                    int typeId = getType(type->token());
                    if (typeId == -1)
                    {
                        const auto& it = _resolvedTypes.find(type->token().string());
                        if (it != _resolvedTypes.end())
                        {
                            typeId = it->second;
                        }
                    }
                    
                    const int variableType = getPointerType(typeId, pointerCount);
                    fun.addArgument(variableType);
                }
            }
        }
    }
}



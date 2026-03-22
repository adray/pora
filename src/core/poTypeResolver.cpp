#include "poTypeResolver.h"
#include "poModule.h"
#include "poType.h"
#include "poAST.h"
#include "poEval.h"
#include "poMorph.h"
#include "poUtil.h"

#include <assert.h>

using namespace po;

poTypeResolver::poTypeResolver(poModule& module)
    :
    _module(module),
    _isError(false),
    _errorLine(0),
    _errorFile(0)
{
}

void poTypeResolver::setError(const std::string& errorText, const poToken& token)
{
    _isError = true;
    _errorText = errorText;
    _errorLine = token.line();
    _errorFile = token.fileId();
}

int poTypeResolver::getTypeSize(const int type)
{
    return _module.types()[type].size();
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

    // Locate which generic specializations to generate
    poMorph morph(_module);
    morph.scan(nodes);
    if (morph.isError()) {
        _errorText = morph.errorText();
        _errorFile = morph.errorFile();
        _errorLine = morph.errorLine();
        _isError = true;
        return;
    }

    resolveTypes(morph);

    if (_unresolvedTypes.size() > 0)
    {
        // TODO: If the worklist still contains items
        // then we need to a raise an error for each of the items that
        // cannot be resolved and terminate the build
        poToken& token = _unresolvedTypes[0]->token();
        setError("Could not resolve type '" + token.string() + "'", token);
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
                int dataType = poUtil::getType(token);
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
                    setError("Unable to resolve constant for static variable: " + declNode->token().string(), declNode->token());
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
        else if (child->type() == poNodeType::ENUM)
        {
            getEnum(child, ns, id);
        }
        else if (child->type() == poNodeType::EXTERN)
        {
            getExtern(child, ns);
        }
    }
    _module.addNamespace(ns);

    _namespaces.insert(std::pair<poNode*, int>(node, id));
}

void poTypeResolver::addType(poNode* node, poNamespace& ns, const int nsId, const int baseType, const int kind)
{
    const std::string& name = node->token().string();
    const int id = int(_module.types().size());
    _module.addType(poType(id,
        baseType,
        name,
        ns.name() + "::" + name));
    ns.addType(id);
    _namespaceForTypes.insert(std::pair<poNode*, int>(node, nsId));
}

void poTypeResolver::getEnum(poNode* node, poNamespace& ns, const int nsId)
{
    assert(node->type() == poNodeType::ENUM);
    _userTypes.push_back(node);

    addType(node, ns, nsId, TYPE_ENUM, int(poTypeKind::Enum));
}

void poTypeResolver::getClass(poNode* node, poNamespace& ns, const int nsId)
{
    assert(node->type() == poNodeType::CLASS);
    _userTypes.push_back(node);

    addType(node, ns, nsId, TYPE_OBJECT, int(poTypeKind::Class));
}

void poTypeResolver::getStruct(poNode* node, poNamespace& ns, const int nsId)
{
    assert(node->type() == poNodeType::STRUCT);
    _userTypes.push_back(node);

    addType(node, ns, nsId, TYPE_OBJECT, int(poTypeKind::Struct));
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
    poGenericNode* genericNode = nullptr;
    for (poNode* child : functionNode->list())
    {
        if (child->type() == poNodeType::GENERIC_ARGS)
        {
            genericNode = static_cast<poGenericNode*>(child);
        }
    }

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
                    setError("Duplicate member function: " + fullname, node->token());
                    return;
                }
                _memberFunctions.insert(std::pair<std::string, int>(fullname, id));
            }
            else
            {
                ns.addFunction(id);
            }

            int attributes = int(poAttributes::PUBLIC);
            if (genericNode) {
                attributes |= int(poAttributes::GENERIC);
            }

            _module.addFunction(poFunction(
                node->token().string(),
                fullname,
                int(args->list().size()),
                (poAttributes)attributes,
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
    poGenericNode* genericNode = nullptr;
    for (poNode* child : constructorNode->list())
    {
        if (child->type() == poNodeType::GENERIC_ARGS)
        {
            genericNode = static_cast<poGenericNode*>(child);
        }
    }

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
                    setError("Duplicate constructor: " + fullname, node->token());
                    return;
                }
                _memberFunctions.insert(std::pair<std::string, int>(fullname, id));
            }
            else
            {
                ns.addFunction(id);
            }

            int attributes = int(poAttributes::PUBLIC);
            if (genericNode) {
                attributes |= int(poAttributes::GENERIC);
            }

            _module.addFunction(poFunction(
                node->token().string(),
                fullname,
                int(args->list().size()),
                poAttributes(attributes),
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
            newType.setKind(poTypeKind::Pointer);
            newType.setSize(8);
            newType.setAlignment(8);
            _module.addType(newType);
        }
        pointerType = type;

    }
    return pointerType;
}

bool poTypeResolver::isTypeResolved(poNode* node, bool isPointer, poListNode* parametricArgs)
{
    bool resolved = false;
    if (parametricArgs) {
        for (poNode* arg : parametricArgs->list())
        {
            if (arg->type() == poNodeType::CONSTRAINT) {
                arg = static_cast<poUnaryNode*>(arg)->child();
            }

            if (arg->token().string() == node->token().string())
            {
                resolved = true;
                break;
            }
        }
    }

    if (!resolved)
    {
        const int type = poUtil::getType(node->token());
        if (type != -1) 
        {
            resolved = true;
        }
        else /* is not a primitive or a pointer to a primitive type */
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
    }
    return resolved;
}

void poTypeResolver::resolveTypes(poMorph& morph)
{
    //
    // Loop over the unresolved types attempting to resolve them.
    // Types can be resolved when all the constituent types are known.
    //

    std::vector<poNode*> worklist;
    std::vector<poMorphNode*> morphWorklist;
    
    for (poNode* item : _userTypes)
    {
        worklist.push_back(item);
    }

    for (poMorphNode* item : morph.nodes())
    {
        morphWorklist.push_back(item);
    }

    bool changes = true;
    while (changes)
    {
        changes = false;

        std::vector<poNode*> work = worklist;
        worklist.clear();
        std::vector<poMorphNode*> morphWork = morphWorklist;
        morphWorklist.clear();

        changes = processWorklist(morph, work, worklist);
        changes |= processMorphWorklist(morph, morphWork, morphWorklist);
    }

    _unresolvedTypes = worklist;
}

bool poTypeResolver::processMorphWorklist(poMorph& morph, std::vector<poMorphNode*>& work, std::vector<poMorphNode*>& worklist)
{
    // Generate generic types specializations

    bool changes = false;

    for (poMorphNode* node : work) {

        bool resolved = true;
        for (poMorphParameter& parameter : node->parameters()) {
            bool isPointer = false;
            poNode* paramNode = parameter.node();
            if (paramNode->type() == poNodeType::POINTER) {
                poPointerNode* pointerNode = static_cast<poPointerNode*>(paramNode);
                paramNode = pointerNode->child();
                isPointer = true;
            }

            resolved &= isTypeResolved(paramNode, isPointer, nullptr);
        }

        poListNode* childNode = static_cast<poListNode*>( node->type()->node() );
        poListNode* genericArgs = nullptr;
        for (poNode* child : childNode->list()) {
            if (child->type() == poNodeType::GENERIC_ARGS) {
                genericArgs = static_cast<poListNode*>(child);
            }
        }

        for (poNode* child : childNode->list()) {
            if (child->type() == poNodeType::DECL) {
                poUnaryNode* decl = static_cast<poUnaryNode*>(child);
                poAttributeNode* attributes = nullptr;
                poNode* typeNode = decl->child();
                if (decl->child()->type() == poNodeType::ATTRIBUTES) {
                    attributes = static_cast<poAttributeNode*>(decl->child());
                    typeNode = static_cast<poUnaryNode*>( attributes->child() );
                }

                if (typeNode->type() == poNodeType::GENERIC) {
                    poGenericNode* genericNode = static_cast<poGenericNode*>(typeNode);
                    typeNode = genericNode->child();

                    std::string genericName = typeNode->token().string();
                    getGenericTypeName(genericName, genericNode, genericArgs, node);
                    resolved &= _resolvedTypes.find(genericName) != _resolvedTypes.end();
                }

                if (typeNode->type() == poNodeType::TYPE) {
                    resolved &= isTypeResolved(typeNode, false, genericArgs);
                }
            }
        }

        if (resolved) {
            for (poNode* astNode : _userTypes) {
                poListNode* typeNode = static_cast<poListNode*>(astNode);
                const std::string& name = typeNode->token().string();
                const int id = _module.getTypeFromName(name);
                changes |= generateSpecializations(node, typeNode, name, id);
            }
        }
        else {
            worklist.push_back(node);
        }
    }

    return changes;
}

void poTypeResolver::getGenericTypeName(std::string& genericName, poGenericNode* genericNode, poListNode* genericArgs, poMorphNode* node)
{
    genericName += "<";
    for (int i = 0; i < int(genericNode->nodes().size()); i++) {
        if (i > 0) {
            genericName += ",";
        }

        poNode* genericArg = genericNode->nodes()[i];
        std::string name = genericArg->token().string();
        int index = 0;
        for (poNode* parameter : genericArgs->list()) {
            if (parameter->type() == poNodeType::TYPE &&
                parameter->token().string() == name) {
                const int type = node->parameters()[index].type();
                name = _module.types()[type].name();
            }
            else if (parameter->type() == poNodeType::CONSTRAINT) {
                poUnaryNode* constraint = static_cast<poUnaryNode*>(parameter);
                poNode* child = constraint->child();
                if (child->type() == poNodeType::TYPE) {
                    const int type = node->parameters()[index].type();
                    name = _module.types()[type].name();
                }
            }
            index++;
        }

        genericName += name;
    }
    genericName += ">";
}

bool poTypeResolver::processWorklist(poMorph& morph, std::vector<poNode*>& work, std::vector<poNode*>& worklist)
{
    bool changes = false;

    for (poNode* node : work)
    {
        bool resolved = true;
        poListNode* typeNode = static_cast<poListNode*>(node);
        const std::string& name = typeNode->token().string();
        poListNode* parametricArgs = nullptr;
        for (poNode* child : typeNode->list())
        {
            if (child->type() == poNodeType::GENERIC_ARGS)
            {
                parametricArgs = static_cast<poListNode*>(child);
            }
        }

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

            if (decl->child()->type() == poNodeType::GENERIC)
            {
                poGenericNode* genericNode = static_cast<poGenericNode*>(decl->child());
                decl = genericNode;

                if (!parametricArgs) {
                    resolved &= isGenericResolved(genericNode);
                }
            }

            if (decl->child()->type() == poNodeType::ARRAY)
            {
                // An array type is in a good state if the base type is resolved.

                poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
                poNode* typeNode = arrayNode->child();

                resolved &= isTypeResolved(typeNode, isPointer, parametricArgs);
            }

            else if (decl->child()->type() == poNodeType::TYPE)
            {
                resolved &= isTypeResolved(decl->child(), isPointer, parametricArgs);
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
                            resolved &= isTypeResolved(argNode->child(), isPointer, parametricArgs);
                        }
                    }
                }
            }
            else if (decl->child()->type() == poNodeType::EXPRESSION)
            {
                poListNode* expressionNode = static_cast<poListNode*>(decl->child());
                for (poNode* node : expressionNode->list())
                {
                    if (node->type() == poNodeType::RETURN_TYPE)
                    {
                        poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(node);
                        if (returnTypeNode->child()->type() == poNodeType::POINTER)
                        {
                            poPointerNode* pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
                            returnTypeNode = pointerNode;
                            isPointer = true;
                        }

                        resolved &= isTypeResolved(returnTypeNode, isPointer, parametricArgs);
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

            // For non-generic types - obtain the poMorphType and checks its nodes (members) are all resolved
            const int id = _module.getTypeFromName(name);

            poMorphType* morphType = morph.getMorphType(id);
            if (morphType && parametricArgs == nullptr) {
                for (poMorphNode* node : morphType->nodes()) {
                    for (poMorphParameter& param : node->parameters()) {
                        resolved &= isTypeResolved(param.node(),
                            false,
                            nullptr);
                    }
                }
            }

            if (resolved) {
                poNamespace& ns = _module.namespaces()[namespaceIt->second];
                resolveType(name, ns, typeNode);

                changes = true;
            }
        }

        if (!resolved)
        {
            worklist.push_back(node);
        }
    }

    return changes;
}

bool po::poTypeResolver::isGenericResolved(poGenericNode* genericNode)
{
    bool resolved = true;
    std::string typeName = genericNode->token().string();
    typeName += "<";

    for (int i = 0; i < int(genericNode->nodes().size()); i++) {
        if (i > 0) {
            typeName += ",";
        }

        const int nodeType = poUtil::unpackTypeNode(_module, genericNode->nodes()[i]);
        if (nodeType == -1) {
            resolved = false;
            continue;
        }

        typeName += _module.types()[nodeType].name();
    }
    typeName += ">";

    if (resolved) {
        resolved &= _resolvedTypes.find(typeName) != _resolvedTypes.end();
    }
    return resolved;
}

void poTypeResolver::resolveType(const std::string& name, poNamespace& ns, poListNode* typeNode)
{
    poEvaluator evaluator;

    const int id = _module.getTypeFromName(name);
    int offset = 0;
    int sizeOfLargestField = 0;
    poListNode* parametricArgs = nullptr;
    for (poNode* child : typeNode->list())
    {
        if (child->type() == poNodeType::GENERIC_ARGS) {
            parametricArgs = static_cast<poListNode*>(child);
            break;
        }
    }

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

        poGenericNode* genericArgs = nullptr;
        if (decl->child()->type() == poNodeType::GENERIC) {
            genericArgs = static_cast<poGenericNode*>(decl->child());
            decl = genericArgs;
        }

        if (decl->child()->type() == poNodeType::TYPE)
        {
            poNode* typeNode = decl->child();
            int fieldType = poUtil::getType(typeNode->token());
            int size = 0;
            if (fieldType == -1)
            {
                std::string typeName = typeNode->token().string();

                // Check the specialized generic type
                if (genericArgs && !parametricArgs) {
                    getGenericTypeName(typeName, genericArgs);
                }

                const int type = _module.getTypeFromName(typeName);
                if (type != -1) {
                    auto& typeData = _module.types()[type];
                    fieldType = getPointerType(typeData.id(), pointerCount);
                    size = getTypeSize(fieldType);
                }
                else {
                    fieldType = getParametricType(typeName, parametricArgs);
                    if (fieldType == -1) {
                        setError("Unable to find type '" + typeName + "'", typeNode->token());
                        return;
                    }

                    auto& typeData = _module.types()[fieldType];
                    fieldType = getPointerType(typeData.id(), pointerCount);
                }
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
            poField field(attributes, offset, fieldType, 1, child->token().string());
            if (genericArgs) {
                for (poNode* genericNode : genericArgs->nodes()) {
                    field.addParametricArg(genericNode->token().string());
                }
            }

            type.addField(field);

            offset += size;
            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
        }
        else if (decl->child()->type() == poNodeType::ENUM_VALUE)
        {
            attributes = (poAttributes)(int(attributes) | int(poAttributes::STATIC) | int(poAttributes::CONST));

            auto& type = _module.types()[id];
            int64_t enumValue = int(type.fields().size());

            poUnaryNode* enumValueNode = static_cast<poUnaryNode*>(decl->child());
            if (enumValueNode->child() != nullptr)
            {
                enumValue = evaluator.evaluateI64(enumValueNode->child());
                if (evaluator.isError())
                {
                    setError("Unable to evaluate enum value for '" + decl->token().string() + "'", enumValueNode->token());
                    return;
                }
            }

            int enumConstantId = _module.constants().getConstant(int(enumValue));
            if (enumConstantId == -1)
            {
                enumConstantId = _module.constants().addConstant(int(enumValue));
            }

            const int fieldType = TYPE_I32;
            type.addField(poField(attributes, offset, id, 1, child->token().string(), enumConstantId));
            const int size = _module.types()[fieldType].size();
            sizeOfLargestField = size;
            offset += size;
        }
        else if (decl->child()->type() == poNodeType::ARRAY)
        {
            poArrayNode* arrayNode = static_cast<poArrayNode*>(decl->child());
            poNode* typeNode = arrayNode->child();
            int fieldType = poUtil::getType(typeNode->token());
            int size = 0;
            if (fieldType == -1)
            {
                const std::string& typeName = typeNode->token().string();
                const int type = _module.getTypeFromName(typeName);
                if (type != -1) {
                    auto& typeData = _module.types()[type];
                    fieldType = getPointerType(typeData.id(), pointerCount);
                    size = getTypeSize(fieldType);
                }
                else {
                    fieldType = getParametricType(typeName, parametricArgs);
                    if (fieldType == -1) {
                        setError("Unable to find type '" + typeName + "'", typeNode->token());
                        return;
                    }
                }
            }
            else
            {
                fieldType = getPointerType(fieldType, pointerCount);
                size = getTypeSize(fieldType);
            }

            const int arrayType = getArrayType(fieldType);

            const int alignment = /*size **/ _module.types()[fieldType].alignment();
            const int totalSize = size * int(arrayNode->arraySize());
            const int padding = align(offset, alignment);
            offset += padding;

            auto& type = _module.types()[id];
            type.addField(poField(poAttributes::PUBLIC, offset, arrayType, int(arrayNode->arraySize()), child->token().string()));
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
                const int returnType = getReturnType(prototypeNode, parametricArgs);

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
                getPrototypeArgs(prototypeNode, parametricArgs, argTypes);

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
                poConstructor constructor(attributes, type.name());
                const auto& it = _memberFunctions.find(fullName);
                if (it != _memberFunctions.end())
                {
                    constructor.setId(it->second);
                }

                std::vector<int> argTypes;
                getPrototypeArgs(prototypeNode, parametricArgs, argTypes);

                for (const int arg : argTypes)
                {
                    constructor.addArgument(arg);
                }

                type.addConstructor(constructor);
            }
        }
        else if (decl->child()->type() == poNodeType::EXPRESSION)
        {
            poListNode* expressionNode = static_cast<poListNode*>(decl->child());
            poUnaryNode* returnTypeNode = nullptr;
            poUnaryNode* variableNode = nullptr;
            for (poNode* node : expressionNode->list())
            {
                if (node->type() == poNodeType::RETURN_TYPE)
                {
                    returnTypeNode = static_cast<poUnaryNode*>(node);
                }
                else if (node->type() == poNodeType::VARIABLE)
                {
                    variableNode = static_cast<poUnaryNode*>(node);
                }
            }

            if (returnTypeNode && variableNode)
            {
                poPointerNode* pointerNode = nullptr;
                if (returnTypeNode->child()->type() == poNodeType::POINTER)
                {
                    pointerNode = static_cast<poPointerNode*>(returnTypeNode->child());
                    returnTypeNode = pointerNode;
                }

                int returnType = poUtil::getType(returnTypeNode->token());
                if (returnType == -1)
                {
                    const std::string& typeName = returnTypeNode->token().string();
                    int typeId = getParametricType(typeName, parametricArgs);
                    if (typeId == -1) {
                        typeId = _module.getTypeFromName(typeName);
                        if (typeId == -1) {
                            setError("Return type could not be found.", returnTypeNode->token());
                            return;
                        }
                    }
                    auto& typeData = _module.types()[typeId];
                    returnType = typeData.id();
                }
                returnType = getPointerType(returnType, pointerNode->count());
                poType& type = _module.types()[id];
                poMemberProperty memberProperty(attributes, returnType, expressionNode->token().string(), variableNode->token().string());
                type.addProperty(memberProperty);
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
            poConstructor constructor(poAttributes::PUBLIC, type.name());
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

    if (parametricArgs) {
        type.setGeneric(true);
        for (poNode* node : parametricArgs->list())
        {
            poNode* arg = node;
            int traitId = 0;
            if (node->type() == poNodeType::CONSTRAINT) {
                if (node->token().token() == poTokenType::NEW) {
                    traitId = TYPE_TRAIT_NEW;
                } else {
                    const std::string constraint = node->token().string();
                    traitId = _module.getTypeFromName(constraint);
                }
                arg = static_cast<poUnaryNode*>(node)->child();
            }

            type.addParametricArg(arg->token().string(), traitId);
        }
    }

    _resolvedTypes.insert(std::pair<std::string, int>(name, id));
}

int poTypeResolver::getArrayType(int fieldType)
{
    int arrayType = _module.getArrayType(fieldType);
    if (arrayType == -1)
    {
        arrayType = int(_module.types().size());
        std::string baseName = _module.types()[fieldType].name();
        poType type(arrayType, fieldType, baseName + "[]");
        type.setKind(poTypeKind::Array);
        _module.addType(type);
    }
    return arrayType;
}

void poTypeResolver::getGenericTypeName(std::string& typeName, poGenericNode* genericArgs)
{
    typeName += "<";
    for (int i = 0; i < int(genericArgs->nodes().size()); i++) {
        if (i > 0) {
            typeName += ",";
        }

        const int type = poUtil::unpackTypeNode(_module, genericArgs->nodes()[i]);
        typeName += _module.types()[type].name();
    }
    typeName += ">";
}

bool poTypeResolver::generateSpecializations(poMorphNode* node, poListNode* typeNode, const std::string& name, const int typeId)
{
    bool changes = false;

    poMorphType* morphType = node->type();
    if (morphType->node() == typeNode) {
        std::string newName = name;
        getGenericTypeName(newName, node);

        auto const nsIt = _namespaceForTypes.find(typeNode);
        if (nsIt == _namespaceForTypes.end()) {
            return false;
        }

        const int numParameters = int(node->parameters().size());

        poListNode* genericParameters = nullptr;
        for (poNode* node : typeNode->list()) {
            if (node->type() == poNodeType::GENERIC_ARGS) {
                genericParameters = static_cast<poListNode*>(node);
            }
        }

        const std::vector<poParametricArgument>& parametricArgs = _module.types()[typeId].parametricArgs();
        for (int i = 0; i < int(parametricArgs.size()); i++) {
            const poParametricArgument& arg = parametricArgs[i];
            bool ok = true;
            if (arg.traitId() == TYPE_TRAIT_NEW) {
                // Check the type argument satisfies the NEW (pseudo) trait
                const int paramType = node->parameters()[i].type();
                const poType& paramTypeData = _module.types()[paramType];
                ok = paramTypeData.isPointer() ||
                    paramTypeData.isPrimitive();

                ok |= paramTypeData.baseType() == TYPE_OBJECT && paramTypeData.constructors().size() == 0;

                if (!ok && paramTypeData.constructors().size() > 0)
                {
                    for (const poConstructor& constructor : paramTypeData.constructors())
                    {
                        ok = int(constructor.arguments().size()) == 0;
                        if (ok)
                        {
                            break;
                        }
                    }
                }
            }
            else if (arg.traitId() > 0) {
                ok = false;
            }

            if (!ok) {
                setError("Type does not satisfy constraint of '" + arg.identifier() + "'.", typeNode->token());
            }
        }

        const int newTypeId = int(_module.types().size());
        poType genericType(newTypeId, TYPE_OBJECT, newName);
        _module.addType(genericType);
        _module.namespaces()[nsIt->second].addType(genericType.id());
        _resolvedTypes.insert(std::pair<std::string, int>(newName, genericType.id()));

        const poTypeKind kind = _module.types()[typeId].kind();

        // Take a copy in case a new type is inserted during the loop
        std::vector<poField> fields = _module.types()[typeId].fields();

        int offset = 0;
        int sizeOfLargestField = 0;
        for (const poField& field : fields) {
            int fieldType = field.type();

            bool isArray = false;
            if (_module.types()[fieldType].isArray()) {
                const poType& arrayType = _module.types()[fieldType];
                fieldType = arrayType.baseType();
                isArray = true;
            }

            int pointerCount = 0;
            while (_module.types()[fieldType].isPointer()) {
                pointerCount++;
                fieldType = _module.types()[fieldType].baseType();
            }

            if (fieldType >= TYPE_PARAMETRIC_1 &&
                fieldType <= TYPE_PARAMETRIC_4) {
                fieldType = node->parameters()[fieldType - TYPE_PARAMETRIC_1].type();
            }

            poType& typeOfField = _module.types()[fieldType];
            if (typeOfField.isGeneric()) {
                // Look up the specialized type
                std::string specializedName = typeOfField.name() + "<";
                for (int i = 0; i < int(field.parametricArgs().size()); i++) {
                    if (i > 0)
                    {
                        specializedName += ",";
                    }
                    for (int j = 0; j < numParameters; j++) {
                        poNode* argNode = genericParameters->list()[j];
                        if (argNode->type() == poNodeType::CONSTRAINT) {
                            argNode = static_cast<poUnaryNode*>(argNode)->child();
                        }

                        const std::string& arg = argNode->token().string();
                        if (arg == field.parametricArgs()[i]) {
                            const int parameterType = node->parameters()[j].type();
                            specializedName += _module.types()[parameterType].name();
                        }
                    }
                }
                specializedName += ">";
                fieldType = _module.getTypeFromName(specializedName);
                if (fieldType == -1) {
                    setError("Unable to find '" + specializedName + "'", typeNode->token());
                    return false;
                }
            }

            fieldType = getPointerType(fieldType, pointerCount);

            int size = getTypeSize(fieldType);
            if (isArray) {
                fieldType = getArrayType(fieldType);
                size *= field.numElements();
            }

            // Align the offset to the size of the type
            const int alignment = _module.types()[fieldType].alignment();
            const int padding = align(offset, alignment);
            offset += padding;

            _module.types()[newTypeId].addField(poField(field.attributes(),
                offset,
                fieldType,
                field.numElements(),
                field.name()));

            offset += size;
            sizeOfLargestField = std::max(sizeOfLargestField, alignment);
        }

        poType& newType = _module.types()[newTypeId];

        // Align the size of the type to the size of the largest field
        const int padding = align(offset, sizeOfLargestField);
        newType.setSize(offset + padding);
        newType.setAlignment(sizeOfLargestField);
        newType.setKind(kind);

        changes = true;
    }

    return changes;
}

void po::poTypeResolver::getGenericTypeName(std::string& newName, po::poMorphNode* node)
{
    const int numParameters = int(node->parameters().size());
    newName += "<";
    for (int i = 0; i < numParameters; i++) {
        if (i > 0) {
            newName += ",";
        }

        const poMorphParameter& parameter = node->parameters()[i];
        newName += _module.types()[parameter.type()].name();
    }
    newName += ">";
}

int poTypeResolver::getParametricType(const std::string& name, poListNode* parametricArgs)
{
    if (parametricArgs) {
        for (int i = 0; i < parametricArgs->list().size(); i++) {
            poNode* arg = parametricArgs->list()[i];
            if (arg->type() == poNodeType::CONSTRAINT) {
                arg = static_cast<poUnaryNode*>(arg)->child();
            }

            if (arg->token().string() == name) {
                return TYPE_PARAMETRIC_1 + i;
            }
        }
    }

    return -1;
}

void poTypeResolver::getPrototypeArgs(poListNode* prototypeNode, poListNode* parametricArgs, std::vector<int>& argTypes)
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

                int typeId = getParametricType(type->token().string(), parametricArgs);
                if (typeId == -1) {
                    typeId = poUtil::getType(type->token());
                    if (typeId == -1)
                    {
                        const auto& it = _resolvedTypes.find(type->token().string());
                        if (it != _resolvedTypes.end())
                        {
                            typeId = it->second;
                        }
                    }
                }

                const int variableType = getPointerType(typeId, pointerCount);
                argTypes.push_back(variableType);
            }
        }
    }
}

int poTypeResolver::getReturnType(poListNode* prototypeNode, poListNode* parametricArgs)
{
    int returnType = -1;
    for (poNode* node : prototypeNode->list())
    {
        if (node->type() == poNodeType::RETURN_TYPE)
        {
            poUnaryNode* returnTypeNode = static_cast<poUnaryNode*>(node);

            if (parametricArgs) {
                for (int i = 0; i < int(parametricArgs->list().size()); i++) {
                    poToken& token = parametricArgs->list()[i]->token();
                    if (token.string() == returnTypeNode->token().string()) {
                        returnType = TYPE_PARAMETRIC_1 + i;
                        break;
                    }
                }
            }

            if (returnType == -1) {
                returnType = poUtil::getType(returnTypeNode->token());
            }

            if (returnType == -1)
            {
                const auto& it = _resolvedTypes.find(returnTypeNode->token().string());
                if (it != _resolvedTypes.end())
                {
                    returnType = it->second;
                }
            }
            if (returnType != -1 && returnTypeNode->child()->type() == poNodeType::POINTER)
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
                    int typeId = poUtil::getType(type->token());
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



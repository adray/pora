#include "poMorph.h"
#include "poModule.h"
#include "poUtil.h"
#include "poCFG.h"
#include "poCycle.h"

#include <assert.h>
#include <iostream>

using namespace po;


//
// poMorphCache
//

void poMorphCache:: add(poMorphNode* node)
{
    const int type = node->type()->type();
    const auto& parameters = node->parameters();
    if (parameters.size() == 0) {
        return;
    }

    int hashCode = parameters[0].type();
    for (int i = 1; i < int(parameters.size()); i++) {
        const poMorphParameter& param = parameters[i];
        hashCode = (~hashCode << 3) ^ param.type();
    }

    const auto& it = _items.find(hashCode);
    if (it != _items.end()) {
        it->second.push_back(node);
    }
    else {
        std::vector<poMorphNode*> nodes;
        nodes.push_back(node);
        _items.insert(std::pair<int, std::vector<poMorphNode*>>(hashCode, nodes));
    }
}

poMorphNode* poMorphCache::find(const int type, const std::vector<int>& parameters)
{
    poMorphNode* node = nullptr;
    if (parameters.size() == 0) {
        return node;
    }

    int hashCode = parameters[0];
    for (int i = 1; i < int(parameters.size()); i++) {
        hashCode = (~hashCode << 3) ^ parameters[i];
    }

    const auto& it = _items.find(hashCode);
    if (it != _items.end()) {
        for (poMorphNode* morphNode : it->second) {
            if (morphNode->parameters().size() != parameters.size()) {
                continue;
            }

            if (type != morphNode->type()->type()) {
                continue;
            }

            bool ok = true;
            for (int i = 0; i < int(morphNode->parameters().size()); i++) {
                const poMorphParameter& param = morphNode->parameters()[i];
                const poMorphParameter& morphParam = morphNode->parameters()[i];

                ok &= param.type() == morphParam.type() &&
                    param.isTypeName() == morphParam.isTypeName() &&
                    param.index() == morphParam.index();
                if (ok) { break; }
            }

            if (ok) {
                node = morphNode;
                break;
            }
        }
    }

    return node;
}

//
// poMorph
//

poMorphType* poMorph:: getMorphType(const int type) {
    const auto& it = _typeMap.find(type);
    if (it != _typeMap.end()) {
        return it->second;
    }
    return nullptr;
}

void poMorph::dumpNode(poMorphNode* node) {
    poMorphType* type = node->type();
    for (poMorphParameter& param : node->parameters()) {
        std::cout << param.type();
        if (param.isTypeName()) {
            std::cout << ":" << param.index();
        }
        std::cout << " ";
    }
    std::cout << "-> " << type->type() << std::endl;
}

void poMorph::dumpType(poMorphType* type) {

    std::cout << type->type() << " <" << type->numParameters() << ">" << std::endl;
    for (poMorphNode* next : type->nodes()) {
            std::cout << "    ";
        for (poMorphParameter& param : next->parameters()) {
            if (param.isTypeName()) {
                std::cout << param.type() << ":" << param.index() << " ";
            }
            else {
                std::cout << param.type() << " ";
            }
        }
        std::cout << std::endl;
    }
    
}

void poMorph::dump() {
    std::cout << "===============" << std::endl;
    std::cout << "Morphs" << std::endl;
    std::cout << "===============" << std::endl;

    for (poMorphType* type : _types) {
        dumpType(type);
    }

    for (poMorphNode* node : _nodes) {
        dumpNode(node);
    }

    std::cout << "===============" << std::endl;
}

void poMorph::gatherTypesFromNamespace(poNode* child, std::vector<poNode*>& userTypes)
{
    if (child->type() == poNodeType::NAMESPACE) {
        poListNode* ns = static_cast<poListNode*>(child);
        for (poNode* child : ns->list()) {
            switch (child->type()) {
            case poNodeType::CLASS:
            case poNodeType::STRUCT:
            case poNodeType::ENUM:
                userTypes.push_back(child);
                break;
            }
        }
    }
}

void poMorph:: gatherTypes(const std::vector<poNode*>& nodes, std::vector<poNode*>& userTypes) {
    for (poNode* node : nodes) {
        if (node->type() != poNodeType::MODULE) {
            continue;
        }

        poListNode* mod = static_cast<poListNode*>(node);
        for (poNode* child : mod->list()) {
            gatherTypesFromNamespace(child, userTypes);
        }
    }
}

void poMorph:: scan(const std::vector<poNode*>& nodes) {
    std::vector<poNode*> userTypes;
    gatherTypes(nodes, userTypes);

    for (int i = 0; i < int(userTypes.size()); i++) {
        poNode* node = userTypes[i];
        _map[node->token().string()] = i;
    }

    buildGraph(nodes, userTypes);

    if (_isError) {
        return;
    }
    
    generateMorphs();
    //dump();
}

void poMorph::morph(const std::vector<poNode*>& nodes)
{
    std::vector<poNode*> userTypes;
    gatherTypes(nodes, userTypes);

    for (int i = 0; i < int(userTypes.size()); i++) {
        poNode* node = userTypes[i];
        _map[node->token().string()] = i;
    }

    buildGraph(nodes, userTypes);

    if (_isError) {
        return;
    }


    generateMorphs();
    checkConstraints();
    morphASTs(nodes, userTypes);
}

void poMorph::morphASTs(const std::vector<poNode*>& nodes, std::vector<poNode*>& userTypes) {

    // Here, we clone the ASTs to create specialized functions.

    for (poMorphNode* type : _nodes) {

        std::string baseName = type->type()->node()->token().string();
        const int baseType = _module.getTypeFromName(baseName);

        std::string name = baseName;
        getGenericType(name, type);

        const int typeId = _module.getTypeFromName(name);
        poType& newType = _module.types()[typeId];

        const std::vector<poMemberProperty>& properties = _module.types()[baseType].properties();

        for (const poMemberProperty& property : properties) {
            int propertyType = property.type();
            if (propertyType >= TYPE_PARAMETRIC_1 &&
                propertyType <= TYPE_PARAMETRIC_4) {
                propertyType = type->parameters()[propertyType - TYPE_PARAMETRIC_1].type();
            }

            newType.addProperty(poMemberProperty(
                property.attributes(),
                propertyType,
                property.name(),
                property.backingFieldName()
            ));
        }
    }

    for (poNode* node : nodes) {
        if (node->type() != poNodeType::MODULE) {
            continue;
        }

        poListNode* moduleNode = static_cast<poListNode*>(node);
        for (poNode* child : moduleNode->list()) {
            if (child->type() != poNodeType::NAMESPACE) {
                continue;
            }

            std::vector<poNode*> newNodes;
            poListNode* ns = static_cast<poListNode*>(child);
            for (poNode* funcNode : ns->list()) {
                if (funcNode->type() == poNodeType::FUNCTION) {
                    substituteFunctions(ns->token().string(), newNodes, funcNode, userTypes);
                }
                else if (funcNode->type() == poNodeType::CONSTRUCTOR) {
                    substituteConstructor(ns->token().string(), newNodes, funcNode, userTypes);
                }
            }

            for (poNode* newNode : newNodes) {
                ns->list().push_back(newNode);
            }
        }
    }
}

void poMorph:: substituteConstructor(const std::string& namespaceName, std::vector<poNode*>& ns, poNode* node, std::vector<poNode*>& userTypes) {
    poListNode* func = static_cast<poListNode*>(node);
    poResolverNode* resolver = nullptr;
    poListNode* body = nullptr;
    poListNode* args = nullptr;
    poListNode* generic = nullptr;
    for (poNode* child : func->list()) {
        if (child->type() == poNodeType::RESOLVER) {
            resolver = static_cast<poResolverNode*>(child);
        }
        else if (child->type() == poNodeType::GENERIC_ARGS) {
            generic = static_cast<poListNode*>(child);
        }
    }

    // Only substitute generic constructors
    if (generic == nullptr) {
        return;
    }

    assert(resolver);

    if (resolver->path().size() == 0) {
        return;
    }

    const std::string path = resolver->path()[0];

    const auto& it = _map.find(path);
    if (it == _map.end()) {
        return;
    }

    const int typeId = it->second + TYPE_OBJECT + 1;
    const auto& it2 = _typeMap.find(typeId);
    if (it2 == _typeMap.end()) {
        return;
    }

    poMorphType* type = it2->second;

    // Create a clone for each morph node

    for (poMorphNode* morph : _nodes) {

        const int morphType = morph->type()->type();
        if (morphType != typeId) {
            continue;
        }

        poListNode* clone = static_cast<poListNode*>(node->clone());
        for (poNode* child : clone->list()) {
            if (child->type() == poNodeType::RESOLVER) {
                resolver = static_cast<poResolverNode*>(child);
            }
            else if (child->type() == poNodeType::BODY) {
                body = static_cast<poListNode*>(child);
            }
            else if (child->type() == poNodeType::ARGS) {
                args = static_cast<poListNode*>(child);
            }
            else if (child->type() == poNodeType::GENERIC_ARGS) {
                generic = static_cast<poListNode*>(child);
            }
        }

        assert(args);
        assert(body);

        clone->list().clear();

        std::string name = resolver->path()[0];
        getGenericType(name, morph);

        std::vector<poNode*> argList;
        substituteArgs(args, generic, morph, argList);

        std::vector<poNode*> nodes;
        substituteBody(body, generic, morph, nodes);

        std::vector<std::string> path{ name };
        clone->list().push_back(new poResolverNode(resolver->token(), path));
        clone->list().push_back(generic);
        clone->list().push_back(new poListNode(poNodeType::ARGS, argList, args->token()));
        clone->list().push_back(new poListNode(poNodeType::BODY, nodes, body->token()));

        delete resolver;
        delete args;
        delete body;

        ns.push_back(clone);

        std::string fullName = namespaceName + "::" + name + "::" + type->node()->token().string();

        const int functionId = int(_module.functions().size());
        _module.addFunction(poFunction(
            name,
            fullName,
            int(argList.size()),
            poAttributes::PUBLIC,
            poCallConvention::X86_64
        ));

        for (poNamespace& ns : _module.namespaces()) {
            if (ns.name() == namespaceName) {
                ns.addFunction(functionId);
                break;
            }
        }

        poType& genericType = _module.types()[_module.getTypeFromName(name)];
        poConstructor constructor(
            poAttributes::PUBLIC,
            type->node()->token().string()
        );
        constructor.setId(functionId);
        genericType.addConstructor(constructor);
    }
}

void poMorph:: substituteFunctions(const std::string& namespaceName, std::vector<poNode*>& ns, poNode* node, std::vector<poNode*>& userTypes) {

    poListNode* func = static_cast<poListNode*>(node);
    poResolverNode* resolver = nullptr;
    poListNode* body = nullptr;
    poListNode* args = nullptr;
    poNode* returnType = nullptr;
    poListNode* generic = nullptr;
    for (poNode* child : func->list()) {
        if (child->type() == poNodeType::RESOLVER) {
            resolver = static_cast<poResolverNode*>(child);
        }
        else if (child->type() == poNodeType::GENERIC_ARGS) {
            generic = static_cast<poListNode*>(child);
        }
    }

    // Only substitute generic functions
    if (generic == nullptr) {
        return;
    }

    assert(resolver);

    if (resolver->path().size() == 0) {
        return;
    }

    const std::string path = resolver->path()[0];

    const auto& it = _map.find(path);
    if (it == _map.end()) {
        return;
    }

    const int typeId = it->second + TYPE_OBJECT + 1;
    const auto& it2 = _typeMap.find(typeId);
    if (it2 == _typeMap.end()) {
        return;
    }

    poMorphType* type = it2->second;

    // Create a clone for each morph node

    for (poMorphNode* morph : _nodes) {

        const int morphType = morph->type()->type();
        if (morphType != typeId) {
            continue;
        }

        poListNode* clone = static_cast<poListNode*>(node->clone());
        for (poNode* child : clone->list()) {
            if (child->type() == poNodeType::RESOLVER) {
                resolver = static_cast<poResolverNode*>(child);
            }
            else if (child->type() == poNodeType::BODY) {
                body = static_cast<poListNode*>(child);
            }
            else if (child->type() == poNodeType::ARGS) {
                args = static_cast<poListNode*>(child);
            }
            else if (child->type() == poNodeType::GENERIC_ARGS) {
                generic = static_cast<poListNode*>(child);
            }
            else if (child->type() == poNodeType::RETURN_TYPE) {
                returnType = child;
            }
        }

        assert(args);
        assert(body);

        clone->list().clear();

        std::string name = resolver->path()[0];
        getGenericType(name, morph);

        std::vector<poNode*> argList;
        substituteArgs(args, generic, morph, argList);

        std::vector<poNode*> nodes;
        substituteBody(body, generic, morph, nodes);

        poUnaryNode* morphedReturnType = static_cast<poUnaryNode*>(substitute(returnType, generic, morph));

        std::vector<std::string> path{ name };
        clone->list().push_back(new poResolverNode(resolver->token(), path));
        clone->list().push_back(generic);
        clone->list().push_back(new poListNode(poNodeType::ARGS, argList, args->token()));
        clone->list().push_back(new poListNode(poNodeType::BODY, nodes, body->token()));
        clone->list().push_back(morphedReturnType);
        
        delete resolver;
        delete args;
        delete body;
        delete returnType;

        ns.push_back(clone);

        std::string fullName = namespaceName + "::" + name + "::" + node->token().string();

        const int functionId = int(_module.functions().size());
        _module.addFunction(poFunction(
            node->token().string(),
            fullName,
            int(argList.size()),
            poAttributes::PUBLIC,
            poCallConvention::X86_64
        ));

        for (poNamespace& ns : _module.namespaces()) {
            if (ns.name() == namespaceName) {
                ns.addFunction(functionId);
                break;
            }
        }

        int retType = poUtil::getType(morphedReturnType->token());
        if (morphedReturnType->child()->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(morphedReturnType->child());
            for (int i = 0; i < pointerNode->count(); i++) {
                retType = _module.getPointerType(retType);
            }
        }

        poType& genericType = _module.types()[_module.getTypeFromName(name)];
        poMemberFunction function(
            poAttributes::PUBLIC,
            retType,
            node->token().string()
        );
        function.setId(functionId);
        genericType.addMethod(function);
    }
}

void po::poMorph::substituteBody(poListNode* body, poListNode* generic, poMorphNode* morph, std::vector<poNode*>& nodes)
{
    for (poNode* child : body->list()) {
        if (child->type() == poNodeType::STATEMENT) {
            nodes.push_back(substituteStatement(child, generic, morph));
        }
        else {
            nodes.push_back(child->clone());
        }
    }
}

poNode* poMorph::substituteStatement(poNode* statement, poListNode* generic, poMorphNode* morph)
{
    poNode* clone = nullptr;
    poUnaryNode* node = static_cast<poUnaryNode*>(statement);
    switch (node->child()->type())
    {
    case poNodeType::ASSIGNMENT:
        clone = substituteAssignment(node->child(), generic, morph);
        break;
    case poNodeType::DECL:
        clone = substituteDecl(node->child(), generic, morph);
        break;
    default:
        clone = node->child()->clone();
        break;
    }

    return new poUnaryNode(
        poNodeType::STATEMENT,
        clone,
        node->token());
}

poNode* poMorph::substituteDecl(poNode* decl, poListNode* generic, poMorphNode* morph)
{
    poUnaryNode* node = static_cast<poUnaryNode*>(decl);
    poNode* clone = nullptr;
    switch (node->child()->type()) {
    case poNodeType::ASSIGNMENT:
        clone = substituteAssignment(node->child(), generic, morph);
        break;
    default:
        clone = substitute(node, generic, morph);
        break;
    }

    poNode* param = match(decl->token(), generic, morph);

    if (param) {
        poNode* paramType = param;
        if (paramType->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(paramType);
            paramType = pointerNode->child();
        }

        return
            new poUnaryNode(poNodeType::DECL, clone,
                poToken(paramType->token().token(),
                    paramType->token().string(),
                    decl->token().line(),
                    decl->token().column(),
                    decl->token().fileId()));
    }
    else {
        return new poUnaryNode(
            poNodeType::DECL,
            clone,
            decl->token());
    }
}

poNode* poMorph:: substituteAssignment(poNode* assignment, poListNode* generic, poMorphNode* morph)
{
    poBinaryNode* node = static_cast<poBinaryNode*>(assignment);
    poNode* left = substitute(node->left(), generic, morph);
    poNode* right = substitute(node->right(), generic, morph);
    poNode* clone = new poBinaryNode(
        poNodeType::ASSIGNMENT,
        left,
        right,
        node->token());
    return clone;
}

poNode* poMorph:: substituteCast(poNode* cast, poListNode* generic, poMorphNode* morph)
{
    poBinaryNode* node = static_cast<poBinaryNode*>(cast);
    poNode* cloneLeft = substitute(node->left(), generic, morph);
    poNode* cloneRight = substitute(node->right(), generic, morph);

    poNode* param = match(cast->token(), generic, morph);

    if (param) {
        poNode* paramType = param;
        if (paramType->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(paramType);
            paramType = pointerNode->child();
        }

        return new poBinaryNode(poNodeType::CAST, cloneLeft, cloneRight,
            poToken(paramType->token().token(),
            paramType->token().string(),
            cast->token().line(),
            cast->token().column(),
            cast->token().fileId()));
    }

    return new poBinaryNode(poNodeType::CAST, cloneLeft, cloneRight, cast->token());
}

poNode* poMorph:: substituteType(poNode* type, poListNode* generic, poMorphNode* morph)
{
    poNode* node = static_cast<poNode*>(type);

    poNode* param = match(type->token(), generic, morph);
    if (param)
    {
        poNode* paramType = param;
        poPointerNode* pointerNode = nullptr;
        if (paramType->type() == poNodeType::POINTER) {
            pointerNode = static_cast<poPointerNode*>(paramType);
            paramType = pointerNode->child();
        }

        poNode* type =
            new poNode(poNodeType::TYPE, 
                poToken(paramType->token().token(),
                    paramType->token().string(),
                    node->token().line(),
                    node->token().column(),
                    node->token().fileId()
                ));
        if (pointerNode) {
            type = new poPointerNode(poNodeType::POINTER, type, type->token(), pointerNode->count());
        }
        return type;
    }

    return new poNode(poNodeType::TYPE, type->token());
}

poNode* poMorph:: substituteReturnType(poNode* type, poListNode* generic, poMorphNode* morph)
{
    poUnaryNode* node = static_cast<poUnaryNode*>(type);

    poNode* clone = substitute(node->child(), generic, morph);

    poNode* param = match(type->token(), generic, morph);
    if (param)
    {
        poNode* paramType = param;
        if (param->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(param);
            const int count = pointerNode->count();
            paramType = pointerNode->child();
            clone = new poPointerNode(poNodeType::POINTER, clone, poToken(pointerNode->token().token(),
                pointerNode->token().string(),
                node->token().line(),
                node->token().column(),
                node->token().fileId()), count);
        }

        poToken token(paramType->token().token(),
            paramType->token().string(),
            node->token().line(),
            node->token().column(),
            node->token().fileId()
        );

        return new poUnaryNode(poNodeType::RETURN_TYPE, clone, token);
    }

    return  new poUnaryNode(poNodeType::RETURN_TYPE, clone, type->token());
}

poNode* poMorph:: match(const poToken& token, poListNode* generic, poMorphNode* morph)
{
    // Substitute the generic parameter for the specialized type.
    // Otherwise return nullptr.

    poNode* found = nullptr;
    for (int i = 0; i < int(generic->list().size()); i++) {
        poNode* arg = generic->list()[i];
        if (arg->type() == poNodeType::CONSTRAINT) {
            arg = static_cast<poUnaryNode*>(arg)->child();
        }

        const std::string& typeName = arg->token().string();
        if (token.string() == typeName) {
            poMorphParameter& param = morph->parameters()[i];
            found = param.node();
            break;
        }
    }
    return found;
}

poNode* poMorph::substituteCall(poNode* call, poListNode* generic, poMorphNode* morph)
{
    poListNode* node = static_cast<poListNode*>(call);
    std::vector<poNode*> list;
    for (poNode* child : node->list()) {
        if (child->type() == poNodeType::ARGS) {
            poListNode* callArgs = static_cast<poListNode*>(child);

            std::vector<poNode*> args;
            for (poNode* arg : callArgs->list()) {
                args.push_back(substitute(arg, generic, morph));
            }

            list.push_back(new poListNode(poNodeType::ARGS, args, callArgs->token()));
        }
        else {
            list.push_back(child->clone());
        }
    }

    return new poListNode(poNodeType::CALL, list, call->token());
}

poNode* poMorph::substitutePointer(poNode* pointer, poListNode* generic, poMorphNode* morph)
{
    poPointerNode* node = static_cast<poPointerNode*>(pointer);
    poNode* clone = substitute(node->child(), generic, morph);

    // Merge pointer nodes
    if (clone->type() == poNodeType::POINTER) {
        poPointerNode* pointerNode = static_cast<poPointerNode*>(clone);
        pointerNode->setCount(pointerNode->count() + node->count());
        return pointerNode;
    }

    return new poPointerNode(poNodeType::POINTER, clone, node->token(), node->count());
}

poNode* poMorph:: substituteArray(poNode* array, poListNode* generic, poMorphNode* morph)
{
    poArrayNode* node = static_cast<poArrayNode*>(array);
    poNode* clone = substitute(node->child(), generic, morph);

    poNode* param = match(array->token(), generic, morph);
    if (param) {
        poNode* paramType = param;
        if (paramType->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(paramType);
            paramType = pointerNode->child();
        }

        return new poArrayNode(node->arraySize(), clone, poNodeType::ARRAY,
            poToken(paramType->token().token(),
                paramType->token().string(),
                array->token().line(),
                array->token().column(),
                array->token().fileId()));
    }

    return new poArrayNode(node->arraySize(), clone, poNodeType::ARRAY, array->token());
}

poNode* poMorph::substituteDynamicArray(poNode* array, poListNode* generic, poMorphNode* morph)
{
    poBinaryNode* node = static_cast<poBinaryNode*>(array);
    poNode* type = substitute(node->right(), generic, morph);
    poNode* variable = node->left()->clone();

    return new poBinaryNode(array->type(), variable, type, array->token());
}

poNode* poMorph::substituteUnary(poNode* unary, poListNode* generic, poMorphNode* morph)
{
    poUnaryNode* node = static_cast<poUnaryNode*>(unary);
    poNode* clone = substitute(node->child(), generic, morph);

    poNode* param = match(unary->token(), generic, morph);
    if (param) {
        poNode* paramType = param;
        if (paramType->type() == poNodeType::POINTER) {
            poPointerNode* pointerNode = static_cast<poPointerNode*>(paramType);
            paramType = pointerNode->child();
        }

        return new poUnaryNode(unary->type(), clone,
            poToken(param->token().token(),
                param->token().string(),
                unary->token().line(),
                unary->token().column(),
                unary->token().fileId()));
    }

    return new poUnaryNode(unary->type(), clone, unary->token());
}

poNode* poMorph:: substitute(poNode* node, poListNode* generic, poMorphNode* morph)
{
    poNode* clone = nullptr;
    switch (node->type())
    {
    case poNodeType::CAST:
        clone = substituteCast(node, generic, morph);
        break;
    case poNodeType::TYPE:
        clone = substituteType(node, generic, morph);
        break;
    case poNodeType::RETURN_TYPE:
        clone = substituteReturnType(node, generic, morph);
        break;
    case poNodeType::POINTER:
        clone = substitutePointer(node, generic, morph);
        break;
    case poNodeType::ARRAY:
        clone = substituteArray(node, generic, morph);
        break;
    case poNodeType::NEW:
    case poNodeType::RETURN:
        clone = substituteUnary(node, generic, morph);
        break;
    case poNodeType::DYNAMIC_ARRAY:
        clone = substituteDynamicArray(node, generic, morph);
        break;
    case poNodeType::CALL:
        clone = substituteCall(node, generic, morph);
        break;
    default:
        clone = node->clone();
        break;
    }

    return clone;
}

void po::poMorph::substituteArgs(poListNode* args, poListNode* generic, poMorphNode* morph, std::vector<poNode*>& argList)
{
    for (int i = 0; i < int(args->list().size()); i++) {
        poUnaryNode* parameter = static_cast<poUnaryNode*>(args->list()[i]);
        poNode* type = parameter->child();
        poNode* newType = nullptr;

        for (int j = 0; j < int(generic->list().size()); j++) {
            poToken& token = type->token();
            const std::string& typeName = generic->list()[j]->token().string();
            if (token.string() == typeName) {
                poMorphParameter& param = morph->parameters()[i];
                newType = substitute(param.node(), generic, morph);
            }
        }

        if (!newType) {
            newType = type->clone();
        }

        argList.push_back(new poUnaryNode(poNodeType::PARAMETER,
            newType,
            args->list()[i]->token()));
    }
}

void po::poMorph::getGenericType(std::string& name, poMorphNode* morph)
{
    name += "<";
    for (int i = 0; i < int(morph->parameters().size()); i++) {
        if (i > 0) {
            name += ",";
        }

        poMorphParameter& param = morph->parameters()[i];
        const int paramType = poUtil::unpackTypeNode(_module, param.node());
        name += _module.types()[paramType].name();
    }
    name += ">";
}

void poMorph::generateMorphs() {
    // Finally, if there are no cycles then we 
    // can determine which types we need to create
    // from the generic classes

    std::vector<poMorphNode*> working(_nodes);

    while (working.size() > 0) {
        poMorphNode* node = working.back();
        working.pop_back();

        std::vector<int> parameters;
        for (int i = 0; i < int(node->parameters().size()); i++) {
            poMorphParameter& param = node->parameters()[i];
            assert(!param.isTypeName());

            parameters.push_back(param.type());
        }

        // Flow into the following nodes
        poMorphType* type = node->type();
        assert(int(parameters.size()) == type->numParameters());
        for (poMorphNode* follow : type->nodes()) {

            // Create a new morph node

            std::vector<int> types;
            for (int i = 0; i < int(follow->parameters().size()); i++) {
                types.push_back(parameters[follow->parameters()[i].index()]);
            }

            poMorphNode* newNode = _cache.find(type->type(), types);
            if (newNode != nullptr) {
                continue;
            }

            newNode = new poMorphNode(follow->type());
            working.push_back(newNode);
            _nodes.push_back(newNode);

            for (int i = 0; i < int(follow->parameters().size()); i++) {
                const int index = follow->parameters()[i].index();
                newNode->parameters().push_back(
                    poMorphParameter(parameters[index], node->parameters()[index].node())
                );
            }

            _cache.add(newNode);
        }
    }
}

void poMorph::buildGraph(const std::vector<poNode*>& nodes,
    std::vector<poNode*>& userTypes)
{
    for (poNode* node : userTypes) {
        assert(node->type() == poNodeType::CLASS ||
            node->type() == poNodeType::STRUCT ||
            node->type() == poNodeType::ENUM);

        std::vector<poNode*> parameters;

        poListNode* type = static_cast<poListNode*>(node);
        for (poNode* child : type->list()) {
            if (child->type() == poNodeType::GENERIC_ARGS) {
                poListNode* args = static_cast<poListNode*>(child);
                for (poNode* node : args->list()) {
                    parameters.push_back(node);
                }
                break;
            }
        }

        const std::string& name = type->token().string();
        const auto& it = _map.find(name);
        if (it == _map.end()) {
            // error
            break;
        }

        const int typeName = it->second + TYPE_OBJECT + 1;

        if (parameters.size() > 0) {
            poMorphType* morphType = new poMorphType(typeName, int(parameters.size()), node);
            _types.push_back(morphType);
            _typeMap[typeName] = morphType;
        }
    }

    for (poNode* node : userTypes) {
        assert(node->type() == poNodeType::CLASS ||
            node->type() == poNodeType::STRUCT ||
            node->type() == poNodeType::ENUM);

        poListNode* type = static_cast<poListNode*>(node);
        const std::string& name = type->token().string();
        const auto& it = _map.find(name);
        if (it == _map.end()) {
            // error
            break;
        }

        const int typeName = it->second + TYPE_OBJECT + 1;
        poMorphType* morphType = nullptr;
        const auto& it2 = _typeMap.find(typeName);
        if (it2 != _typeMap.end()) {
            morphType = it2->second;
        }

        for (poNode* child : type->list()) {
            if (child->type() == poNodeType::DECL) {
                walkDecl(morphType, child);
            }
        }
    }

    for (poNode* node : nodes) {
        poListNode* moduleNode = static_cast<poListNode*>(node);
        for (poNode* child : moduleNode->list()) {
            if (child->type() == poNodeType::NAMESPACE) {
                walkNamespace(child, userTypes);
            }
        }
    }
}

void poMorph::walkNamespace(poNode* node, const std::vector<poNode*>& userTypes)
{
    poListNode* ns = static_cast<poListNode*>(node);
    for (poNode* child : ns->list()) {
        if (child->type() == poNodeType::FUNCTION) {
            walkFunction(child, userTypes);
        }
    }
}

void poMorph::walkFunction(poNode* node, const std::vector<poNode*>& userTypes)
{
    poListNode* func = static_cast<poListNode*>(node);
    poListNode* body = nullptr;
    for (poNode* child : func->list()) {
        if (child->type() == poNodeType::BODY) {
            body = static_cast<poListNode*>(child);
        }
    }

    if (body) {
        for (poNode* child : body->list()) {
            if (child->type() == poNodeType::STATEMENT) {
                walkStatement(child, userTypes);
            }
        }
    }
}

void poMorph::walkStatement(poNode* node, const std::vector<poNode*>& userTypes)
{
    poUnaryNode* statement = static_cast<poUnaryNode*>(node);
    if (statement->child()->type() == poNodeType::DECL) {
        walkDecl(statement->child(), userTypes);
    }
}

void poMorph::walkDecl(poNode* node, const std::vector<poNode*>& types)
{
    poUnaryNode* decl = static_cast<poUnaryNode*>(node);
    poNode* type = decl->child();
    poListNode* args = nullptr;
    poGenericNode* generic = nullptr;
    if (type->type() == poNodeType::CONSTRUCTOR) {
        poListNode* constructor = static_cast<poListNode*>(type);
        for (poNode* node : constructor->list()) {
            if (node->type() == poNodeType::ARGS) {
                args = static_cast<poListNode*>(node);
            }
            else if (node->type() == poNodeType::GENERIC) {
                generic = static_cast<poGenericNode*>(node);
            }
        }
    }

    int baseType = -1;
    poNode* typeNode = nullptr;
    if (generic) {
        const std::string baseTypeName = generic->token().string();
        const auto& it = _map.find(baseTypeName);
        if (it == _map.end()) {
            setError("Unable to find " + baseTypeName, generic->token());
            return;
        }
        else {
            baseType = it->second + TYPE_OBJECT + 1;
            typeNode = types[it->second];
        }
    }
    else {
        // Return: not a generic type
        return;
    }

    if (!_isError) {

        poListNode* type = static_cast<poListNode*>(typeNode);
        poListNode* genericArgs = nullptr;
        for (poNode* node : type->list()) {
            if (node->type() == poNodeType::GENERIC_ARGS) {
                genericArgs = static_cast<poListNode*>(node);
            }
        }


        // Constraint [parameters] flows into the decl type

        std::vector<poMorphParameter> types;
        for (poNode* node : generic->nodes()) {
            assert(node->type() == poNodeType::TYPE ||
                node->type() == poNodeType::POINTER);

            int parameterType = poUtil::unpackTypeNode(_module, node);

            if (parameterType == -1) {
                // Error: type not found
                setError("Type not found.", node->token());
                break;
            }

            types.push_back(poMorphParameter(parameterType, node));
        }

        std::vector<int> args;
        for (poMorphParameter& param : types) {
            args.push_back(param.type());
        }

        poMorphNode* morphNode = nullptr;
        if (typeNode) {
            morphNode = _cache.find(baseType, args);
            if (morphNode == nullptr) {
                poMorphType* morphType = _typeMap[baseType];
                poMorphNode* morphNode = new poMorphNode(morphType);

                for (poMorphParameter& param : types) {
                    morphNode->parameters().push_back(param);
                }
                _nodes.push_back(morphNode);
                _cache.add(morphNode);
            }
        }
    }
}

void poMorph::setError(const std::string& errorText, const poToken& token) {
    if (!_isError) {
        _isError = true;
        _errorText = errorText;
        _errorLine = token.line();
        _errorCol = token.column();
        _errorFileId = token.fileId();
    }
}

void poMorph:: checkConstraints() {
    // Check each generic specialization satisfies the constraints.

    bool isValid = true;
    for (poMorphNode* node : _nodes) {
        poMorphType* type = node->type();
        poListNode* astNode = static_cast<poListNode*>(type->node());
        poListNode* args = nullptr;
        for (poNode* child : astNode->list()) {
            if (child->type() == poNodeType::GENERIC_ARGS) {
                args = static_cast<poListNode*>(child);
            }
        }

        assert(args);

        for (int i = 0; i < int(node->parameters().size()); i++) {
            poNode* constraint = args->list()[i];
            if (constraint->type() != poNodeType::CONSTRAINT) {
                continue;
            }

            const poMorphParameter& param = node->parameters()[i];
            if (constraint->token().token() == poTokenType::NEW) {
                // This constraint is valid if it is a primitive type or
                // pointer or a class/struct with a constructor with zero arguments

                assert(!param.isTypeName());

                const int typeId = param.type();
                const poType& type = _module.types()[typeId];
                bool ok = type.isPrimitive() ||
                    type.isPointer() ||
                    type.constructors().size() == size_t(0);
                for (const poConstructor& constructor : type.constructors()) {
                    if (constructor.arguments().size() == size_t(0)) {
                        ok = true;
                        break;
                    }
                }

                if (!ok) {
                    isValid = false;
                    setError("Constraint not satisfied", constraint->token());
                    break;
                }
            }
            else {
                // TODO: search for trait and check the shape matches

                isValid = false;
                setError("Constraint not satisfied", constraint->token());
                break;
            }
        }

        if (!isValid) {
            break;
        }
    }
}

void poMorph::walkDecl(poMorphType* sourceType, poNode* node)
{
    poUnaryNode* decl = static_cast<poUnaryNode*>(node);
    poNode* type = decl->child();
    poGenericNode* generic = nullptr;
    if (type->type() == poNodeType::ATTRIBUTES) {
        poAttributeNode* attrib = static_cast<poAttributeNode*>(type);
        type = attrib->child();
    }

    if (type->type() == poNodeType::GENERIC) {
        generic = static_cast<poGenericNode*>(type);
    }

    bool error = false;
    int baseType = -1;
    poMorphType* typeNode = nullptr;
    if (generic) {
        const std::string baseTypeName = generic->token().string();
        const auto& it = _map.find(baseTypeName);
        if (it == _map.end()) {
            error = true;
        }
        else {
            baseType = it->second + TYPE_OBJECT + 1;
            typeNode = _typeMap[baseType];
        }
    } else {
        error = true;
    }

    if (!error) {
        type = generic->child();
        int pointerCount = 0;
        if (type->type() == poNodeType::POINTER) {
            poPointerNode* ptr = static_cast<poPointerNode*>(type);
            type = ptr->child();
            pointerCount = ptr->count();
        }

        // Constraint [parameters] flows into the decl type

        poListNode* genericArgs = nullptr;
        if (sourceType) {
            poListNode* ast = static_cast<poListNode*>(sourceType->node());
            for (poNode* child : ast->list()) {
                if (child->type() == poNodeType::GENERIC_ARGS) {
                    genericArgs = static_cast<poListNode*>(child);
                }
            }
        }

        std::vector<poMorphParameter> types;
        for (poNode* node : generic->nodes()) {
            int pointerCount = 0;
            if (node->type() == poNodeType::POINTER) {
                poPointerNode* ptr = static_cast<poPointerNode*>(node);
                node = ptr->child();
                pointerCount = ptr->count();
            }

            assert(node->type() == poNodeType::TYPE);
            auto& token = node->token();
            int parameterType = poUtil::getType(token);
            int typeIndex = -1;

            if (parameterType == -1 && genericArgs) {
                for (int i = 0; i < int(genericArgs->list().size()); i++) {
                    poNode* argNode = genericArgs->list()[i];
                    if (argNode->type() == poNodeType::CONSTRAINT) {
                        argNode = static_cast<poUnaryNode*>(argNode)->child();
                    }

                    const poToken& typeToken = argNode->token();
                    if (typeToken.string() == token.string()) {
                        parameterType = baseType;
                        typeIndex = i;
                        break;
                    }
                }
            }

            if (parameterType == -1) {
                // It is a user defined type
                const auto& it = _map.find(token.string());
                if (it != _map.end()) {
                    parameterType = TYPE_OBJECT + it->second + 1;
                }
            }

            if (parameterType == -1) {
                // Error: type not found
                error = true;
                break;
            }

            while (pointerCount > 0) {
                parameterType = poUtil::getPointerType(_module, parameterType);
                pointerCount--;
            }

            if (typeIndex == -1) {
                types.push_back(poMorphParameter(parameterType, node));
            }
            else {
                types.push_back(poMorphParameter(parameterType, typeIndex));
            }
        }

        std::vector<int> args;
        for (poMorphParameter& param : types) {
            args.push_back(param.type());
        }

        poMorphNode* morphNode = nullptr;
        if (typeNode) {
            morphNode = _cache.find(typeNode->type(), args);
            if (morphNode == nullptr) {
                morphNode = new poMorphNode(typeNode);

                for (poMorphParameter& param : types) {
                    morphNode->parameters().push_back(param);
                }

                if (sourceType == nullptr) { _nodes.push_back(morphNode); }
                _cache.add(morphNode);
            }

            if (sourceType) { sourceType->nodes().push_back(morphNode); }
        }

    }
}


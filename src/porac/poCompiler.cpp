#include "poCompiler.h"

#include "poLex.h"
#include "poParser.h"
#include "poModule.h"
#include "poAST.h"
#include "poTypeChecker.h"
#include "poEmit.h"
#include "poOptFold.h"
#include "poOptMemToReg.h"
#include "poOptDCE.h"
#include "poOptCopy.h"
#include "poSSA.h"
#include "poTypeResolver.h"
#include "poTypeValidator.h"

#include <sstream>

using namespace po;

void poCompiler:: addFile(const std::string& file)
{
    _files.push_back(poFile(file, int(_files.size())));
}

void poCompiler::reportError(const std::string& errorPhase, const std::string& errorText, const int fileId, const int colNum, const int lineNum)
{
    poFile& file = _files[fileId];

    std::stringstream ss;
    ss << errorPhase << " " << file.filename() << ": " << errorText << " Line " << lineNum << ":" << colNum << std::endl;

    std::vector<std::string> lines;
    file.getErrorLines(lineNum, 2, lines);
    for (int i = 0; i < int(lines.size()); i++)
    {
        ss << "Line " << (lineNum - 2 + i) << ":" << lines[i] << std::endl;
    }

    _errors.push_back(ss.str());
}

int poCompiler:: compile()
{
    poLexer lexer;

    std::vector<poNode*> nodes;
    for (auto& file : _files)
    {
        file.load(lexer);
        if (file.isError())
        {
            reportError("Lex/Parse Error:", file.errorText(), file.fileId(), file.colNum(), file.lineNum());
            return 0;
        }

        nodes.push_back(file.ast());

        // Reset the lexer
        lexer.reset();
    }

    // Resolve types
    poModule module;
    poTypeResolver typeResolver(module);
    typeResolver.resolve(nodes);
    if (typeResolver.isError())
    {
        reportError("Type Resolution Error:", typeResolver.errorText(), typeResolver.errorFile(), 0, typeResolver.errorLine());
        return 0;
    }

    // Validate types
    poTypeValidator typeValidator;
    if (!typeValidator.validateModule(module))
    {
        reportError("Type Validation Error:", typeValidator.errorText(), typeValidator.errorFile(), typeValidator.errorCol(), typeValidator.errorLine());
        return 0;
    }

    // Type check AST
    poTypeChecker typeChecker(module);
    if (!typeChecker.check(nodes))
    {
        reportError("Type Checking Error:", typeChecker.errorText(), typeChecker.errorFile(), typeChecker.errorCol(), typeChecker.errorLine());
        return 0;
    }

    // Perform constant folding
    poOptFold fold;
    fold.fold(nodes);

    // Convert the AST to a IR (three address code - intermediate representation) formed of BB (basic blocks) and CFG (control flow graph)
    poCodeGenerator generator(module);
    generator.generate(nodes);
    if (generator.isError())
    {
        reportError("Code Generation Error:", generator.errorText(), generator.errorFile(), generator.errorColumn(), generator.errorLine());
        return 0;
    }
    if (_debugDump) { module.dump(); }

    // Convert to SSA form and insert PHI nodes
    poSSA ssa;
    ssa.construct(module);
    //module.dump();
    if (_debugDump) { module.dumpTypes(); }

    // Convert unnecessary memory accesses to registers
    poOptMemToReg regToMem;
    regToMem.optimize(module);
    //module.dump();

    // Perform copy propagation optimization
    poOptCopy copy;
    copy.optimize(module);

    // Eliminate any dead code
    poOptDCE dce;
    dce.optimize(module);
    if (_debugDump) { module.dump(); }

    // Convert the basic blocks/cfg to machine code
    _assembler.setDebugDump(_debugDump);
    _assembler.generate(module);
    //module.dump();

    if (_assembler.isError())
    {
        _errors.push_back(_assembler.errorText());
        return 0;
    }

    return 1;
}


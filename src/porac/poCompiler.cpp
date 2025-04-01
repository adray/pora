#include "poCompiler.h"

#include "poLex.h"
#include "poParser.h"
#include "poModule.h"
#include "poAST.h"
#include "poTypeChecker.h"
#include "poEmit.h"
#include "poOptFold.h"
#include "poSSA.h"

#include <sstream>

using namespace po;

void poCompiler:: addFile(const std::string& file)
{
    _files.push_back(file);
}

int poCompiler:: compile()
{
    poLexer lexer;

    std::vector<poNode*> nodes;
    for (auto& file : _files)
    {
        // Lexer
        lexer.tokenizeFile(file);
        if (lexer.isError())
        {
            std::stringstream ss;
            ss << "Failed building " << file << ": " << lexer.errorText() << ":" << lexer.lineNum();
            _errors.push_back(ss.str());
            return 0;
        }

        // Parse tokens into Abstract syntax tree
        poParser parser(lexer.tokens());
        poModuleParser moduleParser(parser);
        poNode* node = moduleParser.parse();
        if (parser.isError())
        {
            std::stringstream ss;
            ss << "Failed building " << file << ": " << parser.error() << " Line: " << parser.errorLine();
            _errors.push_back(ss.str());
            return 0;
        }

        nodes.push_back(node);

        // Reset the lexer
        lexer.reset();
    }


    // Type check AST
    poTypeChecker typeChecker;
    if (!typeChecker.check(nodes))
    {
        std::stringstream ss;
        ss << typeChecker.errorText() << " Line:" << typeChecker.errorLine() << ":" << typeChecker.errorCol() << std::endl;
        _errors.push_back(ss.str());
        return 0;
    }

    // Perform constant folding
    poOptFold fold;
    fold.fold(nodes);

    // Convert the AST to a IR (three address code - intermediate representation) formed of BB (basic blocks) and CFG (control flow graph)
    poModule module;
    poCodeGenerator generator(module);
    generator.generate(nodes);
    module.dump();

    // Convert to SSA form and insert PHI nodes
    poSSA ssa;
    ssa.construct(module);
    module.dump();

    // TODO: Convert the basic blocks/cfg to machine code


    return 1;
}


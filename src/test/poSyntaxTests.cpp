#include "poSyntaxTests.h"
#include "poLex.h"
#include "poParser.h"
#include "poTypeChecker.h"
#include "poTypeResolver.h"
#include "poTypeValidator.h"
#include "poModule.h"
#include "poAST.h"
#include <string>
#include <iostream>

using namespace po;

static void dumpAST(poNode* ast);

static void dumpASTList(poNode* ast)
{
    poListNode* list = static_cast<poListNode*>(ast);
    for (poNode* node : list->list())
    {
        dumpAST(node);
    }
}

static void dumpASTUnary(poNode* ast)
{
    poUnaryNode* node = static_cast<poUnaryNode*>(ast);
    dumpAST(node->child());
}

static void dumpAST(poNode* ast)
{
    switch (ast->type())
    {
        case poNodeType::ADD:
            break;
        case poNodeType::FUNCTION:
            std::cout << "FUNCTION" << std::endl;
            dumpASTList(ast);
            break;
        case poNodeType::BODY:
            std::cout << "BODY" << std::endl;
            dumpASTList(ast);
            break;
        case poNodeType::STATEMENT:
            std::cout << "STATEMENT" << std::endl;
            dumpASTUnary(ast);
            break;
        case poNodeType::DECL:
            std::cout << "DECL: " << ast->token().string() << std::endl;
            dumpASTUnary(ast);
            break;
        case poNodeType::TYPE:
            std::cout << "Type: ";
            switch (ast->token().token())
            {
                case poTokenType::IDENTIFIER:
                    std::cout << ast->token().string();
                    break;
                case poTokenType::VOID:
                    std::cout << "VOID";
                    break;
                case poTokenType::I64_TYPE:
                    std::cout << "I64";
                    break;
            }
            std::cout << std::endl;
            break;
        case poNodeType::VARIABLE:
            std::cout << "Variable: " << ast->token().string() << std::endl;
            break;
        case poNodeType::NAMESPACE:
            std::cout << "Namespace: " << ast->token().string() << std::endl;
            dumpASTList(ast);
            break;
        case poNodeType::MODULE:
            std::cout << "Module" << std::endl;
            dumpASTList(ast);
            break;
        case poNodeType::STRUCT:
            std::cout << "Struct: " << ast->token().string() << std::endl;
            dumpASTList(ast);
            break;
    }
}

static void checkSyntax(const std::string& testName, const std::string& program, bool success)
{
    std::cout << testName << " ";

    poLexer lexer;
    lexer.tokenizeText(program);
    if (lexer.isError())
    {
        if (success)
        {
            std::cout << "FAILED Lexer: " << lexer.errorText() << " Line:" << lexer.lineNum() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    poParser parser(lexer.tokens());
    poModuleParser moduleParser(parser);
    poNode* ast = moduleParser.parse();
    if (parser.isError())
    {
        if (success)
        {
            std::cout << "FAILED Parser: " << parser.error() << " Line:" << parser.errorLine() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    //dumpAST(ast);

    std::vector<poNode*> nodes;
    nodes.push_back(ast);

    poModule module;
    poTypeResolver resolver(module);
    resolver.resolve(nodes);
    if (resolver.isError())
    {
        if (success)
        {
            std::cout << "FAILED Type Resolver: " << resolver.errorText() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    poTypeChecker checker(module);
    if (!checker.check(nodes))
    {
        if (success)
        {
            std::cout << "FAILED Type Checker: " << checker.errorText() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    poTypeValidator validator;
    if (!validator.validateModule(module))
    {
        if (success)
        {
            std::cout << "FAILED Type Validator: " << validator.errorText() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    if (success)
    {
        std::cout << "OK" << std::endl;
    }
    else
    {
        std::cout << "FAILED" << std::endl;
    }
}

void po::syntaxTest()
{
    checkSyntax("Namespace Test #1", "namespace Test { }", true);
    checkSyntax("Namespace Test #2", "namespace Test ", false);
    checkSyntax("Namespace Test #3", "namespace Test {", false);
    checkSyntax("Decl Test #1", "namespace Test { static void main() { i64 x; } }", true);
    checkSyntax("Decl Test #2", "namespace Test { static void main() { i64 x = 0; } }", true);
    checkSyntax("Decl Test #3", "namespace Test { static void main() { i64 x = } }", false);
    checkSyntax("Function Test #1", "namespace Test { static i64 main() { return 0; } }", true);
    checkSyntax("Function Test #2", "namespace Test {"\
        "static i64 test(i64 x) {"\
        "   return x + 1;"\
        "}"\
        "static void main() {"\
        "   i64 x = test(55);"\
        "}"\
        "}", true);
    checkSyntax("Function Test #3", "namespace Test {"\
        "static i64 test(i64 x) {"\
        "   return x + 1;"\
        "}"\
        "static void main() {"\
        "   test(55);"\
        "}"\
        "}", true);
    checkSyntax("Function Test #4", "namespace Test {"\
        "static void test(i32 x) {"\
        "}"\
        "static void main() {"\
        "   test(55);"\
        "}"\
        "}", false);
    checkSyntax("Function Test #5", "namespace Test {"\
        "static void main() {"\
        "}", false);
    checkSyntax("Function Test #6", "namespace Test {"\
        "static void main()"\
        "}", false);
    checkSyntax("Function Test #7", "namespace Test { static void main() { ", false);
    checkSyntax("Function Test #8", "namespace Test {"\
        "static i64 test() {"\
        "   return 5;"\
        "}"\
        "static void main() {"\
        "   i64 x = 6 + test();"\
        "}"\
        "}", true);
    checkSyntax("Function Test #9", "namespace Test {"\
        "static i64 test() {"\
        "   return 5;"\
        "}"\
        "static void main() {"\
        "   test(;"\
        "}"\
        "}", false);
    checkSyntax("Function Test #10", "namespace Test {"\
        "static i64 test(i64 x, i64 u) {"\
        "   return x + u;"\
        "}"\
        "static void main() {"\
        "   test(5, ;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #1", "namespace Test {"\
        "static void main() {"\
        "   i32 x = 5;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   i32 y = x;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #3", "namespace Test {"\
        "static void main() {"\
        "   u8 x = '5';"\
        "}"\
        "}", true);
    checkSyntax("Type Test #4", "namespace Test {"\
        "static void main() {"\
        "   boolean x = true;"\
        "   boolean y = false;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64 x;"\
        "   x = 0;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #6", "namespace Test {"\
        "static void main() {"\
        "   i64* x;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #7", "namespace Test {"\
        "static void main() {"\
        "   i64** x;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #8", "namespace Test {"\
        "static void main() {"\
        "   f64 x = 90.0;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #9", "namespace Test {"\
        "static void main() {"\
        "   f32 x = 90.0f;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #10", "namespace Test {"\
        "static void main() {"\
        "   f64 x = 90.0d;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #11", "namespace Test {"\
        "static void main() {"\
        "   u8 x = 90b;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #12", "namespace Test {"\
        "static void main() {"\
        "   u8*[16] x;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #13", "namespace Test {"\
        "struct test{}"
        "static void func(test* t){}"
        "static void main() {"\
        "   test t;"\
        "   func(t);"\
        "}"\
        "}", false);
    checkSyntax("Type Test #14", "namespace Test {"\
        "struct test{}"\
        "static void func(test* t){}"\
        "static void main() {"\
        "}"\
        "}", true);
    checkSyntax("Type Test #15", "namespace Test {"\
        "struct test{ i64* ptr; }"\
        "static void main() {"\
        "   test t;"\
        "   i64* ptr = t.ptr;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #16", "namespace Test {"\
        "struct test{ i64*[16] ptr; }"\
        "static void main() {"\
        "   test t;"\
        "   i64* ptr = t.ptr[0];"\
        "}"\
        "}", true);
    checkSyntax("Type Test #17", "namespace Test {"\
        "struct test{ i64*[16] ptr; }"\
        "static void main() {"\
        "   test t;"\
        "   test s = -t;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #18", "namespace Test {"\
        "struct test{ i64*[16] ptr; }"\
        "static void main() {"\
        "   test t;"\
        "   test s = t+t;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #19", "namespace Test {"\
        "struct test{ i64*[16] ptr; }"\
        "static void main() {"\
        "   test t;"\
        "   test s = t || t;"\
        "}"\
        "}", false);
    checkSyntax("Type Test #20", "namespace Test {"\
        "static void main() {"\
        "   i16 i;"\
        "   u16 u;"\
        "}"\
        "}", true);
    checkSyntax("Type Test #21", "namespace Test {"\
        "static i16 test1() {"\
        "   i16 x;"\
        "   return x;"\
        "}"\
        "static u16 test2() {"\
        "   u16 x;"\
        "   return x;"\
        "}"\
        "static void main() {"\
        "   i16 i = test1();"\
        "   u16 u = test2();"\
        "}"\
        "}", true);
    checkSyntax("Type Test #22", "namespace Test {"\
        "static void main() {"\
        "   object o;"\
        "}"\
        "}", true);
    checkSyntax("Numeric Test #1", "namespace Test {"\
        "static void main() {"\
        "   u64 x = 10000000000000000000u;"\
        "}"\
        "}", true);
    checkSyntax("Numeric Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 9223372036854775807;"\
        "}"\
        "}", true);
    checkSyntax("Numeric Test #3", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 9223372036854775808;"\
        "}"\
        "}", false);
    checkSyntax("Numeric Test #4", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 9223372036854775810;"\
        "}"\
        "}", false);
    checkSyntax("Numeric Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 9223372036854775810;"\
        "}"\
        "}", false);
    checkSyntax("Numeric Test #6", "namespace Test {"\
        "static void main() {"\
        "   u8 x = 255b;"\
        "}"\
        "}", true);
    checkSyntax("Numeric Test #7", "namespace Test {"\
        "static void main() {"\
        "   u8 x = 256b;"\
        "}"\
        "}", false);
    checkSyntax("Numeric Test #8", "namespace Test {"\
        "static void main() {"\
        "   u8 x = 300b;"\
        "}"\
        "}", false);
    checkSyntax("Arithmetic Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   x = x * 5;"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   x = x * 5 + 10;"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #3", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   x = x / 5 - 10;"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #4", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   i64 y = 15;"\
        "   x = x * (y + 10);"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   i64 y = 15;"\
        "   x = x * y"\
        "}"\
        "}", false);
    checkSyntax("Arithmetic Test #6", "namespace Test {"\
        "static void main() {"\
        "   f64 x = 5.0;"\
        "   f64 y = 15.0;"\
        "   x = x * (y + 10.0);"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #7", "namespace Test {"\
        "static void main() {"\
        "   f64 x = 5.0;"\
        "   f64 y = 15.0;"\
        "   x += y;"\
        "}"\
        "}", true);
    checkSyntax("Arithmetic Test #8", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5 % 3;"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   if (x == 10)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   if (x == 10)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "   else"\
        "   {"\
        "       i64 w = 0;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #3", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   if (x == 10)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "   else if (x == 5)"\
        "   {"\
        "       i64 p = 0;"\
        "   }"\
        "   else"\
        "   {"\
        "       i64 w = 0;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #4", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 5;"\
        "   i64 y = 15;"\
        "   if ((x == 10 && y == 15) || (x == 5))"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #5", "namespace Test {"\
        "static i64 foo() {"\
        "   return 0;"\
        "}"\
        "static void main() {"\
        "   if (foo() == 0)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #6", "namespace Test {"\
        "static i64 foo() {"\
        "   return 0;"\
        "}"\
        "static void main() {"\
        "   if (foo())"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", false);
    checkSyntax("Branch Test #7", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0; i64 y = 2;"\
        "   if (x >= y)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #8", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0; i64 y = 2;"\
        "   if (x >= y)"\
        "   {"\
        "       return;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("Branch Test #9", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0; i64 y = 2;"\
        "   if (x >= y)"\
        "   {"\
        "}"\
        "}", false);
    checkSyntax("Branch Test #10", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0; i64 y = 2;"\
        "   if (x >= y)"\
        "}"\
        "}", false);
    checkSyntax("While Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0; i64 y = 2;"\
        "   while (x >= y)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("While Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 0;"\
        "   while (x)"\
        "   {"\
        "       i64 z;"\
        "   }"\
        "}"\
        "}", false);
    checkSyntax("While Test #3", "namespace Test {"\
        "static void main() {"\
        "   boolean x = true;"\
        "   boolean y = false;"\
        "   while (x || y)"\
        "   {"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("While Test #4", "namespace Test {"\
        "static void main() {"\
        "   while (true)"\
        "   {"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #1", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "   {"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 i = 0;"\
        "   for (; i < 10; i = i + 1)"\
        "   {"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #3", "namespace Test {"\
        "static void main() {"\
        "   i64 i;"\
        "   for (i = 0; i < 10; i = i + 1)"\
        "   {"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #4", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "   {"\
        "   }"\
        "   i64 k = i;"\
        "}"\
        "}", false);
    checkSyntax("For Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64 k = 0;"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "   {"\
        "       k = k + i;"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #6", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "}"\
        "}", false);
    checkSyntax("For Test #7", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1"\
        "}"\
        "}", false);
    checkSyntax("For Test #8", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; "\
        "}"\
        "}", false);
    checkSyntax("For Test #10", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0 "\
        "}"\
        "}", false);
    checkSyntax("For Test #11", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10 "\
        "}"\
        "}", false);
    checkSyntax("For Test #12", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "   {"\
        "}"\
        "}", false);
    checkSyntax("For Test #13", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i = 0; i < 10; i = i + 1)"\
        "   {"\
        "       if (i == 5)"\
        "       {"\
        "           break;"\
        "       }"\
        "   }"\
        "}"\
        "}", true);
    checkSyntax("For Test #14", "namespace Test {"\
        "static void main() {"\
        "   continue;"\
        "}"\
        "}", false);
    checkSyntax("For Test #15", "namespace Test {"\
        "static void main() {"\
        "   break;"\
        "}"\
        "}", false);
    checkSyntax("For Test #16", "namespace Test {"\
        "static void main() {"\
        "   for (i64 i =0; i < 10; i+=2){}"\
        "}"\
        "}", true);
    //checkSyntax("For Test #13", "namespace Test {"\
    //    "static void main() {"\
    //    "   for (;;){} "\
    //    "}"\
    //    "}", true);
    checkSyntax("Struct Test #1", "namespace Test {"\
        "struct myStruct {"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #2", "namespace Test {"\
        "struct myStruct {"\
        "}", false);
    checkSyntax("Struct Test #3", "namespace Test {"\
        "struct myStruct"\
        "}", false);
    checkSyntax("Struct Test #4", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "   i64 y;"\
        "   boolean z;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #5", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x"\
        "}"
        "}", false);
    checkSyntax("Struct Test #6", "namespace Test {"\
        "struct myStruct {"\
        "   i64"\
        "}"\
        "}", false);
    checkSyntax("Struct Test #7", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #8", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   myStruct v2 = v1;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #9", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   i64 p = v1.x;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #10", "namespace Test {"\
        "struct myStruct {"\
        "   myOtherStruct y;"\
        "}"\
        "struct myOtherStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   i64 p = v1.y.x;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #11", "namespace Test {"\
        "static void myTest(myStruct x)"\
        "{"\
        "}"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   myTest(v1);"\
        "}"\
        "}", true); 
    checkSyntax("Struct Test #12", "namespace Test {"\
        "static void myTest(myStructx x)"\
        "{"\
        "}"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   myTest(v1);"\
        "}"\
        "}", false);
    checkSyntax("Struct Test #13", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStructx v1;"\
        "}"\
        "}", false);
    checkSyntax("Struct Test #14", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   v1.x = 4;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #15", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   i64 y = 10;"\
        "   v1.x = y;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #16", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   f64 y = 10.0;"\
        "   v1.x = y;"\
        "}"\
        "}", false);
    checkSyntax("Struct Test #17", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   v1.x = 9;"\
        "   v1.x += 12;"\
        "}"\
        "}", true);
    checkSyntax("Struct Test #18", "namespace Test {"\
        "struct myStruct {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   myStruct v1;"\
        "   myStruct v2;"\
        "   if (v1 == v2) { v1.x = 4; }"\
        "}"\
        "}", false);
    checkSyntax("Class Test #1", "namespace Test {"\
        "class myClass {"\
        "   public i64 x;"\
        "}"\
        "static void main() {"\
        "   myClass v1;"\
        "   v1.x = 9;"\
        "   v1.x += 12;"\
        "}"\
        "}", true);
    checkSyntax("Class Test #2", "namespace Test {"\
        "class myClass {"\
        "   private i64 _x;"\
        "   i64 getX();"\
        "   i64 setX(i64 x);"\
        "}"\
        "i64 myClass::getX() { return _x; } "\
        "void myClass::setX(i64 x) { _x = x; } "\
        "static void main() {"\
        "   myClass v1;"\
        "   v1.setX(10);"\
        "   i64 x = v1.getX();"\
        "}"\
        "}", true);
    checkSyntax("Class Test #3", "namespace Test {"\
        "class myClass {"\
        "   private i64 x;"\
        "}"\
        "static void main() {"\
        "   myClass v1;"\
        "   v1.x = 9;"\
        "}"\
        "}", false);
    checkSyntax("Class Test #4", "namespace Test {"\
        "class myClass {"\
        "   private i64 x;"\
        "   private i64 getX();"\
        "}"\
        "i64 myClass::getX() { return x; } "\
        "static void main() {"\
        "   myClass v1;"\
        "   i64 x = v1.getX();"\
        "}"\
        "}", false);
    checkSyntax("Class Test #5", "namespace Test {"\
        "class myClass {"\
        "   private i64 x;"\
        "   private i64 innerGetX();"\
        "   public i64 getX();"\
        "}"\
        "i64 myClass::innerGetX() { return x; } "\
        "i64 myClass::getX() { return innerGetX(); } "\
        "static void main() {"\
        "   myClass v1;"\
        "   i64 x = v1.getX();"\
        "}"\
        "}", true);
    checkSyntax("Class Test #6", "namespace Test {"\
        "class myClass {"\
        "   private i64 x;"\
        "   public i64 getX();"\
        "}"\
        "i64 myClass:getX() { return innerGetX(); } "\
        "static void main() {"\
        "   myClass v1;"\
        "}"\
        "}", false);
    checkSyntax("Class Test #7", "namespace Test {"\
        "class myClass {"\
        "   private i64 _x;"\
        "   public myClass(i64 x);"\
        "}"\
        "myClass::myClass(i64 x) { _x = x; } "\
        "static void main() {"\
        "   myClass v1(77);"\
        "}"\
        "}", true);
    checkSyntax("Class Test #8", "namespace Test {"\
        "class myClass {"\
        "   private i64 _x;"\
        "   public myClass();"\
        "}"\
        "myClass::myClass() { _x = 99; } "\
        "static void main() {"\
        "   myClass v1();"\
        "}"\
        "}", true);
    checkSyntax("Class Test #9", "namespace Test {"\
        "class myClass {"\
        "   private i64 _x;"\
        "   public myClass(i64 x);"\
        "}"\
        "myClass::myClass(i64 x) { _x = x; } "\
        "static void main() {"\
        "   myClass v1;"\
        "}"\
        "}", false);
    checkSyntax("Class Test #10", "namespace Test {"\
        "class myClass {"\
        "   private i64 _x;"\
        "}"\
        "static void main() {"\
        "   myClass v1(10);"\
        "}"\
        "}", false);
    checkSyntax("Class Test #11", "namespace Test {"\
        "class myClass {"\
        "   private u16 _x;"\
        "   void setX(u16 x);"\
        "}"\
        "void myClass::setX(u16 x) { _x = x; }"
        "static void main() {"\
        "   myClass v1;"\
        "   v1.setX(60);"
        "}"\
        "}", false);
    checkSyntax("Class Test #12", "namespace Test {"\
        "class myClass {"\
        "   private u16 _x;"\
        "   void setX(u16 x);"\
        "}"\
        "void myClass::setX(u16 x) { _x = x; }"
        "void myClass::setX(u16 x) { _x = x; }"
        "static void main() {"\
        "   myClass v1;"\
        "   v1.setX((u16)60);"
        "}"\
        "}", false);
    checkSyntax("Class Test #13", "namespace Test {"\
        "class myClass {"\
        "   private u16 _x;"\
        "   private u16 _x;"\
        "}"\
        "static void main() {"\
        "   myClass v1;"\
        "}"\
        "}", false);
    checkSyntax("Class Test #14", "namespace Test {"\
        "class myClass {"\
        "   private u16 _x;"\
        "   void setX(u16 x);"\
        "}"\
        "void myClass::setX(u16 x) { _x = x; }"
        "static void main() {"\
        "   myClass[2] v1;"\
        "   v1[0].setX((u16)20);"\
        "}"\
        "}", true);
    checkSyntax("Class Test #15", "namespace Test {"\
        "class myClass {"\
        "   u16 x;"\
        "}"\
        "static void main() {"\
        "   myClass[2] v1;"\
        "   v1[0].x = (u16)20;"\
        "}"\
        "}", true);
    checkSyntax("Class Test #16", "namespace Test {"\
        "class myClass {"\
        "   i64 x;"\
        "   myClass();"\
        "}"\
        "myClass::myClass() { x = 0; }"\
        "static void main() {"\
        "   myClass[2] v1();"\
        "}"\
        "}", false);
    checkSyntax("Class Test #17", "namespace Test {"\
        "class myClass {"\
        "   i64 x;"\
        "   myClass(i64 y);"\
        "}"\
        "myClass::myClass(i64 y) { x = y; }"\
        "static void main() {"\
        "   myClass[2] v1;"\
        "}"\
        "}", false);
    checkSyntax("Class Test #18", "namespace Test {"\
        "class myClass {"\
        "   private u16 _x;"\
        "   void setX(u16 x);"\
        "   void setX(u16 x);"\
        "}"\
        "void myClass::setX(u16 x) { _x = x; }"
        "static void main() {"\
        "   myClass v1;"\
        "   v1.setX((u16)60);"
        "}"\
        "}", false);
    checkSyntax("Array Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64[10] myArray;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64[10 myArray;"\
        "}"\
        "}", false);
    checkSyntax("Array Test #3", "namespace Test {"\
        "static void main() {"\
        "   i64] myArray;"\
        "}"\
        "}", false);
    checkSyntax("Array Test #4", "namespace Test {"\
        "static void main() {"\
        "   i64[16] myArray;"\
        "   myArray[0] = 0;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64[8] myArray;"\
        "   i64 x = myArray[1];"\
        "}"\
        "}", true);
    checkSyntax("Array Test #6", "namespace Test {"\
        "static void main() {"\
        "   i64[4] myArray;"\
        "   myArray[2 = 4;"\
        "}"\
        "}", false);
    checkSyntax("Array Test #7", "namespace Test {"\
        "struct test{"\
        "   i64[2] x;"\
        "}"\
        "static void main() {"\
        "   test t;"\
        "   t.x[0] = 5;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #8", "namespace Test {"\
        "struct test{"\
        "   i64[2] x;"\
        "}"\
        "static void main() {"\
        "   test t;"\
        "   i64 x = t.x[0];"\
        "}"\
        "}", true);
    checkSyntax("Array Test #9", "namespace Test {"\
        "static void main() {"\
        "   i64[5] t;"\
        "   i64[9] s = t;"\
        "}"\
        "}", false);
    checkSyntax("Array Test #10", "namespace Test {"\
        "struct test{"\
        "   i64 x;"\
        "   i64 y;"\
        "}"\
        "static void main() {"\
        "   test[5] t;"\
        "   t[0].x = 1;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #11", "namespace Test {"\
        "struct test{"\
        "   i64 x;"\
        "   i64 y;"\
        "}"\
        "static void main() {"\
        "   test[5] t;"\
        "   i64 p = 3;"
        "   t[p].x = 1;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #12", "namespace Test {"\
        "struct test{"\
        "   i64[4] x;"\
        "   i64 y;"\
        "}"\
        "static void main() {"\
        "   test[5] t;"\
        "   i64 p = 3;"
        "   t[p].x[0] = 1;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #13", "namespace Test {"\
        "struct test{"\
        "   i64[4] x;"\
        "   i64 y;"\
        "}"\
        "static void main() {"\
        "   test[5] t;"\
        "   i64 p = 3;"
        "   t[p].x[0 = 1;"\
        "}"\
        "}", false);
    checkSyntax("Array Test #14", "namespace Test {"\
        "struct test{"\
        "   i64[4] x;"\
        "   i64 y;"\
        "}"\
        "static void test(test t){"\
        "   i64 y = t.y;"\
        "}"\
        "static void main() {"\
        "   test[5] t;"\
        "   t.y = 4;"\
        "   test(t);"\
        "}"\
        "}", false);
    checkSyntax("Array Test #15", "namespace Test {"\
        "extern u8* test();"
        "static void main() {"\
        "   u8[] arr;"\
        "   arr = (u8[12])test();"
        "}", false);
    checkSyntax("Array Test #16", "namespace Test {"\
        "static void main() {"\
        "   i64[8] arr;"\
        "   arr[0] = 5;"\
        "   arr[0] += 2;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #17", "namespace Test {"\
        "struct test { i64 x; }"\
        "static void main() {"\
        "   test[8] arr;"\
        "   arr[0].x = 5;"\
        "   arr[0].x += 2;"\
        "}"\
        "}", true);
    checkSyntax("Array Test #18", "namespace Test {"\
        "static void main() {"\
        "   i64[-1] arr;"\
        "}"\
        "}", false);
    checkSyntax("Pointers Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 900;"\
        "   i64* y = &x;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #2", "namespace Test {"\
        "static void test(i64* q) {"\
        "   i64 p = *q;"\
        "}"\
        "static void main() {"\
        "   i64 x = 900;"\
        "   test(&x);"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #3", "namespace Test {"\
        "struct test { i64 p; }"\
        "static void main() {"\
        "   test x;"\
        "   test* y = &x;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #4", "namespace Test {"\
        "struct test { i64 p; }"\
        "static void main() {"\
        "   test x;"\
        "   test* y = &x;"\
        "   test z = *y;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #5", "namespace Test {"\
        "static void main() {"\
        "   i64 x;"\
        "   i64* y = &x;"\
        "   *y = 6;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #6", "namespace Test {"\
        "struct test { i64 p; }"\
        "static void main() {"\
        "   test x;"\
        "   test* y = &x;"\
        "   test z;"\
        "   *y = z;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #7", "namespace Test {"\
        "static void main() {"\
        "   i64 x;"\
        "   i64* y = &x;"\
        "   *y ;"\
        "}"\
        "}", false);
    checkSyntax("Pointers Test #8", "namespace Test {"\
        "static void main() {"\
        "   i64 x;"\
        "   i64* y = &x;"\
        "   *y = ;"\
        "}"\
        "}", false);
    checkSyntax("Pointers Test #9", "namespace Test {"\
        "static i64* test(i64* x) {"\
        "   return x;"\
        "}"
        "static void main() {"\
        "   i64 x;"\
        "   i64* y = &x;"\
        "   i64* w = test(y);"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #10", "namespace Test {"\
        "static void main() {"\
        "   i64* x = null;"\
        "}"\
        "}", true);
    checkSyntax("Pointers Test #11", "namespace Test {"\
        "struct test {"\
        "   i64 x;"\
        "}"\
        "static void main() {"\
        "   test x;"\
        "   test* y = &x;"\
        "   i64 z = y.x;"\
        "}"\
        "}", true);
    checkSyntax("Cast Test #1", "namespace Test {"\
        "static void main() {"\
        "   i32 id = (i32)10;"\
        "}"\
        "}", true);
    checkSyntax("Cast Test #2", "namespace Test {"\
        "static void main() {"\
        "   i32 id = (i32)5 + (i32)10;"\
        "}"\
        "}", true);
    checkSyntax("Cast Test #3", "namespace Test {"\
        "static void main() {"\
        "   i32 id = (i32)true + (i32)10;"\
        "}"\
        "}", false);
    checkSyntax("Cast Test #4", "namespace Test {"\
        "static void main() {"\
        "   boolean id = (boolean)10;"\
        "}"\
        "}", false);
    checkSyntax("Cast Test #5", "namespace Test {"\
        "static void main() {"\
        "   boolean id = (boolean)true;"\
        "}"\
        "}", true);
    checkSyntax("Cast Test #6", "namespace Test {"\
        "static void main() {"\
        "   u8 q = 100b;"\
        "   u8* p = &q;"\
        "   i8* w = (i8*)p;"\
        "}"\
        "}", true);
    checkSyntax("Cast Test #7", "namespace Test {"\
        "static void main() {"\
        "   u8* q;"\
        "   u8[] p = (u8[1])q;"\
        "}"\
        "}", true);
    checkSyntax("FFI Test #1", "namespace Test {"\
        "extern i32 GetCurrentProcess(); "\
        "static void main() {"\
        "   i32 id = GetCurrentProcess();"\
        "}"\
        "}", true);
    checkSyntax("FFI Test #2", "namespace Test {"\
        "extern i32 MyFunction(i64 x, i64 y, i64 z); "\
        "static void main() {"\
        "   i32 id = MyFunction(0, 1, 2);"\
        "}"\
        "}", true);
    checkSyntax("Sizeof Test #1", "namespace Test {"\
        "static void main() {"\
        "   u64 size = sizeof(i32);"\
        "}"\
        "}", true);
    checkSyntax("Sizeof Test #2", "namespace Test {"\
        "struct test { i32 x; }"\
        "static void main() {"\
        "   u64 size = sizeof(test);"\
        "}"\
        "}", true);
    checkSyntax("Sizeof Test #3", "namespace Test {"\
        "static void main() {"\
        "   u64 size = sizeof(boolean);"\
        "}"\
        "}", true);
    checkSyntax("Sizeof Test #4", "namespace Test {"\
        "static void main() {"\
        "   u64 size = sizeof(foo);"\
        "}"\
        "}", false);
    checkSyntax("Sizeof Test #5", "namespace Test {"\
        "static void main() {"\
        "   u64 size = sizeof("\
        "}"\
        "}", false);
    checkSyntax("Sizeof Test #6", "namespace Test {"\
        "static void main() {"\
        "   u64 size = sizeof(i32"\
        "}"\
        "}", false);
    checkSyntax("Bitshift Test #1", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 1 << 4;"\
        "}"\
        "}", true);
    checkSyntax("Bitshift Test #2", "namespace Test {"\
        "static void main() {"\
        "   i64 x = 8 >> 2;"\
        "}"\
        "}", true);

}

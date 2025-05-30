#include "poSyntaxTests.h"
#include "poLex.h"
#include "poParser.h"
#include "poTypeChecker.h"
#include <string>
#include <iostream>

using namespace po;

static void checkSyntax(const std::string& testName, const std::string& program, bool success)
{
    std::cout << testName << " ";

    poLexer lexer;
    lexer.tokenizeText(program);
    if (lexer.isError())
    {
        if (success)
        {
            std::cout << "FAILED " << lexer.errorText() << " Line:" << lexer.lineNum() << std::endl;
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
            std::cout << "FAILED " << parser.error() << " Line:" << parser.errorLine() << std::endl;
        }
        else
        {
            std::cout << "OK" << std::endl;
        }
        return;
    }

    std::vector<poNode*> nodes;
    nodes.push_back(ast);

    poTypeChecker checker;
    if (!checker.check(nodes))
    {
        if (success)
        {
            std::cout << "FAILED " << checker.errorText() << std::endl;
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
    checkSyntax("Array Test #1", "namespace Test {"\
        "static void main"\
        "   i64[10] myArray;"\
        "}"\
        "}", true);
}

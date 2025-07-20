#include "poDump.h"
#include "poCOFF.h"
#include "poPE.h"

#include <iostream>
#include <cstring>

using namespace po;

const char* const getExtension(const char* const filename)
{
    const size_t len = strlen(filename);
    for (size_t i = len - 1; i >= 0; i--)
    {
        if (filename[i] == '.')
        {
            return filename + i;
        }
    }

    return filename;
}

int main(const int numArgs, const char** const args)
{
    if (numArgs > 1)
    {
        const char* const extension = getExtension(args[1]);
        if (strcmp(extension, ".dll") == 0)
        {
            poPortableExecutable pe;
            if (pe.open(args[1]))
            {
                std::cout << "DLL file opened." << std::endl;
                pe.dumpExports();
            }
        }
        else if (strcmp(extension, ".lib") == 0)
        {
            poCommonObjectFileFormat coff;
            if (coff.open(args[1]))
            {
                std::cout << "Import LIB opened" << std::endl;
                coff.dump();
            }
        }
        else
        {
            std::cout << "File extension not supported" << std::endl;
        }
    }

    return 0;
}


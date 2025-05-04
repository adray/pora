#include "poApp.h"
#include "poPE.h"
#include "poCompiler.h"
#include <iostream>
#include <string>

using namespace po;


int main(const int numArgs, const char** const args)
{
    std::cout << "Pora Compiler" << std::endl;
    
    if (numArgs <= 1)
    {
        return 0;
    }

    const std::string option = args[1];

    if (option == "build")
    {
        poCompiler compiler;

        for (int i = 2; i < numArgs; i++)
        {
            const std::string file = args[i];
            std::cout << "Compiling " << file << std::endl;
            compiler.addFile(file);
        }

        if (compiler.compile() == 0)
        {
            for (auto& error : compiler.errors())
            {
                std::cout << error << std::endl;
            }
            return 0;
        }

        if (compiler.assembler().entryPoint() == -1)
        {
            std::cout << "No entry point defined." << std::endl;
            return 0;
        }

        const std::vector<unsigned char>& programData = compiler.assembler().programData();
        const std::vector<unsigned char>& initializedData = compiler.assembler().initializedData();

        // Create the data sections
        poPortableExecutableSection text(poSectionType::TEXT);
        text.data().resize(1024);

        poPortableExecutableSection initialized(poSectionType::INITIALIZED);
        initialized.data().resize(1024);

        poPortableExecutableSection uninitialized(poSectionType::UNINITIALIZED);
        uninitialized.data().resize(1024);

        poPortableExecutableSection iData(poSectionType::IDATA);
        iData.data().resize(1024);

        // Final linking..
        compiler.assembler().link(0, 0x1000 /* image alignment? */);

        // Write program data
        std::memcpy(text.data().data(), programData.data(), programData.size());
        std::memcpy(initialized.data().data(), initializedData.data(), initializedData.size());

        // Write the executable file
        poPortableExecutable exe;
        exe.setEntryPoint(compiler.assembler().entryPoint());
        exe.addSection(text);
        exe.addSection(initialized);
        exe.addSection(uninitialized);
        exe.addSection(iData);
        exe.addDataSections();
        exe.write("app.exe");

        std::cout << "Program compiled successfully: app.exe" << std::endl;
    }

    return 0;
}

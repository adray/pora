#include "poApp.h"
#include "poPE.h"
#include "poCompiler.h"
#include "poCOFF.h"
#include "poELF.h"
#include <iostream>
#include <string>
#include <cstring>
#include <filesystem>

using namespace po;

#ifdef WIN32
static bool openLibraryFile(const std::string& fileName, poCommonObjectFileFormat& coff)
{
    const std::vector<std::string> searchPaths = {
        "c:\\Program Files (x86)\\Windows Kits\\10\\Lib",
        "c:\\Program Files\\Windows Kits\\10\\Lib"
    };

    std::string targetDir;
    for (const std::string& path : searchPaths)
    {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
        {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir))
        {
            if (entry.is_directory())
            {
                const std::string fullPath = entry.path().string() + "\\um\\x64\\" + fileName;
                if (!std::filesystem::exists(fullPath))
                {
                    continue;
                }

                /* Choose the most recent version of the windows kits */
                if (targetDir.empty())
                {
                    targetDir = fullPath;
                }
                else if (fullPath > targetDir)
                {
                    targetDir = fullPath;
                }
            }
        }
    }

    if (coff.open(targetDir))
    {
        std::cout << "Loaded " << targetDir << std::endl;
        return true;
    }

    std::cout << "Failed to open " << fileName << std::endl;
    return false;
}
#else
static bool openLibraryFile(const std::string& fileName, poELF& elf)
{
    std::filesystem::path dir("/usr/lib");
    if (!std::filesystem::exists(dir) || !std::filesystem::is_directory(dir))
    {
        std::cout << "Failed to open " << fileName << std::endl;
        return false;
    }

    int version = -1;
    std::filesystem::path path;
    for (const std::filesystem::path& file : std::filesystem::directory_iterator(dir))
    {
        const std::string& candidate = file.filename().string();
        const std::string& substring = candidate.substr(0, fileName.size());

        if (substring == fileName)
        {
            if (version == -1 &&
                file.extension().string() == ".so")
            {
                path = file;
                version = 0;
            }
            else
            {
                const std::string& extension = file.extension().string();
                if (extension.size() >= 2)
                {
                    const int newVersion = std::atoi(extension.substr(1).c_str());
                    if (newVersion > version)
                    {
                        version = newVersion;
                        path = file;
                    }
                }
            }
        }
    }

    if (!path.empty())
    {
        if (elf.open(path.string()))
        {
            std::cout << "Loaded " << path.string() << std::endl;
            return true;
        }
    }

    return false;
}
#endif

static int align(const int size, const int alignment)
{
    if (size == 0)
    {
        return alignment;
    }

    const int remainder = size % alignment;
    if (remainder == 0)
    {
        return size;
    }
    return size + (alignment - remainder);
}

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
#if WIN32
        // Link the Windows libraries
        poCommonObjectFileFormat kernel32;
        if (!openLibraryFile("kernel32.lib", kernel32))
        {
            std::cout << "Failed to open kernel32.lib" << std::endl;
            return 0;
        }

        poCommonObjectFileFormat user32;
        if (!openLibraryFile("user32.lib", user32))
        {
            std::cout << "Failed to open user32.lib" << std::endl;
            return 0;
        }
#else
        // Link the C runtime
        poELF elf;
        if (!openLibraryFile("libc.so", elf))
        {
            std::cout << "Failed to open libc.so" << std::endl;
            return 0;
        }
#endif

        poCompiler compiler;

        for (int i = 2; i < numArgs; i++)
        {
            const std::string arg = args[i];
            if (arg == "/dump")
            {
                compiler.setDebugDump(true);
            }
            else
            {
                std::cout << "Compiling " << arg << std::endl;
                compiler.addFile(arg);
            }
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
        const std::vector<unsigned char>& readOnlyData = compiler.assembler().readOnlyData();
        std::unordered_map<std::string, int>& imports = compiler.assembler().imports();

        // Create portable executable
        poPortableExecutable exe;

        // Build the import tables
#ifdef WIN32
        const int kernel32ImportTable = exe.addImportTable("Kernel32.dll");
        const int user32ImportTable = exe.addImportTable("User32.dll");

        for (const poImport& import : kernel32.imports())
        {
            if (imports.find(import.importName()) != imports.end())
            {
                // TODO: we adding duplicate entries?
                poPortableExecutableImportTable& table = exe.importTable(kernel32ImportTable);
                table.addImport(import.importName(), import.importOrdinal());
            }
        }

        for (const poImport& import : user32.imports())
        {
            if (imports.find(import.importName()) != imports.end())
            {
                // TODO: we adding duplicate entries?
                poPortableExecutableImportTable& table = exe.importTable(user32ImportTable);
                table.addImport(import.importName(), import.importOrdinal());
            }
        }
#endif

        // Create the data sections
        exe.setEntryPoint(compiler.assembler().entryPoint());
        exe.addSection(poSectionType::TEXT, align(int(programData.size()), 1024));
        exe.addSection(poSectionType::INITIALIZED, align(int(initializedData.size()), 1024));
        exe.addSection(poSectionType::UNINITIALIZED, 1024);
        exe.addSection(poSectionType::IDATA, 1024 * 2);
        exe.addSection(poSectionType::READONLY, align(int(readOnlyData.size()), 1024));
        exe.initializeSections();
        
        // Final linking..
#ifdef WIN32
        for (poImportEntry& importEntry : exe.importTable(kernel32ImportTable).imports())
        {
            const auto& it = imports.find(importEntry.name());
            if (it != imports.end())
            {
                it->second = importEntry.addressRVA();
            }
        }

        for (poImportEntry& importEntry : exe.importTable(user32ImportTable).imports())
        {
            const auto& it = imports.find(importEntry.name());
            if (it != imports.end())
            {
                it->second = importEntry.addressRVA();
            }
        }
#endif
        compiler.assembler().link(0x1000, exe.initializedDataImagePos(), exe.readonlyDataImagePos());

        // Write program data
        std::memcpy(exe.textSection().data().data(), programData.data(), programData.size());
        std::memcpy(exe.initializedDataSection().data().data(), initializedData.data(), initializedData.size());
        std::memcpy(exe.readOnlyDataSection().data().data(), readOnlyData.data(), readOnlyData.size());

        // Write the executable file
        exe.write("app.exe");

        std::cout << "Program compiled successfully: app.exe" << std::endl;
    }

    return 0;
}

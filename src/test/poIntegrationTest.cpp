#include "poIntegrationTest.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>

#ifdef WIN32
#include <Windows.h>

static void safeClose(HANDLE handle)
{
    if (handle)
    {
        CloseHandle(handle);
    }
}

static bool executeCommand(const std::string& args, bool redirectOutput)
{
    HANDLE hStdOut = nullptr;
    if (redirectOutput)
    {
        SECURITY_ATTRIBUTES sa = {};
        sa.bInheritHandle = TRUE;
        sa.lpSecurityDescriptor = nullptr;
        sa.nLength = sizeof(SECURITY_ATTRIBUTES);

        hStdOut = CreateFileA(
            "output.txt",
            GENERIC_WRITE,
            FILE_SHARE_WRITE,
            &sa,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if (hStdOut == INVALID_HANDLE_VALUE)
        {
            return false;
        }
    }

    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hStdOut;
    
    BOOL res = CreateProcess(
        nullptr,
        const_cast<char*>(args.c_str()),
        nullptr,
        nullptr,
        TRUE,
        0,//CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);
    if (!res)
    {
        return false;
    }

    const int result = WaitForSingleObject(pi.hProcess, 30000);

    if (result == WAIT_TIMEOUT)
    {
        TerminateProcess(pi.hProcess, 0);
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    safeClose(pi.hProcess);
    safeClose(pi.hThread);
    safeClose(hStdOut);

    return WAIT_TIMEOUT != result;
}
#else
static bool executeCommand(const std::string& args, bool redirectOutput)
{
    // TODO
}

#endif

using namespace po;

static bool compareOutput(const std::string& sourceFile, const std::string& actualFile, const bool interactive)
{
    std::ifstream sourceStream(sourceFile);
    std::ifstream actualStream(actualFile);

    if (!actualStream.is_open())
    {
        if (interactive)
        {
            std::cout << "Unable to open " << actualFile << std::endl;
        }
        return false;
    }

    bool failed = false;
    if (!sourceStream.is_open())
    {
        if (interactive)
        {
            std::cout << "Unable to open " << sourceFile << std::endl;
        }
        failed = true;
    }

    std::string expected;
    std::string actual;
    while (!failed)
    {
        if (sourceStream.eof() && actualStream.eof())
        {
            break;
        }
        else if (sourceStream.eof() != actualStream.eof())
        {
            failed = true;
            break;
        }

        std::getline(sourceStream, expected);
        std::getline(actualStream, actual);
        
        if (expected != actual)
        {
            failed = true;

            if (interactive)
            {
                std::cout << "Output mismatch:" << std::endl;
                std::cout << "Expected: " << expected << std::endl;
                std::cout << "Actual:   " << actual << std::endl;
            }
        }
    }

    if (failed)
    {
        if (interactive)
        {
            std::cout << "Accept output? (y/n): ";

            std::string line;
            do
            {
                std::getline(std::cin, line);
            } while (line != "y" && line != "n");

            if (line == "y")
            {
                std::filesystem::copy(actualFile, sourceFile, std::filesystem::copy_options::overwrite_existing);
                return true;
            }
        }

        return false;
    }

    return true;
}

static void runIntegrationTest(const std::string& name, const std::string& path, const std::string& compiler, const std::string& std, const bool interactive)
{
    std::cout << "Integration Test " << name;

    if (std::filesystem::exists("app.exe"))
    {
        std::filesystem::remove("app.exe");
    }

    std::stringstream ss;
    ss << "\"" << compiler << "\" build \"" << path << "\"" <<
        " \"" << std << "\\os.po" << "\""
        " \"" << std << "\\io.po" << "\""
        " \"" << std << "\\string.po" << "\""
        " \"" << std << "\\string_class.po" << "\""
        " \"" << std << "\\clock.po" << "\""
        ;
    if (!executeCommand(ss.str(), false))
    {
        std::cout << " FAILED: compiler timed out or failed to start." << std::endl;
        return;
    }

    if (std::filesystem::exists("app.exe"))
    {
        if (executeCommand("app.exe", true))
        {
            /* Test that the application output is what is expected... */

            if (compareOutput(path + ".out", "output.txt", interactive))
            {
                std::cout << " SUCCESS" << std::endl;
            }
            else
            {
                std::cout << " FAILED: output mismatch." << std::endl;
            }
        }
        else
        {
            std::cout << " FAILED: application timed out or failed to start." << std::endl;
        }
    }
    else
    {
        std::cout << " FAILED: build failed." << std::endl;
    }
}

void po::runIntegrationTests(const std::string& testDir, const std::string& compiler, const std::string& std, const bool interactive)
{
    std::filesystem::path rootDir(testDir);
    if (std::filesystem::exists(rootDir))
    {
        std::filesystem::directory_iterator it(rootDir);
        for (auto& item : it)
        {
            if (item.is_directory())
            {
                std::filesystem::directory_iterator tests(item);
                for (auto& test : tests)
                {
                    const auto& path = test.path();
                    if (path.extension() == ".po")
                    {
                        runIntegrationTest(path.filename().string(), path.string(), compiler, std, interactive);
                    }
                }
            }
        }
    }
    else
    {
        std::cout << "Integration tests: FAILED Unable to find test directory." << std::endl;
    }
}


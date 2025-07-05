#include "poIntegrationTest.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>

#ifdef WIN32
#include <Windows.h>

static void safeClose(HANDLE handle)
{
    if (handle)
    {
        CloseHandle(handle);
    }
}

static bool executeCommand(const std::string& args)
{
    STARTUPINFOA si = {};
    PROCESS_INFORMATION pi = {};

    si.cb = sizeof(si);

    HRESULT res = CreateProcess(
        nullptr,
        const_cast<char*>(args.c_str()),
        nullptr,
        nullptr,
        FALSE,
        CREATE_NO_WINDOW,
        nullptr,
        nullptr,
        &si,
        &pi);
    if (FAILED(res))
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

    return WAIT_TIMEOUT != result;
}
#else
static bool executeCommand(const std::string& args)
{
    // TODO
}

#endif

using namespace po;

static void runIntegrationTest(const std::string& name, const std::string& path, const std::string& compiler)
{
    std::cout << "Integration Test " << name;

    if (std::filesystem::exists("app.exe"))
    {
        std::filesystem::remove("app.exe");
    }

    std::stringstream ss;
    ss << "\"" << compiler << "\" build \"" << path << "\"";
    if (!executeCommand(ss.str()))
    {
        std::cout << " FAILED: compiler timed out or failed to start." << std::endl;
        return;
    }

    if (std::filesystem::exists("app.exe"))
    {
        if (executeCommand("app.exe"))
        {
            /* TODO: Test that the application output is what is expected... */

            std::cout << " SUCCESS" << std::endl;
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

void po::runIntegrationTests(const std::string& testDir, const std::string& compiler)
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
                        runIntegrationTest(path.filename().string(), path.string(), compiler);
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


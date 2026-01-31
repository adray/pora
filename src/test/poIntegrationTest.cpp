#include "poIntegrationTest.h"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef WIN32
#include <Windows.h>

static void safeClose(HANDLE handle)
{
    if (handle)
    {
        CloseHandle(handle);
    }
}

static bool executeCommand(const std::vector<std::string>& args, bool redirectOutput)
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
 
    std::stringstream ss;
    for (const std::string& arg : args) {
        ss << "\"" << arg << "\"";
    }

    BOOL res = CreateProcess(
        nullptr,
        const_cast<char*>(ss.str().c_str()),
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
        safeClose(hStdOut);
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
 
static bool executeCommand(const std::vector<std::string>& args, bool redirectOutput)
{
    fflush(stdout); // flush before forking, otherwise child will inherit output
    const int pid = fork();
    if (pid == 0) {
        if (redirectOutput) {
            const int fd = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR);
            dup2(fd, STDOUT_FILENO);
        } else {
            const int fd = open("/dev/null", O_APPEND);
            dup2(fd, STDOUT_FILENO);
        }
        std::vector<char*> cmd;
        for (const std::string& arg : args) {
            cmd.push_back(const_cast<char*>(arg.data()));
        }
        cmd.push_back(nullptr);

        const int result = execv(args[0].c_str(), cmd.data());
        if (result == -1) {
            std::cout << "Process error";
        }
        fflush(stdout);
        exit(result);
    } else if (pid == -1) {
        return false;
    } else {
        // Wait for child process to terminate
        int status;
        waitpid(pid, &status, 0);
    }

    return true;
}

#endif

using namespace po;

// Compare strings ignoring line endings
static int compareStrings(const std::string& str1, const std::string& str2) {
    size_t pos = 0;
    while (pos < str1.size() && pos < str2.size()) {
        if (str1[pos] != str2[pos]) {
            return int(pos);
        }

        pos++;
    }
    
    if (std::abs(int(str1.size() - str2.size())) > 1) {
        return int(pos);
    }

    if (str1.size() > str2.size()) {
        if (str1[pos] != '\r') {
            return pos;
        }
    } else if (str2.size() > str1.size()) {
        if (str2[pos] != '\r') {
            return pos;
        }
    }

    return -1;
}

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
        
        const int pos = compareStrings(expected, actual);
        if (pos != -1)
        {
            failed = true;

            if (interactive)
            {
                std::cout << "Output mismatch:" << std::endl;
                std::cout << "Expected: " << expected << std::endl;
                std::cout << "Actual:   " << actual << std::endl;
                std::cout << "Error Pos: " << pos << std::endl;
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

#ifdef WIN32
    const std::vector<std::string> app = { "app.exe" };
#else
    const std::vector<std::string> app = { "./app" };
#endif
    if (std::filesystem::exists(app[0]))
    {
        std::filesystem::remove(app[0]);
    }

#ifdef WIN32
    const std::string os = "win\\";
    const std::string dir = std + "\\";
#else
    const std::string os = "unix/";
    const std::string dir = std + "/";
#endif

    std::vector<std::string> args;
    args.push_back(compiler);
    args.push_back("build");
    args.push_back(path);
    args.push_back(dir + os + "os.po");
    args.push_back(dir + os + "io.po");
    args.push_back(dir + "string.po");
    args.push_back(dir + "string_class.po");
    args.push_back(dir + os + "clock.po");
    args.push_back(dir + os + "memory.po");
    //args.push_back(dir + "pora.po");
    //args.push_back(dir + os + "control.po");
    args.push_back(dir + "calendar.po");
    args.push_back(dir + os + "date.po");

    if (!executeCommand(args, false))
    {
        std::cout << " FAILED: compiler timed out or failed to start." << std::endl;
        return;
    }

    if (std::filesystem::exists(app[0]))
    {
#ifndef WIN32
        // Set the binary to executable
        const int result = chmod(app[0].c_str(), S_IXUSR | S_IRUSR);
        if (result == -1) {
            std::cout << "Unable to give execution pemissions." << std::endl;
            return;
        }
#endif

        if (executeCommand(app, true))
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


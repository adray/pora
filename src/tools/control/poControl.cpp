#include "poControl.h"

#include <Windows.h>
#include <iostream>
#include <string>

struct sharedMemoryHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t command_count;
    uint32_t buffer_offset;
    int64_t mutex;
    uint8_t reserved[32];
};

static void monitorProcess(const std::string& pid)
{
    // Open shared memory using the name of the process ID
    HANDLE hMapFile = OpenFileMappingA(
        FILE_MAP_ALL_ACCESS,
        false,
        pid.c_str());
    // Map the shared memory to the process's address space
    LPVOID pBuf = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(sharedMemoryHeader));
    if (pBuf == nullptr) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return;
    }

    sharedMemoryHeader* header = reinterpret_cast<sharedMemoryHeader*>(pBuf);
    std::cout << header->magic << " " << header->buffer_offset << std::endl;

    std::cin >> std::ws; // Wait for input to keep the process alive
}

static void run()
{
    const int pid = GetCurrentProcessId();
    std::cout << "Process ID: " << pid << std::endl;

    // Create shared memory using the name of the process ID
    HANDLE hMapFile = CreateFileMappingA(
        INVALID_HANDLE_VALUE,
        nullptr,
        PAGE_READWRITE,
        0,
        256,
        std::to_string(pid).c_str());
    if (hMapFile == nullptr) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return;
    }

    // Map the shared memory to the process's address space
    LPVOID pBuf = MapViewOfFile(
        hMapFile,
        FILE_MAP_ALL_ACCESS,
        0,
        0,
        sizeof(sharedMemoryHeader));
    if (pBuf == nullptr) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return;
    }
}

int main(int argc, char** argv)
{
    if (argc > 1) {
        monitorProcess(argv[1]);
    } else {
        run();
    }

    return 0;
}

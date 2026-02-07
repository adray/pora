#pragma once
#include <string>

namespace po
{
    void runIntegrationTests(const std::string& testDir, const std::string& compiler, const std::string& std, const bool interactive, const std::string& optimizationLevel);
}

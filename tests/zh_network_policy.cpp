#include "../src/libslic3r/NetworkPolicy.hpp"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

int main(int argc, char** argv)
{
    using namespace Slic3r::NetworkPolicy;
    const std::string mode = argc > 1 ? argv[1] : "default";

    if (mode == "default") {
        require(!is_offline(), "network policy must default to online");
        require(!cli_offline.load() && !config_offline.load(), "default sources must be false");
    } else if (mode == "cli") {
        enable_cli_offline();
        require(is_offline(), "CLI offline source must enable the policy");
        require(cli_offline.load() && !config_offline.load(), "CLI source must remain independent");
    } else if (mode == "config") {
        enable_config_offline();
        require(is_offline(), "persisted offline source must enable the policy");
        require(!cli_offline.load() && config_offline.load(), "config source must remain independent");
    } else if (mode == "both") {
        enable_config_offline();
        enable_cli_offline();
        require(is_offline(), "CLI and config sources must combine with OR semantics");
        require(cli_offline.load() && config_offline.load(), "both sources must stay enabled");
    } else if (mode == "threads") {
        std::atomic<bool> observed{false};
        std::vector<std::thread> readers;
        for (int i = 0; i < 4; ++i) {
            readers.emplace_back([&observed] {
                for (int j = 0; j < 10000; ++j)
                    observed = observed.load() || is_offline();
            });
        }
        enable_config_offline();
        enable_cli_offline();
        for (auto& reader : readers)
            reader.join();
        require(is_offline() && observed.load(), "policy publication must be visible across threads");
    } else {
        throw std::runtime_error("unknown network policy test mode");
    }

    std::cout << "ZH network policy " << mode << " test passed.\n";
}

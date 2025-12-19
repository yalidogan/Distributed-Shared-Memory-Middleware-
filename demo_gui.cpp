#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>
#include <iomanip>
#include <atomic>

// --- Project Includes ---
#include "include/dsm/DsmCore.h"
#include "include/sync/LockManager.h"
#include "include/net/GrpcDsmNetwork.h"
#include "include/net/DsmServiceImpl.h"
#include "src/utils/Config.h"

using namespace dsm;

// --- Global Sync ---
std::atomic<bool> g_running{true};
std::string g_current_status = "IDLE";
std::string g_last_printed_status = "";
std::mutex g_io_mutex; // Protects cout so lines don't get mixed

// --- Helpers ---
void logStatus(const std::string& msg) {
    std::lock_guard<std::mutex> lock(g_io_mutex);
    std::cout << "\n[STATUS] " << msg << std::endl;
    std::cout << "> " << std::flush; // Re-print prompt
}

void printCache(DsmCore* core) {
    std::lock_guard<std::mutex> lock(g_io_mutex);
    auto cache = core->debugGetCache();

    std::cout << "\n--- LOCAL CACHE ---" << std::endl;
    if (cache.empty()) {
        std::cout << "(Empty)" << std::endl;
    }
    for (const auto& kv : cache) {
        std::cout << "  " << std::left << std::setw(15) << kv.first.str()
                  << " : " << kv.second << std::endl;
    }
    std::cout << "-------------------\n> " << std::flush;
}

// --- Network Server ---
void RunServer(std::shared_ptr<DsmServiceImpl> service, std::string address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
}

// --- Background Monitor (Non-Intrusive) ---
// This thread checks if the status changed (e.g., "Waiting for Lock...")
// and prints it ONCE. It won't spam your screen.
void monitorStatus(DsmCore* core) {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        if (g_current_status != g_last_printed_status) {
            logStatus(g_current_status);
            g_last_printed_status = g_current_status;
        }
    }
}

// --- Main Interactive Shell ---
void runShell(DsmCore* core) {
    std::string line, cmd, key, val;

    std::cout << "\n--- DSM INTERACTIVE CONSOLE ---" << std::endl;
    std::cout << "Commands: get <k>, put <k> <v>, slowput <k> <v>, exit" << std::endl;
    std::cout << "> " << std::flush;

    while (g_running && std::getline(std::cin, line)) {
        if (line.empty()) {
            std::cout << "> " << std::flush;
            continue;
        }

        std::stringstream ss(line);
        ss >> cmd;

        g_current_status = "Processing " + cmd + "...";

        try {
            if (cmd == "exit") {
                g_running = false;
                break;
            }
            else if (cmd == "get") {
                if (!(ss >> key)) {
                    std::cout << "Usage: get <key>" << std::endl;
                } else {
                    g_current_status = "Acquiring READ lock (" + key + ")...";
                    auto h = core->getRead<std::string>(ObjectId(key));
                    g_current_status = "Reading Data...";
                    std::cout << "   [RESULT] " << key << " = " << h.get() << std::endl;
                }
            }
            else if (cmd == "put") {
                if (!(ss >> key >> val)) {
                    std::cout << "Usage: put <key> <val>" << std::endl;
                } else {
                    g_current_status = "Acquiring WRITE lock (" + key + ")...";
                    {
                        auto h = core->getWrite<std::string>(ObjectId(key));
                        *h = val;
                    }
                    std::cout << "   [RESULT] Write Committed." << std::endl;
                }
            }
            else if (cmd == "slowput") {
                if (!(ss >> key >> val)) {
                    std::cout << "Usage: slowput <key> <val>" << std::endl;
                } else {
                    g_current_status = "Acquiring WRITE lock (Will hold 5s)...";
                    {
                        auto h = core->getWrite<std::string>(ObjectId(key));

                        // Artificial Delay Loop
                        for(int i=5; i>0; i--) {
                            g_current_status = "LOCKED (" + key + ") by ME. Releasing in " + std::to_string(i) + "s...";
                            std::this_thread::sleep_for(std::chrono::seconds(1));
                        }

                        *h = val;
                    } // Lock releases here
                    std::cout << "   [RESULT] Slow Write Committed." << std::endl;
                }
            }
            else {
                std::cout << "Unknown command." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "   [ERROR] " << e.what() << std::endl;
        }

        g_current_status = "IDLE";

        // Auto-print cache after any operation to show state
        printCache(core);
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: ./dsm_gui <my_id> <config_file>" << std::endl;
        return 1;
    }

    int my_id = std::stoi(argv[1]);
    std::string config_path = argv[2];

    // Setup
    ClusterConfig config;
    config.load(config_path);
    NodeInfo my_info = config.getMyInfo(my_id);

    auto net = new GrpcDsmNetwork(my_id);
    auto lock = new LockManager();
    auto core = new DsmCore(my_id, config.nodes.size(), lock, net);
    auto service = std::make_shared<DsmServiceImpl>(core);

    std::thread server_thread(RunServer, service, "0.0.0.0:" + std::to_string(my_info.port));

    // Simple startup message
    std::cout << "Node " << my_id << " started. Waiting 5s for peers..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    for (const auto& [id, info] : config.nodes) {
        if (id == my_id) continue;
        net->connect(id, info.getAddress());
    }

    // Start Monitor Thread
    std::thread mon_thread(monitorStatus, core);

    // Run Shell
    runShell(core);

    // Cleanup
    g_running = false;
    mon_thread.join();
    // Force exit to kill server
    exit(0);
}
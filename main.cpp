#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

// --- Project Includes ---
#include "include/dsm/DsmCore.h"
#include "include/sync/LockManager.h"
#include "include/net/GrpcDsmNetwork.h"
#include "include/net/DsmServiceImpl.h"
#include "src/utils/Config.h"

using namespace dsm;

// --------------------------------------------------------------------------
// Helper: Start the Server (Background)
// --------------------------------------------------------------------------
void RunServer(std::shared_ptr<DsmServiceImpl> service, std::string address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
        std::cerr << "[Server] Failed to bind to " << address << std::endl;
        exit(1);
    }
    server->Wait();
}

// --------------------------------------------------------------------------
// TEST 1: Automated Handle Logic (Flag Synchronization)
// --------------------------------------------------------------------------
void runHandleTest(DsmCore* core, int my_id) {
    ObjectId counter_id("smart_counter");
    ObjectId flag_init("flag_node0_initialized");
    ObjectId flag_done("flag_node1_updated");

    if (my_id == 0) {
        // 1. Init
        {
            std::cout << "[Node 0] Acquiring Write Handle (Init)..." << std::endl;
            auto h = core->getWrite<int>(counter_id);
            *h = 100;
        }
        // 2. Signal Ready
        {
            auto h = core->getWrite<int>(flag_init);
            *h = 1;
            std::cout << "[Node 0] Set flag_init = 1. Waiting for Node 1..." << std::endl;
        }
        // 3. Wait for Node 1
        while (true) {
            auto h = core->getRead<int>(flag_done);
            if (h.get() == 1) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        // 4. Verify
        {
            auto h = core->getRead<int>(counter_id);
            std::cout << "[Node 0] Final Read: " << h.get() << std::endl;
            if (h.get() == 150) std::cout << "[PASS] Value is 150." << std::endl;
            else std::cout << "[FAIL] Expected 150, got " << h.get() << std::endl;
        }

    } else {
        // Node 1: Wait for Init
        std::cout << "[Node 1] Waiting for Node 0 flag..." << std::endl;
        while (true) {
            auto h = core->getRead<int>(flag_init);
            if (h.get() == 1) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Update
        {
            std::cout << "[Node 1] Acquiring WRITE Handle..." << std::endl;
            auto h = core->getWrite<int>(counter_id);
            *h += 50;
        }

        // Signal Done
        {
            auto h = core->getWrite<int>(flag_done);
            *h = 1;
            std::cout << "[Node 1] Set flag_done = 1." << std::endl;
        }
    }
}

// --------------------------------------------------------------------------
// TEST 2: Interactive CLI (New Handle API)
// --------------------------------------------------------------------------
void runInteractive(DsmCore* core, int my_id) {
    std::string cmd;
    std::cout << ">>> Interactive Mode. Commands:" << std::endl;
    std::cout << "    get <key>" << std::endl;
    std::cout << "    put <key> <value>" << std::endl;
    std::cout << "    rm  <key>" << std::endl;
    std::cout << "    slowput <key> <value>  (Simulate long write 10 seconds)" << std::endl;
    std::cout << "    slowget <key>          (Simulate long read 10 seconds)" << std::endl;

    while (true) {
        std::cout << "Node " << my_id << "> ";
        std::cin >> cmd;

        if (cmd == "get") {
            std::string key; std::cin >> key;
            try {
                // Scope 1: Read-Only Access
                // This acquires the lock, checks local cache, fetches if needed.
                auto h = core->getRead<std::string>(ObjectId(key));
                std::cout << "   [READ] " << key << " = '" << h.get() << "'" << std::endl;
            } catch (const std::exception& e) {
                std::cout << "   [ERROR] " << e.what() << std::endl;
            }
        }
        else if (cmd == "put") {
            std::string key, val; std::cin >> key >> val;
            try {
                // Scope 2: Read-Write Access
                // This acquires lock.
                auto h = core->getWrite<std::string>(ObjectId(key));
                // Update local value
                *h = val;
                // Destructor runs here: Sends update to Home + Releases Lock
                std::cout << "   [WRITE] " << key << " <- " << val << " committed." << std::endl;
            } catch (const std::exception& e) {
                std::cout << "   [ERROR] " << e.what() << std::endl;
            }
        }
        else if (cmd == "slowput") {
            std::string key, val; std::cin >> key >> val;
            try {
                std::cout << "   [SLOW WRITE] Simulating long WRITE operation for 10 seconds" << std::endl;
                auto h = core->getWrite<std::string>(ObjectId(key));
                *h = val;
                std::cout << "   [SLOW WRITE] " << key << " <- " << val << " committed." << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(10));
            } catch (const std::exception& e) {
                std::cout << "   [ERROR] " << e.what() << std::endl;
            }
        }
        else if (cmd == "slowget") {
            std::string key; std::cin >> key;
            try {
                std::cout << "   [SLOW READ] Simulating long READ operation for 10 seconds" << std::endl;
                auto h = core->getRead<std::string>(ObjectId(key));
                std::cout << "   [SLOW READ] " << key << " = '" << h.get() << "'" << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(10));
            } catch (const std::exception& e) {
                std::cout << "   [ERROR] " << e.what() << std::endl;
            }
        }
        else if (cmd == "rm") {
            std::string key; std::cin >> key;
            core->remove(ObjectId(key));
            std::cout << "   [REMOVE] " << key << " deleted." << std::endl;
        }
        else {
            std::cout << "Unknown command." << std::endl;
        }
    }
}

// --------------------------------------------------------------------------
// Main Setup
// --------------------------------------------------------------------------
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: ./dsm_node <my_id> <config_file> [mode]" << std::endl;
        std::cerr << "Modes: handle (default), interactive" << std::endl;
        return 1;
    }

    int my_id = std::stoi(argv[1]);
    std::string config_path = argv[2];
    std::string mode = (argc > 3) ? argv[3] : "handle";

    // 1. Load Config
    ClusterConfig config;
    config.load(config_path);
    NodeInfo my_info = config.getMyInfo(my_id);

    // 2. Initialize Components
    auto net = new GrpcDsmNetwork(my_id);
    auto lock = new LockManager();
    auto core = new DsmCore(my_id, config.nodes.size(), lock, net);
    auto service = std::make_shared<DsmServiceImpl>(core);

    // 3. Start Server
    std::string bind_address = "0.0.0.0:" + std::to_string(my_info.port);
    std::cout << "[System] Node " << my_id << " listening on " << bind_address << std::endl;
    std::thread server_thread(RunServer, service, bind_address);

    // 4. Connection Phase (Wait for everyone to start)
    std::cout << "[System] Waiting 5 seconds for cluster startup..." << std::endl;
    std::cout << "         (Start other nodes NOW if you haven't!)" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));

    std::cout << "[System] Connecting to peers..." << std::endl;
    for (const auto& [id, info] : config.nodes) {
        if (id == my_id) continue;
        net->connect(id, info.getAddress());
    }

    std::cout << "[System] Connection phase complete. Waiting 2s for mesh stabilization..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "[System] Ready. Running mode: " << mode << std::endl;
    std::cout << "---------------------------------------------------" << std::endl;

    if (mode == "interactive") {
        runInteractive(core, my_id);
    } else {
        runHandleTest(core, my_id);
    }

    // Keep Alive
    std::cout << "\n[System] Session finished. Press ENTER to exit..." << std::endl;
    std::cin.get();
    if (std::cin.peek() == '\n') std::cin.get();
    std::cin.get();

    server_thread.detach();
    return 0;
}
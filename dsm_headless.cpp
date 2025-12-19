#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <mutex>
#include <sstream>
#include <atomic>

// --- Project Includes ---
#include "include/dsm/DsmCore.h"
#include "include/sync/LockManager.h"
#include "include/net/GrpcDsmNetwork.h"
#include "include/net/DsmServiceImpl.h"
#include "src/utils/Config.h"

using namespace dsm;

// Global State
std::atomic<bool> g_running{true};
std::mutex g_io_mutex;
int g_my_id = -1;
int g_total_nodes = 0;

// --- Hashing Logic (To determine Role) ---
inline uint32_t fnv1a_hash_gui(const std::string& str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

std::string getRole(const std::string& key) {
    uint32_t h = fnv1a_hash_gui(key);
    int home = static_cast<int>(h % g_total_nodes);
    int backup = static_cast<int>((h + 1) % g_total_nodes);

    if (home == g_my_id) return "HOME (Master)";
    if (backup == g_my_id) return "BACKUP";
    return "CACHE (Replica)";
}

// --- Helpers ---
void sendToGui(const std::string& type, const std::string& payload) {
    std::lock_guard<std::mutex> lock(g_io_mutex);
    // Format: "TYPE|PAYLOAD"
    std::cout << type << "|" << payload << std::endl;
}

// --- Status Monitor ---
void monitorLoop(DsmCore* core) {
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        auto current_cache = core->debugGetCache();

        for (const auto& kv : current_cache) {
            std::string key = kv.first.str();
            std::string val = kv.second;
            std::string role = getRole(key);

            // Format: CACHE|key|value|role
            sendToGui("CACHE", key + "|" + val + "|" + role);
        }
    }
}

// --- Server ---
void RunServer(std::shared_ptr<DsmServiceImpl> service, std::string address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, NULL, _IONBF, 0);

    if (argc < 3) return 1;
    g_my_id = std::stoi(argv[1]);
    std::string config_path = argv[2];

    // Setup
    ClusterConfig config;
    config.load(config_path);
    NodeInfo my_info = config.getMyInfo(g_my_id);
    g_total_nodes = config.nodes.size();

    auto net = new GrpcDsmNetwork(g_my_id);
    auto lock = new LockManager();
    auto core = new DsmCore(g_my_id, g_total_nodes, lock, net);
    auto service = std::make_shared<DsmServiceImpl>(core);

    std::thread server_thread(RunServer, service, "0.0.0.0:" + std::to_string(my_info.port));

    sendToGui("STATUS", "WAITING FOR PEERS (5s)...");
    std::this_thread::sleep_for(std::chrono::seconds(5));
    for (const auto& [id, info] : config.nodes) {
        if (id == g_my_id) continue;
        net->connect(id, info.getAddress());
    }
    sendToGui("STATUS", "IDLE");

    std::thread cache_thread(monitorLoop, core);

    // Command Loop
    std::string line, cmd, key, val;
    while (std::getline(std::cin, line)) {
        std::stringstream ss(line);
        ss >> cmd;

        try {
            if (cmd == "get") {
                ss >> key;
                sendToGui("STATUS", "LOCKING (READ)...");
                {
                    auto h = core->getRead<std::string>(ObjectId(key));
                    // CHECK IF FOUND: If it's not in store after a fetch, it doesn't exist.
                    if (core->exists(ObjectId(key))) {
                        sendToGui("RESULT", "Read: " + key + " = " + h.get());
                    } else {
                        sendToGui("RESULT", "Read: " + key + " NOT FOUND");
                    }
                }
                sendToGui("STATUS", "IDLE");
            }
            else if (cmd == "slowget") {
                ss >> key;
                sendToGui("STATUS", "READ LOCK HELD (5s)...");
                {
                    auto h = core->getRead<std::string>(ObjectId(key));
                    std::this_thread::sleep_for(std::chrono::seconds(5));

                    if (core->exists(ObjectId(key))) {
                        sendToGui("RESULT", "Read: " + key + " = " + h.get());
                    } else {
                        sendToGui("RESULT", "Read: " + key + " NOT FOUND");
                    }
                }
                sendToGui("STATUS", "IDLE");
            }
            else if (cmd == "put") {
                ss >> key >> val;
                sendToGui("STATUS", "LOCKING (WRITE)...");
                {
                    auto h = core->getWrite<std::string>(ObjectId(key));
                    *h = val;
                }
                sendToGui("RESULT", "Write Committed: " + key);
                sendToGui("STATUS", "IDLE");
            }
            else if (cmd == "slowput") {
                ss >> key >> val;
                sendToGui("STATUS", "WRITE LOCK HELD (5s)...");
                {
                    auto h = core->getWrite<std::string>(ObjectId(key));
                    std::this_thread::sleep_for(std::chrono::seconds(5));
                    *h = val;
                }
                sendToGui("RESULT", "Write Committed: " + key);
                sendToGui("STATUS", "IDLE");
            }
        } catch (const std::exception& e) {
            sendToGui("ERROR", e.what());
        }
    }

    g_running = false;
    exit(0);
}
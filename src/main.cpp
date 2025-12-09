#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <chrono>
#include <fstream>

// 1. Your Custom Server Wrapper
#include "server/server.hpp" 

// 2. Your DSM Logic (The "Ear")
#include "service/dsm_service.hpp" 

// 3. Your Network Client (The "Mouth")
#include "network/grpc_network.hpp"

// Namespace for convenience (so we don't have to type dsm:: every time)
using dsm::RaRequestMsg;
using dsm::DsmUpdateMsg;
using dsm::DsmFetchRequest;
using dsm::DsmFetchReply;

void RunServerThread(Server* server_ptr) {
    server_ptr->Start();
}

int main(int argc, char** argv) {
    if(argc < 2) {
        std::cout << "Usage: " << argv[0] << " <node_id>" << std::endl;
        return 1;
    }

    std::cout << argv[0] << " starting with Node ID: " << argv[1] << std::endl;
    int node_id = std::stoi(argv[1]);
    NodeConfig::getInstance().setNodeId(node_id);
    std::cout << "Assigned Node ID: " << node_id << std::endl;
    
    std::cout << "Finding self port from peers.txt" << std::endl;

    std::ifstream peers_file("peers.txt");
    if (!peers_file.is_open()) {
        std::cout << "Error: Could not open peers.txt" << std::endl;
        return 1;
    }
    
    std::string self_address;
    std::string read_address;
    int line_idx = 0;
    while (std::getline(peers_file, read_address)) {
        if (!read_address.empty() && line_idx == NodeConfig::getInstance().getNodeId()) {
            self_address = read_address;
            std::cout << "Found self-port at line " << line_idx << ": " << self_address << std::endl;
        }
        line_idx++;
    }
    peers_file.close();
    if (self_address.empty()) {
        std::cout << "Error: Could not find self port in peers.txt" << std::endl;
        return 1;
    }

    // 1. CONFIGURATION

    std::cout << "=== DSM Node Startup [" << self_address << "] ===" << std::endl;

    // 2. SERVER SETUP (Background Thread)
    // Create the Logic
    auto dsm_service = std::make_shared<DSMServiceImpl>();

    // Create the Server Wrapper
    Server my_server(self_address, dsm_service, "DSM_Node_" + self_address);

    // Launch Server in a detached thread so it doesn't block input
    std::thread server_thread(RunServerThread, &my_server);
    server_thread.detach(); 

    // Allow a moment for the server to spin up
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 3. CLIENT SETUP
    GrpcNetwork network;
    // If you have a peers.txt, uncomment this:
    // network.LoadPeers("peers.txt"); 

    // 4. COMMAND LOOP
    std::string command;
    while (true) {
        std::cout << "\n[" << self_address << "] > ";
        std::cin >> command;

        if (command == "help") {
            std::cout << "Commands:\n"
                      << "  connect                  -> Connect to peers in 'peers.txt'\n"
                      << "  ra <target_id>           -> Send Lock Request\n"
                      << "  update <id> <key> <val>  -> Write data to remote node\n"
                      << "  read <id> <key>          -> Fetch data from remote node\n"
                      << "  exit                     -> Quit" << std::endl;
        }
        else if (command == "connect") {
            std::cout << "Loading peers from peers.txt" << std::endl;

            std::ifstream peers_file("peers.txt");
            if (!peers_file.is_open()) {
                std::cout << "Error: Could not open peers.txt" << std::endl;
                continue;
            }
            
            std::string address;
            int line_idx = 0;
            while (std::getline(peers_file, address)) {
                if (!address.empty() && line_idx != NodeConfig::getInstance().getNodeId()) {
                    network.connect(line_idx, address);
                    std::cout << "Connected to Node " << line_idx << " at " << address << std::endl;
                }
                line_idx++;
            }
            peers_file.close();
            std::cout << "Loaded " << (line_idx - 1) << " peers from peers.txt" << std::endl;
        }
        else if (command == "ra") {
            int target_id;
            std::cin >> target_id;
            
            std::cout << "Sending RA Lock Request to Node " << target_id << "..." << std::endl;
            
            // TODO: RicAgrawala
        }
        else if (command == "update") {
            int target_id;
            std::string key, val;
            std::cin >> target_id >> key >> val;
            
            std::cout << "Writing '" << key << "=" << val << "' to Node " << target_id << "..." << std::endl;
            
            // Create Protobuf Message
            DsmUpdateMsg msg;
            msg.set_object_name(key);
            msg.set_data(val);
            
            network.sendDsmUpdate(target_id, msg);
        }
        else if (command == "read") {
            int target_id;
            std::string key;
            std::cin >> target_id >> key;
            
            std::cout << "Fetching '" << key << "' from Node " << target_id << "..." << std::endl;
            
            DsmFetchRequest req;
            req.set_object_name(key);
            
            // Send request and get response directly
            DsmFetchReply reply = network.sendDsmFetch(target_id, req);
            
            if (reply.found()) {
                std::cout << " -> [SUCCESS] Value: " << reply.data() << std::endl;
            } else {
                std::cout << " -> [FAILED] Object not found on remote node." << std::endl;
            }
        }
        else if (command == "exit") {
            break;
        }
        else {
            std::cout << "Unknown command. Type 'help'." << std::endl;
        }
    }

    // Clean shutdown
    my_server.Stop();
    return 0;
}
#include <iostream>
#include <memory>
#include <thread>
#include <string>
#include <chrono>

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
    // 1. CONFIGURATION
    std::string port = "50051";
    if (argc > 1) port = argv[1];
    std::string address = "0.0.0.0:" + port;

    std::cout << "=== DSM Node Startup [" << port << "] ===" << std::endl;

    // 2. SERVER SETUP (Background Thread)
    // Create the Logic
    auto dsm_service = std::make_shared<DSMServiceImpl>();

    // Create the Server Wrapper
    Server my_server(address, dsm_service, "DSM_Node_" + port);

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
        std::cout << "\n[" << port << "] > ";
        std::cin >> command;

        if (command == "help") {
            std::cout << "Commands:\n"
                      << "  connect <id> <ip:port>   -> Add a peer manually\n"
                      << "  ra <target_id>           -> Send Lock Request\n"
                      << "  update <id> <key> <val>  -> Write data to remote node\n"
                      << "  read <id> <key>          -> Fetch data from remote node\n"
                      << "  exit                     -> Quit" << std::endl;
        }
        else if (command == "connect") {
            int id;
            std::string ip;
            std::cin >> id >> ip;
            network.connect(id, ip);
            std::cout << "Connected to Node " << id << " at " << ip << std::endl;
        }
        else if (command == "ra") {
            int target_id;
            std::cin >> target_id;
            
            std::cout << "Sending RA Lock Request to Node " << target_id << "..." << std::endl;
            
            // Create Protobuf Message
            RaRequestMsg msg;
            msg.set_node_id(1); // My ID (Should be dynamic later)
            msg.set_timestamp(100); 
            
            network.sendRaRequest(target_id, msg); 
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
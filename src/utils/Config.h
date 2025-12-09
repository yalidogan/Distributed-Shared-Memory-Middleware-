#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>

namespace dsm {

    struct NodeInfo {
        int id;
        std::string ip;
        int port;

        std::string getAddress() const {
            return ip + ":" + std::to_string(port);
        }
    };

    class ClusterConfig {
    public:
        std::map<int, NodeInfo> nodes;

        void load(const std::string& filepath) {
            std::ifstream file(filepath);
            if (!file.is_open()) {
                std::cerr << "[Config] Error: Could not open " << filepath << std::endl;
                exit(1);
            }

            std::string line;
            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue; // Skip comments/empty lines

                std::stringstream ss(line);
                NodeInfo info;
                if (ss >> info.id >> info.ip >> info.port) {
                    nodes[info.id] = info;
                }
            }
            std::cout << "[Config] Loaded " << nodes.size() << " nodes from " << filepath << std::endl;
        }

        NodeInfo getMyInfo(int my_id) {
            if (nodes.find(my_id) == nodes.end()) {
                std::cerr << "[Config] Error: My ID (" << my_id << ") is not in the config file!" << std::endl;
                exit(1);
            }
            return nodes[my_id];
        }
    };

} // namespace dsm
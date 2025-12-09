#pragma once 

#include <string>
#include <cstdlib>
#include <map>
#include <memory>
#include "protos/dsm_proto/dsm.grpc.pb.h"

inline std::string get_env(const std::string &key, const std::string &default_value)
{
    const char* value = std::getenv(key.c_str());
    return value ? value  : default_value;
}

class NodeConfig {
private:
    static NodeConfig* instance_;
    int node_id_;
    std::string host_;
    std::string port_;
    std::map<int, std::unique_ptr<dsm::DSMService::Stub>> peers_;
    
    NodeConfig() : node_id_(1), host_("0.0.0.0"), port_("50051") {}
    
public:
    // Delete copy constructor and assignment operator
    NodeConfig(const NodeConfig&) = delete;
    NodeConfig& operator=(const NodeConfig&) = delete;
    
    static NodeConfig& getInstance() {
        if (!instance_) {
            instance_ = new NodeConfig();
        }
        return *instance_;
    }
    
    // Getters
    int getNodeId() const { return node_id_; }
    std::string getHost() const { return host_; }
    std::string getPort() const { return port_; }
    std::string getAddress() const { return host_ + ":" + port_; }
    std::map<int, std::unique_ptr<dsm::DSMService::Stub>>& getPeers() { return peers_; }
    
    // Setters
    void setNodeId(int id) { node_id_ = id; }
    void setHost(const std::string& host) { host_ = host; }
    void setPort(const std::string& port) { port_ = port; }
    
    void initialize(const std::string& port, int node_id) {
        port_ = port;
        node_id_ = node_id;
        host_ = get_env("GRPC_HOST", "0.0.0.0");
    }
};

// Initialize static member
inline NodeConfig* NodeConfig::instance_ = nullptr;


// Legacy Config struct for backward compatibility
struct Config 
{
    std::string host; 
    std::string port;

    static Config New()
    {
        Config config;
        config.host = get_env("GRPC_HOST", "0.0.0.0");
        config.port = get_env("GRPC_PORT", "50055");
        
        return config;
    }
};

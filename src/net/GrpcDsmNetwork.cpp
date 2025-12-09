#include "GrpcDsmNetwork.h"
#include <iostream>

namespace dsm {

    GrpcDsmNetwork::GrpcDsmNetwork(int my_node_id)
            : my_node_id_(my_node_id) {}

    GrpcDsmNetwork::~GrpcDsmNetwork() = default;

    void GrpcDsmNetwork::connect(int node_id, const std::string& address) {
        auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
        peers_[node_id] = DSMService::NewStub(channel);
        std::cout << "[Net] Connected to Node " << node_id << " at " << address << std::endl;
    }

    DSMService::Stub* GrpcDsmNetwork::getStub(int node_id) {
        auto it = peers_.find(node_id);
        if (it == peers_.end()) {
            std::cerr << "[Net] Error: No connection to Node " << node_id << std::endl;
            return nullptr;
        }
        return it->second.get();
    }

    std::string GrpcDsmNetwork::fetchFromHome(int home_node_id, const ObjectId& id) {
        auto stub = getStub(home_node_id);
        if (!stub) return "";

        grpc::ClientContext context;
        DsmFetchRequest request;
        request.set_object_name(id.str());
        request.set_requester_node_id(my_node_id_); // Critical for Stage 3 caching

        DsmFetchReply response;
        grpc::Status status = stub->ReceiveDsmFetch(&context, request, &response);

        if (status.ok() && response.found()) {
            return response.data();
        } else if (!status.ok()) {
            std::cerr << "[Net] Fetch failed: " << status.error_message() << std::endl;
        }
        return "";
    }

    void GrpcDsmNetwork::sendWriteToHome(int home_node_id, const ObjectId& id, const std::string& value_bytes) {
        auto stub = getStub(home_node_id);
        if (!stub) return;

        grpc::ClientContext context;
        DsmUpdateMsg request;
        request.set_object_name(id.str());
        request.set_data(value_bytes);
        request.set_sender_node_id(my_node_id_);

        Empty response;
        grpc::Status status = stub->ReceiveWriteToHome(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "[Net] WriteToHome failed: " << status.error_message() << std::endl;
        }
    }

    void GrpcDsmNetwork::sendCacheUpdate(int cache_node_id, const ObjectId& id, const std::string& value_bytes) {
        auto stub = getStub(cache_node_id);
        if (!stub) return;

        grpc::ClientContext context;
        DsmUpdateMsg request;
        request.set_object_name(id.str());
        request.set_data(value_bytes);
        request.set_sender_node_id(my_node_id_);

        Empty response;
        grpc::Status status = stub->ReceiveCacheUpdate(&context, request, &response);

        if (!status.ok()) {
            std::cerr << "[Net] CacheUpdate failed: " << status.error_message() << std::endl;
        }
    }

} // namespace dsm
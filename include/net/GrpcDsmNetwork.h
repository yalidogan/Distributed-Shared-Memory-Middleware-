#pragma once

#include "../../include/net/DsmNetwork.h"
#include <grpcpp/grpcpp.h>
#include "dsm.grpc.pb.h"
#include <map>
#include <memory>
#include <string>

namespace dsm {

    class GrpcDsmNetwork : public DsmNetwork {
    public:
        explicit GrpcDsmNetwork(int my_node_id);
        ~GrpcDsmNetwork() override;

        void connect(int node_id, const std::string& address);

        std::string fetchFromHome(int home_node_id, const ObjectId& id) override;

        // Write Operations
        void sendWriteToHome(int home_node_id, const ObjectId& id, const std::string& value_bytes) override;
        void sendCacheUpdate(int cache_node_id, const ObjectId& id, const std::string& value_bytes) override;

        // Remove Operations
        void sendRemoveToHome(int home_node_id, const ObjectId& id) override;
        void sendCacheRemove(int cache_node_id, const ObjectId& id) override;

        // Locking Operations (For Home-Based Locking)
        // void sendLockRequest(int home_id, const ObjectId& id, LockType type) override;

    private:
        int my_node_id_;
        std::map<int, std::unique_ptr<DSMService::Stub>> peers_;

        // Helper to get a connection or fail gracefully
        DSMService::Stub* getStub(int node_id);
    };

} // namespace dsm
#pragma once
#include "network_interface.hpp"
#include <grpcpp/grpcpp.h>
#include "protos/dsm_proto/dsm.grpc.pb.h"
#include <map>
#include <iostream>

class GrpcNetwork : public NetworkInterface {
private:
    std::map<int, std::unique_ptr<dsm::DSMService::Stub>> peers_;

public:
    void connect(int node_id, std::string ip_address) {
        auto channel = grpc::CreateChannel(ip_address, grpc::InsecureChannelCredentials());
        peers_[node_id] = dsm::DSMService::NewStub(channel);
    }

    //IMPLEMENTATION 

    void sendRaRequest(int to_node_id, const dsm::RaRequestMsg& msg) override {
        grpc::ClientContext context;
        dsm::Empty response;

        //Send msg
        if (peers_.count(to_node_id)) {
            peers_[to_node_id]->ReceiveRaRequest(&context, msg, &response);
        }
    }

    void sendRaReply(int to_node_id, const dsm::RaReplyMsg& msg) override {
        grpc::ClientContext context;
        dsm::Empty response;

        if (peers_.count(to_node_id)) {
            peers_[to_node_id]->ReceiveRaReply(&context, msg, &response);
        }
    }

    void sendDsmUpdate(int to_node_id, const dsm::DsmUpdateMsg& msg) override {
        grpc::ClientContext context;
        dsm::Empty response;

        if (peers_.count(to_node_id)) {
            peers_[to_node_id]->ReceiveDsmUpdate(&context, msg, &response);
        }
    }

    dsm::DsmFetchReply sendDsmFetch(int to_node_id, const dsm::DsmFetchRequest& msg) override {
        grpc::ClientContext context;
        dsm::DsmFetchReply response;

        if (peers_.count(to_node_id)) {
            peers_[to_node_id]->ReceiveDsmFetch(&context, msg, &response);
        }
        return response; // Return the Proto object directly
    }
};
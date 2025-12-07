#pragma once

#include <grpcpp/grpcpp.h>
#include "protos/dsm_proto/dsm.grpc.pb.h" 
#include <mutex>
#include <map>
#include <string>

//proto dosyasından namespaceler 
using grpc::ServerContext;
using grpc::Status;
using dsm::DSMService;
using dsm::RaRequestMsg;
using dsm::RaReplyMsg;
using dsm::DsmUpdateMsg;
using dsm::DsmFetchRequest;
using dsm::DsmFetchReply;
using dsm::Empty;

class DSMServiceImpl final : public DSMService::Service {
private:
    std::mutex mutex_;
    //actual shared memory storage
    std::map<std::string, std::string> local_memory_;

public:
    DSMServiceImpl();

    //RA
    Status ReceiveRaRequest(ServerContext* context, const RaRequestMsg* request, Empty* response) override;
    Status ReceiveRaReply(ServerContext* context, const RaReplyMsg* request, Empty* response) override;

    //Consistency
    Status ReceiveDsmUpdate(ServerContext* context, const DsmUpdateMsg* request, Empty* response) override;
    Status ReceiveDsmFetch(ServerContext* context, const DsmFetchRequest* request, DsmFetchReply* response) override;
};
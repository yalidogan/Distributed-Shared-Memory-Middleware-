#pragma once

#include <grpcpp/grpcpp.h>
#include "dsm.grpc.pb.h"
// Forward declaration to avoid circular include issues if they arise
namespace dsm { class DsmCore; }

class DsmServiceImpl final : public dsm::DSMService::Service {
public:
    // We need a pointer to the Core to update memory
    explicit DsmServiceImpl(dsm::DsmCore* core);

    grpc::Status ReceiveDsmFetch(grpc::ServerContext* context, const dsm::DsmFetchRequest* request, dsm::DsmFetchReply* response) override;
    grpc::Status ReceiveWriteToHome(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) override;
    grpc::Status ReceiveCacheUpdate(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) override;

    // RA Stubs
    grpc::Status ReceiveLockAcquire(grpc::ServerContext* context, const dsm::LockRequest* request, dsm::Empty* response) override;
    grpc::Status ReceiveLockRelease(grpc::ServerContext* context, const dsm::LockRequest* request, dsm::Empty* response) override;

    grpc::Status ReceiveRemoveToHome(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) override;
    grpc::Status ReceiveCacheRemove(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) override;

private:
    dsm::DsmCore* core_;
};
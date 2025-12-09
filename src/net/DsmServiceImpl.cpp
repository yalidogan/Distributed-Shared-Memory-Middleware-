#include "../../include/net/DsmServiceImpl.h"
#include "../../include/dsm/DsmCore.h"
#include "../../include/dsm/ObjectId.h"

DsmServiceImpl::DsmServiceImpl(dsm::DsmCore* core) : core_(core) {}

grpc::Status DsmServiceImpl::ReceiveDsmFetch(grpc::ServerContext* context, const dsm::DsmFetchRequest* request, dsm::DsmFetchReply* response) {
    // 1. Core Logic: Get data + Register requester as cache
    std::string val = core_->onFetchFromHome(request->requester_node_id(), dsm::ObjectId(request->object_name()));

    if (!val.empty()) {
        response->set_found(true);
        response->set_data(val);
        response->set_object_name(request->object_name());
    } else {
        response->set_found(false);
    }
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveWriteToHome(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) {
    core_->onWriteToHome(request->sender_node_id(), dsm::ObjectId(request->object_name()), request->data());
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveCacheUpdate(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) {
    core_->onCacheUpdate(dsm::ObjectId(request->object_name()), request->data());
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveRaRequest(grpc::ServerContext* context, const dsm::RaRequestMsg* request, dsm::Empty* response) {
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveRaReply(grpc::ServerContext* context, const dsm::RaReplyMsg* request, dsm::Empty* response) {
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveRemoveToHome(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) {
    core_->onRemoveToHome(request->sender_node_id(), dsm::ObjectId(request->object_name()));
    return grpc::Status::OK;
}

grpc::Status DsmServiceImpl::ReceiveCacheRemove(grpc::ServerContext* context, const dsm::DsmUpdateMsg* request, dsm::Empty* response) {
    core_->onCacheRemove(dsm::ObjectId(request->object_name()));
    return grpc::Status::OK;
}
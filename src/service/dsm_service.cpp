#include "service/dsm_service.hpp"
#include "network/network_interface.hpp"
#include "config/config.hpp"
#include <iostream>

DSMServiceImpl::DSMServiceImpl() {
    std::cout << "[DSM Service] Initialized local memory storage." << std::endl;
}

//RICART AGRAWALA LOGIC

Status DSMServiceImpl::ReceiveRaRequest(ServerContext* context, const RaRequestMsg* request, Empty* response) {
    // This logs when a friend asks for the lock
    std::cout << "[RA] Node " << request->sender_id() << " requests lock (Timestamp: " << request->timestamp() << ")" << std::endl;
    return Status::OK;
}

Status DSMServiceImpl::ReceiveRaReply(ServerContext* context, const RaReplyMsg* request, Empty* response) {
    std::cout << "[RA] Node " << request->sender_id() << " replied OK." << std::endl;
    return Status::OK;
}

//DSM DATA LOGIC

Status DSMServiceImpl::ReceiveDsmUpdate(ServerContext* context, const DsmUpdateMsg* request, Empty* response) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "[DSM] Update received for object: " << request->object_name() << std::endl;
    
    // Save the data to your local RAM
    local_memory_[request->object_name()] = request->data();
    
    return Status::OK;
}

Status DSMServiceImpl::ReceiveDsmFetch(ServerContext* context, const DsmFetchRequest* request, DsmFetchReply* response) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cout << "[DSM] Read request for object: " << request->object_name() << std::endl;

    auto it = local_memory_.find(request->object_name());
    if (it != local_memory_.end()) {
        response->set_found(true);
        response->set_data(it->second);
        response->set_object_name(request->object_name());
    } else {
        std::cout << " -> Object not found." << std::endl;
        response->set_found(false);
    }

    return Status::OK;
}
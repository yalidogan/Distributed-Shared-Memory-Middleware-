#pragma once
#include <string>
#include "protos/dsm_proto/dsm.pb.h" 

class NetworkInterface {
public:
    virtual ~NetworkInterface() = default;
   
    virtual void sendRaRequest(int to_node_id, const dsm::RaRequestMsg& msg) = 0;
    virtual void sendRaReply(int to_node_id, const dsm::RaReplyMsg& msg) = 0;
    virtual void sendDsmUpdate(int to_node_id, const dsm::DsmUpdateMsg& msg) = 0;
    virtual dsm::DsmFetchReply sendDsmFetch(int to_node_id, const dsm::DsmFetchRequest& msg) = 0;
};
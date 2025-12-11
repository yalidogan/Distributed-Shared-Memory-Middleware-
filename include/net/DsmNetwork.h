#pragma once

#include <string>
#include "../dsm/ObjectId.h"

namespace dsm {

    class DsmNetwork {
    public:
        virtual ~DsmNetwork() = default;


        virtual std::string fetchFromHome(int home_node_id,
                                          const ObjectId& id) = 0;

        virtual void sendWriteToHome(int home_node_id,
                                     const ObjectId& id,
                                     const std::string& value_bytes) = 0;

        virtual void sendCacheUpdate(int cache_node_id,
                                     const ObjectId& id,
                                     const std::string& value_bytes) = 0;

        virtual void sendRemoveToHome(int home_node_id, const ObjectId& id) = 0;
        virtual void sendCacheRemove(int cache_node_id, const ObjectId& id) = 0;

        virtual void sendLockAcquire(int home_id, const ObjectId& id, bool is_write_lock) = 0;
        virtual void sendLockRelease(int home_id, const ObjectId& id, bool is_write_lock) = 0;
    };

} // namespace dsm

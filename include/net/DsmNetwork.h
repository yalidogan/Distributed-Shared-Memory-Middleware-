#pragma once

#include <string>
#include "../dsm/ObjectId.h"

namespace dsm {

/**
 * @brief Networking API for DSM operations.
 *
 * Your networking friend will implement this using gRPC/Protobuf
 * in src/net/Network.cpp (or GrpcNetwork.cpp).
 *
 * All methods are synchronous from DsmCore’s perspective, but the
 * implementation can use async RPC under the hood.
 */
    class DsmNetwork {
    public:
        virtual ~DsmNetwork() = default;

        /**
         * @brief Fetch the latest value of an object from its home node.
         *
         * @param home_node_id  Node ID of the object's home.
         * @param id            Object identifier.
         * @return std::string  Serialized bytes of the object. Empty if
         *                      the object does not exist yet at the home.
         *
         * The network implementation is responsible for telling the home
         * which node is requesting, so the home can register it as a cache.
         * On the home side, it should call:
         *   DsmCore::onFetchFromHome(from_node_id, id)
         */
        virtual std::string fetchFromHome(int home_node_id,
                                          const ObjectId& id) = 0;

        /**
         * @brief Send a write/update to the home's canonical copy.
         *
         * @param home_node_id  Node ID of the object's home.
         * @param id            Object identifier.
         * @param value_bytes   Serialized bytes of new value.
         *
         * On the home side, the network impl should call:
         *   DsmCore::onWriteToHome(from_node_id, id, value_bytes)
         */
        virtual void sendWriteToHome(int home_node_id,
                                     const ObjectId& id,
                                     const std::string& value_bytes) = 0;

        /**
         * @brief Send a cache update from the home to a cache node.
         *
         * @param cache_node_id Target node that caches this object.
         * @param id            Object identifier.
         * @param value_bytes   Serialized bytes of new value.
         *
         * On the cache side, the network impl should call:
         *   DsmCore::onCacheUpdate(id, value_bytes)
         */
        virtual void sendCacheUpdate(int cache_node_id,
                                     const ObjectId& id,
                                     const std::string& value_bytes) = 0;
    };

} // namespace dsm

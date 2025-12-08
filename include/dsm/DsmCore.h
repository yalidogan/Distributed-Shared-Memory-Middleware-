#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>

#include "ObjectId.h"
#include "ObjectStore.h"
#include "Serialization.h"
#include "../ra/RaManager.h"
#include "../net/DsmNetwork.h"

namespace dsm {

/**
 * @brief Metadata kept on the home node for each object.
 *
 * For now we only track which nodes cache this object, so the home
 * can broadcast updates to them (Stage 3 caching).
 */
    struct ObjectMeta {
        std::unordered_set<int> cached_nodes;
    };

/**
 * @brief Core DSM logic: home-based, cached, object-based DSM.
 *
 * Responsibilities:
 *  - For each ObjectId, compute its home node.
 *  - On get()/put():
 *      * Use RaManager to serialize access per object across cluster.
 *      * Contact home node if needed.
 *      * Maintain local cache in ObjectStore.
 *  - On the home node:
 *      * Track which nodes cache each object.
 *      * Broadcast cache updates on writes.
 *
 * Networking and RA are injected as RaManager* and Network*.
 */
    class DsmCore {
    public:
        DsmCore(int my_node_id,
                int total_nodes,
                RaManager* ra,
                DsmNetwork* net);

        /**
         * @brief Typed DSM read: acquire RA, ensure up-to-date value, deserialize.
         *
         * @tparam T  C++ type to deserialize into (must have dsm::deserialize).
         * @param id  Object identifier.
         * @return T  The current value.
         */
        template <typename T>
        T get(const ObjectId& id);

        /**
         * @brief Typed DSM write: acquire RA, serialize value, send to home.
         *
         * @tparam T   C++ type to serialize (must have dsm::serialize).
         * @param id   Object identifier.
         * @param val  New value to write.
         */
        template <typename T>
        void put(const ObjectId& id, const T& val);

        //
        // Handlers to be called by networking layer on incoming DSM RPCs:
        //

        /**
         * @brief Handle a fetch request on the home node.
         *
         * @param from_node_id  Node ID of requester (cache node).
         * @param id            Object identifier.
         * @return std::string  Serialized bytes of the current value, or empty
         *                      if not present.
         *
         * The home should also register from_node_id as a cache for this object.
         */
        std::string onFetchFromHome(int from_node_id,
                                    const ObjectId& id);

        /**
         * @brief Handle a write/update on the home node from a remote writer.
         *
         * @param from_node_id  Node ID of the writer.
         * @param id            Object identifier.
         * @param value_bytes   Serialized bytes of new value.
         *
         * The home updates its canonical store and broadcasts cache updates.
         */
        void onWriteToHome(int from_node_id,
                           const ObjectId& id,
                           const std::string& value_bytes);

        /**
         * @brief Handle a cache update on any node.
         *
         * @param id           Object identifier.
         * @param value_bytes  Serialized bytes of new value.
         *
         * This simply updates the local ObjectStore.
         */
        void onCacheUpdate(const ObjectId& id,
                           const std::string& value_bytes);

    private:
        int my_id_;
        int total_nodes_;
        RaManager* ra_;   // not owned
        DsmNetwork*  net_;   // not owned

        ObjectStore store_;

        // Only meaningful on home nodes
        std::unordered_map<ObjectId, ObjectMeta> meta_;
        std::mutex meta_mtx_;

        // Compute home node for a given object
        int homeFor(const ObjectId& id) const;

        // Raw (byte-based) operations under RA
        std::string getRaw(const ObjectId& id);
        void putRaw(const ObjectId& id, const std::string& bytes);

        // Metadata helpers (home only)
        void registerCacheNode(const ObjectId& id, int node_id);
        std::vector<int> cachedNodesFor(const ObjectId& id);
    };

// ======================= Template Implementation ======================= //

    template <typename T>
    T DsmCore::get(const ObjectId& id) {
        std::string bytes = getRaw(id);
        T value{};
        deserialize(bytes, value);   // uses dsm::deserialize from Serialization.h
        return value;
    }

    template <typename T>
    void DsmCore::put(const ObjectId& id, const T& val) {
        std::string bytes = serialize(val); // uses dsm::serialize
        putRaw(id, bytes);
    }

} // namespace dsm

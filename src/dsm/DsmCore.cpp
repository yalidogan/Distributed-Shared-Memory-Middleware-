#include "../../include/dsm/DsmCore.h"

#include <functional>
// #include <iostream>

namespace dsm {

    DsmCore::DsmCore(int my_node_id,
                     int total_nodes,
                     RaManager* ra,
                     DsmNetwork* net)
            : my_id_(my_node_id),
              total_nodes_(total_nodes),
              ra_(ra),
              net_(net) {}

/**
 * @brief Simple hash-based home assignment: home = hash(id) % total_nodes.
 */
    int DsmCore::homeFor(const ObjectId& id) const {
        std::hash<std::string> h;
        return static_cast<int>(h(id.str()) % total_nodes_);
    }

// ------------------------- Metadata (home) ------------------------- //

    void DsmCore::registerCacheNode(const ObjectId& id, int node_id) {
        int home = homeFor(id);
        if (home != my_id_) {
            // Not the home; nothing to do.
            return;
        }
        std::lock_guard<std::mutex> lock(meta_mtx_);
        meta_[id].cached_nodes.insert(node_id);
    }

    std::vector<int> DsmCore::cachedNodesFor(const ObjectId& id) {
        std::vector<int> result;
        int home = homeFor(id);
        if (home != my_id_) {
            return result;
        }

        std::lock_guard<std::mutex> lock(meta_mtx_);
        auto it = meta_.find(id);
        if (it == meta_.end()) {
            return result;
        }
        result.reserve(it->second.cached_nodes.size());
        for (int nid : it->second.cached_nodes) {
            result.push_back(nid);
        }
        return result;
    }

// ---------------------------- Raw API ----------------------------- //

    std::string DsmCore::getRaw(const ObjectId& id) {
        // Serialize access across the cluster for this object.
        ra_->acquire(id);

        int home = homeFor(id);
        std::string bytes;

        if (home == my_id_) {
            // I'm the home: canonical value is local.
            bytes = store_.get(id);
        } else {
            // I'm a non-home node: ask home for the latest.
            bytes = net_->fetchFromHome(home, id);
            if (!bytes.empty()) {
                // Cache it locally.
                store_.put(id, bytes);
            }
        }

        ra_->release(id);
        return bytes;
    }

    void DsmCore::putRaw(const ObjectId& id, const std::string& bytes) {
        // Serialize access across the cluster for this object.
        ra_->acquire(id);

        int home = homeFor(id);

        if (home == my_id_) {
            // I'm the home:
            // 1. Update canonical store.
            store_.put(id, bytes);

            // 2. Broadcast updated value to all cached nodes.
            auto caches = cachedNodesFor(id);
            for (int node : caches) {
                if (node == my_id_) {
                    continue; // my own store already updated.
                }
                net_->sendCacheUpdate(node, id, bytes);
            }
        } else {
            // I'm a non-home node:
            // 1. Send write to home (home will update canonical & broadcast).
            net_->sendWriteToHome(home, id, bytes);

            // 2. Optionally update my own cache immediately.
            store_.put(id, bytes);
        }

        ra_->release(id);
    }

// ------------------------ Incoming handlers ------------------------ //

    std::string DsmCore::onFetchFromHome(int from_node_id,
                                         const ObjectId& id) {
        // Home registers this node as cache.
        registerCacheNode(id, from_node_id);

        // Return canonical value (may be empty if not initialized).
        return store_.get(id);
    }

    void DsmCore::onWriteToHome(int from_node_id,
                                const ObjectId& id,
                                const std::string& value_bytes) {
        (void)from_node_id; // currently unused; kept for future logging/logic.

        int home = homeFor(id);
        if (home != my_id_) {
            // This should not happen; you can add logging here.
            // std::cerr << "onWriteToHome called on non-home node\n";
            return;
        }

        // 1. Update canonical store.
        store_.put(id, value_bytes);

        // 2. Broadcast updated value to all cached nodes.
        auto caches = cachedNodesFor(id);
        for (int node : caches) {
            if (node == my_id_) {
                continue; // local store already updated.
            }
            net_->sendCacheUpdate(node, id, value_bytes);
        }
    }

    void DsmCore::onCacheUpdate(const ObjectId& id,
                                const std::string& value_bytes) {
        // Any node (home or non-home) updates its local cache.
        store_.put(id, value_bytes);
    }

} // namespace dsm

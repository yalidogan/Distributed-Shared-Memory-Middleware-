#include "../../include/dsm/DsmCore.h"
#include <functional>
#include <iostream>

// deterministic hash function
inline uint32_t fnv1a_hash(const std::string& str) {
    uint32_t hash = 2166136261u;
    for (char c : str) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 16777619u;
    }
    return hash;
}

namespace dsm {

    DsmCore::DsmCore(int my_node_id, int total_nodes, RaManager* ra, DsmNetwork* net)
            : my_id_(my_node_id), total_nodes_(total_nodes), ra_(ra), net_(net) {}

    int DsmCore::homeFor(const ObjectId& id) const {
        uint32_t h = fnv1a_hash(id.str());
        int home = static_cast<int>(h % total_nodes_);
        return home;
    }


    std::string DsmCore::fetchRawInternal(const ObjectId& id) {
        // 1. Check Local Cache
        if (store_.exists(id)) {
            std::cout << "[DEBUG] Node " << my_id_ << ": Local Cache HIT for " << id.str() << std::endl;
            return store_.get(id);
        }

        // 2. Fetch from Home
        int home = homeFor(id);
        std::cout << "[DEBUG] Node " << my_id_ << ": Cache MISS for " << id.str()
                  << ". Fetching from Home " << home << "..." << std::endl;

        if (home != my_id_) {
            std::string bytes = net_->fetchFromHome(home, id);
            if (!bytes.empty()) {
                std::cout << "[DEBUG] Node " << my_id_ << ": Fetch SUCCESS. Storing " << bytes.size() << " bytes." << std::endl;
                store_.put(id, bytes); // Cache it
            } else {
                std::cout << "[DEBUG] Node " << my_id_ << ": Fetch returned EMPTY (Not found or Net Error)." << std::endl;
            }
            return bytes;
        }

        // I am home but I don't have it
        std::cout << "[DEBUG] Node " << my_id_ << ": I am Home for " << id.str() << " but it is empty." << std::endl;
        return "";
    }

    void DsmCore::putRawInternal(const ObjectId& id, const std::string& bytes) {
        int home = homeFor(id);
        std::cout << "[DEBUG] Node " << my_id_ << ": putRawInternal " << id.str()
                  << " (" << bytes.size() << " bytes). Home is " << home << std::endl;

        if (home == my_id_) {
            // I am Home
            store_.put(id, bytes);
            // Broadcast
            auto caches = cachedNodesFor(id);
            std::cout << "[DEBUG] Node " << my_id_ << ": I am Home. Broadcasting update to "
                      << caches.size() << " nodes." << std::endl;

            for (int node : caches) {
                if (node == my_id_) continue;
                net_->sendCacheUpdate(node, id, bytes);
            }
        } else {
            // I am Client
            std::cout << "[DEBUG] Node " << my_id_ << ": Sending Write to Home " << home << std::endl;
            net_->sendWriteToHome(home, id, bytes);

            // Optimistic local update
            store_.put(id, bytes);
        }
    }

    void DsmCore::releaseRawInternal(const ObjectId& id) {
        // std::cout << "[DEBUG] Node " << my_id_ << ": Releasing Lock " << id.str() << std::endl;
        ra_->release(id);
    }

// ------------------------- Metadata (Home) ------------------------- //

    void DsmCore::registerCacheNode(const ObjectId& id, int node_id) {
        int home = homeFor(id);
        if (home != my_id_) return;

        std::lock_guard<std::mutex> lock(meta_mtx_);
        if (meta_[id].cached_nodes.find(node_id) == meta_[id].cached_nodes.end()) {
            std::cout << "[DEBUG] Node " << my_id_ << ": Registered Node " << node_id
                      << " as listener for " << id.str() << std::endl;
            meta_[id].cached_nodes.insert(node_id);
        }
    }

    std::vector<int> DsmCore::cachedNodesFor(const ObjectId& id) {
        std::vector<int> result;
        int home = homeFor(id);
        if (home != my_id_) return result;

        std::lock_guard<std::mutex> lock(meta_mtx_);
        auto it = meta_.find(id);
        if (it == meta_.end()) return result;

        result.reserve(it->second.cached_nodes.size());
        for (int nid : it->second.cached_nodes) {
            result.push_back(nid);
        }
        return result;
    }

// ------------------------- Handlers ------------------------- //

    std::string DsmCore::onFetchFromHome(int from_node_id, const ObjectId& id) {
        std::cout << "[DEBUG] Node " << my_id_ << ": Received FETCH request from "
                  << from_node_id << " for " << id.str() << std::endl;

        registerCacheNode(id, from_node_id);

        if (store_.exists(id)) {
            return store_.get(id);
        } else {
            std::cout << "[DEBUG] Node " << my_id_ << ": Object " << id.str() << " not found in my store." << std::endl;
            return "";
        }
    }

    void DsmCore::onWriteToHome(int from_node_id, const ObjectId& id, const std::string& value_bytes) {
        std::cout << "[DEBUG] Node " << my_id_ << ": Received WRITE request from "
                  << from_node_id << " for " << id.str() << std::endl;

        int home = homeFor(id);
        if (home != my_id_) {
            std::cerr << "[ERROR] Node " << my_id_ << ": Received Write but I am NOT home! (Calc Home: " << home << ")" << std::endl;
            return;
        }

        store_.put(id, value_bytes);

        auto caches = cachedNodesFor(id);
        for (int node : caches) {
            if (node == my_id_) continue;
            std::cout << "[DEBUG] Node " << my_id_ << ": Forwarding update to Cache Node " << node << std::endl;
            net_->sendCacheUpdate(node, id, value_bytes);
        }
    }

    void DsmCore::onCacheUpdate(const ObjectId& id, const std::string& value_bytes) {
        std::cout << "[DEBUG] Node " << my_id_ << ": Received Cache Update for " << id.str() << std::endl;
        store_.put(id, value_bytes);
    }

// --- Remove Logic ---

    void DsmCore::remove(const ObjectId& id) {
        removeRaw(id);
    }

    void DsmCore::removeRaw(const ObjectId& id) {
        ra_->acquire(id);
        int home = homeFor(id);

        if (home == my_id_) {
            store_.erase(id);
            auto caches = cachedNodesFor(id);
            for (int node : caches) {
                if (node == my_id_) continue;
                net_->sendCacheRemove(node, id);
            }
        } else {
            net_->sendRemoveToHome(home, id);
            store_.erase(id);
        }
        ra_->release(id);
    }

    void DsmCore::onRemoveToHome(int from_node_id, const ObjectId& id) {
        if (homeFor(id) != my_id_) return;
        store_.erase(id);
        auto caches = cachedNodesFor(id);
        for (int node : caches) {
            if (node == my_id_) continue;
            net_->sendCacheRemove(node, id);
        }
    }

    void DsmCore::onCacheRemove(const ObjectId& id) {
        store_.erase(id);
    }

} // namespace dsm
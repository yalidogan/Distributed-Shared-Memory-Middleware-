#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <vector>

#include "ObjectId.h"
#include "ObjectStore.h"
#include "Serialization.h"
#include "../sync/RaManager.h"
#include "../net/DsmNetwork.h"

#include "DsmHandle.h"

namespace dsm {

    struct ObjectMeta {
        std::unordered_set<int> cached_nodes;
    };

    class DsmCore {
    public:
        DsmCore(int my_node_id, int total_nodes, RaManager* ra, DsmNetwork* net);


        template <typename T>
        DsmHandle<T> getRead(const ObjectId& id);


        template <typename T>
        DsmHandle<T> getWrite(const ObjectId& id);

        void putRawInternal(const ObjectId& id, const std::string& bytes);
        void releaseRawInternal(const ObjectId& id);

        // -----------------------------------------------------------
        // NETWORK HANDLERS
        // -----------------------------------------------------------
        std::string onFetchFromHome(int from_node_id, const ObjectId& id);
        void onWriteToHome(int from_node_id, const ObjectId& id, const std::string& value_bytes);
        void onCacheUpdate(const ObjectId& id, const std::string& value_bytes);

        void onRemoveToHome(int from_node_id, const ObjectId& id);
        void onCacheRemove(const ObjectId& id);

        // Public Remove API
        void remove(const ObjectId& id);
        bool exists(const ObjectId& id) const { return store_.exists(id); }

    private:
        int my_id_;
        int total_nodes_;
        RaManager* ra_;
        DsmNetwork* net_;
        ObjectStore store_;

        std::unordered_map<ObjectId, ObjectMeta> meta_;
        std::mutex meta_mtx_;

        int homeFor(const ObjectId& id) const;
        void registerCacheNode(const ObjectId& id, int node_id);
        std::vector<int> cachedNodesFor(const ObjectId& id);

        void removeRaw(const ObjectId& id);

        std::string fetchRawInternal(const ObjectId& id);
    };



    template <typename T>
    DsmHandle<T>::~DsmHandle() {
        if (!core_) return;

        if (modified_ && is_writable_) {
            // Serialize and push
            core_->putRawInternal(id_, serialize(value_));
        }

        core_->releaseRawInternal(id_);
    }

    template <typename T>
    DsmHandle<T> DsmCore::getRead(const ObjectId& id) {
        ra_->acquire(id);

        std::string bytes = fetchRawInternal(id);

        T val{};
        if (!bytes.empty()) {
            deserialize(bytes, val);
        }

        return DsmHandle<T>(this, id, val, false);
    }

    template <typename T>
    DsmHandle<T> DsmCore::getWrite(const ObjectId& id) {
        ra_->acquire(id);

        std::string bytes = fetchRawInternal(id);

        T val{};
        if (!bytes.empty()) {
            deserialize(bytes, val);
        }

        return DsmHandle<T>(this, id, val, true);
    }

} // namespace dsm
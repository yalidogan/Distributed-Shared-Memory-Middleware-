#pragma once

#include <string>
#include <unordered_map>
#include <mutex>

#include "ObjectId.h"

namespace dsm {

/**
 * @brief Thread-safe local object storage.
 *
 * Internally this is just a map<ObjectId, std::string> where:
 *  - key   = ObjectId
 *  - value = serialized bytes of the object (e.g., int64_t, protobuf, etc.)
 *
 * Both home nodes and cache nodes use this.
 */
    class ObjectStore {
    public:
        ObjectStore() = default;
        ~ObjectStore() = default;

        ObjectStore(const ObjectStore&) = delete;
        ObjectStore& operator=(const ObjectStore&) = delete;

        std::string get(const ObjectId& id) const {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = objects_.find(id);
            if (it == objects_.end()) {
                throw std::runtime_error("ObjectStore::get - Object not found: " + id.str());
            }
            return it->second;
        }


        void put(const ObjectId& id, const std::string& value) {
            std::lock_guard<std::mutex> lock(mtx_);
            objects_[id] = value;
        }


        bool exists(const ObjectId& id) const {
            std::lock_guard<std::mutex> lock(mtx_);
            return objects_.find(id) != objects_.end();
        }


        bool erase(const ObjectId& id) {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = objects_.find(id);
            if (it == objects_.end()) {
                return false;
            }
            objects_.erase(it);
            return true;
        }

    private:
        // Mutable because get()/exists() are logically const, but need to lock.
        mutable std::mutex mtx_;
        std::unordered_map<ObjectId, std::string> objects_;
    };

} // namespace dsm

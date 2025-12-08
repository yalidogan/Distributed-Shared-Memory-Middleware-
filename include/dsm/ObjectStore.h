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

        /**
         * @brief Get the serialized value for a given object ID.
         *
         * @param id Object identifier.
         * @return std::string Serialized bytes. If the object is not found,
         *                     returns an empty string.
         */
        std::string get(const ObjectId& id) const {
            std::lock_guard<std::mutex> lock(mtx_);
            auto it = objects_.find(id);
            if (it == objects_.end()) {
                return {};
            }
            return it->second;
        }

        /**
         * @brief Store or overwrite the serialized value for a given object ID.
         *
         * @param id    Object identifier.
         * @param value Serialized bytes to store.
         */
        void put(const ObjectId& id, const std::string& value) {
            std::lock_guard<std::mutex> lock(mtx_);
            objects_[id] = value;
        }

        /**
         * @brief Check whether an object exists in the local store.
         *
         * @param id Object identifier.
         * @return true  if a value is present.
         * @return false otherwise.
         */
        bool exists(const ObjectId& id) const {
            std::lock_guard<std::mutex> lock(mtx_);
            return objects_.find(id) != objects_.end();
        }

        /**
         * @brief Remove an object from the local store, if present.
         *
         * @param id Object identifier.
         * @return true  if an object was removed.
         * @return false if it wasn't present.
         */
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

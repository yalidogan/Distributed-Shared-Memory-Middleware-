#pragma once

#include "../dsm/ObjectId.h"

namespace dsm {
struct ObjectLockState {
    int readers = 0;
    bool writer_active = false;
    int write_waiters = 0;
    
    std::mutex state_mutex;
    std::condition_variable cv;
};


class LockManager {
public:
    LockManager() = default;
    ~LockManager() = default;

    LockManager(const LockManager&) = delete;
    LockManager& operator=(const LockManager&) = delete;

    void acquire(const ObjectId& id, bool is_write_lock);

    void release(const ObjectId& id, bool is_write_lock);

private:
    std::mutex map_mutex_;
    std::unordered_map<std::string, std::shared_ptr<ObjectLockState>> lock_map_;

    std::shared_ptr<ObjectLockState> getObjectState(const std::string& object_id);
};

} // namespace dsm

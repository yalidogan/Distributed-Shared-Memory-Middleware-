#include "../../include/sync/LockManager.h"
#include <iostream>

struct ObjectLockState {
    int readers = 0;
    bool writer_active = false;
    int write_waiters = 0;
    
    std::mutex state_mutex;
    std::condition_variable cv;
};

namespace dsm {

void LockManager::acquire(const ObjectId& id, bool is_write_lock) {
    std::cout << "[LockManager] Lock acquire requested for " << id.str() << " type: " << (is_write_lock ? "write" : "read") << std::endl;
    
    auto state = getObjectState(id.str());
    
    std::unique_lock<std::mutex> lock(state->state_mutex);
    if (is_write_lock) {
        state->write_waiters++;
        
        // Wait until: No readers AND No active writer
        state->cv.wait(lock, [&state] { 
            return state->readers == 0 && !state->writer_active; 
        });

        state->write_waiters--;
        state->writer_active = true;
    } 
    else {
        // Wait until: No active writer. 
        state->cv.wait(lock, [&state] { 
            return !state->writer_active && state->write_waiters == 0; 
        });

        state->readers++;
    }
    
    std::cout << "[LockManager] Lock acquired for " << id.str() << " type: " << (is_write_lock ? "write" : "read") << std::endl;
}

void LockManager::release(const ObjectId& id, bool is_write_lock) {
    std::cout << "[LockManager] Lock release requested for " << id.str() << " type: " << (is_write_lock ? "write" : "read") << std::endl;
    auto state = getObjectState(id.str());   
    {
        std::lock_guard<std::mutex> lock(state->state_mutex);
        
        if (is_write_lock) {
            state->writer_active = false;
        } else {
            state->readers--;
        }
    } 

    state->cv.notify_all();
    std::cout << "[LockManager] Lock released for " << id.str() << " type: " << (is_write_lock ? "write" : "read") << std::endl;
}

std::shared_ptr<ObjectLockState> LockManager::getObjectState(const std::string& object_id) {
    std::lock_guard<std::mutex> lock(map_mutex_);
    if (lock_map_.find(object_id) == lock_map_.end()) {
        lock_map_[object_id] = std::make_shared<ObjectLockState>();
    }
    return lock_map_[object_id];
}
}
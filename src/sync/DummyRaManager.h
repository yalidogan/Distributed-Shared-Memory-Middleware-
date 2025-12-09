#pragma once
#include "../../include/ra/RaManager.h"
#include <iostream>

namespace dsm {
    class DummyRaManager : public RaManager {
    public:
        void acquire(const ObjectId& id) override {
            // Immediately succeed (No locking logic yet)
            // std::cout << "[DummyRA] Lock acquired for " << id.str() << std::endl;
        }

        void release(const ObjectId& id) override {
            // No-op
            // std::cout << "[DummyRA] Lock released for " << id.str() << std::endl;
        }
    };
}
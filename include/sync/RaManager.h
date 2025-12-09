#pragma once

#include "../dsm/ObjectId.h"

namespace dsm {

/**
 * @brief Distributed mutual exclusion manager (Ricart–Agrawala).
 *
 * This is the *concrete* RA API that DsmCore depends on.
 * Your RA friend will implement this class in src/ra/RaManager.cpp.
 */
    class RaManager {
    public:
        virtual ~RaManager() = default;

        /**
         * @brief Acquire the distributed lock for a given object.
         *
         * This call should block until this node has exclusive access to
         * the critical section for the given ObjectId across the cluster.
         */
        virtual void acquire(const ObjectId& id) = 0;

        /**
         * @brief Release the distributed lock for a given object.
         */
        virtual void release(const ObjectId& id) = 0;
    };

} // namespace dsm

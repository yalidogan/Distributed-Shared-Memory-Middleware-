#pragma once

#include "../dsm/ObjectId.h"

namespace dsm {

    class RaManager {
    public:
        virtual ~RaManager() = default;

        virtual void acquire(const ObjectId& id) = 0;

        virtual void release(const ObjectId& id) = 0;
    };

} // namespace dsm

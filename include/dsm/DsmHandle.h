#pragma once

#include <stdexcept>
#include <string>
#include "ObjectId.h"
#include "Serialization.h"

namespace dsm {

    class DsmCore;

    template <typename T>
    class DsmHandle {
    public:
        DsmHandle(DsmCore* core, ObjectId id, T value, bool is_writable)
                : core_(core), id_(id), value_(value), is_writable_(is_writable), modified_(false) {}

        ~DsmHandle();

        DsmHandle(const DsmHandle&) = delete;
        DsmHandle& operator=(const DsmHandle&) = delete;

        DsmHandle(DsmHandle&& other) noexcept
                : core_(other.core_), id_(std::move(other.id_)),
        value_(std::move(other.value_)),
        is_writable_(other.is_writable_), modified_(other.modified_) {
            other.core_ = nullptr; // Invalidate source
        }

        const T& get() const {
            return value_;
        }

        T* operator->() {
            ensureWritable();
            modified_ = true;
            return &value_;
        }

        T& operator*() {
            ensureWritable();
            modified_ = true;
            return value_;
        }

    private:
        DsmCore* core_;
        ObjectId id_;
        T value_;
        bool is_writable_;
        bool modified_;

        void ensureWritable() {
            if (!is_writable_) {
                throw std::runtime_error("DsmHandle Error: Attempted to write to a Read-Only handle.");
            }
        }
    };

} // namespace dsm
#pragma once

#include <string>
#include <functional>

namespace dsm {

/**
 * @brief Lightweight identifier for a distributed shared object.
 *
 * For now this is just a string wrapper. Later, if you switch to a
 * protobuf-generated type, you can either:
 *  - adapt this class to wrap that type, or
 *  - replace uses of ObjectId with your proto type and adjust a bit.
 */
    class ObjectId {
    public:
        ObjectId() = default;

        explicit ObjectId(std::string name)
                : name_(std::move(name)) {}

        // Access underlying string
        const std::string& str() const noexcept { return name_; }

        // For convenience: implicit conversion to string if you like
        operator const std::string&() const noexcept { return name_; }

        bool operator==(const ObjectId& other) const noexcept {
            return name_ == other.name_;
        }

        bool operator!=(const ObjectId& other) const noexcept {
            return !(*this == other);
        }

    private:
        std::string name_;
    };

} // namespace dsm

// Allow ObjectId to be used as a key in unordered_map / unordered_set
namespace std {

    template<>
    struct hash<dsm::ObjectId> {
        std::size_t operator()(const dsm::ObjectId& id) const noexcept {
            return std::hash<std::string>{}(id.str());
        }
    };

} // namespace std

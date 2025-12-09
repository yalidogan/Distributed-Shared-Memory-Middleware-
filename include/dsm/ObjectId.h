#pragma once

#include <string>
#include <functional>

namespace dsm {

    class ObjectId {
    public:
        ObjectId() = default;

        explicit ObjectId(std::string name)
                : name_(std::move(name)) {}

        const std::string& str() const noexcept { return name_; }

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

namespace std {

    template<>
    struct hash<dsm::ObjectId> {
        std::size_t operator()(const dsm::ObjectId& id) const noexcept {
            return std::hash<std::string>{}(id.str());
        }
    };

} // namespace std

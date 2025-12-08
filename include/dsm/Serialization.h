#pragma once

#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>

namespace dsm {

/**
 * Basic serialization helpers.
 *
 * These convert between arbitrary C++ types and std::string (raw bytes).
 * The DSM core always deals with std::string internally, and your
 * templated API in DsmCore will call these.
 *
 * For now we support:
 *  - std::string (trivial)
 *  - int64_t     (binary, host endianness)
 *
 * You can add overloads/specializations for more types later.
 */

//
// std::string
//

    inline std::string serialize(const std::string& s) {
        return s; // already bytes
    }

    inline void deserialize(const std::string& bytes, std::string& out) {
        out = bytes;
    }

//
// int64_t
//

    inline std::string serialize(int64_t value) {
        std::string bytes(sizeof(int64_t), '\0');
        // NOTE: this uses host endianness; fine for a controlled cluster.
        std::memcpy(bytes.data(), &value, sizeof(int64_t));
        return bytes;
    }

    inline void deserialize(const std::string& bytes, int64_t& out) {
        if (bytes.size() != sizeof(int64_t)) {
            throw std::runtime_error("dsm::deserialize<int64_t>: size mismatch");
        }
        std::memcpy(&out, bytes.data(), sizeof(int64_t));
    }

//
// You can add more overloads here as needed, e.g.:
//
// inline std::string serialize(int32_t value) { ... }
// inline void deserialize(const std::string&, int32_t&);
//
// Or for your own structs, or later, protobuf messages.
// For protobuf later, it'll look roughly like:
//
// template <typename T>
// std::enable_if_t<std::is_base_of_v<google::protobuf::MessageLite, T>, std::string>
// serialize(const T& msg) { ... }
//
// template <typename T>
// std::enable_if_t<std::is_base_of_v<google::protobuf::MessageLite, T>, void>
// deserialize(const std::string& bytes, T& msg) { ... }
//

} // namespace dsm

#pragma once

#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>
#include <type_traits>

namespace dsm {

    // ----------------------------------------------------------------
    // std::string
    // ----------------------------------------------------------------
    inline std::string serialize(const std::string& s) {
        return s;
    }

    inline void deserialize(const std::string& bytes, std::string& out) {
        out = bytes;
    }

    // ----------------------------------------------------------------
    // Generic Arithmetic Types (int, float, double, bool, etc.)
    // ----------------------------------------------------------------
    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, std::string>::type
    serialize(T value) {
        std::string bytes(sizeof(T), '\0');
        std::memcpy(bytes.data(), &value, sizeof(T));
        return bytes;
    }

    template <typename T>
    typename std::enable_if<std::is_arithmetic<T>::value, void>::type
    deserialize(const std::string& bytes, T& out) {
        if (bytes.size() != sizeof(T)) {
            // It's okay to have zero bytes for empty init, but good to warn
            if (bytes.empty()) { out = 0; return; }
            throw std::runtime_error("dsm::deserialize size mismatch for arithmetic type");
        }
        std::memcpy(&out, bytes.data(), sizeof(T));
    }

    // ----------------------------------------------------------------
    // 3. Generic Vector<T>
    // ----------------------------------------------------------------
    template <typename T>
    std::string serialize(const std::vector<T>& v) {
        std::string buffer;

        size_t count = v.size();
        buffer.append(reinterpret_cast<const char*>(&count), sizeof(size_t));

        for (const auto& elem : v) {
            std::string elem_bytes = serialize(elem);
            size_t elem_size = elem_bytes.size();
            buffer.append(reinterpret_cast<const char*>(&elem_size), sizeof(size_t));
            buffer.append(elem_bytes);
        }
        return buffer;
    }

    template <typename T>
    void deserialize(const std::string& bytes, std::vector<T>& out) {
        if (bytes.size() < sizeof(size_t)) {
            out.clear();
            return;
        }

        const char* ptr = bytes.data();
        const char* end = bytes.data() + bytes.size();

        size_t count = 0;
        std::memcpy(&count, ptr, sizeof(size_t));
        ptr += sizeof(size_t);

        out.resize(count);

        for (size_t i = 0; i < count; ++i) {
            if (ptr + sizeof(size_t) > end) break; // Safety check

            size_t elem_size = 0;
            std::memcpy(&elem_size, ptr, sizeof(size_t));
            ptr += sizeof(size_t);

            if (ptr + elem_size > end) break; // Safety check

            std::string elem_bytes(ptr, elem_size);
            ptr += elem_size;

            deserialize(elem_bytes, out[i]);
        }
    }

} // namespace dsm
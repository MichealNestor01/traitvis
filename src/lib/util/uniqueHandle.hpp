#pragma once

#include <utility>

// Owns one GPU id. The deleter runs when the handle is destroyed or replaced.
// A zero id is empty and is never passed to the deleter.
template <typename Deleter>
class UniqueHandle {
    unsigned int handle = 0;
    [[no_unique_address]] Deleter deleter{};

public:
    UniqueHandle() = default;
    explicit UniqueHandle(unsigned int h) noexcept : handle(h) {}
    UniqueHandle(unsigned int h, Deleter d) noexcept : handle(h), deleter(std::move(d)) {}
    ~UniqueHandle() { reset(); }

    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;

    UniqueHandle(UniqueHandle&& other) noexcept
        : handle(std::exchange(other.handle, 0)), deleter(std::move(other.deleter)) {}

    UniqueHandle& operator=(UniqueHandle&& other) noexcept {
        if (this != &other) {
            reset();
            handle = std::exchange(other.handle, 0);
            deleter = std::move(other.deleter);
        }
        return *this;
    }

    void reset() noexcept {
        if (handle) deleter(handle);
        handle = 0;
    }

    [[nodiscard]] unsigned int get() const noexcept { return handle; }
    explicit operator bool() const noexcept { return handle != 0; }
};

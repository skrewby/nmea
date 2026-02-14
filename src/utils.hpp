#pragma once

#include <utility>

template <typename F> class scope_exit {
public:
    explicit scope_exit(F fn) : m_cleanup_fn(std::move(fn)) {}

    ~scope_exit() {
        if (!m_released) {
            m_cleanup_fn();
        }
    }

    scope_exit(const scope_exit &other) = delete;
    scope_exit &operator=(const scope_exit &other) = delete;
    scope_exit(scope_exit &&other) noexcept = delete;
    scope_exit &operator=(scope_exit &&other) noexcept = delete;

    void release() { m_released = true; }

private:
    F m_cleanup_fn;
    bool m_released = false;
};

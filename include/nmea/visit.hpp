#pragma once

#include "nmea/message.hpp"

namespace nmea {
namespace internal {
template <typename... Ts> struct overload : Ts... {
    using Ts::operator()...;
};

} // namespace internal

template <typename... Visitors> auto visit(NmeaMessage &msg, Visitors &&...visitors) {
    return std::visit(internal::overload{std::forward<Visitors>(visitors)...}, msg);
}

template <typename... Visitors> auto visit(const NmeaMessage &msg, Visitors &&...visitors) {
    return std::visit(internal::overload{std::forward<Visitors>(visitors)...}, msg);
}
} // namespace nmea

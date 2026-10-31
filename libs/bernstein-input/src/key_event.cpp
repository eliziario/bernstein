#include "bernstein/input/key_event.hpp"

#include <utility>

namespace bernstein::input {
namespace {

bool is_continuation(char byte) {
    return (static_cast<unsigned char>(byte) & 0xC0U) == 0x80U;
}

} // namespace

KeyEvent KeyEvent::character(std::string value) {
    return KeyEvent{Key::Character, std::move(value)};
}

KeyEvent KeyEvent::of(Key key) {
    return KeyEvent{key, {}};
}

std::vector<std::string> split_code_points(std::string_view text) {
    std::vector<std::string> points;
    for (const char byte : text) {
        if (!is_continuation(byte) || points.empty()) {
            points.emplace_back(1, byte);
        } else {
            points.back().push_back(byte);
        }
    }
    return points;
}

void erase_last_code_point(std::string& text) {
    while (!text.empty() && is_continuation(text.back())) {
        text.pop_back();
    }
    if (!text.empty()) {
        text.pop_back();
    }
}

} // namespace bernstein::input

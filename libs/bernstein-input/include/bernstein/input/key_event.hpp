#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace bernstein::input {

// A keystroke named in Bernstein's own terms, so that turning input into intent does not depend
// on a terminal library. A terminal adapter maps whatever event type it has onto this one.
enum class Key {
    Unknown,
    Character,
    Submit,
    DeleteBefore,
    ClearDraft,
    CancelOperation,
    ReconcileOperation,
    Quit,
};

struct KeyEvent {
    Key key = Key::Unknown;
    std::string text; // the code point, when key is Key::Character

    [[nodiscard]] static KeyEvent character(std::string value);
    [[nodiscard]] static KeyEvent of(Key key);
};

// Splits UTF-8 text into whole code points, so that input arriving as a string can be fed
// through the same path as input arriving one keystroke at a time.
[[nodiscard]] std::vector<std::string> split_code_points(std::string_view text);

// Removes the last code point, which is not the same as removing the last byte.
void erase_last_code_point(std::string& text);

} // namespace bernstein::input

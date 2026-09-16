#include "headless.hpp"

#include "input_intent.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>

namespace bernstein::tui {
namespace {

void dispatch(core::FakeSession& session, ComposerState& composer, const ftxui::Event& event) {
    const auto snapshot = session.projection();
    auto translation = translate_event(event, std::move(composer), snapshot->operations);
    composer = std::move(translation.composer);
    if (translation.intent.has_value() &&
        !std::holds_alternative<core::Quit>(*translation.intent)) {
        session.dispatch_control(std::move(*translation.intent));
    }
}

} // namespace

std::string render_plain(const core::UiProjection& projection, std::string_view draft,
                         ViewSize size) {
    auto document = render_projection(projection, draft, size);
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(size.width),
                                        ftxui::Dimension::Fixed(size.height));
    ftxui::Render(screen, document);

    std::string output;
    for (int y = 0; y < screen.dimy(); ++y) {
        std::string line;
        bool previous_fullwidth = false;
        for (int x = 0; x < screen.dimx(); ++x) {
            const auto& character = screen.CellAt(x, y).character;
            if (previous_fullwidth) {
                previous_fullwidth = false;
                continue;
            }
            // Cells that no element painted hold an empty string.
            line += character.empty() ? " " : character;
            previous_fullwidth = ftxui::string_width(character) == 2;
        }
        line.erase(line.find_last_not_of(' ') + 1);
        output += line;
        output += '\n';
    }
    return output;
}

std::string run_headless(core::FakeSession& session, std::istream& input, ViewSize size) {
    ComposerState composer;
    std::string line;
    while (std::getline(input, line)) {
        for (const auto& glyph : ftxui::Utf8ToGlyphs(line)) {
            if (!glyph.empty()) {
                dispatch(session, composer, ftxui::Event::Character(glyph));
            }
        }
        dispatch(session, composer, ftxui::Event::Return);
        while (session.simulate_next().has_value()) {
        }
    }
    return render_plain(*session.projection(), composer.draft, size);
}

} // namespace bernstein::tui

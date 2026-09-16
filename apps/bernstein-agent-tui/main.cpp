#include "bernstein/core/session.hpp"
#include "projection_view.hpp"

#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

int main() {
    using namespace ftxui;

    bernstein::core::FakeSession session;
    auto document = bernstein::tui::render_projection(session.projection());
    auto screen = Screen::Create(Dimension::Fit(document), Dimension::Fit(document));
    Render(screen, document);
    screen.Print();

    return EXIT_SUCCESS;
}

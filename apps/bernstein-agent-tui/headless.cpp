#include "headless.hpp"

#include "projection_view.hpp"

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

namespace bernstein::tui {

std::string render_headless(const core::UiProjection& projection, std::string_view draft, int width,
                            int height) {
    auto document = render_projection(projection, draft);
    auto screen =
        ftxui::Screen::Create(ftxui::Dimension::Fixed(width), ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, document);
    return screen.ToString();
}

} // namespace bernstein::tui

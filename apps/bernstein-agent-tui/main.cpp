#include <cstdlib>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

int main() {
    using namespace ftxui;

    auto document =
        vbox({text("Bernstein TUI"), separator(), text("FTXUI screen target connected."),
              text("Projection rendering begins in the next change.")}) |
        border;
    auto screen = Screen::Create(Dimension::Fit(document), Dimension::Fit(document));
    Render(screen, document);
    screen.Print();

    return EXIT_SUCCESS;
}

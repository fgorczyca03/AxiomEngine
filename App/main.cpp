#include "AxiomEngine/core/Application.h"

int main() {
    // Keep the executable thin; Application owns the window and engine lifetime.
    axiom::core::Application app{};
    return app.Run();
}

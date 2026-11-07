#include "AxiomEngine/core/Application.h"

int main() {
    // The sandbox currently boots the same editor/runtime shell as AxiomRuntime.
    axiom::core::Application app;
    return app.Run();
}

#include "application.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

int main() {
    Application app;
    if (!app.init()) return 1;
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* pointer) {
        static_cast<Application*>(pointer)->frame();
    }, &app, 0, true);
#else
    while (!app.shouldClose()) app.frame();
    app.shutdown();
#endif
    return 0;
}

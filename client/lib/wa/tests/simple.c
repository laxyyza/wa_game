#include <wa.h>
#include <string.h>

int main(int argc, const char** argv)
{
    bool fullscreen = false;
    if (argc == 2)
        if (strcmp(argv[1], "fullscreen") == 0)
            fullscreen = true;

    wa_window_t* window = wa_window_create("WA Simple Window", 640, 480, fullscreen);
    wa_print_opengl();

    wa_window_mainloop(window);

    wa_window_delete(window);

    return 0;
}

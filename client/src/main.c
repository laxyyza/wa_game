#include "app.h"

static waapp_t app = {0};

int 
main(int argc, char* const* argv)
{
    if (waapp_init(&app, argc, argv) == -1)
        return -1;

    waapp_run(&app);

    waapp_cleanup(&app);

    return 0;
}

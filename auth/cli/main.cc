#include <stdio.h>
#include <string.h>
#include <CLI/App.hpp>
#include <CLI/CLI.hpp>
#include "cmd/register.h"
#include "cmd/unregister.h"
#include "cmd/enable.h"
#include "cmd/disable.h"
#include "cmd/config.h"

int main(int argc, char **argv)
{
    CLI::App app{"Facepass CLI"};
    app.require_subcommand(1);

    char *username = getenv("USER");
    if (username == NULL)
    {
        fprintf(stderr, "Failed to get the current username.\n");
        return 1;
    }
    setup_config(username);

    setup_register_command(app, username);
    setup_unregister_command(app, username);
    setup_enable_command(app);
    setup_disable_command(app);
    setup_config_command(app);

    CLI11_PARSE(app, argc, argv);
    return 0;
}
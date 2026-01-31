#include "unregister.h"

void setup_unregister_command(CLI::App &app, const std::string &username)
{
    CLI::App *unreg = app.add_subcommand("unregister", "Remove your face from the database.");
    unreg->callback([&]() {
        return remove_face(username);
    });
}
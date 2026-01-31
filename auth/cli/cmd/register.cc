#include "register.h"

void setup_register_command(CLI::App &app, const std::string &username)
{
    CLI::App *reg = app.add_subcommand("register", "Register your face.");
    reg->callback([&]() {
        return add_face(username);
    });
}
#include "disable.h"

void setup_disable_command(CLI::App &app)
{
    auto module_list = CLI::IsMember({"sudo", "su", "login"});
    CLI::App *disable = app.add_subcommand("disable", "Disable face authentication for modules.");
    shared_ptr<string> auth_module = make_shared<string>();
    disable->add_option("module", *auth_module, "Module to disable face authentication for (sudo, su, login).")
        ->check(module_list)
        ->default_val("login");
    disable->callback([auth_module]() {
        if (*auth_module != "login")
        {
            cerr << "Face authentication for " << *auth_module << " is not supported yet.\n";
            return;
        }
        bool result = disable_auth(*auth_module);
        if (result)
            cout << "Face authentication disabled for " << *auth_module << ".\n";
        else
            cerr << "Failed to disable face authentication for " << *auth_module << ".\n";
    });
}
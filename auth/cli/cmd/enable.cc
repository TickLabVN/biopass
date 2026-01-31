#include "enable.h"

void setup_enable_command(CLI::App &app)
{
    CLI::App *enable = app.add_subcommand("enable", "Enable face authentication for modules.");
    auto module_list = CLI::IsMember({"sudo", "su", "login"});
    // Must be a shared_ptr to avoid dangling reference
    // Because the callback is called after the function returns
    shared_ptr<string> auth_module = make_shared<string>();
    enable->add_option("module", *auth_module, "Module to enable face authentication for (sudo, su, login).")
        ->check(module_list)
        ->default_val("login");

    // Capture the shared pointer
    enable->callback([auth_module]() { // Capture the shared pointer
        if (*auth_module != "login")
        {
            cerr << "Face authentication for " << *auth_module << " is not supported yet.\n";
            return;
        }
        PAMConfig *config = load_current_config(*auth_module);
        // Error occurred while loading the config
        if (config == nullptr)
            return;

        bool result = enable_auth(*auth_module, *config);
        if (result)
            cout << "Face authentication enabled for " << *auth_module << ".\n";
        else
            cerr << "Failed to enable face authentication for " << *auth_module << ".\n";
        delete config; // Free the allocated memory
    });
}
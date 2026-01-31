#ifndef __UNREGISTER_CMD_H
#define __UNREGISTER_CMD_H

#include <string>
#include <CLI/App.hpp>
#include "face.h"

void setup_unregister_command(CLI::App &app, const std::string &username);

#endif // __UNREGISTER_CMD_H
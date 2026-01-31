#ifndef __REGISTER_CMD_H
#define __REGISTER_CMD_H

#include <CLI/App.hpp>
#include "face.h"

void setup_register_command(CLI::App &app, const std::string &username);

#endif // __REGISTER_CMD_H
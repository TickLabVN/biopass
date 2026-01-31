#ifndef __CONFIG_CMD_H
#define __CONFIG_CMD_H

#include <CLI/App.hpp>
#include "pam_conf.h"
#include <string>

using namespace std;

void setup_config_command(CLI::App &app);

#endif // __CONFIG_CMD_H
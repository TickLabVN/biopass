#ifndef __PAM_CONF_H__
#define __PAM_CONF_H__

#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <sys/stat.h>

using namespace std;

struct PAMConfig
{
    int num_retries;
    int gap;
    bool anti_spoof;
};

bool enable_auth(const string &, const PAMConfig &);
bool disable_auth(const string &);
PAMConfig* load_current_config(const string &);
bool is_auth_enabled(const string &);

#endif
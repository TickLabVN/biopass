#include "pam_conf.h"

void trim_string(string &str)
{
    // Trim leading whitespace
    str.erase(0, str.find_first_not_of(" \t"));
    // Trim trailing whitespace
    str.erase(str.find_last_not_of(" \t") + 1);
}

int count_module_above_pam_deny()
{
    ifstream file("/etc/pam.d/common-auth");
    if (!file.is_open())
    {
        cerr << "Error: Unable to open /etc/pam.d/common-auth for reading.\n";
        return -1;
    }

    string line;
    int count = 0;
    while (getline(file, line))
    {
        // Trim leading and trailing whitespace
        trim_string(line);

        // Skip comments and empty lines
        if (line.empty() || line[0] == '#')
            continue;

        if (line.find("pam_deny.so") != string::npos)
            break;

        if (line.find("auth") != string::npos)
            count++;
    }

    file.close();
    // Add 1 for the pam_deny.so module
    // For example of the common-auth file:
    // auth    [success=2 default=ignore]      pam_fingerprint.so
    // auth    [success=1 default=ignore]      pam_unix.so nullok
    // auth    requisite                       pam_deny.so
    // There're two modules above the pam_deny.so module
    // We should add the line "auth [success=3 default=ignore] libfacepass_pam.so"
    // to bypass all auth modules above pam_deny.so and pam_deny.so itself
    return count + 1;
}

string get_pam_config_file_path(string auth_module)
{
    string cfg_file;
    if (auth_module == "login")
    {
        // Display Manager  | Commonly Used With    | PAM Config File
        // GDM              | GNOME                 | /etc/pam.d/gdm-password
        // SDDM             | KDE Plasma            | /etc/pam.d/sddm
        // LightDM          | XFCE, MATE, Budgie    | /etc/pam.d/lightdm
        // LXDM             | LXDE                  | /etc/pam.d/lxdm
        // XDM              | Basic X environments  | /etc/pam.d/xdm
        // Entrance         | Enlightenment (E17+)  | /etc/pam.d/entrance
        const vector<string> pam_files = {
            "/etc/pam.d/gdm-password",
            "/etc/pam.d/sddm",
            "/etc/pam.d/lightdm",
            "/etc/pam.d/lxdm",
            "/etc/pam.d/xdm",
            "/etc/pam.d/entrance"};
        for (const auto &pam_file : pam_files)
        {
            struct stat buffer;
            if (stat(pam_file.c_str(), &buffer) != 0)
                continue;
            cfg_file = pam_file;
            break;
        }
    }
    else if (auth_module == "sudo")
        cfg_file = "/etc/pam.d/sudo";
    else if (auth_module == "su")
        cfg_file = "/etc/pam.d/su";
    return cfg_file;
}

bool disable_auth(const string &module)
{
    string cfg_file_path = get_pam_config_file_path(module);
    if (cfg_file_path.empty())
    {
        cerr << "Error: Unable to determine PAM configuration file for module: " << module << "\n";
        return false;
    }
    ifstream in_file(cfg_file_path);
    if (!in_file.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for reading.\n";
        return false;
    }

    string file_content;
    string line;

    // Read the file and check if the PAM configuration already exists
    while (getline(in_file, line))
    {
        trim_string(line);
        if (line.find("libfacepass_pam.so") != string::npos)
        {
            if (line[0] != '#')
                line = "# " + line;
        }

        file_content.append(line);
        file_content.append("\n");
    }
    in_file.close();

    ofstream out_file(cfg_file_path, std::ios::out | std::ios::trunc);
    if (!out_file.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for writing.\n";
        return false;
    }

    // Write the modified content back to the file
    out_file << file_content;
    out_file.close();

    return true;
}

bool enable_auth(const string &module, const PAMConfig &config)
{
    string cfg_file_path = get_pam_config_file_path(module);
    if (cfg_file_path.empty())
    {
        cerr << "Error: Unable to determine PAM configuration file for module: " << module << "\n";
        return false;
    }

    ifstream infile(cfg_file_path);
    if (!infile.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for reading.\n";
        return false;
    }

    string file_content;
    string line;
    bool config_exists = false;

    // Read the file and check if the PAM configuration already exists
    const int module_count = count_module_above_pam_deny();
    while (getline(infile, line))
    {
        trim_string(line);
        if (line.find("libfacepass_pam.so") != string::npos)
        {
            char pam_config_line[256];
            config_exists = true;
            sprintf(pam_config_line,
                    "auth [success=%d default=ignore] libfacepass_pam.so retries=%d retry_delay=%d%s",
                    module_count, config.num_retries,
                    config.gap, config.anti_spoof ? " anti_spoof" : "");
            line = pam_config_line;
        }

        file_content.append(line);
        file_content.append("\n");
    }
    infile.close();

    bool success = true;
    if (!config_exists)
    {
        // Insert the PAM configuration line before "@include common-auth"
        size_t pos = file_content.find("@include common-auth");
        if (pos != string::npos)
        {
            char pam_config_line[256];
            sprintf(pam_config_line,
                    "auth [success=%d default=ignore] libfacepass_pam.so retries=%d retry_delay=%d%s\n",
                    module_count, config.num_retries,
                    config.gap, config.anti_spoof ? " anti_spoof" : "");
            file_content.insert(pos, pam_config_line);
        }
        else
        {
            cerr << "Error: Invalid PAM configuration file: '@include common-auth' not found.\n";
            success = false;
        }
    }

    ofstream outfile(cfg_file_path, std::ios::out | std::ios::trunc);
    if (!outfile.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for writing.\n";
        return false;
    }
    // Write the modified content back to the file
    outfile << file_content;
    outfile.close();

    return success;
}

bool is_auth_enabled(const string &module)
{
    string cfg_file_path = get_pam_config_file_path(module);
    if (cfg_file_path.empty())
    {
        cerr << "Error: Unable to determine PAM configuration file for module: " << module << "\n";
        return false;
    }

    ifstream cfg_file(cfg_file_path);
    if (!cfg_file.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for reading.\n";
        return false;
    }

    string readline;
    bool enabled = false;
    while (getline(cfg_file, readline))
    {
        trim_string(readline);
        if (readline.find("libfacepass_pam.so") != string::npos)
        {
            enabled = readline[0] != '#';
            break;
        }
    }
    cfg_file.close();
    return enabled;
}

PAMConfig *load_current_config(const string &module)
{
    string cfg_file_path = get_pam_config_file_path(module);
    if (cfg_file_path.empty())
    {
        cerr << "Error: Unable to determine PAM configuration file for module: " << module << "\n";
        return NULL;
    }

    ifstream cfg_file(cfg_file_path);
    if (!cfg_file.is_open())
    {
        cerr << "Error: Unable to open " << cfg_file_path << " for reading.\n";
        return NULL;
    }

    // auth [success=2 default=ignore] libfacepass_pam.so retries=10 retry_delay=200 anti_spoof
    string readline, cfg_line;
    bool enabled = false;
    while (getline(cfg_file, readline))
    {
        trim_string(readline);
        if (readline.find("libfacepass_pam.so") != string::npos)
        {
            cfg_line = readline;
            break;
        }
    }
    cfg_file.close();

    // Parse the PAM configuration line
    PAMConfig *result = new PAMConfig {
        .num_retries = 10, // Default value
        .gap = 200,        // Default value
        .anti_spoof = false,
    };
    if (cfg_line.empty())
        return result;

    vector<string> tokens;
    stringstream ss(cfg_line);
    string token;

    // Tokenize the line by spaces/tabs
    while (ss >> token)
        tokens.push_back(token);

    for (const auto &t : tokens)
    {
        if (t.find("retries=") == 0)
            result->num_retries = stoi(t.substr(8));
        else if (t.find("retry_delay=") == 0)
            result->gap = stoi(t.substr(12));
        else if (t == "anti_spoof")
            result->anti_spoof = true;
    }
    return result;
}
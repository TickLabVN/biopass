#ifndef __FACEPASS_CONFIG_H__
#define __FACEPASS_CONFIG_H__

#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;

enum ModelType {
    FACE_DETECTION,
    FACE_RECOGNITION,
    FACE_ANTI_SPOOFING,
};

string user_face_path(const string &username);
string model_path(const string &username, const ModelType &modelType);
string debug_path(const string &username);
int setup_config(const string &username);

#endif // __FACEPASS_CONFIG_H__
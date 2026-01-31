#ifndef __IDENTIFY_H
#define __IDENTIFY_H

#include <stdio.h>
#include <string.h>
#include <security/_pam_types.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <opencv2/opencv.hpp>
#include "face_recognition.h"
#include "face_detection.h"
#include "face_config.h"
#include "face_as.h"
#include <random>
#include <sstream>

int scan_face(const string &, int8_t, const int, bool anti_spoofing = false);
#endif
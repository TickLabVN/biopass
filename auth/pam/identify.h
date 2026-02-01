#ifndef __IDENTIFY_H
#define __IDENTIFY_H

#include <security/_pam_types.h>
#include <stdio.h>
#include <string.h>

#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <random>
#include <sstream>
#include <thread>

#include "face_as.h"
#include "face_config.h"
#include "face_detection.h"
#include "face_recognition.h"

int scan_face(const string &, int8_t, const int, bool anti_spoofing = false);
#endif
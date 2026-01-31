#ifndef __FACE_CMD_H
#define __FACE_CMD_H

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <unistd.h>
#include <string>
#include "face_detection.h"
#include "face_config.h"

int capture_face(cv::Mat &);
cv::Mat detect_face(cv::Mat &);
int add_face(const std::string &);
int remove_face(const string &);

#endif // __FACE_CMD_H
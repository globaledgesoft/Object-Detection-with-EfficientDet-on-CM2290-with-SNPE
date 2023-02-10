#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <vector>
#include <map>
#include <unistd.h>
#include <algorithm>
#include <string>
#include <numeric>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>

#include <opencv2/opencv.hpp>
#include <android/asset_manager.h>
#include <android/native_window.h>
#include <jni.h>

//Macros for main.cpp
#define IOU_THRESHOLD 0.01
#define MOBILENET_IMG_HEIGHT 640
#define MOBILENET_IMG_WIDTH 640
#define LOOP_COUNT 150
#define OUT_FRAME_PATH "output.jpg"
#define OUTPUT_LAYER_1 "concat_34"
#define OUTPUT_LAYER_2 "concat_35"
#define ROI_CONFIG_PATH "./config.json"

#define BOXES_TENSOR "StatefulPartitionedCall:1"
#define SCORES_TENSOR "StatefulPartitionedCall:0"
#define CLASSES_TENSOR "detection_classes:0"
#define PERSON_CLASS_INDEX 1
#define PROBABILITY_THRESHOLD 0.5
#define AWS_SCRIPT_PATH "aws_send.py"

cv::Rect parse_json(std::string filepath);
float calculate_iou(cv::Rect rec1, cv::Rect rec2);
float find_average(std::vector<float> &vec);
std::vector<cv::Rect> postprocess(std::map<std::string, std::vector<float>> out, float video_height, float video_width);
bool aws_iot_send(char *time_str, unsigned int total_people_count, unsigned int inframe_count);

#endif
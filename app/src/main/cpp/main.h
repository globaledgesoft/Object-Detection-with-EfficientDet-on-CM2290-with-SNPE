
#ifndef OBJECTRECOGNITION_MAIN_H
#define OBJECTRECOGNITION_MAIN_H


#include <android/asset_manager.h>
#include <android/native_window.h>
#include <jni.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>
#include <opencv2/features2d.hpp>

#include "Image_R.h"
#include "Native_C.h"
#include "Util.h"

#include <unistd.h>
#include <time.h>

#include <cstdlib>
#include <string>
#include <vector>
#include <thread>
#include "qcs.hpp"
#include "utils.hpp"

class main{
public:
    main();
    ~main();
    main(const main& other) = delete;
    main& operator=(const main& other) = delete;

    void OnCreate();
    void OnPause();
    void OnDestroy();
    void SetJavaVM(JavaVM* pjava_vm) { java_vm = pjava_vm; }
    void SetNativeWindow(ANativeWindow* native_indow);

    void SetUpCamera();

    void Cameraiter();
private:
    JavaVM* java_vm;
    jobject calling_activity_obj;
    ANativeWindow* native_window;
    ANativeWindow_Buffer native_buffer;
    Native_C* native_camera;
    camera_type selected_camera_type = BACK_CAMERA; // Default
    ImageFormat m_view{0, 0, 0};
    Image_R* image_reader;
    AImage* m_image;

    volatile bool camera_ok;
    // for timing OpenCV bottlenecks
    clock_t start_t, end_t;
    // Used to detect up and down motion
    bool scan_mode;
    // OpenCV values
    cv::Mat crop;
    std::vector<int> class_indexs;
    bool camera_thread_stopped = false;
    Qcsnpe *qc;
    int i;
    int arr_cont[91];


    std::string model_path = "/storage/emulated/0/appData/model/EfficientDet.dlc";
    std::vector<cv::Rect> found;
    std::vector<int> class_indexes;
    std::vector<std::string> output_layers {OUTPUT_LAYER_1, OUTPUT_LAYER_2};
    std::vector<std::string> class_names {"person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat", "traffic light", "fire hydrant", "street sign", "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "hat", "backpack", "umbrella", "shoe", "eye glasses", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle", "plate", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch", "potted plant", "bed", "mirror", "dining table", "window", "desk", "toilet", "door", "tv", "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "blender", "book", "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush", "hair brush"};
    std::map<std::string, std::vector<float>> out;


    std::vector<cv::Rect> postprocess(std::map<std::string, std::vector<float>> out, float video_height, float video_width);
    float find_average(std::vector<float> &vec);
    };


#endif

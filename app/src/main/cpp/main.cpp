
#include "main.h"
#include <unistd.h>
#include <cmath>
#include <opencv2/core/core.hpp>
#include <string>
#include <cstdlib>
#include <mutex>
#include <glob.h>
#include <dirent.h>

std::mutex mutex;
#define OUTPUT_LAYER_1 "concat_34"
#define OUTPUT_LAYER_2 "concat_35"
const float CONF_THRESHOLD = 0.4;
const float NMS_THRESHOLD = 0.4;
const int N_OF_BBOXES =  76725;
std::string coco_labels[90] = {
        "person", "bicycle", "car", "motorcycle", "airplane",
        "bus", "train", "truck", "boat", "traffic light",
        "fire hydrant", "stop sign", "parking meter", "bench", "bird",
        "cat", "dog", "horse", "sheep", "cow",
        "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat",
        "baseball glove", "skateboard", "surfboard", "tennis racket", "bottle",
        "wine glass", "cup", "fork", "knife", "spoon",
        "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut",
        "cake", "chair", "couch", "potted plant", "bed",
        "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven",
        "toaster", "sink", "refrigerator", "book", "clock",
        "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
};

float preprocess_image(cv::Mat &image, cv::Mat &output, int image_size) {
    int img_h = image.rows;
    int img_w = image.cols;
    float scale;
    int resized_height, resized_width;
    if (img_h > img_w) {
        scale = (float)image_size / img_h;
        resized_height = image_size;
        resized_width = (int)(img_w * scale);
    } else {
        scale = (float)image_size / img_w;
        resized_height = (int)(img_h * scale);
        resized_width = image_size;
    }
    cv::resize(image, output, cv::Size(resized_width, resized_height));

    std::vector<float> mean{0.485, 0.456, 0.406};
    std::vector<float> std{0.229, 0.224, 0.225};


    int pad_h = image_size - resized_height;
    int pad_w = image_size - resized_width;
    cv::copyMakeBorder(output, output, 0, pad_h, 0, pad_w, cv::BORDER_CONSTANT, cv::Scalar::all(0));
    return scale;
}


float calculate_iou(std::vector<float> boxA, std::vector<float> boxB) {
    // Assumes boxA and boxB are in the format [xmin, ymin, xmax, ymax]
    float xminA = boxA[0], yminA = boxA[1], xmaxA = boxA[2], ymaxA = boxA[3];
    float xminB = boxB[0], yminB = boxB[1], xmaxB = boxB[2], ymaxB = boxB[3];

    // Calculate the intersection area
    float areaA = (xmaxA - xminA) * (ymaxA - yminA);
    float areaB = (xmaxB - xminB) * (ymaxB - yminB);
    float xminI = std::max(xminA, xminB), yminI = std::max(yminA, yminB);
    float xmaxI = std::min(xmaxA, xmaxB), ymaxI = std::min(ymaxA, ymaxB);
    float areaI = std::max(0.0f, xmaxI - xminI) * std::max(0.0f, ymaxI - yminI);

    // Calculate the union area
    float areaU = areaA + areaB - areaI;

    // Calculate the IOU
    return areaI / areaU;
}


void postprocess_boxes(std::vector<float> &boxes, float scale, int height, int width, int num_boxes) {

    for (int i = 0; i < num_boxes; i++) {
        boxes[i * 4] /= scale;
        boxes[i * 4 + 1] /= scale;
        boxes[i * 4 + 2] /= scale;
        boxes[i * 4 + 3] /= scale;

        boxes[i * 4] = std::max(std::min(boxes[i * 4], (float)(width - 1)), 0.0f);
        boxes[i * 4 + 1] = std::max(std::min(boxes[i * 4 + 1], (float)(height - 1)), 0.0f);
        boxes[i * 4 + 2] = std::max(std::min(boxes[i * 4 + 2], (float)(width - 1)), 0.0f);
        boxes[i * 4 + 3] = std::max(std::min(boxes[i * 4 + 3], (float)(height - 1)), 0.0f);
    }
}

void draw_boxes(cv::Mat &image2, std::vector<float> boxes, std::vector<float> scores, std::vector<int> labels) {
    int i = 0;
    int class_id, baseline;
    std::string score, class_name, label;

    for (int j = 0; j < labels.size(); j++) {
        class_id = labels[j];
        class_name = coco_labels[class_id];
        int xmin = boxes[j*4 + 0], ymin = boxes[j*4 + 1], xmax = boxes[j*4 + 2], ymax = boxes[j*4 + 3];
        score = std::to_string(scores[j]);
        std::string label = class_name + "-" + score;

        baseline = 0;
        cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
        cv::rectangle(image2, cv::Point(xmin, ymin), cv::Point(xmax, ymax), cv::Scalar(0,255,0), 3);
        cv::rectangle(image2, cv::Point(xmin, ymax - textSize.height - baseline), cv::Point(xmin + textSize.width, ymax), cv::Scalar(255,0,0), -1);

        cv::putText(image2, label, cv::Point(xmin, ymax - baseline), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1);

    }
}


main::main()
    : camera_ok(false), m_image(nullptr), image_reader(nullptr), native_camera(nullptr){}

main::~main(){
    JNIEnv *env;
    java_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
    env->DeleteGlobalRef(calling_activity_obj);
    calling_activity_obj = nullptr;

    // ACameraCaptureSession_stopRepeating(m_capture_session);
    if (native_camera != nullptr) {
        delete native_camera;
        native_camera = nullptr;
    }

    // make sure we don't leak native windows
    if (native_window != nullptr) {
        ANativeWindow_release(native_window);
        native_window = nullptr;
    }

    if (image_reader != nullptr) {
        delete (image_reader);
        image_reader = nullptr;
    }
}

void main::OnCreate() {

    output_layers = {OUTPUT_LAYER_1, OUTPUT_LAYER_2};
    qc = new Qcsnpe(model_path, 0, output_layers);
}


void main::OnPause() {}
void main::OnDestroy() {}

void main::SetNativeWindow(ANativeWindow* native_window) {
    // Save native window
    native_window = native_window;
}

void main::SetUpCamera() {

    native_camera = new Native_C(selected_camera_type);
    native_camera->MatchCaptureSizeRequest(&m_view,
                                             ANativeWindow_getWidth(native_window),
                                             ANativeWindow_getHeight(native_window));

    LOGI("______________mview %d\t %d\n", m_view.width, m_view.height);
    LOGI("______________mview %d\t %d\n", ANativeWindow_getWidth(native_window),ANativeWindow_getHeight(native_window));
    ASSERT(m_view.width && m_view.height, "Could not find supportable resolution");

    ANativeWindow_setBuffersGeometry(native_window, m_view.width, m_view.height,
                                     WINDOW_FORMAT_RGBX_8888);
    image_reader = new Image_R(&m_view, AIMAGE_FORMAT_YUV_420_888);
    image_reader->SetPresentRotation(native_camera->GetOrientation());
    ANativeWindow* image_reader_window = image_reader->GetNativeWindow();
    camera_ok = native_camera->CreateCaptureSession(image_reader_window);
}


void main::Cameraiter() {
    bool buffer_printout = false;
    while (1) {
        if (camera_thread_stopped) { break; }
        if (!camera_ok || !image_reader) { continue; }
        //reading the image from ndk reader
        m_image = image_reader->GetNextImage();
        if (m_image == nullptr) { continue; }

        ANativeWindow_acquire(native_window);
        ANativeWindow_Buffer buffer;
        if (ANativeWindow_lock(native_window, &buffer, nullptr) < 0) {
            image_reader->DeleteImage(m_image);
            m_image = nullptr;
            continue;
        }
        if (false == buffer_printout) {
            buffer_printout = true;
            LOGI("/// H-W-S-F: %d, %d, %d, %d", buffer.height, buffer.width, buffer.stride,
                 buffer.format);
        }

        image_reader->DisplayImage(&buffer, m_image);

        cv::Mat img_mat = cv::Mat(buffer.height, buffer.stride, CV_8UC4, buffer.bits);
        cv::Mat frame =  cv::Mat(img_mat.rows, img_mat.cols, CV_8UC3);
        cv::cvtColor(img_mat, frame, cv::COLOR_RGBA2BGR);
        cv::Mat image_copy = frame.clone();
        cv::Mat output;
        int input_width = 640;
        int input_height = 640;
        output = cv::Mat(input_height, input_width, CV_8UC3);

        int image_height = frame.rows;
        int image_width = frame.cols;
        float scale;

        scale = preprocess_image(frame, output, input_width);
        out = qc->predict(output);
        auto &bboxes = out["StatefulPartitionedCall:1"];
        auto &class_scores = out["StatefulPartitionedCall:0"];

        int num_detected_objects = 0;

        std::vector<int> label;
        std::vector<float> final_class_scores;
        std::vector<int> final_class_idx;
        std::vector<float> final_bboxes;

        for (int class_idx = 0; class_idx < 90; ++class_idx) {
            std::vector<float> class_scores_flat;
            std::vector<std::vector<float>> bboxes_flat;

            // Flatten the score and bbox array for each class
            for (int i = 0; i < N_OF_BBOXES; ++i) {
                float score = class_scores[0*N_OF_BBOXES*90 + i*90 + class_idx];
                if (score > CONF_THRESHOLD) {
                    class_scores_flat.push_back(score);
                    std::vector<float> a = {bboxes[0*N_OF_BBOXES*4 + i*4 + 0], bboxes[0*N_OF_BBOXES*4 + i*4 + 1], bboxes[0*N_OF_BBOXES*4 + i*4 + 2], bboxes[0*N_OF_BBOXES*4 + i*4 + 3]};
                    bboxes_flat.push_back(a);
                }
            }

            while (class_scores_flat.size() > 0) {
                // Find the index of the highest scoring bounding box
                int idx = -1;
                float max_score = -1;
                for (int i = 0; i < class_scores_flat.size(); ++i) {
                    if (class_scores_flat[i] > max_score) {
                        max_score = class_scores_flat[i];
                        idx = i;
                    }
                }

                // Create a vector to store the indexes of bounding boxes that need to be removed
                std::vector<int> to_remove;
                for (int i = 0; i < class_scores_flat.size(); ++i) {
                    if (i != idx) {
                        float ious = calculate_iou(bboxes_flat[idx], bboxes_flat[i]);
                        if (ious > NMS_THRESHOLD) {
                            to_remove.push_back(i);
                        }
                    }
                }

                // Remove the bounding boxes that are too close to each other
                for (int i = to_remove.size() - 1; i >= 0; --i) {
                    class_scores_flat.erase(class_scores_flat.begin() + to_remove[i]);
                    bboxes_flat.erase(bboxes_flat.begin() + to_remove[i]);
                    num_detected_objects--;
                }

                for (float coord : bboxes_flat[idx]) {
                    final_bboxes.push_back(coord);
                }
                if (bboxes_flat[idx].size() > 0)
                {
                    final_class_idx.push_back(class_idx);
                    final_class_scores.push_back(class_scores_flat[idx]);
                    label.push_back(class_idx);
                }

                if (idx < class_scores_flat.size()){
                    // remove the highest scoring box
                    class_scores_flat.erase(class_scores_flat.begin() + idx);
                    bboxes_flat.erase(bboxes_flat.begin() + idx);

                }

                num_detected_objects++;

            }

        }
        postprocess_boxes(final_bboxes, scale, image_height, image_width, final_class_idx.size());

        std::vector<int> indices;
        std::vector<float> final_selected_boxes;
        std::vector<float> final_selected_scores;
        std::vector<int> final_selected_labels;

        // filter with confidence
        for (int i = 0; i < final_class_scores.size(); i++) {
            if (final_class_scores[i] >= CONF_THRESHOLD) {
                indices.push_back(i);

                final_selected_labels.push_back(label[i]);
                final_selected_scores.push_back(final_class_scores[i]);
            }
        }

        for(auto index : indices)
        {
            final_selected_boxes.push_back(final_bboxes[index * 4 + 0]);
            final_selected_boxes.push_back(final_bboxes[index * 4 + 1]);
            final_selected_boxes.push_back(final_bboxes[index * 4 + 2]);
            final_selected_boxes.push_back(final_bboxes[index * 4 + 3]);

        }

        draw_boxes(img_mat, final_selected_boxes, final_selected_scores, final_selected_labels);

        out.clear();
        ANativeWindow_unlockAndPost(native_window);
        ANativeWindow_release(native_window);
    }
}

std::vector<cv::Rect> main::postprocess(std::map<std::string, std::vector<float>> out, float video_height, float video_width) {
    float probability;
    int class_index;

    auto &boxes = out[BOXES_TENSOR];
    auto &scores = out[SCORES_TENSOR];
    auto &classes = out[CLASSES_TENSOR];

    LOGI("%d\n", boxes.size());
    for(size_t cur=0; cur < scores.size(); cur++) {
        probability = scores[cur];

        if(probability < PROBABILITY_THRESHOLD)
            continue;
        auto y1 = static_cast<int>(boxes[4 * cur] * video_height);
        auto x1 = static_cast<int>(boxes[4 * cur + 1] * video_width);
        auto y2 = static_cast<int>(boxes[4 * cur + 2] * video_height);
        auto x2 = static_cast<int>(boxes[4 * cur + 3] * video_width);
        found.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
    }
    return found;
}

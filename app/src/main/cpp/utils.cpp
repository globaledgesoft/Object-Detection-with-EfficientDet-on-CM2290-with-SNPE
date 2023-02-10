
#include  "utils.hpp"
#include <android/log.h>


float find_average(std::vector<float> &vec) {
    return 1.0 * (std::accumulate(vec.begin(), vec.end(), 0.0) / vec.size());
}


std::vector<cv::Rect> postprocess(std::map<std::string, std::vector<float>> out, float video_height, float video_width) {
    float probability;
    int class_index;
    std::vector<cv::Rect> found;
    std::vector<int> class_indexs;

    auto &boxes = out[BOXES_TENSOR];
    auto &scores = out[SCORES_TENSOR];
    auto &classes = out[CLASSES_TENSOR];

    for(size_t cur=0; cur < scores.size(); cur++) {
        probability = scores[cur];
        class_index = static_cast<int>(classes[cur]);

        if(class_index != PERSON_CLASS_INDEX || probability < PROBABILITY_THRESHOLD)
            continue;

        auto y1 = static_cast<int>(boxes[4 * cur] * video_height);
        auto x1 = static_cast<int>(boxes[4 * cur + 1] * video_width);
        auto y2 = static_cast<int>(boxes[4 * cur + 2] * video_height);
        auto x2 = static_cast<int>(boxes[4 * cur + 3] * video_width);
        found.push_back(cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)));
        class_indexs.push_back(class_index);

    }
    return found;
}


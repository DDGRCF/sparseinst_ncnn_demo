#include <layer.h>
#include <net.h>

#include <stdio.h>

#include <vector>
#include <algorithm>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "common.h"

struct Object {
    Object() = default;
    Object(const Object & object) = default;
    Object(Object && object) = default;

    Object(const cv::Mat & m, 
           const int & l, 
           const float & p): mask(m), label(l), prob(p) {}
    cv::Mat mask;
    int label;
    float prob;
};

struct RunArgs {
    int target_size = 640;
    float conf_thre = 0.1;
	float mask_thre = 0.45;
    std::vector<std::string> class_names = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus",
        "train", "truck", "boat", "traffic light", "fire hydrant",
        "stop sign", "parking meter", "bench", "bird", "cat", "dog",
        "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe",
        "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat",
        "baseball glove", "skateboard", "surfboard", "tennis racket",
        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl",
        "banana", "apple", "sandwich", "orange", "broccoli", "carrot",
        "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop",
        "mouse", "remote", "keyboard", "cell phone", "microwave",
        "oven", "toaster", "sink", "refrigerator", "book", "clock",
        "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };
    std::vector<cv::Scalar> class_colors;
};

class SparseInstDetector {

public:
    SparseInstDetector() {
        state = 0;
    }
    SparseInstDetector(const std::string & param_path, 
                       const std::string & bin_path, 
                       const RunArgs & ra) {
        detector.opt.use_vulkan_compute = true;
        if ((state = detector.load_param(param_path.c_str())) != 0) {
            return;
        }
        if ((state = detector.load_model(bin_path.c_str())) != 0) {
            return;
        }
        run_args = ra;
    }
    ~SparseInstDetector() = default;

public:
    int get_state() noexcept {
        return state;
    }
    int load_structure(const std::string & param_path, const std::string & bin_path) noexcept {
        detector.clear(); 
        if (detector.load_param(param_path.c_str()) != 0) {
            return -1;
        }

        if (detector.load_model(bin_path.c_str()) != 0) {
            return -1;
        }
    }

    void load_run_args(const RunArgs & ra) noexcept {
        run_args = ra;
    }
public:
    int detect(const std::string & image_path, std::vector<Object> & objects) noexcept;
    int inference(const cv::Mat & image, std::vector<Object> & objects) noexcept;
    int visualize(cv::Mat & image, const std::vector<Object> & objects) noexcept;

private:
    int state;
    ncnn::Net detector;
    RunArgs run_args;
};


int SparseInstDetector::detect(const std::string & image_path, std::vector<Object> & objects) noexcept {
    int ret;
    cv::Mat image = cv::imread(image_path);
    if (image.empty()) {
        return -1;
    }

    ret = inference(image, objects);
    if (ret != 0) {
        return -1;
    }

    ret = visualize(image, objects);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int SparseInstDetector::inference(const cv::Mat & image, std::vector<Object> & objects) noexcept {
    int image_w = image.cols; int image_h = image.rows;

    int w = image_w; int h = image_h; float scale = 1.f;

    if (w > h) {
        scale = (float) run_args.target_size / w;
        w = run_args.target_size;
        h = h * scale;
    } else {
        scale = (float) run_args.target_size / h;
        h = run_args.target_size;
        w = w  * scale;
    }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(image.data, ncnn::Mat::PIXEL_BGR, image_w, image_h, w, h);

    int w_pad = run_args.target_size - w;
    int h_pad = run_args.target_size - h;

    ncnn::Mat in_pad;
    ncnn::copy_make_border(in, in_pad, 0, h_pad, 0, w_pad, ncnn::BORDER_CONSTANT, 114.f);

    ncnn::Extractor ex = detector.create_extractor();

    ncnn::Mat masks_out; ncnn::Mat scores_out;

    ex.input("input_image", in_pad);
    ex.extract("masks", masks_out);
    ex.extract("scores", scores_out);

    auto num_proposals = scores_out.h;
    auto num_classes = scores_out.w;
	auto mask_h = masks_out.h;
	auto mask_w = masks_out.w;

    auto ori_mask_h = (int) (mask_h / scale + 0.5); // confirm larger than ori
    auto ori_mask_w = (int) (mask_w / scale + 0.5); // confirm larger than ori
    for (int y = 0; y < num_proposals; y++) {
        float* scores_row = scores_out.row(y);
        ncnn::Mat mask_ncnn = masks_out.channel(y);
        int cls = std::max_element(scores_row, scores_row + num_classes) - scores_row;
		auto score = scores_row[cls];
		if (score < run_args.conf_thre) continue;
		cv::Mat ori_mask = cv::Mat::zeros(mask_w, mask_h, CV_32FC1);
        memcpy((uchar*)ori_mask.data, mask_ncnn.data, mask_ncnn.w * mask_ncnn.h * sizeof(float));
        cv::Mat tmp_mask = cv::Mat::zeros(mask_w, mask_h, CV_8UC1);
        tmp_mask = ori_mask > run_args.mask_thre;
        cv::Mat pad_mask = cv::Mat::zeros(ori_mask_w, ori_mask_h, CV_8UC1);
        cv::resize(tmp_mask, pad_mask, pad_mask.size());
        objects.emplace_back(pad_mask, cls, score);
    }
    return 0;
}

int SparseInstDetector::visualize(cv::Mat & image, const std::vector<Object> & objects) noexcept {
    cv::Scalar txt_color = cv::Scalar(38, 38, 38);
    for (auto i = 0; i < (int)objects.size(); i++) {
        const Object & object = objects[i];
        const cv::Mat & pad_mask = object.mask; 

        cv::Mat points;
        cv::findNonZero(pad_mask, points);
        cv::Rect min_rect = cv::boundingRect(points);

        // obj 
        const int & label = object.label;
        const float & prob = object.prob;
        const cv::Scalar & box_color = run_args.class_colors[label];

        // text
        char text[256];
        int base_line = 0;
        sprintf(text, "%s %.3f", run_args.class_names[label].c_str(), prob);
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &base_line);
        auto x = min_rect.x; auto y = min_rect.y;
        auto width = min_rect.width; auto height = min_rect.height;
        auto ctr_x = x + width / 2; auto ctr_y = y + height / 2;

        cv::rectangle(image, 
                      cv::Rect(cv::Point(ctr_x, ctr_y), cv::Size(label_size.width, label_size.height + base_line)), 
                      txt_color, -1);
        cv::putText(image, text, cv::Point(ctr_x, ctr_y + label_size.height + 1), cv::FONT_HERSHEY_SIMPLEX, 0.4, box_color, 1);

        // instance
        cv::Mat color_mask(image.rows, image.cols, CV_8UC3, box_color);
        cv::Mat mask = cv::Mat::zeros(color_mask.size(), CV_8UC3);
        color_mask.copyTo(mask, pad_mask(cv::Rect(0, 0, image.cols, image.rows)));
        cv::addWeighted(image, 1.0, mask, 0.5, 0., image);
    }
    cv::imwrite("results.jpg", image);
    return 0;
}

int main(int argc, char ** argv) {
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s [param_path] [bin_path] [image_path]\n", argv[0]);
        return -1;
    }

    const char* param_path = argv[1];
    const char* bin_path = argv[2];
    const char* image_path = argv[3];

    RunArgs run_args = {640, 0.2, 0.45};

    auto color_list_len = color_list.size();
    for (int i = 0; i < (int)run_args.class_names.size(); i++) {
        auto color = color_list[i % color_list_len];
        run_args.class_colors.emplace_back(cv::Scalar(color[0], color[1], color[2]));
    }

    SparseInstDetector detector(param_path, bin_path, run_args);
    if (detector.get_state() != 0) {
        fprintf(stderr, "initilize model fail!");
        return 0;
    }

    std::vector<Object> objects;
    if (detector.detect(image_path, objects) != 0) {
        fprintf(stderr, "detect image fail!");
        return -1;
    }

    return 0;
}

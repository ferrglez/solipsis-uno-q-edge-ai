#include <chrono>
// Copyright 2019 Tencent
// SPDX-License-Identifier: BSD-3-Clause

#include "net.h"

#if defined(USE_NCNN_SIMPLEOCV)
#include "simpleocv.h"
#else
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#endif
#include <stdio.h>
#include <vector>

struct FaceObject
{
    cv::Rect_<float> rect;
    cv::Point2f landmark[5];
    float prob;
};

static inline float intersection_area(const FaceObject& a, const FaceObject& b)
{
    cv::Rect_<float> inter = a.rect & b.rect;
    return inter.area();
}

static void qsort_descent_inplace(std::vector<FaceObject>& faceobjects, int left, int right)
{
    int i = left;
    int j = right;
    float p = faceobjects[(left + right) / 2].prob;

    while (i <= j)
    {
        while (faceobjects[i].prob > p)
            i++;

        while (faceobjects[j].prob < p)
            j--;

        if (i <= j)
        {
            // swap
            std::swap(faceobjects[i], faceobjects[j]);

            i++;
            j--;
        }
    }

    #pragma omp parallel sections
    {
        #pragma omp section
        {
            if (left < j) qsort_descent_inplace(faceobjects, left, j);
        }
        #pragma omp section
        {
            if (i < right) qsort_descent_inplace(faceobjects, i, right);
        }
    }
}

static void qsort_descent_inplace(std::vector<FaceObject>& faceobjects)
{
    if (faceobjects.empty())
        return;

    qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
}

static void nms_sorted_bboxes(const std::vector<FaceObject>& faceobjects, std::vector<int>& picked, float nms_threshold)
{
    picked.clear();

    const int n = faceobjects.size();

    std::vector<float> areas(n);
    for (int i = 0; i < n; i++)
    {
        areas[i] = faceobjects[i].rect.area();
    }

    for (int i = 0; i < n; i++)
    {
        const FaceObject& a = faceobjects[i];

        int keep = 1;
        for (int j = 0; j < (int)picked.size(); j++)
        {
            const FaceObject& b = faceobjects[picked[j]];

            // intersection over union
            float inter_area = intersection_area(a, b);
            float union_area = areas[i] + areas[picked[j]] - inter_area;
            //             float IoU = inter_area / union_area
            if (inter_area / union_area > nms_threshold)
                keep = 0;
        }

        if (keep)
            picked.push_back(i);
    }
}

// copy from src/layer/proposal.cpp
static ncnn::Mat generate_anchors(int base_size, const ncnn::Mat& ratios, const ncnn::Mat& scales)
{
    int num_ratio = ratios.w;
    int num_scale = scales.w;

    ncnn::Mat anchors;
    anchors.create(4, num_ratio * num_scale);

    const float cx = base_size * 0.5f;
    const float cy = base_size * 0.5f;

    for (int i = 0; i < num_ratio; i++)
    {
        float ar = ratios[i];

        int r_w = round(base_size / sqrt(ar));
        int r_h = round(r_w * ar); //round(base_size * sqrt(ar));

        for (int j = 0; j < num_scale; j++)
        {
            float scale = scales[j];

            float rs_w = r_w * scale;
            float rs_h = r_h * scale;

            float* anchor = anchors.row(i * num_scale + j);

            anchor[0] = cx - rs_w * 0.5f;
            anchor[1] = cy - rs_h * 0.5f;
            anchor[2] = cx + rs_w * 0.5f;
            anchor[3] = cy + rs_h * 0.5f;
        }
    }

    return anchors;
}

static void generate_proposals(const ncnn::Mat& anchors, int feat_stride, const ncnn::Mat& score_blob, const ncnn::Mat& bbox_blob, const ncnn::Mat& landmark_blob, float prob_threshold, std::vector<FaceObject>& faceobjects)
{
    int w = score_blob.w;
    int h = score_blob.h;

    // generate face proposal from bbox deltas and shifted anchors
    const int num_anchors = anchors.h;

    for (int q = 0; q < num_anchors; q++)
    {
        const float* anchor = anchors.row(q);

        const ncnn::Mat score = score_blob.channel(q + num_anchors);
        const ncnn::Mat bbox = bbox_blob.channel_range(q * 4, 4);
        const ncnn::Mat landmark = landmark_blob.channel_range(q * 10, 10);

        // shifted anchor
        float anchor_y = anchor[1];

        float anchor_w = anchor[2] - anchor[0];
        float anchor_h = anchor[3] - anchor[1];

        for (int i = 0; i < h; i++)
        {
            float anchor_x = anchor[0];

            for (int j = 0; j < w; j++)
            {
                int index = i * w + j;

                float prob = score[index];

                if (prob >= prob_threshold)
                {
                    // apply center size
                    float dx = bbox.channel(0)[index];
                    float dy = bbox.channel(1)[index];
                    float dw = bbox.channel(2)[index];
                    float dh = bbox.channel(3)[index];

                    float cx = anchor_x + anchor_w * 0.5f;
                    float cy = anchor_y + anchor_h * 0.5f;

                    float pb_cx = cx + anchor_w * dx;
                    float pb_cy = cy + anchor_h * dy;

                    float pb_w = anchor_w * exp(dw);
                    float pb_h = anchor_h * exp(dh);

                    float x0 = pb_cx - pb_w * 0.5f;
                    float y0 = pb_cy - pb_h * 0.5f;
                    float x1 = pb_cx + pb_w * 0.5f;
                    float y1 = pb_cy + pb_h * 0.5f;

                    FaceObject obj;
                    obj.rect.x = x0;
                    obj.rect.y = y0;
                    obj.rect.width = x1 - x0 + 1;
                    obj.rect.height = y1 - y0 + 1;
                    obj.landmark[0].x = cx + (anchor_w + 1) * landmark.channel(0)[index];
                    obj.landmark[0].y = cy + (anchor_h + 1) * landmark.channel(1)[index];
                    obj.landmark[1].x = cx + (anchor_w + 1) * landmark.channel(2)[index];
                    obj.landmark[1].y = cy + (anchor_h + 1) * landmark.channel(3)[index];
                    obj.landmark[2].x = cx + (anchor_w + 1) * landmark.channel(4)[index];
                    obj.landmark[2].y = cy + (anchor_h + 1) * landmark.channel(5)[index];
                    obj.landmark[3].x = cx + (anchor_w + 1) * landmark.channel(6)[index];
                    obj.landmark[3].y = cy + (anchor_h + 1) * landmark.channel(7)[index];
                    obj.landmark[4].x = cx + (anchor_w + 1) * landmark.channel(8)[index];
                    obj.landmark[4].y = cy + (anchor_h + 1) * landmark.channel(9)[index];
                    obj.prob = prob;

                    faceobjects.push_back(obj);
                }

                anchor_x += feat_stride;
            }

            anchor_y += feat_stride;
        }
    }
}

static int detect_retinaface(const cv::Mat& bgr, std::vector<FaceObject>& faceobjects, bool use_gpu)
{
    ncnn::Net retinaface;

    if (use_gpu)
        retinaface.set_vulkan_device(0);

    retinaface.opt.use_vulkan_compute = use_gpu;

    // Experimental configuration: FP16 arithmetic ON, storage OFF
    retinaface.opt.use_fp16_packed = false;
    retinaface.opt.use_fp16_storage = false;
    retinaface.opt.use_fp16_arithmetic = true;


    fprintf(stderr,
            "NCNN_OPTIONS vulkan=%d fp16_packed=%d fp16_storage=%d fp16_arithmetic=%d\n",
            (int)retinaface.opt.use_vulkan_compute,
            (int)retinaface.opt.use_fp16_packed,
            (int)retinaface.opt.use_fp16_storage,
            (int)retinaface.opt.use_fp16_arithmetic);


    // model is converted from
    // https://github.com/deepinsight/insightface/tree/master/RetinaFace#retinaface-pretrained-models
    // https://github.com/deepinsight/insightface/issues/669
    // the ncnn model https://github.com/nihui/ncnn-assets/tree/master/models
    //     retinaface.load_param("retinaface-R50.param");
    //     retinaface.load_model("retinaface-R50.bin");
    if (retinaface.load_param("mnet.25-opt.param"))
        exit(-1);
    if (retinaface.load_model("mnet.25-opt.bin"))
        exit(-1);

    const float prob_threshold = 0.8f;
    const float nms_threshold = 0.4f;

    int img_w = bgr.cols;
    int img_h = bgr.rows;

    ncnn::Mat in = ncnn::Mat::from_pixels(bgr.data, ncnn::Mat::PIXEL_BGR2RGB, img_w, img_h);

    double total_ms = 0.0;

    for (int iteration = 0; iteration < 10; iteration++)
    {
        faceobjects.clear();
        double proposals_ms = 0.0;

        auto start = std::chrono::steady_clock::now();


    ncnn::Extractor ex = retinaface.create_extractor();

    ex.input("data", in);

    std::vector<FaceObject> faceproposals;

    // stride 32
    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
	auto t0 = std::chrono::steady_clock::now();
        ex.extract("face_rpn_cls_prob_reshape_stride32", score_blob);
        ex.extract("face_rpn_bbox_pred_stride32", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride32", landmark_blob);

	auto t1 = std::chrono::steady_clock::now();
	
	double extract32_ms =
		std::chrono::duration<double, std::milli>(t1 - t0).count();
	fprintf(stderr, "EXTRACT_STRIDE32 %s: %.3f ms\n",
		use_gpu ? "GPU" : "CPU", extract32_ms);

        const int base_size = 16;
        const int feat_stride = 32;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 32.f;
        scales[1] = 16.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<FaceObject> faceobjects32;
        auto gp32_start = std::chrono::steady_clock::now();
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects32);
        auto gp32_end = std::chrono::steady_clock::now();
        proposals_ms += std::chrono::duration<double, std::milli>(
            gp32_end - gp32_start).count();

        faceproposals.insert(faceproposals.end(), faceobjects32.begin(), faceobjects32.end());
    }

    // stride 16
    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
      
	auto t16_start = std::chrono::steady_clock::now();

        ex.extract("face_rpn_cls_prob_reshape_stride16", score_blob);
        ex.extract("face_rpn_bbox_pred_stride16", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride16", landmark_blob);

	auto t16_end = std::chrono::steady_clock::now();

	double extract16_ms =
    		std::chrono::duration<double, std::milli>(
        		t16_end - t16_start
    		).count();

	fprintf(stderr, "EXTRACT_STRIDE16 %s: %.3f ms\n",
        	use_gpu ? "GPU" : "CPU",
        	extract16_ms);

        const int base_size = 16;
        const int feat_stride = 16;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 8.f;
        scales[1] = 4.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<FaceObject> faceobjects16;
        auto gp16_start = std::chrono::steady_clock::now();
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects16);
        auto gp16_end = std::chrono::steady_clock::now();
        proposals_ms += std::chrono::duration<double, std::milli>(
            gp16_end - gp16_start).count();

        faceproposals.insert(faceproposals.end(), faceobjects16.begin(), faceobjects16.end());
    }

    // stride 8
    {
        ncnn::Mat score_blob, bbox_blob, landmark_blob;
     	auto t8_start = std::chrono::steady_clock::now();

        ex.extract("face_rpn_cls_prob_reshape_stride8", score_blob);
        ex.extract("face_rpn_bbox_pred_stride8", bbox_blob);
        ex.extract("face_rpn_landmark_pred_stride8", landmark_blob);
	
	auto t8_end = std::chrono::steady_clock::now();

	double extract8_ms =
    		std::chrono::duration<double, std::milli>(
        		t8_end - t8_start
    		).count();

	fprintf(stderr, "EXTRACT_STRIDE8 %s: %.3f ms\n",
        	use_gpu ? "GPU" : "CPU",
        	extract8_ms);

        const int base_size = 16;
        const int feat_stride = 8;
        ncnn::Mat ratios(1);
        ratios[0] = 1.f;
        ncnn::Mat scales(2);
        scales[0] = 2.f;
        scales[1] = 1.f;
        ncnn::Mat anchors = generate_anchors(base_size, ratios, scales);

        std::vector<FaceObject> faceobjects8;
        auto gp8_start = std::chrono::steady_clock::now();
        generate_proposals(anchors, feat_stride, score_blob, bbox_blob, landmark_blob, prob_threshold, faceobjects8);
        auto gp8_end = std::chrono::steady_clock::now();
        proposals_ms += std::chrono::duration<double, std::milli>(
            gp8_end - gp8_start).count();

        faceproposals.insert(faceproposals.end(), faceobjects8.begin(), faceobjects8.end());
    }

    auto post_start = std::chrono::steady_clock::now();

    // sort all proposals by score from highest to lowest
    qsort_descent_inplace(faceproposals);

    // apply nms with nms_threshold
    std::vector<int> picked;
    nms_sorted_bboxes(faceproposals, picked, nms_threshold);

    int face_count = picked.size();

    faceobjects.resize(face_count);
    for (int i = 0; i < face_count; i++)
    {
        faceobjects[i] = faceproposals[picked[i]];

        // clip to image size
        float x0 = faceobjects[i].rect.x;
        float y0 = faceobjects[i].rect.y;
        float x1 = x0 + faceobjects[i].rect.width;
        float y1 = y0 + faceobjects[i].rect.height;

        x0 = std::max(std::min(x0, (float)img_w - 1), 0.f);
        y0 = std::max(std::min(y0, (float)img_h - 1), 0.f);
        x1 = std::max(std::min(x1, (float)img_w - 1), 0.f);
        y1 = std::max(std::min(y1, (float)img_h - 1), 0.f);

        faceobjects[i].rect.x = x0;
        faceobjects[i].rect.y = y0;
        faceobjects[i].rect.width = x1 - x0;
        faceobjects[i].rect.height = y1 - y0;
    }


        auto post_end = std::chrono::steady_clock::now();

        double post_ms = std::chrono::duration<double, std::milli>(
            post_end - post_start).count();

        fprintf(stderr,
                "POSTPROCESS_TOTAL %s: %.3f ms (proposals=%.3f, sort_nms_clip=%.3f)\n",
                use_gpu ? "GPU" : "CPU",
                proposals_ms + post_ms,
                proposals_ms,
                post_ms);

        auto end_time = std::chrono::steady_clock::now();

        double elapsed_ms =
            std::chrono::duration<double, std::milli>(
                end_time - start
            ).count();

        fprintf(stderr, "ITERATION %d %s: %.3f ms\n",
                iteration + 1,
                use_gpu ? "GPU" : "CPU",
                elapsed_ms);

        if (iteration > 0)
            total_ms += elapsed_ms;
    }

    fprintf(stderr, "AVERAGE %s: %.3f ms\n",
            use_gpu ? "GPU" : "CPU",
            total_ms / 9.0);

    return 0;
}

static void draw_faceobjects(const cv::Mat& bgr, const std::vector<FaceObject>& faceobjects)
{
    cv::Mat image = bgr.clone();

    for (size_t i = 0; i < faceobjects.size(); i++)
    {
        const FaceObject& obj = faceobjects[i];

        fprintf(stderr, "%.5f at %.2f %.2f %.2f x %.2f\n", obj.prob,
                obj.rect.x, obj.rect.y, obj.rect.width, obj.rect.height);

        cv::rectangle(image, obj.rect, cv::Scalar(0, 255, 0));

        cv::circle(image, obj.landmark[0], 2, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, obj.landmark[1], 2, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, obj.landmark[2], 2, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, obj.landmark[3], 2, cv::Scalar(0, 255, 255), -1);
        cv::circle(image, obj.landmark[4], 2, cv::Scalar(0, 255, 255), -1);

        char text[256];
        sprintf(text, "%.1f%%", obj.prob * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

        int x = obj.rect.x;
        int y = obj.rect.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > image.cols)
            x = image.cols - label_size.width;

        cv::rectangle(image, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      cv::Scalar(255, 255, 255), -1);

        cv::putText(image, text, cv::Point(x, y + label_size.height),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0));
    }

    if (cv::imwrite("retinaface_result.jpg", image))
        fprintf(stderr, "Imagen guardada: retinaface_result.jpg\n");
    else
        fprintf(stderr, "ERROR: No se pudo guardar la imagen\n");
}

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s [imagepath]\n", argv[0]);
        return -1;
    }

    const char* imagepath = argv[1];

    bool use_gpu = false;

    if (strcmp(argv[2], "gpu") == 0)
        use_gpu = true;
    else if (strcmp(argv[2], "cpu") != 0)
    {
        fprintf(stderr, "Modo invalido. Usa cpu o gpu.\\n");
        return -1;
    }

    cv::Mat m = cv::imread(imagepath, 1);
    if (m.empty())
    {
        fprintf(stderr, "cv::imread %s failed\n", imagepath);
        return -1;
    }

    std::vector<FaceObject> faceobjects;
    detect_retinaface(m, faceobjects, use_gpu);

    draw_faceobjects(m, faceobjects);

    return 0;
}

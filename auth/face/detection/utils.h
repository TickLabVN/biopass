#ifndef FACE_DET_UTILS_H
#define FACE_DET_UTILS_H

// CPP native
#include <vector>

struct RawDet {
  float x1, y1, x2, y2, conf;
  int cls;
};

std::vector<RawDet> non_max_suppression(const float* output, int num_preds, int pred_dim,
                                        float conf_thres = 0.25, float iou_thres = 0.45,
                                        int max_det = 300);

std::vector<int> nms_boxes(const std::vector<RawDet>& dets, float iou_threshold);

void scale_boxes(const std::vector<int>& img1_shape, std::vector<RawDet>& dets,
                 const std::vector<int>& img0_shape);

#endif  // FACE_DET_UTILS_H

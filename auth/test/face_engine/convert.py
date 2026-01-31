import torch
from ultralytics import YOLO

model = YOLO("./weights/yolov8n-face.pt").to(torch.device("cuda"))
model.export(format="torchscript", imgsz=(640, 640))
# model.export(format="onnx", imgsz=(640, 640), opset=12)
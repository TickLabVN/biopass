import type { VideoDeviceInfo } from "@/types/config";
import { invokeCommand } from "./core";

function listImages() {
  return invokeCommand<string[]>("list_faces");
}

function saveImage(data: string) {
  return invokeCommand<string>("capture_face", { data });
}

function deleteImage(path: string) {
  return invokeCommand<void>("delete_face", { path });
}

function listVideoDevices() {
  return invokeCommand<VideoDeviceInfo[]>("list_video_devices");
}

function captureCameraFrame(devicePath: string) {
  return invokeCommand<string>("capture_camera_frame", {
    devicePath,
  });
}

function startCameraPreview(devicePath: string) {
  return invokeCommand<void>("start_camera_preview", {
    devicePath,
  });
}

function getPreviewFrame() {
  return invokeCommand<string | null>("get_preview_frame");
}

function stopCameraPreview() {
  return invokeCommand<void>("stop_camera_preview");
}

export const face = {
  listImages,
  saveImage,
  deleteImage,
  listVideoDevices,
  captureCameraFrame,
  startCameraPreview,
  getPreviewFrame,
  stopCameraPreview,
};

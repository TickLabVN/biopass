import type { VideoDeviceInfo } from "@/types/config";
import { invokeCommand } from "./core";

function listImages() {
  return invokeCommand<string[]>("list_faces");
}

function saveImage(data: string) {
  return invokeCommand<string>("capture_face", { data });
}

function captureNative(camera?: string | null) {
  return invokeCommand<string>("capture_face_native", {
    camera: camera ?? null,
  });
}

function startPreview(camera?: string | null) {
  return invokeCommand<void>("start_face_preview", { camera: camera ?? null });
}

function stopPreview() {
  return invokeCommand<void>("stop_face_preview");
}

function captureInSession() {
  return invokeCommand<string>("capture_face_in_session");
}

function deleteImage(path: string) {
  return invokeCommand<void>("delete_face", { path });
}

function listVideoDevices() {
  return invokeCommand<VideoDeviceInfo[]>("list_video_devices");
}
export const face = {
  listImages,
  saveImage,
  captureNative,
  startPreview,
  stopPreview,
  captureInSession,
  deleteImage,
  listVideoDevices,
};

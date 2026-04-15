#include "camera_capture.h"

#include <fcntl.h>
#include <linux/videodev2.h>
#include <openpnp-capture.h>
#include <poll.h>
#include <spdlog/spdlog.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace biopass {

namespace {

constexpr int kDefaultWarmupFrames = 5;
constexpr int kDefaultCaptureTimeoutMs = 10000;
constexpr int kDefaultCapturePollIntervalMs = 10;

void captureLog();
std::optional<CapDeviceID> resolveCameraDeviceIdx(
    CapContext ctx, const std::optional<std::string>& linux_video_device_path);

std::string device_label(const std::optional<std::string>& linux_video_device_path) {
  return linux_video_device_path.has_value() ? *linux_video_device_path : std::string("<default>");
}

std::vector<std::string> enumerate_linux_video_capture_paths() {
  std::vector<std::string> paths;
  constexpr uint32_t kMaxDevices = 64;

  for (uint32_t index = 0; index < kMaxDevices; ++index) {
    char device_path[32];
    std::snprintf(device_path, sizeof(device_path), "/dev/video%u", index);

    const int fd = ::open(device_path, O_RDWR | O_NONBLOCK);
    if (fd == -1) {
      continue;
    }

    v4l2_capability video_cap {};
    if (::ioctl(fd, VIDIOC_QUERYCAP, &video_cap) == 0 &&
        (video_cap.device_caps & V4L2_CAP_VIDEO_CAPTURE) != 0) {
      paths.emplace_back(device_path);
    }

    ::close(fd);
  }

  return paths;
}

std::optional<std::string> linux_device_path_from_index(CapDeviceID device_index) {
  const auto paths = enumerate_linux_video_capture_paths();
  if (device_index >= paths.size()) {
    return std::nullopt;
  }
  return paths[device_index];
}

int xioctl_retry(int fd, unsigned long request, void* arg) {
  int rc = 0;
  do {
    rc = ::ioctl(fd, request, arg);
  } while (rc == -1 && errno == EINTR);
  return rc;
}

std::optional<CapFormatInfo> find_camera_format_by_fourcc(CapContext ctx, CapDeviceID device_index,
                                                           uint32_t fourcc) {
  const int32_t format_count = Cap_getNumFormats(ctx, device_index);
  if (format_count <= 0) {
    return std::nullopt;
  }

  for (int32_t format_index = 0; format_index < format_count; ++format_index) {
    CapFormatInfo format {};
    if (Cap_getFormatInfo(ctx, device_index, static_cast<CapFormatID>(format_index), &format) !=
        CAPRESULT_OK) {
      continue;
    }
    if (format.fourcc == fourcc) {
      return format;
    }
  }

  return std::nullopt;
}

bool capture_frame_openpnp(CapContext ctx, CapStream stream, uint8_t* buffer, size_t buffer_size,
                           int capture_timeout_ms, int poll_interval_ms) {
  const auto capture_deadline = std::chrono::steady_clock::now() +
                                std::chrono::milliseconds(std::max(0, capture_timeout_ms));
  const bool has_timeout = capture_timeout_ms > 0;
  const auto sleep_interval = std::chrono::milliseconds(std::max(1, poll_interval_ms));

  while (!has_timeout || std::chrono::steady_clock::now() < capture_deadline) {
    if (!Cap_hasNewFrame(ctx, stream)) {
      std::this_thread::sleep_for(sleep_interval);
      continue;
    }

    if (Cap_captureFrame(ctx, stream, buffer, buffer_size) != CAPRESULT_OK) {
      continue;
    }

    return true;
  }

  return false;
}

class OpenPnpCameraSession : public ICameraCaptureSession {
 public:
  OpenPnpCameraSession(CapContext ctx, CapStream stream, uint32_t width, uint32_t height,
                       std::string camera_label, int capture_timeout_ms, int poll_interval_ms)
      : ctx_(ctx),
        stream_(stream),
        width_(width),
        height_(height),
        camera_label_(std::move(camera_label)),
        capture_timeout_ms_(capture_timeout_ms),
        poll_interval_ms_(poll_interval_ms),
        buffer_(static_cast<size_t>(width) * static_cast<size_t>(height) * 3) {}

  ~OpenPnpCameraSession() override { close(); }

  bool isOpen() const override { return ctx_ != nullptr && stream_ >= 0; }

  ImageRGB capture() override {
    if (!isOpen()) {
      return {};
    }

    if (!capture_frame_openpnp(ctx_, stream_, buffer_.data(), buffer_.size(), capture_timeout_ms_,
                               poll_interval_ms_)) {
      spdlog::error("FaceAuth: Failed to capture frame from '{}'", camera_label_);
      close();
      return {};
    }

    return ImageRGB(static_cast<int>(width_), static_cast<int>(height_), buffer_.data());
  }

 private:
  void close() {
    if (ctx_ && stream_ >= 0) {
      Cap_closeStream(ctx_, stream_);
      stream_ = -1;
    }
    if (ctx_) {
      Cap_releaseContext(ctx_);
      ctx_ = nullptr;
    }
  }

  CapContext ctx_ = nullptr;
  CapStream stream_ = -1;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  std::string camera_label_;
  int capture_timeout_ms_ = 0;
  int poll_interval_ms_ = 0;
  std::vector<uint8_t> buffer_;
};

class V4L2GreyCameraSession : public ICameraCaptureSession {
 public:
  V4L2GreyCameraSession(std::string device_path, const CapFormatInfo& format, int warmup_frames,
                        int capture_timeout_ms, int poll_interval_ms)
      : device_path_(std::move(device_path)),
        warmup_frames_(std::max(0, warmup_frames)),
        capture_timeout_ms_(capture_timeout_ms),
        poll_interval_ms_(std::max(1, poll_interval_ms)) {
    if (format.fourcc != V4L2_PIX_FMT_GREY) {
      spdlog::error("FaceAuth: V4L2 GREY session requires GREY camera format");
      return;
    }

    fd_ = ::open(device_path_.c_str(), O_RDWR | O_NONBLOCK);
    if (fd_ == -1) {
      spdlog::error("FaceAuth: Failed to open {} for V4L2 fallback: {}", device_path_,
                    std::strerror(errno));
      return;
    }

    v4l2_format v4l2_format_info {};
    v4l2_format_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    v4l2_format_info.fmt.pix.width = format.width;
    v4l2_format_info.fmt.pix.height = format.height;
    v4l2_format_info.fmt.pix.pixelformat = format.fourcc;
    v4l2_format_info.fmt.pix.field = V4L2_FIELD_NONE;
    if (xioctl_retry(fd_, VIDIOC_S_FMT, &v4l2_format_info) == -1) {
      spdlog::error("FaceAuth: VIDIOC_S_FMT failed for {}: {}", device_path_, std::strerror(errno));
      close();
      return;
    }

    if (format.fps > 0) {
      v4l2_streamparm stream_params {};
      stream_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      stream_params.parm.capture.timeperframe.numerator = 1;
      stream_params.parm.capture.timeperframe.denominator = format.fps;
      xioctl_retry(fd_, VIDIOC_S_PARM, &stream_params);
    }

    width_ = v4l2_format_info.fmt.pix.width;
    height_ = v4l2_format_info.fmt.pix.height;
    bytes_per_line_ =
        std::max<uint32_t>(v4l2_format_info.fmt.pix.bytesperline, v4l2_format_info.fmt.pix.width);

    v4l2_requestbuffers request_buffers {};
    request_buffers.count = 4;
    request_buffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request_buffers.memory = V4L2_MEMORY_MMAP;
    if (xioctl_retry(fd_, VIDIOC_REQBUFS, &request_buffers) == -1 || request_buffers.count == 0) {
      spdlog::error("FaceAuth: VIDIOC_REQBUFS failed for {}: {}", device_path_, std::strerror(errno));
      close();
      return;
    }

    buffers_.resize(request_buffers.count);
    for (uint32_t index = 0; index < request_buffers.count; ++index) {
      v4l2_buffer buffer {};
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;
      buffer.index = index;
      if (xioctl_retry(fd_, VIDIOC_QUERYBUF, &buffer) == -1) {
        spdlog::error("FaceAuth: VIDIOC_QUERYBUF failed for {}: {}", device_path_,
                      std::strerror(errno));
        close();
        return;
      }

      buffers_[index].length = buffer.length;
      buffers_[index].start = ::mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                                     fd_, buffer.m.offset);
      if (buffers_[index].start == MAP_FAILED) {
        spdlog::error("FaceAuth: mmap failed for {}: {}", device_path_, std::strerror(errno));
        close();
        return;
      }
    }

    for (uint32_t index = 0; index < buffers_.size(); ++index) {
      v4l2_buffer buffer {};
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;
      buffer.index = index;
      if (xioctl_retry(fd_, VIDIOC_QBUF, &buffer) == -1) {
        spdlog::error("FaceAuth: VIDIOC_QBUF failed for {}: {}", device_path_, std::strerror(errno));
        close();
        return;
      }
    }

    if (xioctl_retry(fd_, VIDIOC_STREAMON, &buffer_type_) == -1) {
      spdlog::error("FaceAuth: VIDIOC_STREAMON failed for {}: {}", device_path_,
                    std::strerror(errno));
      close();
      return;
    }

    stream_started_ = true;
  }

  ~V4L2GreyCameraSession() override { close(); }

  bool isOpen() const override { return fd_ >= 0 && stream_started_; }

  ImageRGB capture() override {
    if (!isOpen()) {
      return {};
    }

    const int total_frames_needed = warmup_frames_ + 1;
    int captured_frames = 0;
    const bool has_timeout = capture_timeout_ms_ > 0;
    const auto capture_deadline = std::chrono::steady_clock::now() +
                                  std::chrono::milliseconds(std::max(0, capture_timeout_ms_));

    while (captured_frames < total_frames_needed) {
      pollfd poll_info {};
      poll_info.fd = fd_;
      poll_info.events = POLLIN;

      int poll_timeout_ms = -1;
      if (has_timeout) {
        const auto now = std::chrono::steady_clock::now();
        if (now >= capture_deadline) {
          spdlog::error("FaceAuth: Timed out waiting for V4L2 GREY frame from {}", device_path_);
          close();
          return {};
        }

        const auto remaining_ms =
            std::chrono::duration_cast<std::chrono::milliseconds>(capture_deadline - now).count();
        poll_timeout_ms =
            std::max(1, std::min(poll_interval_ms_, static_cast<int>(remaining_ms)));
      }

      const int poll_rc = ::poll(&poll_info, 1, poll_timeout_ms);
      if (poll_rc == -1) {
        if (errno == EINTR) {
          continue;
        }
        spdlog::error("FaceAuth: poll failed for {}: {}", device_path_, std::strerror(errno));
        close();
        return {};
      }
      if (poll_rc == 0) {
        continue;
      }

      v4l2_buffer buffer {};
      buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      buffer.memory = V4L2_MEMORY_MMAP;
      if (xioctl_retry(fd_, VIDIOC_DQBUF, &buffer) == -1) {
        if (errno == EAGAIN) {
          continue;
        }
        spdlog::error("FaceAuth: VIDIOC_DQBUF failed for {}: {}", device_path_,
                      std::strerror(errno));
        close();
        return {};
      }

      const uint8_t* grey = static_cast<const uint8_t*>(buffers_.at(buffer.index).start);
      ++captured_frames;

      ImageRGB image;
      if (captured_frames >= total_frames_needed) {
        image = ImageRGB(static_cast<int>(width_), static_cast<int>(height_));
        for (uint32_t y = 0; y < height_; ++y) {
          const uint8_t* src_row = grey + y * bytes_per_line_;
          uint8_t* dst_row = image.ptr() + y * width_ * 3;
          for (uint32_t x = 0; x < width_; ++x) {
            const uint8_t value = src_row[x];
            dst_row[x * 3 + 0] = value;
            dst_row[x * 3 + 1] = value;
            dst_row[x * 3 + 2] = value;
          }
        }
      }

      if (xioctl_retry(fd_, VIDIOC_QBUF, &buffer) == -1) {
        spdlog::error("FaceAuth: VIDIOC_QBUF failed for {}: {}", device_path_, std::strerror(errno));
        close();
        return {};
      }

      if (!image.empty()) {
        return image;
      }
    }

    return {};
  }

 private:
  struct MappedBuffer {
    void* start = MAP_FAILED;
    size_t length = 0;
  };

  void close() {
    if (fd_ >= 0 && stream_started_) {
      xioctl_retry(fd_, VIDIOC_STREAMOFF, &buffer_type_);
      stream_started_ = false;
    }

    for (auto& buffer : buffers_) {
      if (buffer.start != MAP_FAILED) {
        ::munmap(buffer.start, buffer.length);
        buffer.start = MAP_FAILED;
        buffer.length = 0;
      }
    }
    buffers_.clear();

    if (fd_ >= 0) {
      ::close(fd_);
      fd_ = -1;
    }
  }

  std::string device_path_;
  int fd_ = -1;
  uint32_t width_ = 0;
  uint32_t height_ = 0;
  uint32_t bytes_per_line_ = 0;
  int warmup_frames_ = 0;
  int capture_timeout_ms_ = 0;
  int poll_interval_ms_ = 0;
  std::vector<MappedBuffer> buffers_;
  v4l2_buf_type buffer_type_ = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  bool stream_started_ = false;
};

void captureLogCallback(uint32_t level, const char* message) {
  if (!message) {
    return;
  }
  if (std::strstr(message, "tjDecompressHeader2 failed: No error") != nullptr) {
    return;
  }

  std::string msg(message);
  while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
    msg.pop_back();
  }

  if (level <= 3) {
    spdlog::error("openpnp-capture: {}", msg);
  } else if (level == 4) {
    spdlog::warn("openpnp-capture: {}", msg);
  } else {
    spdlog::info("openpnp-capture: {}", msg);
  }
}

void captureLog() {
  static std::once_flag once;
  std::call_once(once, []() { Cap_installCustomLogFunction(captureLogCallback); });
}

std::optional<CapDeviceID> resolveCameraDeviceIdx(
    CapContext ctx, const std::optional<std::string>& linux_video_device_path) {
  const uint32_t count = Cap_getDeviceCount(ctx);
  if (count == 0) {
    spdlog::error("FaceAuth: No camera devices reported by openpnp-capture");
    return std::nullopt;
  }

  if (linux_video_device_path.has_value()) {
    const auto capture_paths = enumerate_linux_video_capture_paths();
    for (size_t index = 0; index < capture_paths.size(); ++index) {
      if (capture_paths[index] == *linux_video_device_path) {
        if (index < count) {
          return static_cast<CapDeviceID>(index);
        }
        spdlog::error(
            "FaceAuth: Camera path '{}' resolved to index {} but openpnp reports only {} device(s)",
            *linux_video_device_path, index, count);
        return std::nullopt;
      }
    }

    for (uint32_t i = 0; i < count; ++i) {
      const char* uid = Cap_getDeviceUniqueID(ctx, static_cast<CapDeviceID>(i));
      if (uid && std::string(uid).find(*linux_video_device_path) != std::string::npos) {
        return static_cast<CapDeviceID>(i);
      }
    }

    spdlog::error("FaceAuth: Camera path '{}' was not found among capture-capable /dev/video* devices",
                  *linux_video_device_path);
    return std::nullopt;
  }

  return static_cast<CapDeviceID>(0);
}

}  // namespace

bool checkCameraAvailability(const std::optional<std::string>& linux_video_device_path) {
  auto session = openCameraSession(linux_video_device_path);
  return session && session->isOpen();
}

std::unique_ptr<ICameraCaptureSession> openCameraSession(
    const std::optional<std::string>& linux_video_device_path, CameraCaptureFormat format,
    int warmup_frames, int capture_timeout_ms, int poll_interval_ms) {
  captureLog();
  CapContext ctx = Cap_createContext();
  if (!ctx) {
    spdlog::error("FaceAuth: Failed to create capture context for '{}'",
                  device_label(linux_video_device_path));
    return nullptr;
  }

  const auto device_index = resolveCameraDeviceIdx(ctx, linux_video_device_path);
  if (!device_index.has_value()) {
    Cap_releaseContext(ctx);
    return nullptr;
  }

  if (format == CameraCaptureFormat::V4L2Grey) {
    if (!linux_video_device_path.has_value()) {
      spdlog::error("FaceAuth: Direct V4L2 GREY capture requires a /dev/video* path");
      Cap_releaseContext(ctx);
      return nullptr;
    }

    const auto grey_format = find_camera_format_by_fourcc(ctx, *device_index, V4L2_PIX_FMT_GREY);
    if (!grey_format.has_value()) {
      spdlog::error("FaceAuth: Camera '{}' does not expose a GREY format for direct V4L2 capture",
                    *linux_video_device_path);
      Cap_releaseContext(ctx);
      return nullptr;
    }

    Cap_releaseContext(ctx);
    auto session = std::make_unique<V4L2GreyCameraSession>(*linux_video_device_path, *grey_format,
                                                           warmup_frames, capture_timeout_ms,
                                                           poll_interval_ms);
    if (!session->isOpen()) {
      return nullptr;
    }
    return session;
  }

  CapFormatInfo fmt {};
  CapResult fmt_result = Cap_getFormatInfo(ctx, *device_index, 0, &fmt);
  if (fmt_result != CAPRESULT_OK) {
    spdlog::error("FaceAuth: Failed to get camera format info for '{}' (index {}, code {})",
                  device_label(linux_video_device_path), *device_index, static_cast<int>(fmt_result));
    Cap_releaseContext(ctx);
    return nullptr;
  }

  if (fmt.fourcc == V4L2_PIX_FMT_GREY) {
    auto linux_path = linux_video_device_path;
    if (!linux_path.has_value()) {
      linux_path = linux_device_path_from_index(*device_index);
    }

    if (!linux_path.has_value()) {
      spdlog::error(
          "FaceAuth: Device index {} requires GREY fallback but has no resolvable /dev/video path",
          *device_index);
      Cap_releaseContext(ctx);
      return nullptr;
    }

    spdlog::warn(
        "FaceAuth: Device '{}' reports GREY format; using V4L2 GREY fallback on '{}'",
        device_label(linux_video_device_path), *linux_path);
    Cap_releaseContext(ctx);
    auto session = std::make_unique<V4L2GreyCameraSession>(*linux_path, fmt, warmup_frames,
                                                           capture_timeout_ms, poll_interval_ms);
    if (!session->isOpen()) {
      return nullptr;
    }
    return session;
  }

  CapStream stream = Cap_openStream(ctx, *device_index, 0);
  if (stream < 0 || !Cap_isOpenStream(ctx, stream)) {
    spdlog::error("FaceAuth: Failed to open camera stream for '{}' (index {})",
                  device_label(linux_video_device_path), *device_index);
    Cap_releaseContext(ctx);
    return nullptr;
  }

  auto session = std::make_unique<OpenPnpCameraSession>(
      ctx, stream, fmt.width, fmt.height, device_label(linux_video_device_path), capture_timeout_ms,
      poll_interval_ms);
  if (!session->isOpen()) {
    return nullptr;
  }
  return session;
}

ImageRGB captureImage(const std::optional<std::string>& linux_video_device_path,
                      CameraCaptureFormat format) {
  auto session = openCameraSession(linux_video_device_path, format, kDefaultWarmupFrames,
                                   kDefaultCaptureTimeoutMs, kDefaultCapturePollIntervalMs);
  if (!session) {
    return {};
  }

  return session->capture();
}

ImageRGB captureImageByIRCamera(const std::string& device_path, int warmup_frames,
                                int capture_timeout_ms, int poll_interval_ms) {
  if (device_path.empty()) {
    spdlog::error("FaceAuth: IR camera capture requires a /dev/video* path");
    return {};
  }

  auto session = openCameraSession(device_path, CameraCaptureFormat::V4L2Grey, warmup_frames,
                                   capture_timeout_ms, poll_interval_ms);
  if (!session) {
    return {};
  }

  return session->capture();
}

}  // namespace biopass

#include <CLI/CLI.hpp>
#include <openpnp-capture.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#ifdef __linux__
#include <fcntl.h>
#include <linux/videodev2.h>
#include <poll.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <unistd.h>
#endif

#include "image_utils.h"

namespace {

struct CaptureDeviceInfo {
  CapDeviceID index;
  std::string name;
  std::string unique_id;
};

enum class CaptureBackend {
  OpenPnp,
#ifdef __linux__
  V4L2Grey,
#endif
};

std::string fourcc_to_string(uint32_t fourcc) {
  char text[5];
  text[0] = static_cast<char>(fourcc & 0xff);
  text[1] = static_cast<char>((fourcc >> 8) & 0xff);
  text[2] = static_cast<char>((fourcc >> 16) & 0xff);
  text[3] = static_cast<char>((fourcc >> 24) & 0xff);
  text[4] = '\0';

  for (int i = 0; i < 4; ++i) {
    if (text[i] == '\0' || !std::isprint(static_cast<unsigned char>(text[i]))) {
      text[i] = '.';
    }
  }

  return text;
}

std::vector<CaptureDeviceInfo> enumerate_devices(CapContext ctx) {
  std::vector<CaptureDeviceInfo> devices;
  const uint32_t count = Cap_getDeviceCount(ctx);
  devices.reserve(count);
  for (uint32_t index = 0; index < count; ++index) {
    const char *name = Cap_getDeviceName(ctx, index);
    const char *unique_id = Cap_getDeviceUniqueID(ctx, index);
    devices.push_back({
        index,
        name ? name : "",
        unique_id ? unique_id : "",
    });
  }
  return devices;
}

bool is_openpnp_supported_fourcc(uint32_t fourcc) {
#ifdef __linux__
  return fourcc == V4L2_PIX_FMT_RGB24 || fourcc == V4L2_PIX_FMT_YUYV || fourcc == V4L2_PIX_FMT_NV12 ||
         fourcc == V4L2_PIX_FMT_MJPEG;
#else
  return true;
#endif
}

const char *backend_name(CaptureBackend backend) {
  switch (backend) {
    case CaptureBackend::OpenPnp:
      return "openpnp";
#ifdef __linux__
    case CaptureBackend::V4L2Grey:
      return "v4l2-grey-fallback";
#endif
  }

  return "unknown";
}

#ifdef __linux__
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

std::optional<CapDeviceID> resolve_linux_device_path(const std::string &device_path) {
  const auto paths = enumerate_linux_video_capture_paths();
  for (size_t index = 0; index < paths.size(); ++index) {
    if (paths[index] == device_path) {
      return static_cast<CapDeviceID>(index);
    }
  }
  return std::nullopt;
}

std::optional<std::string> linux_device_path_from_index(CapDeviceID device_index) {
  const auto paths = enumerate_linux_video_capture_paths();
  if (device_index >= paths.size()) {
    return std::nullopt;
  }
  return paths[device_index];
}

int xioctl_retry(int fd, unsigned long request, void *arg) {
  int rc = 0;
  do {
    rc = ::ioctl(fd, request, arg);
  } while (rc == -1 && errno == EINTR);
  return rc;
}
#endif

bool is_unsigned_number(const std::string &value) {
  return !value.empty() &&
         std::all_of(value.begin(), value.end(),
                     [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

void print_devices(CapContext ctx) {
  const auto devices = enumerate_devices(ctx);
#ifdef __linux__
  const auto linux_paths = enumerate_linux_video_capture_paths();
#endif

  if (devices.empty()) {
    std::cout << "No capture devices found.\n";
    return;
  }

  std::cout << "Available devices:\n";
  for (const auto &device : devices) {
    std::cout << "  [" << device.index << "] " << device.name << '\n';
    std::cout << "      unique id: " << device.unique_id << '\n';
#ifdef __linux__
    if (device.index < linux_paths.size()) {
      std::cout << "      linux path: " << linux_paths[device.index] << '\n';
    }
#endif
  }
}

void print_formats(CapContext ctx, CapDeviceID device_index) {
  const int32_t format_count = Cap_getNumFormats(ctx, device_index);
  if (format_count <= 0) {
    std::cout << "No formats reported for device " << device_index << ".\n";
    return;
  }

  std::cout << "Available formats for device " << device_index << ":\n";
  for (int32_t format_index = 0; format_index < format_count; ++format_index) {
    CapFormatInfo format {};
    if (Cap_getFormatInfo(ctx, device_index, static_cast<CapFormatID>(format_index), &format) !=
        CAPRESULT_OK) {
      continue;
    }

    const char *format_backend = nullptr;
    if (is_openpnp_supported_fourcc(format.fourcc)) {
      format_backend = "openpnp";
    }
#ifdef __linux__
    else if (format.fourcc == V4L2_PIX_FMT_GREY) {
      format_backend = "v4l2-grey-fallback";
    }
#endif
    else {
      format_backend = "unsupported";
    }

    std::cout << "  [" << format_index << "] " << format.width << 'x' << format.height
              << " fourcc=" << fourcc_to_string(format.fourcc) << " fps=" << format.fps
              << " backend=" << format_backend << '\n';
  }
}

CapFormatInfo get_format_info_or_throw(CapContext ctx, CapDeviceID device_index,
                                       CapFormatID format_index) {
  CapFormatInfo format {};
  if (Cap_getFormatInfo(ctx, device_index, format_index, &format) != CAPRESULT_OK) {
    throw std::runtime_error("Failed to query format " + std::to_string(format_index) +
                             " for device " + std::to_string(device_index));
  }
  return format;
}

CapFormatID choose_format(CapContext ctx, CapDeviceID device_index, std::optional<uint32_t> requested) {
  const int32_t format_count = Cap_getNumFormats(ctx, device_index);
  if (format_count <= 0) {
    throw std::runtime_error("No formats reported for device " + std::to_string(device_index));
  }

  if (requested.has_value()) {
    if (*requested >= static_cast<uint32_t>(format_count)) {
      throw std::runtime_error("Format index out of range: " + std::to_string(*requested));
    }
    return *requested;
  }

  for (int32_t format_index = 0; format_index < format_count; ++format_index) {
    const CapFormatInfo format =
        get_format_info_or_throw(ctx, device_index, static_cast<CapFormatID>(format_index));
    if (is_openpnp_supported_fourcc(format.fourcc)) {
      return static_cast<CapFormatID>(format_index);
    }
#ifdef __linux__
    if (format.fourcc == V4L2_PIX_FMT_GREY) {
      return static_cast<CapFormatID>(format_index);
    }
#endif
  }

  return 0;
}

CapDeviceID resolve_device_selector(CapContext ctx, const std::string &selector) {
  const auto devices = enumerate_devices(ctx);
  if (devices.empty()) {
    throw std::runtime_error("No capture devices found.");
  }

  if (is_unsigned_number(selector)) {
    const auto parsed = static_cast<uint32_t>(std::stoul(selector));
    if (parsed >= devices.size()) {
      throw std::runtime_error("Device index out of range: " + selector);
    }
    return parsed;
  }

#ifdef __linux__
  if (selector.rfind("/dev/video", 0) == 0) {
    const auto resolved = resolve_linux_device_path(selector);
    if (!resolved.has_value()) {
      throw std::runtime_error("No capture device matches path " + selector);
    }
    if (*resolved >= devices.size()) {
      throw std::runtime_error("Resolved device path is outside the OpenPnP device range: " +
                               selector);
    }
    return *resolved;
  }
#endif

  for (const auto &device : devices) {
    if (selector == device.unique_id || selector == device.name) {
      return device.index;
    }
  }

  std::vector<CapDeviceID> partial_matches;
  for (const auto &device : devices) {
    if (device.unique_id.find(selector) != std::string::npos ||
        device.name.find(selector) != std::string::npos) {
      partial_matches.push_back(device.index);
    }
  }

  if (partial_matches.size() == 1) {
    return partial_matches.front();
  }

  if (partial_matches.size() > 1) {
    throw std::runtime_error("Device selector is ambiguous: " + selector);
  }

  throw std::runtime_error("No device matches selector: " + selector);
}

ImageRGB capture_frame(CapContext ctx, CapDeviceID device_index, CapFormatID format_index,
                       int warmup_frames, int attempts, std::chrono::milliseconds poll_interval) {
  const CapFormatInfo format = get_format_info_or_throw(ctx, device_index, format_index);

  const CapStream stream = Cap_openStream(ctx, device_index, format_index);
  if (stream < 0 || !Cap_isOpenStream(ctx, stream)) {
    throw std::runtime_error("Failed to open capture stream for device " +
                             std::to_string(device_index) + " format " +
                             std::to_string(format_index));
  }

  struct StreamCloser {
    CapContext ctx;
    CapStream stream;
    ~StreamCloser() { Cap_closeStream(ctx, stream); }
  } stream_closer {ctx, stream};

  std::vector<uint8_t> buffer(format.width * format.height * 3);

  int warmed = 0;
  for (int attempt = 0; attempt < attempts && warmed < warmup_frames; ++attempt) {
    if (Cap_hasNewFrame(ctx, stream)) {
      if (Cap_captureFrame(ctx, stream, buffer.data(), buffer.size()) == CAPRESULT_OK) {
        ++warmed;
      }
    } else {
      std::this_thread::sleep_for(poll_interval);
    }
  }

  for (int attempt = 0; attempt < attempts; ++attempt) {
    if (Cap_hasNewFrame(ctx, stream)) {
      if (Cap_captureFrame(ctx, stream, buffer.data(), buffer.size()) == CAPRESULT_OK) {
        return ImageRGB(static_cast<int>(format.width), static_cast<int>(format.height),
                        buffer.data());
      }
    } else {
      std::this_thread::sleep_for(poll_interval);
    }
  }

  throw std::runtime_error("Timed out while waiting for a frame from device " +
                           std::to_string(device_index));
}

#ifdef __linux__
ImageRGB capture_frame_v4l2_grey(const std::string &device_path, const CapFormatInfo &format,
                                 int warmup_frames, int attempts,
                                 std::chrono::milliseconds poll_interval) {
  if (format.fourcc != V4L2_PIX_FMT_GREY) {
    throw std::runtime_error("V4L2 grey fallback only supports GREY format.");
  }

  const int fd = ::open(device_path.c_str(), O_RDWR | O_NONBLOCK);
  if (fd == -1) {
    throw std::runtime_error("Failed to open " + device_path + ": " + std::strerror(errno));
  }

  struct FileCloser {
    int fd;
    ~FileCloser() {
      if (fd >= 0) {
        ::close(fd);
      }
    }
  } file_closer {fd};

  v4l2_format v4l2_format_info {};
  v4l2_format_info.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  v4l2_format_info.fmt.pix.width = format.width;
  v4l2_format_info.fmt.pix.height = format.height;
  v4l2_format_info.fmt.pix.pixelformat = format.fourcc;
  v4l2_format_info.fmt.pix.field = V4L2_FIELD_NONE;
  if (xioctl_retry(fd, VIDIOC_S_FMT, &v4l2_format_info) == -1) {
    throw std::runtime_error("VIDIOC_S_FMT failed for " + device_path + ": " +
                             std::strerror(errno));
  }

  if (format.fps > 0) {
    v4l2_streamparm stream_params {};
    stream_params.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    stream_params.parm.capture.timeperframe.numerator = 1;
    stream_params.parm.capture.timeperframe.denominator = format.fps;
    xioctl_retry(fd, VIDIOC_S_PARM, &stream_params);
  }

  v4l2_requestbuffers request_buffers {};
  request_buffers.count = 4;
  request_buffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  request_buffers.memory = V4L2_MEMORY_MMAP;
  if (xioctl_retry(fd, VIDIOC_REQBUFS, &request_buffers) == -1) {
    throw std::runtime_error("VIDIOC_REQBUFS failed for " + device_path + ": " +
                             std::strerror(errno));
  }
  if (request_buffers.count == 0) {
    throw std::runtime_error("No V4L2 buffers were allocated for " + device_path);
  }

  struct MappedBuffer {
    void *start = MAP_FAILED;
    size_t length = 0;
  };
  std::vector<MappedBuffer> buffers(request_buffers.count);

  for (uint32_t index = 0; index < request_buffers.count; ++index) {
    v4l2_buffer buffer {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    if (xioctl_retry(fd, VIDIOC_QUERYBUF, &buffer) == -1) {
      throw std::runtime_error("VIDIOC_QUERYBUF failed for " + device_path + ": " +
                               std::strerror(errno));
    }

    buffers[index].length = buffer.length;
    buffers[index].start = ::mmap(nullptr, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                                  buffer.m.offset);
    if (buffers[index].start == MAP_FAILED) {
      throw std::runtime_error("mmap failed for " + device_path + ": " + std::strerror(errno));
    }
  }

  struct BufferUnmapper {
    std::vector<MappedBuffer> &buffers;
    ~BufferUnmapper() {
      for (const auto &buffer : buffers) {
        if (buffer.start != MAP_FAILED) {
          ::munmap(buffer.start, buffer.length);
        }
      }
    }
  } buffer_unmapper {buffers};

  for (uint32_t index = 0; index < request_buffers.count; ++index) {
    v4l2_buffer buffer {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = index;
    if (xioctl_retry(fd, VIDIOC_QBUF, &buffer) == -1) {
      throw std::runtime_error("VIDIOC_QBUF failed for " + device_path + ": " +
                               std::strerror(errno));
    }
  }

  v4l2_buf_type buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (xioctl_retry(fd, VIDIOC_STREAMON, &buffer_type) == -1) {
    throw std::runtime_error("VIDIOC_STREAMON failed for " + device_path + ": " +
                             std::strerror(errno));
  }

  struct StreamStopper {
    int fd;
    v4l2_buf_type type;
    ~StreamStopper() { xioctl_retry(fd, VIDIOC_STREAMOFF, &type); }
  } stream_stopper {fd, buffer_type};

  ImageRGB image(static_cast<int>(v4l2_format_info.fmt.pix.width),
                 static_cast<int>(v4l2_format_info.fmt.pix.height));
  const uint32_t bytes_per_line =
      std::max<uint32_t>(v4l2_format_info.fmt.pix.bytesperline, v4l2_format_info.fmt.pix.width);
  const int total_frames_needed = std::max(0, warmup_frames) + 1;
  int captured_frames = 0;

  for (int attempt = 0; attempt < attempts; ++attempt) {
    pollfd poll_info {};
    poll_info.fd = fd;
    poll_info.events = POLLIN;
    const int poll_rc = ::poll(&poll_info, 1, static_cast<int>(poll_interval.count()));
    if (poll_rc == -1) {
      if (errno == EINTR) {
        continue;
      }
      throw std::runtime_error("poll failed for " + device_path + ": " + std::strerror(errno));
    }
    if (poll_rc == 0) {
      continue;
    }

    v4l2_buffer buffer {};
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    if (xioctl_retry(fd, VIDIOC_DQBUF, &buffer) == -1) {
      if (errno == EAGAIN) {
        continue;
      }
      throw std::runtime_error("VIDIOC_DQBUF failed for " + device_path + ": " +
                               std::strerror(errno));
    }

    const uint8_t *grey =
        static_cast<const uint8_t *>(buffers.at(buffer.index).start);
    ++captured_frames;
    if (captured_frames >= total_frames_needed) {
      for (uint32_t y = 0; y < v4l2_format_info.fmt.pix.height; ++y) {
        const uint8_t *src_row = grey + y * bytes_per_line;
        uint8_t *dst_row = image.ptr() + y * v4l2_format_info.fmt.pix.width * 3;
        for (uint32_t x = 0; x < v4l2_format_info.fmt.pix.width; ++x) {
          const uint8_t value = src_row[x];
          dst_row[x * 3 + 0] = value;
          dst_row[x * 3 + 1] = value;
          dst_row[x * 3 + 2] = value;
        }
      }

      if (xioctl_retry(fd, VIDIOC_QBUF, &buffer) == -1) {
        throw std::runtime_error("VIDIOC_QBUF failed for " + device_path + ": " +
                                 std::strerror(errno));
      }
      return image;
    }

    if (xioctl_retry(fd, VIDIOC_QBUF, &buffer) == -1) {
      throw std::runtime_error("VIDIOC_QBUF failed for " + device_path + ": " +
                               std::strerror(errno));
    }
  }

  throw std::runtime_error("Timed out while waiting for a GREY frame from " + device_path);
}
#endif

}  // namespace

int main(int argc, char **argv) {
  namespace fs = std::filesystem;

  CLI::App app("Capture a single image frame from a camera with OpenPnP Capture.");

  std::string device_selector;
  std::string output_path = "capture.jpg";
  uint32_t format_index = 0;
  int warmup_frames = 5;
  int attempts = 1000;
  int poll_interval_ms = 10;
  bool list_devices = false;
  bool list_formats = false;

  app.add_option("device", device_selector,
                 "Device selector. Accepts an OpenPnP device index, name, unique id, or a "
                 "Linux path like /dev/video0.");
  app.add_flag("--list-devices", list_devices, "List available capture devices and exit.");
  app.add_flag("--list-formats", list_formats,
               "List available formats for the selected device and exit.");
  app.add_option("-o,--output", output_path, "Output image path.")
      ->default_val(output_path);
  auto *format_option = app.add_option("-f,--format", format_index,
                                       "OpenPnP format index to capture with.")
      ->default_val(format_index);
  app.add_option("--warmup-frames", warmup_frames, "Frames to discard before capture.")
      ->default_val(warmup_frames);
  app.add_option("--attempts", attempts, "Polling attempts before giving up.")
      ->default_val(attempts);
  app.add_option("--poll-interval-ms", poll_interval_ms, "Sleep between frame polls.")
      ->default_val(poll_interval_ms);

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError &e) {
    return app.exit(e);
  }

  if (!list_devices && device_selector.empty()) {
    std::cerr << "A device selector is required unless --list-devices is used.\n";
    return 1;
  }

  CapContext ctx = Cap_createContext();
  if (!ctx) {
    std::cerr << "Failed to create OpenPnP capture context.\n";
    return 1;
  }

  struct ContextCloser {
    CapContext ctx;
    ~ContextCloser() { Cap_releaseContext(ctx); }
  } context_closer {ctx};

  try {
    if (list_devices) {
      print_devices(ctx);
      if (device_selector.empty()) {
        return 0;
      }
    }

    const CapDeviceID device_index = resolve_device_selector(ctx, device_selector);
    const auto devices = enumerate_devices(ctx);
    const auto &device = devices.at(device_index);
    const CapFormatID selected_format =
        choose_format(ctx, device_index,
                      format_option->count() > 0 ? std::optional<uint32_t>(format_index)
                                                 : std::nullopt);
    const CapFormatInfo format = get_format_info_or_throw(ctx, device_index, selected_format);

    if (list_formats) {
      std::cout << "Device [" << device.index << "] " << device.name << '\n';
      print_formats(ctx, device_index);
      return 0;
    }

    CaptureBackend backend = CaptureBackend::OpenPnp;
#ifdef __linux__
    std::optional<std::string> linux_device_path;
    if (device_selector.rfind("/dev/video", 0) == 0) {
      linux_device_path = device_selector;
    } else {
      linux_device_path = linux_device_path_from_index(device_index);
    }

    if (!is_openpnp_supported_fourcc(format.fourcc)) {
      if (format.fourcc == V4L2_PIX_FMT_GREY && linux_device_path.has_value()) {
        backend = CaptureBackend::V4L2Grey;
      } else {
        throw std::runtime_error("Format " + fourcc_to_string(format.fourcc) +
                                 " is not supported by openpnp-capture on Linux.");
      }
    }
#else
    if (!is_openpnp_supported_fourcc(format.fourcc)) {
      throw std::runtime_error("Selected format is not supported by openpnp-capture.");
    }
#endif

    std::cout << "Capturing from device [" << device.index << "] " << device.name
              << " using format [" << selected_format << "] " << format.width << 'x'
              << format.height << " fourcc=" << fourcc_to_string(format.fourcc)
              << " backend=" << backend_name(backend) << '\n';
    std::cout.flush();

    ImageRGB image;
    if (backend == CaptureBackend::OpenPnp) {
      image = capture_frame(ctx, device_index, selected_format, warmup_frames, attempts,
                            std::chrono::milliseconds(poll_interval_ms));
    }
#ifdef __linux__
    else if (backend == CaptureBackend::V4L2Grey) {
      image = capture_frame_v4l2_grey(*linux_device_path, format, warmup_frames, attempts,
                                      std::chrono::milliseconds(poll_interval_ms));
    }
#endif

    if (image.empty()) {
      std::cerr << "Capture returned an empty image.\n";
      return 1;
    }

    const fs::path absolute_output_path = fs::absolute(fs::path(output_path));
    const fs::path parent_dir = absolute_output_path.parent_path();
    if (!parent_dir.empty()) {
      fs::create_directories(parent_dir);
    }

    if (!saveImage(absolute_output_path.string(), image)) {
      std::cerr << "Failed to save image to " << absolute_output_path << '\n';
      return 1;
    }

    std::cout << "Captured " << image.width << 'x' << image.height << " from device ["
              << device.index << "] " << device.name << '\n';
    std::cout << "Saved image to " << absolute_output_path << '\n';
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}

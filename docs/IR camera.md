# IR Camera Guide

Biopass supports an infrared (IR) camera for face liveness detection. When configured, the IR pipeline waits for the IR emitter/exposure to become visible, verifies that the stream contains a real pulsed illumination pattern, confirms that a face is present in IR, and can classify the selected crops with the MobileNetV3 anti-spoofing model. The model was originally trained on RGB imagery and is evaluated in grayscale-as-RGB mode by replicating the single IR channel.

This texture-based check is designed to help reject printed photos or screen replays. It is not an IR-domain fine-tuned model and does not provide hardware-backed depth or TPM-style guarantees.

## Requirements

- A Linux system where the IR sensor is exposed as a `/dev/video*` device.
- An IR camera/emitter stream that exposes alternating illuminated and dark frames.
- A working face setup in Biopass.
- Permission to access the camera device.

Biopass only reads from the configured IR video device. It does not manage the hardware IR emitter for your laptop or webcam.

When direct V4L2 GREY capture cannot open a device reliably, builds compiled with OpenCV support can fall back to OpenCV's V4L2 backend for the configured IR camera. This fallback is controlled at build time with `BIOPASS_OPENCV_GREY_CAPTURE` (`AUTO`, `ON`, or `OFF`).

## How It Works

The IR anti-spoofing pipeline runs as a layered liveness check:

1. **LED / exposure warm-up** — the IR camera may initially return a dark frame, so Biopass waits until brightness and contrast statistics (`mean`, `max`, and standard deviation) show that the frame is illuminated enough to use.
2. **Illumination pulse validation** — the IR stream must contain a bright-dark-bright pattern. The dark frame must be almost black but not pure black, and it must contain local low-light structure rather than uniform random noise. This helps reject constant captured IR loops and simple black-frame/captured-frame replay streams.
3. **Face crop selection** — a YOLO model (`yolov8n-face.onnx`) locates a face only in illuminated IR frames. The current threshold for IR face presence is intentionally lower than identity recognition because this step only proves that the RGB face area has a usable IR face crop.
4. **Minimum face scale check** — the detected IR face must occupy enough of the frame for reliable texture-based liveness classification. Very small / distant faces are skipped instead of being treated as spoof evidence. The default threshold requires the IR face bounding box to cover at least 8% of the frame and can be tuned with `anti_spoofing.ir_min_face_area_ratio`.
5. **Liveness classification** — a MobileNetV3 model (`mobilenetv3_antispoof.onnx`) classifies each selected crop as **real** or **spoof**. Since the model expects RGB, the single grayscale IR channel is cloned into all 3 color channels.

For the sudo/PAM path, Biopass currently collects 2 usable IR face crops. The IR model result passes only when every selected crop is accepted as real. A crop is accepted as real only when the real score meets the configured threshold and is greater than the spoof score.

The RGB AI and IR anti-spoofing tasks run in parallel. Their results are combined according to `anti_spoofing.ir_antispoof_mode`:

- `balanced` (default): when IR is configured, IR face presence is still required, then either the RGB AI model or the IR anti-spoofing model may satisfy liveness. This tolerates a transient model miss without allowing an absent/covered IR camera to pass.
- `strict`: IR face presence and every enabled liveness model must pass.

When only one anti-spoofing method is enabled, both modes require that method to pass. Existing `ir_model_hard_fail: true` configurations are migrated to `ir_antispoof_mode: strict`; false or missing values migrate to `balanced`.

When debug mode is enabled, Biopass saves additional IR diagnostics, including raw IR attempts, the selected IR face crop, and the resized `128x128` image that is passed to the anti-spoofing model. These images can help distinguish between a real model mismatch, a missing illumination pulse, and insufficient input detail caused by distance, blur, or poor crop scale.

## 1. Find the IR Camera Device

List video devices:

```bash
ls -l /dev/video*
```

If `v4l2-ctl` is available, it is usually easier to identify the correct device with:

```bash
v4l2-ctl --list-devices
```

Look for the device node that belongs to your IR sensor, for example `/dev/video2`.

## 2. Enable It In Biopass

Open the Biopass desktop app and go to the face settings.

In the anti-spoofing section:

1. Enable face anti-spoofing if you want to use the RGB AI anti-spoofing model too.
2. Set `IR Camera` to the correct `/dev/video*` device.
3. Save your configuration.

If you only want IR-based anti-spoofing, selecting the `IR Camera` device is enough.

## 3. If The IR Emitter Stays Off On Linux

On some Linux systems, the IR camera is detected but the IR light emitter does not turn on automatically. In that case, use [`linux-enable-ir-emitter`](https://github.com/EmixamPP/linux-enable-ir-emitter).

To install it:

```bash
VERSION=6.1.2
DIST=linux-enable-ir-emitter-$VERSION-release.systemd.x86-64.tar.gz
wget https://github.com/EmixamPP/linux-enable-ir-emitter/releases/download/$VERSION/$DIST
sudo tar -C / --no-same-owner -m -h -vxzf $DIST
```

Then, configure your IR emitter:

```bash
sudo linux-enable-ir-emitter configure
```

Follow instructions printed when it is configuring your camera. After successfully triggering your IR emitter, please run this command:

```bash
sudo systemctl enable --now linux-enable-ir-emitter
```

Thanks @notherealmarco for help me on this https://github.com/TickLabVN/biopass/discussions/60#discussioncomment-16521628.

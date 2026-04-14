# IR Camera Guide

Biopass uses infrared (IR) camera for face anti-spoofing, rather than relying only on the RGB anti-spoofing AI model. This is usually configured as a Linux video device path such as `/dev/video2`. If your devices supports IR camera, you can turn on this option by using the configuration UI.

## Requirements

- A Linux system where the IR sensor is exposed as a `/dev/video*` device.
- A working face setup in Biopass.
- Permission to access the camera device.

Biopass only reads from the configured IR video device. It does not manage the hardware IR emitter for your laptop or webcam.

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

1. Enable face anti-spoofing if you want to use the AI anti-spoofing model too.
2. Set `IR Camera` to the correct `/dev/video*` device.
3. Save your configuration.

If you only want IR-based anti-spoofing, selecting the `IR Camera` device is enough.

## 3. If The IR Emitter Stays Off On Linux

On some Linux systems, the IR camera is detected but the IR light emitter does not turn on automatically. In that case, use `linux-enable-ir-emitter`:

- Repository: https://github.com/EmixamPP/linux-enable-ir-emitter

That project is designed to enable the emitter for infrared cameras that are recognized by Linux but not activated correctly out of the box.

Please follow that project's README for setup and configuration. In particular, run its configuration flow first so you can confirm that the IR emitter actually activates on your hardware.

Thanks @notherealmarco for help me on this https://github.com/TickLabVN/biopass/discussions/60#discussioncomment-16521628.

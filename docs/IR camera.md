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

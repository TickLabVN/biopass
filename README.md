# Biopass

<p align="center">
    <img src="https://public-r2.ticklab.site/media/tc1oN21KXhMM1B2jOecRhk=" alt="Biopass Logo" width="120" />
</p>

<p align="center">
    <a href="https://github.com/TickLabVN/biopass/releases/latest">
        <img src="https://img.shields.io/github/v/release/TickLabVN/biopass?label=Last%20Release&style=flat-square" alt="Latest release" />
    </a>
    <a href="https://github.com/TickLabVN/biopass/stargazers">
        <img src="https://img.shields.io/github/stars/TickLabVN/biopass?style=flat-square" alt="GitHub stars" />
    </a>
    <a href="https://github.com/TickLabVN/biopass/issues">
        <img src="https://img.shields.io/github/issues/TickLabVN/biopass?style=flat-square" alt="Open Issues" />
    </a>
</p>

<h2 align="center">Biopass</h2>
<p align="center"><b>Modern FaceID login for Linux</b></p>
<p align="center">A fast, secure, and privacy-focused facial recognition module for Linux desktops.</p>

---

> **Why Biopass?**  
> While there is an existing FaceID login module for Linux called [Howdy](https://github.com/boltgolt/howdy), it is no longer actively maintained (last release: Sep 2, 2020). Many of its dependencies are deprecated, and it lacks spoof detection capabilities.  
>  
> To address these issues, we—[@npvinh](https://github.com/npvinh) and [@thaitran24](https://github.com/thaitran24)—created Biopass: a modern, secure, and actively maintained alternative.

## Installation

### Using Debian Package

```sh
VERSION=$(curl -s https://api.github.com/repos/TickLabVN/biopass/releases/latest | jq -r .tag_name)
UBUNTU_VERSION=$(lsb_release -rs)
wget https://github.com/TickLabVN/biopass/releases/download/$VERSION/biopass-$VERSION-ubuntu-$UBUNTU_VERSION.deb -O biopass.deb

# Install the package
sudo dpkg -i biopass.deb

# Fix any dependency issues
sudo apt install --fix-broken
```

### Adding Your Face

Once Biopass is installed, you can register your face by running the following command:

```sh
biopass register
```

A window will open, prompting you to position your face in front of the camera. Ensure your face is well-lit and clearly visible. Press `Enter` or `Esc` to capture your face and close the window. The captured face data will be stored in `~/.config/biopass`.

To remove your face data from the system, use the command:

```sh
biopass unregister
```

### Enabling Face Login

By default, the FaceID login module is disabled. To enable it, execute:

```sh
sudo biopass enable
```

After enabling, suspend your current session and log in again to test the FaceID functionality. If you wish to disable the FaceID login module, you can do so by running:

```sh
sudo biopass disable
```

### Configuration

In some cases, the RGB camera may not be ready, or poor lighting conditions can cause the FaceID login to fail. To mitigate this, you can adjust the PAM configuration. By default, Biopass will retry up to **10 times** before giving up, with a small delay of **200 milliseconds** between attempts. Additionally, Biopass includes an optional **face anti-spoofing** feature. This feature is disabled by default because weaker cameras may lack the resolution required for accurate detection. You can use this command to get the current settings:

```sh
biopass config get
```

To change the configuration, use the following command:

```sh
biopass config set --retries=<number_of_retries> --delay=<delay_in_milliseconds> --anti-spoofing=<true_or_false>
```

For example, to set the retries to 5, the delay to 300 milliseconds, and enable anti-spoofing, you would run:

```sh
biopass config set --retries=5 --delay=300 --anti-spoofing=true
```

**Note**: Ensure the FaceID login module is enabled before using the `biopass config` command.

## How to Contribute

We welcome contributions from the community! Here’s how you can help:

1. **Report Issues**: Found a bug or have a feature request? Open an issue on our [GitHub repository](https://github.com/TickLabVN/biopass/issues).
2. **Submit Pull Requests**: Fix bugs, improve documentation, or add new features. Check out our [contribution guidelines](https://github.com/TickLabVN/biopass/blob/main/docs/contributing.md) for more details.
3. **Test and Provide Feedback**: Help us improve by testing Biopass on different Linux distributions and hardware setups.
4. **Spread the Word**: Share Biopass with your friends and colleagues. The more users we have, the better we can make it!

## Future Work

It's important to note that Biopass is still in its early stages. There are several features it currently lacks:

- [ ] **Extended Login Support**: Apply face recognition to other login methods (e.g., `sudo`, `su`). See [issue #5](https://github.com/TickLabVN/biopass/issues/5).
- [x] **Face Anti-Spoofing**: Enhance security with anti-spoofing measures. This feature will be optional for users with weaker cameras.
- [ ] **IR Camera Support**: Expand compatibility to include infrared cameras. Currently, only RGB cameras are supported.
- [ ] **Keyring Unlock**: User must enter password on the first login time to unlock applications, see https://askubuntu.com/a/238055 for more details. We will find the way to eliminate it.

Feel free to contribute to any of these features or suggest new ones!

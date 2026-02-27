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
<p align="center"><b>Modern multi-modal biometric login for Linux</b></p>
<p align="center">A fast, secure, and privacy-focused biometric recognition module for Linux desktops supporting face, fingerprint, and voice.</p>

---

## Why Biopass?

While Windows Hello provides a seamless multi-modal biometric experience (Face, Fingerprint, PIN) on Windows 11, Linux has historically lacked a modern, unified equivalent. The most well-known project in this space, [Howdy](https://github.com/boltgolt/howdy), focuses exclusively on facial recognition and has not seen significant updates in recent years.

Biopass was developed by [@phucvinh57](https://github.com/phucvinh57) and [@thaitran24](https://github.com/thaitran24) to fill this gap, providing a fast, secure, and modern biometric suite that goes beyond just face ID.

## Comparison

| Feature | [**Biopass**](https://github.com/TickLabVN/biopass) | [**Howdy**](https://github.com/boltgolt/howdy) |
| :--- | :--- | :--- |
| **Modalities** | Face + Fingerprint | Face ID only |
| **User Interface** | Modern GUI for management | Command-line interface only |
| **Configuration** | GUI | Manual |
| **Face Anti-spoofing** | Built-in liveness detection | Limited (Requires IR camera for security) |

## Installation

Please visit the [release page](https://github.com/TickLabVN/biopass/releases) to download newest version.

## Features

- [x] Authentication: User can register multiple biometrics for authentication. Authentication methods can be executed in parallel or sequentially.
    - [x] Face: recognition + anti-spoofing
    - [x] Fingerprint
    - [ ] Voice: recognition + anti-spoofing
- [ ] Local AI model management: User can download, update, and delete AI models for face and voice authentication methods.

Feel free to request new features or report bugs by opening an issue. For contributing, please read [CONTRIBUTING.md](CONTRIBUTING.md).


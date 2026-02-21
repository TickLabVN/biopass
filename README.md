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

I love the Windows Hello feature of Windows 11. It is convenient with supports of face, fingerprint and PIN for login. But there is no similar feature for Linux. There is an existing FaceID login module for Linux, [Howdy](https://github.com/boltgolt/howdy), but it only supports face recognition and has not been updated for a long time.

Therefore, we, [@phucvinh57](https://github.com/phucvinh57) and [@thaitran24](https://github.com/thaitran24), developed Biopass that supports multiple biometric methods including face, fingerprint, and voice for authentication.

## Installation

Please visit the [release page](https://github.com/TickLabVN/biopass/releases) to download newest version.

## Features

- [x] Authentication: User can register multiple biometrics for authentication. Authentication methods can be executed in parallel or sequentially.
    - [x] Face: recognition + anti-spoofing
    - [x] Fingerprint
    - [ ] Voice: recognition + anti-spoofing
- [ ] Local AI model management: User can download, update, and delete AI models for face and voice authentication methods.

Feel free to request new features or report bugs by opening an issue. For contributing, please read [CONTRIBUTING.md](CONTRIBUTING.md).


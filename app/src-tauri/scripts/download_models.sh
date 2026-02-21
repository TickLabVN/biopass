#!/bin/bash
# download_models.sh — Downloads Biopass AI models to the user's data directory.
# This script is invoked by package installer hooks (RPM preinstall / Debian postinst).

set -euo pipefail

MODELS=(
    "https://biopass.ticklab.site/models/yolov11n-face.torchscript|yolov11n-face.torchscript"
    "https://biopass.ticklab.site/models/edgeface_s_gamma_05_ts.pt|edgeface_s_gamma_05_ts.pt"
    "https://biopass.ticklab.site/models/mobilenetv3_antispoof_ts.pt|mobilenetv3_antispoof_ts.pt"
)

# Determine the data dir. If running as root (e.g. system-wide install),
# use SUDO_USER's home if available, otherwise fallback to /root.
if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
    USER_HOME=$(getent passwd "$SUDO_USER" | cut -d: -f6)
    DATA_DIR="${USER_HOME}/.local/share/com.ticklab.biopass/models"
else
    DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/com.ticklab.biopass/models"
fi

echo "Biopass: Ensuring model directory exists at $DATA_DIR"
mkdir -p "$DATA_DIR"

for entry in "${MODELS[@]}"; do
    url="${entry%%|*}"
    filename="${entry##*|}"
    dest="$DATA_DIR/$filename"

    if [ -f "$dest" ]; then
        echo "Biopass: Model already present, skipping: $filename"
    else
        echo "Biopass: Downloading $filename ..."
        if curl -fL --retry 3 --retry-delay 2 -C - -o "$dest" "$url"; then
            echo "Biopass: Downloaded $filename"
        else
            echo "Biopass: WARNING — Failed to download $filename. The app may not function correctly until models are available." >&2
            rm -f "$dest"  # Remove partial file
        fi
    fi
done

echo "Biopass: Model download complete."

# Fix ownership back to the actual user if running under sudo
if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
    chown -R "$SUDO_USER:$SUDO_USER" "${USER_HOME}/.local/share/com.ticklab.biopass"
fi

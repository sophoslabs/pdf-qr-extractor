#!/bin/bash
# Installs system library prerequisites required by PDF QR Extractor.
# Run this once on a fresh or minimal system before install_pdf_qr_extractor.sh.
set -e

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

if ! command -v apt-get &>/dev/null; then
    echo "ERROR: apt-get not found. This script supports Debian/Ubuntu systems only."
    exit 1
fi

echo "Installing PDF QR Extractor prerequisites..."

echo "Refreshing package lists..."
apt-get update -qq

echo "Installing required system libraries..."
apt-get install -y \
    libopenjp2-7 \
    libnss3 \
    libnspr4 \
    libfontconfig1 \
    libjpeg-turbo8 \
    liblcms2-2 \
    libjbig0

echo "Prerequisites installed successfully."

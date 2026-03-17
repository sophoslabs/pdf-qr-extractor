#!/bin/bash
set -e

SERVICE_USER="pdfqr"
BASE_DIR="/opt/pdf-qr-extractor"
SERVICE_FILE="pdf_qr_extractor.service"

echo "Installing PDF QR Extractor..."

# 1. Root check
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

# 2. Verify required files exist
if [ ! -f pdf_qr_extractor ]; then
    echo "Binary pdf_qr_extractor not found"
    exit 1
fi

if [ ! -f pdf_qr_extractor.conf ]; then
    echo "Configuration file not found"
    exit 1
fi

if [ ! -f "$SERVICE_FILE" ]; then
    echo "Service file not found"
    exit 1
fi

# 3. Create service user (system account)
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /usr/sbin/nologin "$SERVICE_USER"
fi

# 4. Create directories
mkdir -p "$BASE_DIR"/{bin,config,run,logs,docs}

# 5. Copy files
cp pdf_qr_extractor "$BASE_DIR/bin/"
cp pdf_qr_extractor.conf "$BASE_DIR/config/"
cp -r docs/* "$BASE_DIR/docs/" 2>/dev/null || true

# 6. Secure ownership & permissions

# Base directory owned by service user
chown -R "$SERVICE_USER:$SERVICE_USER" "$BASE_DIR"

# Restrict directory access
chmod 750 "$BASE_DIR"
chmod 750 "$BASE_DIR/run"
chmod 750 "$BASE_DIR/logs"
chmod 750 "$BASE_DIR/config"
chmod 750 "$BASE_DIR/docs"

# Harden binary permissions
chown root:root "$BASE_DIR/bin/pdf_qr_extractor"
chmod 550 "$BASE_DIR/bin/pdf_qr_extractor"

# 7. Install systemd service
cp "$SERVICE_FILE" /etc/systemd/system/

# Service file must be root-owned
chown root:root /etc/systemd/system/"$SERVICE_FILE"
chmod 644 /etc/systemd/system/"$SERVICE_FILE"

# 8. Reload systemd
systemctl daemon-reload

# 9. Enable & start service
systemctl enable pdf_qr_extractor
systemctl restart pdf_qr_extractor

echo "Installation complete."


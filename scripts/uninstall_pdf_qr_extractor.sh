#!/usr/bin/env bash
set -e

# Require root
if [[ $EUID -ne 0 ]]; then
    echo "Error: This script must be run as root (use sudo)." >&2
    exit 1
fi

SERVICE_USER="pdfqr"
BASE_DIR="/opt/pdf-qr-extractor"
BACKUP_DIR="/tmp/pdf_qr_extractor_backup_$(date +%Y%m%d_%H%M%S)"

echo "Backing up config to $BACKUP_DIR ..."
mkdir -p "$BACKUP_DIR"
cp -r "$BASE_DIR/config" "$BACKUP_DIR/" 2>/dev/null || true
echo "Config backed up to $BACKUP_DIR"

echo "Stopping service..."
systemctl stop pdf_qr_extractor || true
systemctl disable pdf_qr_extractor || true

echo "Removing service file..."
rm -f /etc/systemd/system/pdf_qr_extractor.service
systemctl daemon-reload

echo "Removing directories..."
rm -rf "$BASE_DIR"

echo "Removing user..."
userdel -r "$SERVICE_USER" 2>/dev/null || true

echo "Uninstall complete."


#!/usr/bin/env bash
set -e

SERVICE_USER="pdfqr"
BASE_DIR="/opt/pdf-qr-extractor"

echo "Stopping service..."
sudo systemctl stop pdf_qr_extractor || true
sudo systemctl disable pdf_qr_extractor || true

echo "Removing service file..."
sudo rm -f /etc/systemd/system/pdf_qr_extractor.service
sudo systemctl daemon-reload

echo "Removing directories..."
sudo rm -rf "$BASE_DIR"

echo "Removing user..."
sudo userdel "$SERVICE_USER" || true

echo "Uninstall complete."


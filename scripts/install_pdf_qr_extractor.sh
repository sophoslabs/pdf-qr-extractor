#!/bin/bash
set -e

SERVICE_USER="pdfqr"
BASE_DIR="/opt/pdf-qr-extractor"
SERVICE_FILE="pdf_qr_extractor.service"

# Resolve package root relative to this script's location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PACKAGE_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "Installing PDF QR Extractor..."

# 1. Root check
if [ "$EUID" -ne 0 ]; then
    echo "Please run as root"
    exit 1
fi

# 1a. Check system library prerequisites
MISSING_LIBS=()
for lib in libopenjp2.so.7 libnss3.so libnspr4.so \
           libfontconfig.so.1 libjpeg.so.8 liblcms2.so.2 libjbig.so.0; do
    ldconfig -p | grep -q "^\s*${lib}" || MISSING_LIBS+=("$lib")
done
if [ ${#MISSING_LIBS[@]} -gt 0 ]; then
    echo "ERROR: The following required system libraries are missing:"
    for lib in "${MISSING_LIBS[@]}"; do echo "  $lib"; done
    echo ""
    echo "Run the prerequisites script first:"
    echo "  sudo $SCRIPT_DIR/install_lib_dependencies.sh"
    exit 1
fi

# 2. Verify required files exist
if [ ! -f "$PACKAGE_DIR/bin/pdf_qr_extractor" ]; then
    echo "Binary not found at $PACKAGE_DIR/bin/pdf_qr_extractor"
    exit 1
fi

if [ ! -f "$PACKAGE_DIR/config/pdf_qr_extractor.conf" ]; then
    echo "Configuration file not found at $PACKAGE_DIR/config/pdf_qr_extractor.conf"
    exit 1
fi

if [ ! -f "$SCRIPT_DIR/$SERVICE_FILE" ]; then
    echo "Service file not found at $SCRIPT_DIR/$SERVICE_FILE"
    exit 1
fi

if [ ! -d "$PACKAGE_DIR/lib" ]; then
    echo "Library directory not found at $PACKAGE_DIR/lib"
    exit 1
fi

# 3. Create service user (system account)
if ! id "$SERVICE_USER" &>/dev/null; then
    NOLOGIN=$(command -v nologin 2>/dev/null || echo /usr/sbin/nologin)
    useradd -r -s "$NOLOGIN" "$SERVICE_USER"
    echo "Service user '$SERVICE_USER' created."
else
    echo "Service user '$SERVICE_USER' already exists."
fi

# 4. Create directories
mkdir -p "$BASE_DIR"/{bin,lib,config,run,logs,docs}

# 5. Copy files
cp "$PACKAGE_DIR/bin/pdf_qr_extractor" "$BASE_DIR/bin/"

# Preserve existing config on upgrade; only install if absent
if [ ! -f "$BASE_DIR/config/pdf_qr_extractor.conf" ]; then
    cp "$PACKAGE_DIR/config/pdf_qr_extractor.conf" "$BASE_DIR/config/"
else
    echo "Existing config preserved. New default saved as pdf_qr_extractor.conf.new"
    cp "$PACKAGE_DIR/config/pdf_qr_extractor.conf" "$BASE_DIR/config/pdf_qr_extractor.conf.new"
fi

cp -a "$PACKAGE_DIR/lib/"* "$BASE_DIR/lib/"
cp -r "$PACKAGE_DIR/docs/"* "$BASE_DIR/docs/" 2>/dev/null || true
echo "Files installed to $BASE_DIR."

# 6. Secure ownership & permissions

# Base directory owned by service user
chown -R "$SERVICE_USER:$SERVICE_USER" "$BASE_DIR"

# Restrict directory access
chmod 750 "$BASE_DIR"
chmod 750 "$BASE_DIR/run"
chmod 750 "$BASE_DIR/logs"
chmod 750 "$BASE_DIR/config"
chmod 750 "$BASE_DIR/docs"
chmod 750 "$BASE_DIR/lib"

# Harden binary permissions: root owns it (not modifiable by service user),
# group set to service user so it can execute, others have no access.
chown root:"$SERVICE_USER" "$BASE_DIR/bin/pdf_qr_extractor"
chmod 550 "$BASE_DIR/bin/pdf_qr_extractor"

# 6a. Create and secure the socket directory derived from config
SOCKET_PATH=$(grep -E '^\s*EXTRACTOR_SOCKET_PATH\s*=' "$BASE_DIR/config/pdf_qr_extractor.conf" \
    | sed 's/[^=]*=//' | tr -d '[:space:]' | head -1)
if [ -n "$SOCKET_PATH" ]; then
    SOCKET_DIR=$(dirname "$SOCKET_PATH")
    mkdir -p "$SOCKET_DIR"
    chown "$SERVICE_USER:$SERVICE_USER" "$SOCKET_DIR"
    chmod 750 "$SOCKET_DIR"
    echo "Socket directory '$SOCKET_DIR' created and secured."
else
    echo "WARNING: EXTRACTOR_SOCKET_PATH not found in config; socket directory ($BASE_DIR/run) will be used."
fi

# 7. Install systemd service (skipped in non-systemd environments such as containers)
# Check that systemd files exist, systemctl is available, AND systemd is actually PID 1
SYSTEMD_RUNNING=false
if [ -d /etc/systemd/system ] && command -v systemctl &>/dev/null; then
    if [ "$(cat /proc/1/comm 2>/dev/null)" = "systemd" ]; then
        SYSTEMD_RUNNING=true
    fi
fi

if [ "$SYSTEMD_RUNNING" = "true" ]; then
    cp "$SCRIPT_DIR/$SERVICE_FILE" /etc/systemd/system/

    # Service file must be root-owned
    chown root:root /etc/systemd/system/"$SERVICE_FILE"
    chmod 644 /etc/systemd/system/"$SERVICE_FILE"

    # 8. Reload systemd
    systemctl daemon-reload

    # 9. Enable & start service
    systemctl enable pdf_qr_extractor
    systemctl restart pdf_qr_extractor

    echo "Installation complete. Service started via systemd."
else
    echo ""
    echo "Installation complete (systemd not detected — service not registered)."
    echo "To start the service manually:"
    echo "  sudo -u $SERVICE_USER $BASE_DIR/bin/pdf_qr_extractor --config $BASE_DIR/config/pdf_qr_extractor.conf"
fi

# Final config sanity reminder
if ! grep -qE '^\s*ALLOWED_UID\s*=\s*[0-9]+' "$BASE_DIR/config/pdf_qr_extractor.conf"; then
    echo ""
    echo "WARNING: ALLOWED_UID is not set in $BASE_DIR/config/pdf_qr_extractor.conf"
    echo "  The service will fail to start until this is configured."
    echo "  Set it to the UID of the application user that will connect to the extractor:"
    echo "    id -u <your-app-username>"
    echo "  Then update the config and restart the service."
fi


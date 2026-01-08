#!/bin/bash

# =============================================================================
# Embedded-ControlHub: ESP32 Logger Service Configuration
# -----------------------------------------------------------------------------
# This script logs sensor data from an ESP32 running a web server
# (HTTP JSON API) into a local file and can be installed as a systemd service.
# =============================================================================

# =============================================================================
# PRIVATE CONFIGURATION
# =============================================================================

# IP / hostname of your ESP32
ESP32_HOST="192.168.0.103"   # change to your ESP32 IP

# Port of the ESP32 web server
ESP32_PORT="80"              # usually 80

# Base directory for project files (logs, worker script)
BASE_DIR="$HOME/Embedded-ControlHub"

# Log file path
LOG_FILE="$BASE_DIR/sensors.log"

# =============================================================================
# Configuration
# =============================================================================

SERVICE_NAME="esp32-logger"
USER_NAME="$USER"

# Construct full URL to query ESP32
ESP32_URL="http://${ESP32_HOST}:${ESP32_PORT}/status"

# Time interval between HTTP requests (in seconds)
INTERVAL=5

# Worker script path
SCRIPT_PATH="$BASE_DIR/esp32_logger_worker.sh"

# systemd service file path
SERVICE_FILE="/etc/systemd/system/$SERVICE_NAME.service"

# =============================================================================
# Dependency Checks
# =============================================================================

command -v curl >/dev/null || { echo "❌ curl is not installed"; exit 1; }
command -v jq   >/dev/null || { echo "❌ jq is not installed"; exit 1; }

# =============================================================================
# Worker Script Creation
# =============================================================================

echo "📝 Creating worker script..."

cat <<EOF > "$SCRIPT_PATH"
#!/bin/bash

ESP32_URL="$ESP32_URL"
LOG_FILE="$LOG_FILE"
INTERVAL=$INTERVAL

while true; do
    TS=\$(date "+%Y-%m-%d %H:%M:%S")
    RESP=\$(curl -s --connect-timeout 3 "\$ESP32_URL")

    if [ \$? -eq 0 ] && [ -n "\$RESP" ]; then
        TEMP=\$(echo "\$RESP" | jq -r '.temp // empty')
        HUM=\$(echo "\$RESP" | jq -r '.hum // empty')
        PHOTO=\$(echo "\$RESP" | jq -r '.photo // empty')
        CLAP=\$(echo "\$RESP" | jq -r '.clapAgo // empty')

        if [ -n "\$TEMP" ]; then
            echo "\$TS T=\$TEMP H=\$HUM P=\$PHOTO C=\$CLAP" >> "\$LOG_FILE"
        else
            echo "\$TS ERROR invalid JSON: \$RESP" >> "\$LOG_FILE"
        fi
    else
        echo "\$TS ERROR ESP32 unreachable" >> "\$LOG_FILE"
    fi

    sleep \$INTERVAL
done
EOF

chmod +x "$SCRIPT_PATH"

# =============================================================================
# systemd Service Creation
# =============================================================================

echo "⚙️ Creating systemd service..."

sudo tee "$SERVICE_FILE" > /dev/null <<EOF
[Unit]
Description=ESP32 HTTP Sensors Logger
After=network-online.target
Wants=network-online.target

[Service]
Type=simple
User=$USER_NAME
ExecStart=$SCRIPT_PATH
Restart=always
RestartSec=5

StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

# =============================================================================
# Service Activation
# =============================================================================

echo "🔄 Reloading systemd..."
sudo systemctl daemon-reload

echo "✅ Enabling autostart"
sudo systemctl enable $SERVICE_NAME

echo "▶️ Starting service"
sudo systemctl start $SERVICE_NAME

echo
echo "📡 Service status:"
systemctl status $SERVICE_NAME --no-pager

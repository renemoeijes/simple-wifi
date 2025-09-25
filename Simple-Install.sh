#!/bin/bash

set -e
set -o pipefail

NQPTP_VERSION="1.2.4"
SHAIRPORT_SYNC_VERSION="4.3.2"
TMP_DIR=""

cleanup() {
    if [ -d "${TMP_DIR}" ]; then
        rm -rf "${TMP_DIR}"
    fi
}

verify_os() {
    MSG="Unsupported OS: Raspberry Pi OS 12 (bookworm) is required."

    if [ ! -f /etc/os-release ]; then
        printf "${MSG}\n"
        exit 1
    fi

    . /etc/os-release

    if [[ ("$ID" != "debian" && "$ID" != "raspbian") || "$VERSION_ID" != "12" ]]; then
        printf "${MSG}\n"
        exit 1
    fi
}

set_hostname() {
    CURRENT_PRETTY_HOSTNAME=$(hostnamectl status --pretty)

    read -p "Hostname [$(hostname)]: " HOSTNAME
    sudo raspi-config nonint do_hostname ${HOSTNAME:-$(hostname)}

    read -p "Pretty hostname [${CURRENT_PRETTY_HOSTNAME:-Raspberry Pi}]: " PRETTY_HOSTNAME
    PRETTY_HOSTNAME="${PRETTY_HOSTNAME:-${CURRENT_PRETTY_HOSTNAME:-Raspberry Pi}}"
    sudo hostnamectl set-hostname --pretty "$PRETTY_HOSTNAME"
}


simple_install(){
   APP=$1
   # Controleer of jq geïnstalleerd is, zo niet: installeer het
   if ! command -v jq >/dev/null 2>&1; then
      printf "jq is not installed. Installing jq...\n"
      sudo apt-get update
      sudo apt-get install -y jq
   fi

   # Download en installeer het laatste simple DEB-pakket van GitHub
   TMP_DEB="/tmp/${APP}-latest.deb"
   printf "Downloading latest ${APP} release...\n"
   wget -O "$TMP_DEB" $(curl -s https://api.github.com/repos/renemoeijes/${APP}/releases/latest | jq -r '.assets[] | select(.name | endswith(".deb")) | .browser_download_url')
   printf "Installing ${APP}...\n"
   # sudo dpkg -i "$TMP_DEB"
   # sudo dpkg -i --force-reinstall  "$TMP_DEB"
   # sudo dpkg -i --force-all "$TMP_DEB"
   sudo apt-get install -y --reinstall "$TMP_DEB"
   rm -f "$TMP_DEB"

   printf "${APP} is installed.\n"
}

install_shairport() {
    read -p "Do you want to install Shairport Sync (AirPlay 2 audio player)? [y/N] " REPLY
    if [[ ! "$REPLY" =~ ^(yes|y|Y)$ ]]; then return; fi

    sudo apt update
    sudo apt install -y --no-install-recommends wget unzip autoconf automake build-essential libtool git autoconf automake libpopt-dev libconfig-dev libasound2-dev avahi-daemon libavahi-client-dev libssl-dev libsoxr-dev libplist-dev libsodium-dev libavutil-dev libavcodec-dev libavformat-dev uuid-dev libgcrypt20-dev xxd

    if [[ -z "$TMP_DIR" ]]; then
        TMP_DIR=$(mktemp -d)
    fi

    cd $TMP_DIR

    # Install ALAC
    wget -O alac-master.zip https://github.com/mikebrady/alac/archive/refs/heads/master.zip
    unzip alac-master.zip
    cd alac-master
    autoreconf -fi
    ./configure
    make -j $(nproc)
    sudo make install
    sudo ldconfig
    cd ..
    rm -rf alac-master

    # Install NQPTP
    wget -O nqptp-${NQPTP_VERSION}.zip https://github.com/mikebrady/nqptp/archive/refs/tags/${NQPTP_VERSION}.zip
    unzip nqptp-${NQPTP_VERSION}.zip
    cd nqptp-${NQPTP_VERSION}
    autoreconf -fi
    ./configure --with-systemd-startup
    make -j $(nproc)
    sudo make install
    cd ..
    rm -rf nqptp-${NQPTP_VERSION}

    # Install Shairport Sync
    wget -O shairport-sync-${SHAIRPORT_SYNC_VERSION}.zip https://github.com/mikebrady/shairport-sync/archive/refs/tags/${SHAIRPORT_SYNC_VERSION}.zip
    unzip shairport-sync-${SHAIRPORT_SYNC_VERSION}.zip
    cd shairport-sync-${SHAIRPORT_SYNC_VERSION}
    autoreconf -fi
    ./configure --sysconfdir=/etc --with-alsa --with-soxr --with-avahi --with-ssl=openssl --with-systemd --with-airplay-2 --with-apple-alac
    make -j $(nproc)
    sudo make install
    cd ..
    rm -rf shairport-sync-${SHAIRPORT_SYNC_VERSION}

    # Configure Shairport Sync
    sudo tee /etc/shairport-sync.conf >/dev/null <<EOF
general = {
  name = "${PRETTY_HOSTNAME:-$(hostname)}";
  output_backend = "alsa";
}

sessioncontrol = {
  session_timeout = 20;
};
EOF

    sudo usermod -a -G gpio shairport-sync
    sudo systemctl enable --now nqptp
    sudo systemctl enable --now shairport-sync
}

install_raspotify() {
    read -p "Do you want to install Raspotify (Spotify Connect)? [y/N] " REPLY
    if [[ ! "$REPLY" =~ ^(yes|y|Y)$ ]]; then return; fi

    # Install Raspotify
    curl -sL https://dtcooper.github.io/raspotify/install.sh | sh

    # Configure Raspotify
    LIBRESPOT_NAME="${PRETTY_HOSTNAME// /-}"
    LIBRESPOT_NAME=${LIBRESPOT_NAME:-$(hostname)}

    sudo tee /etc/raspotify/conf >/dev/null <<EOF
LIBRESPOT_QUIET=on
LIBRESPOT_LOG_LEVEL=debug
LIBRESPOT_AUTOPLAY=on
LIBRESPOT_DISABLE_AUDIO_CACHE=on
LIBRESPOT_DISABLE_CREDENTIAL_CACHE=on
LIBRESPOT_ENABLE_VOLUME_NORMALISATION=on
LIBRESPOT_NAME="${LIBRESPOT_NAME}"
LIBRESPOT_DEVICE_TYPE="avr"
LIBRESPOT_BACKEND="alsa"
LIBRESPOT_BITRATE="320"
LIBRESPOT_INITIAL_VOLUME="100"
EOF

    sudo systemctl daemon-reload
    sudo systemctl enable --now raspotify
}





trap cleanup EXIT

printf "Raspberry Pi Audio Receiver\n"

verify_os
set_hostname
# install_bluetooth
# install_shairport
install_raspotify

printf "Installing Simple Wifi\n"
simple_install "simple-wifi"

printf "Installing Simple Bluetooth Agent (ALSA)\n"
simple_install "simple-bt"


printf "Installing Simple Spotify Agent\n"
simple_install "simple-spot"
sudo sed -i 's/^[[:space:]]*LIBRESPOT_QUIET/#LIBRESPOT_QUIET/' /etc/raspotify/conf

# Vervang actieve regel, voeg toe als alleen een gecommentarieerde (of niets) bestaat Dit moet naar het package.

sudo sed -i 's/^[[:space:]]*LIBRESPOT_LOG_LEVEL=.*/LIBRESPOT_LOG_LEVEL=debug/' /etc/raspotify/conf
sudo grep -q '^[[:space:]]*LIBRESPOT_LOG_LEVEL=' /etc/raspotify/conf || echo 'LIBRESPOT_LOG_LEVEL=debug' | sudo tee -a /etc/raspotify/conf >/dev/null

sudo systemctl daemon-reload
sudo systemctl restart raspotify
printf "[+] Librespot setting for Simple-Spot updated and restarted\n"

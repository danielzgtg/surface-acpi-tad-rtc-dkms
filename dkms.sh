#!/bin/sh

set -e

DIR="$(readlink -f "$(dirname "$0")")"
MODULE="$(grep 'PACKAGE_NAME' "$DIR/dkms.conf" | cut -d'=' -f2 | grep -Eo '[^"]+')"

uninstall() {
    sed -i '/surface-acpi-tad-rtc/d' /etc/modules
    for mod in $(dkms status | grep "$MODULE" | cut -d',' -f1); do
        dkms remove "$mod" --force --all
    done
}

install() {
    uninstall
    dkms add "$DIR"
    dkms autoinstall --force
    echo surface-acpi-tad-rtc >> /etc/modules
}

case "$1" in
install)
    install
    ;;
uninstall)
    uninstall
    ;;
esac

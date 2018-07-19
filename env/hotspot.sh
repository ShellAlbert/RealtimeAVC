#!/bin/bash

#创建热点 AVLizard(*)，密码 12345678
nmcli connection add type wifi ifname '*' con-name AVLizard autoconnect no ssid "AVLizard(*)"
nmcli connection modify AVLizard 802-11-wireless.mode ap 802-11-wireless.band bg ipv4.method shared
nmcli connection modify AVLizard 802-11-wireless-security.key-mgmt wpa-psk 802-11-wireless-security.psk 12345678
nmcli connection up AVLizard


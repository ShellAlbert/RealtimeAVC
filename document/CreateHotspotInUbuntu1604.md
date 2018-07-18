# Create a connection
nmcli connection add type wifi ifname '*' con-name my-hotspot autoconnect no ssid my-local-hotspot
# Put it in Access Point
nmcli connection modify my-hotspot 802-11-wireless.mode ap 802-11-wireless.band bg ipv4.method shared
# Set a WPA password (you should change it)
nmcli connection modify my-hotspot 802-11-wireless-security.key-mgmt wpa-psk 802-11-wireless-security.psk myhardpassword
# Enable it (run this command each time you want to enable the access point)
nmcli connection up my-hotspot
# Stop it
nmcli connection down my-hotspot


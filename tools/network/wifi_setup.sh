#!/bin/sh
SCRIPTS_PATH=$( dirname -- "$( readlink -f -- "$0"; )"; )

function change_mode {
    echo change mode: $MODE
    if [ $MODE == AP_DOMO ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        sudo dhcpcd -f $SCRIPTS_PATH/dhcpcd.conf
        sudo dnsmasq -l /tmp/dnsmasq.leases -C $SCRIPTS_PATH/dnsmasq.conf
        sudo hostapd -B $SCRIPTS_PATH/domo_hostapd.conf
        sudo killall node
    elif [ $MODE == AP_LESSIVE ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        sudo dhcpcd -f $SCRIPTS_PATH/dhcpcd.conf
        sudo dnsmasq -l /tmp/dnsmasq.leases -C $SCRIPTS_PATH/dnsmasq.conf
        sudo hostapd -B $SCRIPTS_PATH/lessive_hostapd.conf
        sudo killall node
    elif [ $MODE == CLI_DOMO ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        #sudo wpa_supplicant -B -Dnl80211 -iwlan0 -c$SCRIPTS_PATH/wpa_supplicant.conf
        #sleep 2
        #sudo iwconfig wlan0 essid Domozic
        sudo /usr/local/bin/wifi.sh -a
        sudo ifconfig wlan0 193.168.4.110
        sudo killall node
    else echo Unknown mode: $MODE
    fi
}

MODE=$1
change_mode


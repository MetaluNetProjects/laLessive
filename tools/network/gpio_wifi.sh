#!/bin/sh
SCRIPTS_PATH=$( dirname -- "$( readlink -f -- "$0"; )"; )

PIN_CLI_DOMO=24
PIN_AP_CLOCHE=25
PINS="$PIN_CLI_DOMO $PIN_AP_CLOCHE"
echo pins: $PINS

MODE=NONE

for pin in $PINS; do
    echo configuring pin $pin
    gpio mode $pin in
    gpio mode $pin up
done

function change_mode {
    echo change mode: $MODE
    if [ $MODE == AP_DOMO ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        sudo dhcpcd -f $SCRIPTS_PATH/dhcpcd.conf
        sudo dnsmasq -l /tmp/dnsmasq.leases -C $SCRIPTS_PATH/dnsmasq.conf
        sudo hostapd -B $SCRIPTS_PATH/domo_hostapd.conf
        sudo killall node
    elif [ $MODE == AP_CLOCHE ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        sudo dhcpcd -f $SCRIPTS_PATH/dhcpcd.conf
        sudo dnsmasq -l /tmp/dnsmasq.leases -C $SCRIPTS_PATH/dnsmasq.conf
        sudo hostapd -B $SCRIPTS_PATH/cloche_hostapd.conf
        sudo killall node
    elif [ $MODE == CLI_DOMO ]; then
        sudo killall dhcpcd dnsmasq hostapd wpa_supplicant
        sleep 2
        #sudo wpa_supplicant -B -Dnl80211 -iwlan0 -c$SCRIPTS_PATH/wpa_supplicant.conf
        #sleep 2
        #sudo iwconfig wlan0 essid Domozic
        sudo /usr/local/bin/wifi.sh -a
        sudo killall node
    fi
}

while true; do
    VAL_CLI_DOMO=$(gpio read $PIN_CLI_DOMO)
    VAL_AP_CLOCHE=$(gpio read $PIN_AP_CLOCHE)
    #echo "CLI_DOMO:" $VAL_CLI_DOMO "AP_CLOCHE:" $VAL_AP_CLOCHE
    TMP_MODE=AP_DOMO
    if [ $VAL_CLI_DOMO == 0 ]; then TMP_MODE=CLI_DOMO; fi
    if [ $VAL_AP_CLOCHE == 0 ]; then TMP_MODE=AP_CLOCHE; fi
    if [ $TMP_MODE != $MODE ] ; then
        MODE=$TMP_MODE
        change_mode
        fi
    sleep 2
done


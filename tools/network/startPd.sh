#!/bin/sh

#APP_PATH="/mnt/mmcblk0p2/App"
#MAIN_PATCH=$APP_PATH/`cat /mnt/mmcblk0p2/App/MAIN_PATCH.txt`
#echo $MAIN_PATCH
#exit

APP_PATH=$( dirname -- "$( readlink -f -- "$0"; )"; )
source $APP_PATH/pd_config.txt

ln -s $APP_PATH/ClochesPieds/WAVS ~/Cloches

sudo killall pd

sudo ifconfig eth0 192.168.1.72
sudo hostname ClochesPieds

#sudo iwconfig wlan0 essid Domozic key s:bidouchette

#sudo dhcpcd -f $APP_PATH/dhcpcd.conf
#sudo dnsmasq -l /tmp/dnsmasq.leases -C $APP_PATH/dnsmasq.conf
#sudo hostapd -B $APP_PATH/hostapd.conf

sudo route add default gw localhost

amixer -c 1 cset numid=6 28,28

sleep 5

screen -S OSC -d -m /mnt/mmcblk0p2/App/ClochesPieds/1TELECO_OSC/runOSC.sh

screen -S LINK -d -m /mnt/mmcblk0p2/App/ClochesPieds/domoTools/domonet/Carabiner/rPi/Carabiner

screen -S PD -d -m sudo prlimit --rtprio=99 /home/tc/src/pure-data-ant1r-git/bin/pd -nogui $PDDECLARE -open $MAIN_PATCH

screen -S WIFI -d -m /mnt/mmcblk0p2/App/ClochesPieds/0-picore-scripts/gpio_wifi.sh



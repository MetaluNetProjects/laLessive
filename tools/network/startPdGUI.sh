#!/bin/sh
APP_PATH=$( dirname -- "$( readlink -f -- "$0"; )"; )
source $APP_PATH/pd_config.txt

#MAIN_PATCH="/mnt/mmcblk0p2/App/"`cat /mnt/mmcblk0p2/App/MAIN_PATCH.txt`
#exit
echo $PDDECLARE

sudo killall pd
#sleep 5
tce-load -i unzip
#sudo prlimit --rtprio=99 /home/tc/src/pure-data-ant1r-git/bin/pd -nogui -open /home/tc/Pd/test.pd&
sudo prlimit --rtprio=99 /home/tc/src/pure-data-ant1r-git/bin/pd $PDDECLARE -open $MAIN_PATCH



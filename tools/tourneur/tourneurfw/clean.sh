#!/bin/sh
cd /home/arsene/Patchs/Amalgamix/testStepMotor/microstepfw
PATH=$PATH:/home/arsene/pd-externals/Fraise/bin/utils
echo make_cleaned | to_pd 41111

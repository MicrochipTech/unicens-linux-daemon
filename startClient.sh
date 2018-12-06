#!/bin/sh
killall unicensc > /dev/null 2>&1
./unlink-all-channels.sh 2>&1
./unicensc cfg/config_audio_kit.xml -drv1 0x2B0 &

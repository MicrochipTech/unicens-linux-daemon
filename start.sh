#!/bin/sh
killall unicensd > /dev/null 2>&1
../driver/unlink-all-channels.sh 2>&1
./unicensd cfg/config_audio_kit.xml -drv1 0x200 &

#!/bin/bash

sysfs=/sys/class/most/mostcore
ls -1 $sysfs/aims |while read name; do
    cat $sysfs/aims/$name/links |while read conn; do
        echo "remove $name:$conn"
        [ "${1:-}" == "--show" ] && continue
        [ "${1:-}" == "-s" ] && continue

        echo "$conn" >$sysfs/aims/$name/remove_link
        echo "0" >$sysfs/devices/${conn/://}/set_buffer_size
    done
done

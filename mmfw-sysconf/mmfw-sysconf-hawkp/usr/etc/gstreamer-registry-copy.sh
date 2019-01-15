#!/bin/sh
export GST_REGISTRY=/home/app/.cache/gstreamer-1.0/registry.bin

file_orig="/usr/share/.gstreamer-1.0/registry.bin"
file_new="/home/app/.cache/gstreamer-1.0/registry.bin"
file_time="/home/app/.cache/gstreamer-1.0/.registry-birthtime"

if [ -f "$file_new" ]
then
    if [ "$(<$file_time)" != "$(stat -c %y $file_orig)" ]
    then
        cp $file_orig $file_new
        chsmack -a "system::homedir" $file_new
        echo $(stat -c %y $file_orig) > $file_time
        chown app:app /home/app/.cache/gstreamer-1.0 -R
     fi
else
    if [ ! -d "/home/app/.cache/gstreamer-1.0" ]
    then
        mkdir -p /home/app/.cache/gstreamer-1.0
        chsmack -a "system::homedir" /home/app/.cache
        chsmack -a "system::homedir" /home/app/.cache/gstreamer-1.0/
    fi
    cp $file_orig $file_new
    chsmack -a "system::homedir" $file_new
    echo $(stat -c %y $file_orig) > $file_time
    chown app:app /home/app/.cache/gstreamer-1.0 -R
fi

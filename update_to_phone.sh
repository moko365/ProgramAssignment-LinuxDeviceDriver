#!/bin/sh

adb shell 'mount -o remount,rw / /'
adb push cdata.ko /lib/modules
adb shell 'mount -o remount,ro / /'

#!/bin/sh

adb shell 'mount -o remount,rw / /'
adb shell 'mkdir /lib/'
adb shell 'mkdir /lib/modules/'
adb push cdata.ko /lib/modules
adb shell 'mount -o remount,ro / /'

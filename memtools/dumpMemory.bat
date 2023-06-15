adb wait-for-device
adb root
adb wait-for-device
adb remount


adb push memtools /tmp/
adb shell chmod 777 /tmp/memtools
adb shell sync
adb shell /tmp/memtools -m 0x40000000 -s 512 -f /tmp/xiaogui.bin -d
adb shell sync
adb pull /tmp/xiaogui.bin .


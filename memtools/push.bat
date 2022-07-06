adb wait-for-device
adb root
adb wait-for-device
adb remount


adb push memdump /tmp/
adb shell chmod 777 /tmp/memdump
adb shell sync
adb shell /tmp/memdump -m 0x40000000 -s 512 -f /tmp/xiaogui.bin
adb shell sync
adb pull /tmp/xiaogui.bin .


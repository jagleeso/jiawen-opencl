# ./rl result/file.txt
ndk-build
adb push libs/armeabi-v7a/opencl_aes /data/local/tmp
adb push jni/eng_opencl_aes.cl /data/local/tmp
adb push stress_test.sh /data/local/tmp
# adb shell ./data/local/tmp/opencl_aes
adb shell ./data/local/tmp/stress_test.sh | tee "$1"

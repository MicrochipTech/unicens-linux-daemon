#/bin/sh
rm -f unicensd
mkdir -p outAndroid
cd outAndroid
cmake -DCMAKE_TOOLCHAIN_FILE=../cmake-toolchain/android.cmake ..
make
mv unicensd ..


# Building native executables for Android

**1.) Verify that your current cmake version is suitable for Android**

```bash
$ cmake --version
```
Building Android projects with cmake is listed in the documentation since version 3.7.2. However, version 3.9.4 is recommended.

If your version is in the range mentioned above or higher, you can proceed with step 2.

Uninstall current version of cmake (if installed):
```bash
$ sudo apt-get purge cmake
```
Download cmake from the web:
```bash
$ mkdir -p ~/temp
$ cd ~/temp
$ wget https://cmake.org/files/v3.9/cmake-3.9.4.tar.gz
$ tar -xzvf cmake-3.9.4.tar.gz
$ cd cmake-3.9.4/
```
Build and install cmake:
```bash
$ ./bootstrap
$ make -j4
$ sudo make install
```
Final check that cmake is now at the actual version:
```bash
$ cmake --version
```
**2.) Download current version of Android Standalone Toolchain (NDK)**

```bash
$ wget https://dl.google.com/android/repository/android-ndk-r15c-linux-x86_64.zip
$ unzip android-ndk-r15c-linux-x86_64.zip
$ cd android-ndk-r15c
```
**3.) Identify the API level and CPU architecture of your system**


Choose the correct API level of your Android Version:

| Platform Version       | API Level |
| ---------------------- | --------- |
| Android 8              | 26        |
| Android 7.1            | 25        |
| Android 7.0            | 24        |
| Android 6.0            | 23        |
| Android 5.1            | 22        |
| Android 5.0            | 21        |
| Android 4.4 Watch      | 20        |
| Android 4.4            | 19        |
| Android 4.3            | 18        |
| Android 4.2            | 17        |
| Android 4.1            | 16        |
| Android 4.0.3 4.0.4    | 15        |
| Android 4.0 to 4.0.2   | 14        |
| Android 3.2            | 13        |
| Android 3.1.x          | 12        |
| Android 3.0.x          | 11        |
| Android 2.3.1 to 2.3.4 | 10        |
| Android 2.3.0          | 9         |
| Android 2.2.x          | 8         |
| Android 2.1.x          | 7         |
| Android 2.0.1          | 6         |
| Android 2.0            | 5         |
| Android 1.6            | 4         |
| Android 1.5            | 3         |
| Android 1.1            | 2         |
| Android 1.0            | 1         |

If the exact Android version of your system is not listed or not available in the downloaded NDK, choose the closest one.

Choose the used architecture of the target device:

 - arm
 - arm64
 - mips
 - mips64
 - x86
 - x86_64

**4.) Create custom toolchain for your system**

Adapt following command using your architecture `--arch` and API level `--api`, for example:
```bash
./build/tools/make_standalone_toolchain.py --arch arm --api 21 --install-dir /tmp/arm-linux-androideabi
```

**5.) Create a cmake toolchain file**

Create a new file, for example "android.cmake" and paste this content:
```cmake
set(CMAKE_SYSTEM_NAME  Android)
set(CMAKE_SYSTEM_VERSION 1)

set(CMAKE_C_COMPILER /tmp/arm-linux-androideabi/bin/arm-linux-androideabi-gcc)
set(CMAKE_CXX_COMPILER /tmp/arm-linux-androideabi/bin/arm-linux-androideabi-g++)
set(ANDROID_API_LEVEL 21)

UNSET(CMAKE_EXE_LINKER_FLAGS CACHE)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -pie" CACHE STRING "" FORCE)

set(ADDITIONAL_PLATFORM_LIBS "log")
set(ADDITIONAL_PLATFORM_FLAGS "-DANDROID -D__ANDROID_API__=${ANDROID_API_LEVEL}")
```
Double check that the compilers specified with variables `CMAKE_C_COMPILER` and `CMAKE_CXX_COMPILER` are present on your system (done in step 4)

Also check if the variable `ANDROID_API_LEVEL` matches to your Android API level (step 3).

**6.) Cross compile your cmake project**

Change path to your cmake enabled project and enter:
```bash
$ mkdir out
$ cd out
$ cmake -DCMAKE_TOOLCHAIN_FILE=<your_path>/android.cmake ..
$ make
```
Replace `<your_path>`with the correct path of the file created in step 5.

The executable binary should now be available in the current folder.




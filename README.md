# UNICENS DAEMON (unicensd)

This project is an example integration of the [UNICENS library](https://github.com/MicrochipTech/unicens) for any kind of Linux and Android.

To get it running, you will need to get compilers, cmake and git installed. On debian based machines enter:
```bash
$ sudo apt-get install build-essential cmake git
```

To get the source code, enter:
```bash
$ git clone --recurse-submodules https://github.com/MicrochipTech/unicens-linux-daemon.git
$ cd unicens-linux-daemon
```

Building is easy:
```bash
$ ./build.sh
```

> **Android:**
> See this [tutorial](ANDROID.md), how to build unicensd (or any other application) for Android.
> After performing all steps from the tutorial, you also may use the crossAndroid.sh script.

Now the binaries `unicensd` and `xml2struct` shall be available in the current folder.
In order to run unicensd, first make sure [MOST Linux Driver](https://github.com/microchip-ais/linux) is loaded.
The daemon searches by default for these control enabled character devices (CDEVs):

 - /dev/inic-usb-crx
 - /dev/inic-usb-ctx

If you configured the driver in a different way, you can pass your CDEV names to unicensd by command line parameter:
```bash
-crx <CDEV name of RX channel>
-ctx <CDEV name of TX channel>
```

Make sure you have the rights to access these CDEVs (using `$ sudo unicensd` or using udev rules).

Launch daemon with the correct XML file, for example:
```bash
$ ./unicensd cfg/config_multichannel_audio_kit.xml &
```

To get a static network configuration, which does not need a XML file, enter:
```bash
$ ./xml2struct cfg/config_multichannel_audio_kit.xml > src/default_config.c
$ ./build.sh
$ ./unicensd &
```

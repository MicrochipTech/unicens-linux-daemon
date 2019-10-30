# UNICENS DAEMON (unicensd)

This project is an example integration of the [UNICENS library](https://github.com/MicrochipTech/unicens) for any kind of Linux and Android.

Compatible Hardware can be found here: [K2L Slim Board Familiy](https://www.k2l.de/products/34/MOST150%20Slim%20Board%20Family/)

To get it running, you will need to get compilers, cmake and git installed. On debian based machines enter:
```bash
$ sudo apt-get install build-essential cmake git
```

To get the source code, enter:
```bash
$ git clone --recurse-submodules https://github.com/MicrochipTech/unicens-linux-daemon.git
$ cd unicens-linux-daemon
```

:exclamation: Important note :exclamation: 
If you encounter problems after updating the sources, please perform:
```bash
$ git submodule update --init --recursive
```

Building is easy:
```bash
$ ./build.sh
```

> **Android:**
>
> See this [tutorial](ANDROID.md), how to build unicensd (or any other application) for Android.
> After performing all steps from the tutorial, you also may use the crossAndroid.sh script.

Now the binaries `unicensd` and `xml2struct` shall be available in the current folder.
In order to run unicensd, first make sure that the MOST Linux Driver is up and running.

Please follow this tutorial therefor: [MOST Linux Driver](https://github.com/microchip-ais/linux/blob/mchp-dev/mld/README.md). 

On Raspbian OS its recommend to get the kernel headers by following this tutorial: [rpi_source](https://github.com/notro/rpi-source/wiki).


The daemon searches by default for these control enabled character devices (CDEVs):

 - /dev/inic-control-rx
 - /dev/inic-control-tx

If you configured the driver in a different way, you can pass your CDEV names to unicensd by command line parameter:
```bash
-crx <CDEV name of RX channel>
-ctx <CDEV name of TX channel>
```

Or you let UNICENS let configure and create the needed CDEVs.

Make sure you have the rights to access these CDEVs (using `$ sudo unicensd` or using udev rules).

Launch daemon with the correct XML file, for example:
```bash
$ ./unicensd cfg/config_multichannel_audio_kit.xml &
```

If the XML file contains driver specific informations, you can let UNICENS configure the driver:
```bash
$ ./unicensd cfg/config_multichannel_audio_kit.xml -drv1 0x200 &
```

This also works on an UNICENS enabled slave nodes, use unicensc therfor:
```bash
$ ./unicensc cfg/config_multichannel_audio_kit.xml -drv1 0x2B0 &
```

To get a static network configuration, which does not need a XML file, enter:
```bash
$ ./xml2struct cfg/config_multichannel_audio_kit.xml > src/default_config.c
$ ./build.sh
$ ./unicensd -default &
```

> **UNICENS XML Description:**
>
> See this [tutorial](cfg/README.md), how to write a valid UNICENS XML configuration file.

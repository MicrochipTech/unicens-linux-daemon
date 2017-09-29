# UNICENS DAEMON (unicensd)

This project is an exmample integration of the UNICENS library for any kind of Linux.

To get it running, you will need to get compilers, cmake and git installed.  
On debian based machines enter:  
```bash
$ sudo apt-get install build-essential cmake git
```

To get the source code, enter:
```bash
$ git clone --recurse-submodules https://github.com/tkummermehr/unicens-linux-daemon.git
$ cd unicens-linux-daemon
```

Building is easy:  
```bash
$ ./build.sh
```

Now the binaries unicensd and xml2struct shall be available in the root folder
of the unicensd project.  
Make sure MOST Linux Driver is loaded (loadDriver.sh in "driver" folder).  
The daemon needs to access these control enabled CDEVs:  
`
/dev/inic-usb-crx  
/dev/inic-usb-ctx
`

Make sure you have the rights to access these CDEVs (using sudo e.g.)  
Launch daemon with the correct XML file, for example:  
```bash
$ ./unicensd cfg/config_multichannel_audio_kit.xml &
```

To get a static C-based configuration enter:  
```bash
$ ./xml2struct cfg/config_multichannel_audio_kit.xml > src/default_config.c
```

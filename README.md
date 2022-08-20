## wooten

A low-latency remote musical collaboration software project. Meant to run on a
[Bela](https://bela.io/).

Peer-to-peer session management is handled by the
[wooten server](https://github.com/smr-llc/wooten-server).

I gave a presentation on this project at MAGFest 2022.

[![MAGFest Presentation](https://img.youtube.com/vi/eLhypHRxMv4/hqdefault.jpg)](https://youtu.be/eLhypHRxMv4)

## Install via provided image

The simplest way to set up wooten is with one of the
[release images](https://github.com/smr-llc/wooten/releases).

You can install it onto your Bela-equipped BeagleBone Black revC in the same way
that you would install one of the Bela release images. Instructions are here:
https://learn.bela.io/using-bela/bela-techniques/managing-your-sd-card/

Example steps:

- flash the provided image onto a micro SD card using balena etcher
- insert the micro SD card into the BeagleBone Black
- plug the BeagleBone Black into a computer via USB
- ssh in and flash the eMMC:
```
ssh root@192.168.7.2
/opt/Bela/bela_flash_emmc.sh
```
- now you can remove the SD card and reset the BeagleBone
- you should be able to access the wooten GUI at http://192.168.7.2/gui/


## Build it yourself

### Prerequisite: liquid-dsp

Build and install
[my fork of liquid-dsp](https://github.com/hello-adam/liquid-dsp), which sets
the right compiler flags for the beaglebone black.

```
cd ~
git clone https://github.com/hello-adam/liquid-dsp.git
cd liquid-dsp
./bootstrap.sh
./configure
make
make install
ldconfig
```


### Clone, build, run

```
cd ~
git clone https://github.com/hello-adam/wooten.git
cd wooten
./build.sh
```

Then navigate to the GUI in a web browser at `http://192.168.7.2/gui`


## Notes

Tested with Bela image 0.3.8b and 0.3.8e

Current tests show ~4ms round-trip latency on a LAN, and ~45ms round-trip from
Verizion FiOS (fiber) to Comcast XFinity (cable) in the Baltimore area.

Refer to bela docs for details on how to run a program at startup.

Please report bugs and ideas in the GitHub issues.

![Screenshot of the wooten GUI](docs/wooten-gui.png)
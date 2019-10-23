# Interference testing application for OpenMote-B nodes
This document describes the usage and configuration of the interference testing application for RIOT-OS based on the drivers provided in [#12128](https://github.com/RIOT-OS/RIOT/pull/12128).

## Table of contents
- [Interference testing application for OpenMote-B nodes](#interference-testing-application-for-openmote-b-nodes)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
  - [Tips & Tricks](#tips--tricks)
  - [Usage](#usage)
  - [Recommended Reads](#recommended-reads)

## Getting started
First, you need to download and install a recent version of the ARM gcc compiler as follows:
```bash
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
$ tar xjf gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
$ sudo chmod -R -w gcc-arm-none-eabi-8-2018-q4-major
```

Now open up `.bashrc`:
```bash
$ sudo nano /.bashrc
```

The compiler must then be added to the `PATH` environment variable. Add the following line to end the of the file:
```bash
export PATH="$PATH:$HOME/gcc-arm-none-eabi-8-2018-q4-major/bin"
```

However, before you can actually call the compiler you need to either restart your shel or execute the following command:
```bash
$ exec bash
```

You can check whether intallation was succesfull by requesting the version of the compiler as follows:
```bash
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (GNU Tools for Arm Embedded Processors 8-2018-q4-major) 8.2.1 20181213 (release) [gcc-8-branch revision 267074]
Copyright (C) 2018 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

Next up, you must install some packages:
```bash
$ sudo apt-get update
$ sudo apt-get install build-essential doxygen git python-serial bridge-utils
$ sudo apt-get install python3-pip python-pip
$ sudo apt-get install pkg-config autoconf automake libtool libusb-dev libusb-1.0-0-dev libhidapi-dev libftdi-dev
$ sudo apt-get install gcc-multilib
$ sudo apt-get install netcat-openbsd
```

In order to be able to flash Intel formatted HEX files to the OpenMote-B nodes, you need some libraries:
```bash
$ pip3 install pyserial intelhex
$ pip install pyserial intelhex
```

To be able to access the USB without using sudo, the user should be part of the groups plugdev and dialout. **Don't forget to log out and log in again for these group changes to take effect!**
```bash
$ sudo usermod -a -G plugdev <user>
$ sudo usermod -a -G dialout <user>
```

It might not be essential (I have no clue), but we might as well install OpenOCD:
```bash
$ git clone git://git.code.sf.net/p/openocd/code openocd
$ cd openocd
$ ./bootstrap
$ ./configure
$ make
$ sudo make install
```

Again, you can check whether intallation was succesfull by requesting the version of OpenOCD as follows:
```bash
$ openocd --version
Open On-Chip Debugger 0.10.0+dev-00932-g85a460d5 (2019-09-23-11:58)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
```

Now got to a directory of your choosing and clone the remote repository:
```bash
$ git clone https://github.com/tinstructor/RIOT.git
$ cd RIOT/
$ git submodule update --init --recursive
$ git checkout interfere
```

If you'd like to adopt the proper git workflow (i.e., the forking workflow) for this project, according to the [Atlassian][git-workflow] git workflow tutorial, you'd first need to create your own remote repository (often named `upstream`) and local branch before pushing that branch to your remote and setting the newly created local branch to track the corresponding branch of the remote. The following code snippet shows how this is done. **If you want to send pull requests, make sure that your remote is an actual fork of [RIOT-OS](https://github.com/RIOT-OS/RIOT).**

>**Note:** there are shorter ways to achieve this, but the following snippet breaks it down into easy steps, sort off.

[git-workflow]: https://www.atlassian.com/git/tutorials/comparing-workflows/forking-workflow

```bash
$ git remote add upstream <link to your remote>
$ git checkout -b <new branch>
$ git push upstream <new branch>
$ git branch -u upstream/<new branch>
```

FINALLY you can start making changes to the code base. The interference application is located in `RIOT > examples > interference`. You might wan't to download [VSCode][vs-code] as a convenient IDE for browsing through the code base. It comes with features that are very usefull when coding, especially in terms of code completion.

>**Note:** for more information on the structure of the code base and how to modify it, have a look at the [RIOT-OS Wiki][riot-os].

[vs-code]: https://code.visualstudio.com/
[riot-os]: https://github.com/RIOT-OS/RIOT/wiki

After a while, if you feel like the changes you've made are substantial and usefull, you can always try and send me a pull request on GitHub (for the `interfere` branch). However, before you do that, make sure the changes you've made don't introduce conflicts. You may check this by pulling from the `interfere` branch on the `origin` remote and compiling + uploading. If the code doesn't compile or the flashed node shows unexpected behavior, try and solve the underlying issue before sending a pull request. 

>**Note:** a good entry book explaning git (and the tools it has to resolve conflicts) is [Jump Start Git][jump-start-git] by Shaumik Daityari. More advanced topics are covered in the git bible, i.e., [Pro Git][pro-git] which may be downloaded free of charge but can be hard to read for complete beginners.

[jump-start-git]: https://www.sitepoint.com/premium/books/jump-start-git
[pro-git]: https://git-scm.com/book/en/v2

```bash
$ git pull origin interfere
$ cd RIOT/examples/interference
$ make -j flash BOARD=openmote-b GNRC_NETIF_NUMOF=1 PORT_BSL=/dev/ttyUSB1
```

## Tips & Tricks
An invaluable command is `make list-ttys`. What it does is displaying all available USB devices for flashing like so for example:
```bash
$ make list-ttys
/sys/bus/usb/devices/1-1: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB1, ttyUSB0
/sys/bus/usb/devices/2-2: Silicon Labs Zolertia RE-Mote platform, serial: 'ZOL-RM02-B0240000002', tty(s): ttyUSB4
/sys/bus/usb/devices/1-2: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB2, ttyUSB3
```

The annoying thing is that each FTDI UART bridge shows up as 2 seperate devices. Generally speaking, the highest number is the one you want to specify in your `make flash` command, like so:
```bash
$ make -j flash BOARD=openmote-b GNRC_NETIF_NUMOF=1 PORT_BSL=/dev/ttyUSB1
```

>**Note:** notice the `GNRC_NETIF_NUMOF=1` argument. The number specified can be either 1 or 2 and indicates wheter just the AT86RF215 Sub-GHz interface is made available (when specifying 1) or both interfaces (i.e., Sub-GHz + 2.4GHz) of the AT86RF215 transceiver can be used.

## Usage
The basic usage of this example application is pretty straighforward. Currently, you can only use this application to its full potential if you're using one of the platforms / configurations specified in [this post](https://github.com/RIOT-OS/RIOT/pull/12128#issue-312769776). Everything is written for OpenMote-B nodes, so those should work out of the box. When using a different platform, the pins to be used on that platform can be changes in `RIOT > examples > interference > interference_constants.h`. This header file also includes several other configuration constants that may be changed for your purposes.

Assuming you're using an OpenMote-B node, a rising edge on pin PB0 will trigger a transmission of a message over the AT86RF215's Sub-GHz interface. By default, this interface is configured for the [SUN-FSK PHY in Operating Mode 1](https://www.silabs.com/content/usergenerated/asi/cloud/attachments/siliconlabs/en/community/wireless/proprietary/forum/jcr:content/content/primary/qna/802_15_4_promiscuous-tbzR/hivukadin_vukadi-iTXQ/802.15.4-2015.pdf?#page=499), i.e., 2-FSK with a data-rate of 50 kbps, a modulation index of 1, a channel spacing of 200 kHz and a channel 0 center frequency at 863 125 kHz (for a total of 34 channels in the 863-870 MHz range). A rising edge on pin PB2 will cycle through all available PHY configurations contained in the `phy_cfg_sub_ghz[]` array defined in `interference_constants.h`. Transmissions are also defined as constants of type `if_tx_t` (containing the interface to send on, the destination MAC addr, and the actual payload of the 802.15.4 data frame) and the transmission used can be set on line 45 of `RIOT > examples > interference > main.c`. In similar fashion a rising edge on PB1 and PB3 triggers a transmission and a PHY reconfiguration on the AT86RF215 2.4GHz interface respectively.

For your convenience a python script (see `RIOT > examples > interference > capture.py`) is provided that creates a logfile from the serial output passed to it (via a pipe). Creating a logfile with a name of your choice is done as follows:

```bash
$ make BOARD=openmote-b term PORT=/dev/ttyUSB | python3 capture.py -f <name of logfile>
Created logfile "test.log"
/home/relsas/RIOT-benpicco/dist/tools/pyterm/pyterm -p "/dev/ttyUSB1" -b "115200" 
2019-10-16 13:04:13,124 # Connect to serial port /dev/ttyUSB1
Welcome to pyterm!
Type '/exit' to exit.
2019-10-16 13:04:21,351 # PKTDUMP: data received:
2019-10-16 13:04:21,355 # ~~ SNIP  0 - size:  25 byte, type: NETTYPE_UNDEF (0)
2019-10-16 13:04:21,356 # 00000000  30  31  32  33  34  35  36  37  38  39  30  31  32  33  34  35
2019-10-16 13:04:21,365 # 00000010  36  37  38  39  30  31  32  33  34
2019-10-16 13:04:21,366 # ~~ SNIP  1 - size:  18 byte, type: NETTYPE_NETIF (-1)
2019-10-16 13:04:21,366 # if_pid: 4  rssi: -29  lqi: 0
2019-10-16 13:04:21,367 # flags: 0x0
2019-10-16 13:04:21,367 # src_l2addr: 22:68
2019-10-16 13:04:21,381 # dst_l2addr: 22:68:31:23:9D:F1:96:37
2019-10-16 13:04:21,381 # ~~ PKT    -  2 snips, total size:  43 byte
```

Coming soon.

>**Note:** make sure debugging is enabled in `RIOT > examples > interference > main.c`, otherwise the analyzer script won't work properly.

## Recommended Reads
- [**IEEE 802.15.4-2015**](https://standards.ieee.org/standard/802_15_4-2015.html): The currently active PHY + MAC layer standard for 802.15.4 networks. Although this is the official standard, many developers seem to have a total disregard for certain aspects of it. Especially on the Sub-GHz PHY layers, there seems to be a lot of confusion as to what is actually standardised and what is not. The fact that IEEE standards are very expensive to obtain doesn't help this confusion either.
- [**Jump Start Git**](https://www.sitepoint.com/premium/books/jump-start-git) *by Shaumik Daityari*: If you don't speak Git, this book is where you might want to start your journey.
- [**C in a Nutshell**](http://shop.oreilly.com/product/0636920033844.do) *by Peter Prinz & Tony Crawford*: Although C is an incredibly forgiving language when it comes to getting what you want out of it (hence the abundance of terribly written but otherwise "functional" code), some parts of our source code contain more advanced features and contstructs from the C99 and later C11 specification. We make no mistake about it, seasoned embedded developers would probably have a heart-attack looking at parts of our code, but the important thing to remember is that we're well on our way to write better source code and this book is what's getting us there.
- [**The Quick Python Book**](https://www.manning.com/books/the-quick-python-book-third-edition) *by Naomi Ceder*: Tis book offers a clear introduction to the Python programming language and its famously easy-to-read syntax. Written for programmers new to Python, the latest edition includes new exercises throughout. It covers features common to other languages concisely, while introducing Python's comprehensive standard functions library and unique features in detail.
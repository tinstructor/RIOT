# Interference testing application for OpenMote-B nodes
This document describes the usage and configuration of the interference testing application for RIOT-OS based on the drivers provided in [#12128](https://github.com/RIOT-OS/RIOT/pull/12128).

## Table of contents
- [Interference testing application for OpenMote-B nodes](#interference-testing-application-for-openmote-b-nodes)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
  - [Tips & Tricks](#tips--tricks)
  - [Usage](#usage)
    - [Interference PRR](#interference-prr)
    - [Controller Script](#controller-script)
  - [Visualizing Results](#visualizing-results)
  - [Recommended Reads](#recommended-reads)

## Getting started
First, you need to download and install a recent version of the ARM gcc compiler as follows:
```
$ wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/8-2018q4/gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
$ tar xjf gcc-arm-none-eabi-8-2018-q4-major-linux.tar.bz2
$ sudo chmod -R -w gcc-arm-none-eabi-8-2018-q4-major
```

Now open up `.bashrc`:
```
$ sudo nano /.bashrc
```

The compiler must then be added to the `PATH` environment variable. Add the following line to end the of the file:
```
export PATH="$PATH:$HOME/gcc-arm-none-eabi-8-2018-q4-major/bin"
```

However, before you can actually call the compiler you need to either restart your shel or execute the following command:
```
$ exec bash
```

You can check whether intallation was succesfull by requesting the version of the compiler as follows:
```
$ arm-none-eabi-gcc --version
arm-none-eabi-gcc (GNU Tools for Arm Embedded Processors 8-2018-q4-major) 8.2.1 20181213 (release) [gcc-8-branch revision 267074]
Copyright (C) 2018 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

Next up, you must install some packages:
```
$ sudo apt-get update
$ sudo apt-get install build-essential doxygen git python-serial bridge-utils
$ sudo apt-get install python3-pip python-pip
$ sudo apt-get install pkg-config autoconf automake libtool libusb-dev libusb-1.0-0-dev libhidapi-dev libftdi-dev
$ sudo apt-get install gcc-multilib
$ sudo apt-get install netcat-openbsd
```

In order to be able to flash Intel formatted HEX files to the OpenMote-B nodes, you need some libraries:
```
$ pip3 install pyserial intelhex
$ pip install pyserial intelhex
```

To be able to access the USB without using sudo, the user should be part of the groups plugdev and dialout. **Don't forget to log out and log in again for these group changes to take effect!**
```
$ sudo usermod -a -G plugdev <user>
$ sudo usermod -a -G dialout <user>
```

It might not be essential (I have no clue), but we might as well install OpenOCD:
```
$ git clone git://git.code.sf.net/p/openocd/code openocd
$ cd openocd
$ ./bootstrap
$ ./configure
$ make
$ sudo make install
```

Again, you can check whether intallation was succesfull by requesting the version of OpenOCD as follows:
```
$ openocd --version
Open On-Chip Debugger 0.10.0+dev-00932-g85a460d5 (2019-09-23-11:58)
Licensed under GNU GPL v2
For bug reports, read
        http://openocd.org/doc/doxygen/bugs.html
```

Now got to a directory of your choosing and clone the remote repository:
```
$ git clone https://github.com/tinstructor/RIOT.git
$ cd RIOT/
$ git submodule update --init --recursive
$ git checkout interfere
```

If you'd like to adopt the proper git workflow (i.e., the forking workflow) for this project, according to the [Atlassian][git-workflow] git workflow tutorial, you'd first need to create your own remote repository (often named `upstream`) and local branch before pushing that branch to your remote and setting the newly created local branch to track the corresponding branch of the remote. The following code snippet shows how this is done. **If you want to send pull requests, make sure that your remote is an actual fork of [RIOT-OS](https://github.com/RIOT-OS/RIOT).**

>**Note:** there are shorter ways to achieve this, but the following snippet breaks it down into easy steps, sort off.

[git-workflow]: https://www.atlassian.com/git/tutorials/comparing-workflows/forking-workflow

```
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

```
$ git pull origin interfere
$ cd RIOT/examples/interference
$ make -j flash BOARD=openmote-b GNRC_NETIF_NUMOF=1 PORT_BSL=/dev/ttyUSB1
```

## Tips & Tricks
An invaluable command is `make list-ttys`. What it does is displaying all available USB devices for flashing like so for example:
```
$ make list-ttys
/sys/bus/usb/devices/1-1: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB1, ttyUSB0
/sys/bus/usb/devices/2-2: Silicon Labs Zolertia RE-Mote platform, serial: 'ZOL-RM02-B0240000002', tty(s): ttyUSB4
/sys/bus/usb/devices/1-2: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB2, ttyUSB3
```

The annoying thing is that each FTDI UART bridge shows up as 2 seperate devices. Generally speaking, the highest number is the one you want to specify in your `make flash` command, like so:
```
$ make -j flash BOARD=openmote-b GNRC_NETIF_NUMOF=1 PORT_BSL=/dev/ttyUSB1
```

>**Note:** notice the `GNRC_NETIF_NUMOF=1` argument. The number specified can be either 1 or 2 and indicates wheter just the AT86RF215 Sub-GHz interface is made available (when specifying 1) or both interfaces (i.e., Sub-GHz + 2.4GHz) of the AT86RF215 transceiver can be used.

After flashing you can access the node's terminal by issuing the following command (make sure to specify the correct ttyUSB device):
```
$ make term BOARD=openmote-b PORT=/dev/ttyUSB1
/home/relsas/RIOT-benpicco/dist/tools/pyterm/pyterm -p "/dev/ttyUSB1" -b "115200" 
Twisted not available, please install it if you want to use pyterm's JSON capabilities
2020-03-05 12:33:44,038 # Connect to serial port /dev/ttyUSB1
Welcome to pyterm!
Type '/exit' to exit.
```

You can now list all available shell commands by issuing the `help` command in the terminal window.
```
> help
2020-03-05 12:35:52,034 #  help
2020-03-05 12:35:52,034 # Command              Description
2020-03-05 12:35:52,035 # ---------------------------------------
2020-03-05 12:35:52,050 # numbytesub           set the number of payload bytes in a sub-ghz message
2020-03-05 12:35:52,051 # numbytesup           set the number of payload bytes in a 2.4-ghz message
2020-03-05 12:35:52,052 # taddrsub             toggle between a preset IF and TRX destination address (sub-GHz)
2020-03-05 12:35:52,066 # taddrsup             toggle between a preset IF and TRX destination address (2.4 GHz)
2020-03-05 12:35:52,067 # saddrsub             set a destination address (sub-GHz)
2020-03-05 12:35:52,068 # saddrsup             set a destination address (2.4 GHz)
2020-03-05 12:35:52,083 # physub               set the sub-GHz PHY configuration (via index)
2020-03-05 12:35:52,085 # physup               set the 2.4 GHz PHY configuration (via index)
2020-03-05 12:35:52,085 # reboot               Reboot the node
2020-03-05 12:35:52,098 # ps                   Prints information about running threads.
2020-03-05 12:35:52,099 # random_init          initializes the PRNG
2020-03-05 12:35:52,100 # random_get           returns 32 bit of pseudo randomness
2020-03-05 12:35:52,114 # ifconfig             Configure network interfaces
```

Sometimes you might experience that for some reason yet unknown (see [#13044](https://github.com/RIOT-OS/RIOT/issues/13044)), a node hangs after issuing the `reboot` command. Normally, after rebooting via the shell (resetting via the physical reset button gives the same terminal output but **always** works) you should get the following output:
```
> reboot
2020-03-05 12:56:27,722 #  reboot
2020-03-05 12:56:27,739 # main(): This is RIOT! (Version: 2014.01-22760-gf819c-interfere)
2020-03-05 12:56:27,739 # PHY reconfigured to SUN-FSK 863-870MHz OM1
2020-03-05 12:56:27,754 # Welcome to RIOT!
```

But instead, you might get:
```
> reboot
2020-03-05 12:56:27,722 #  reboot
```

After which the node hangs forever untill you perform a hardware reset via the physical reset button. This behaviour will cause the most usefull script for automating interference testing (i.e., `examples > interference > controller.py`) to fail (it won't explicitely fail but the output will be useless). The "solution" to this problem is to flash the node via jlink after making the following change to the `board.h` file of your platform (in our case: `boards > openmote-b > include > board.h`).
```c
#define CCA_BACKDOOR_ENABLE       (0)
```

Flashing over jlink requires you to install some software available on the [SEGGER website](https://www.segger.com/downloads/jlink/#J-LinkSoftwareAndDocumentationPack). After installation, flashing over jlink is a easy as:
```
$ PROGRAMMER=jlink make flash BOARD=openmote-b GNRC_NETIF_NUMOF=1
```

>**Note:** this is not a good solution because it will prevent you from flashing via USB untill you set `CCA_BACKDOOR_ENABLE` to `(1)` again and flash over jlink once more. Setting `CCA_BACKDOOR_ENABLE` to `(0)` (possibly) also has other unwanted consequences. 

## Usage
The basic usage of this example application is pretty straighforward. Currently, you can only use this application to its full potential if you're using one of the platforms / configurations specified in [this post](https://github.com/RIOT-OS/RIOT/pull/12128#issue-312769776). Everything is written for OpenMote-B nodes, so those should work out of the box. When using a different platform, the pins to be used on that platform can be changes in `RIOT > examples > interference > interference_constants.h`. This header file also includes several other configuration constants that may be changed for your purposes.

Assuming you're using an OpenMote-B node, a rising edge on pin PB0 will trigger a transmission of a message over the AT86RF215's Sub-GHz interface. By default, this interface is configured for the [SUN-FSK PHY in Operating Mode 1](https://www.silabs.com/content/usergenerated/asi/cloud/attachments/siliconlabs/en/community/wireless/proprietary/forum/jcr:content/content/primary/qna/802_15_4_promiscuous-tbzR/hivukadin_vukadi-iTXQ/802.15.4-2015.pdf?#page=499), i.e., 2-FSK with a data-rate of 50 kbps, a modulation index of 1, a channel spacing of 200 kHz and a channel 0 center frequency at 863 125 kHz (for a total of 34 channels in the 863-870 MHz range). A rising edge on pin PB2 will cycle through all available PHY configurations contained in the `phy_cfg_sub_ghz[]` and `phy_cfg_2_4_ghz[]`arrays defined in `interference_constants.h`. However, it's much easier (and exponentially more stable) to set the PHY configuration by issuing the proper shell command,i.e., `physub` or `physup` (for the sub-GHz and 2.4GHz interface config respectively) followed by the index of the proper PHY config in the aforementioned `phy_cfg_sub_ghz[]` and `phy_cfg_2_4_ghz[]`arrays. For example, reconfiguring the sub-GHz PHY config goes like this:
```
> physub 2
2020-03-05 13:21:07,950 #  physub 2
2020-03-05 13:21:07,951 # PHY reconfigured to SUN-OFDM 863-870MHz O4 MCS2
```

>**Note:** In similar fashion, a rising edge on PB1 and PB3 triggers a transmission and a PHY reconfiguration on the AT86RF215 2.4GHz interface. However, for PHY reconfigurations, you should really use the `physup` shell command.

Transmissions are defined as structs of type `if_tx_t` (containing the interface to send on, the destination MAC address, and the actual payload of the 802.15.4 DATA frame) By default, triggering a transmission on a certain interface (by applying a rising edge to the corresponding pin) causes a 120 byte long packet (with a payload containing consecutive numbers from 0 to 9 in periodic fashion) to be transmitted to the `22:68:31:23:9D:F1:96:37` MAC address (on said interface), which in our case is the MAC address of the *receiving node*.

>**Note:** By *receiving node* we mean the node intended to receive the DATA transmission, not the node that's supposed to receive the interfering transmission.

As you may remember from the output of the `help` shell command, you can set the amount of L2 payload bytes as well as the destination address to send to. You can also toggle between predefined destination MAC addresses specified in `examples > interference > interference_constants.h`. When setting the destination address with `saddrsub` and `saddrsup` for sub-GHz and 2.4GHz transmissions respectively, you must make sure to format the specified MAC destination address correctly (or the shell will yell at you like in the third example).
```
> saddrsub 22:68:31:23:9D:F1:96:37
2020-03-05 14:03:27,899 #  saddrsub 22:68:31:23:9D:F1:96:37
> saddrsub "22:68:31:23:9D:F1:96:37"
2020-03-05 14:03:36,426 #  saddrsub "22:68:31:23:9D:F1:96:37"
> saddrsub 22:68:31:23:9D:F1:96:3
2020-03-05 14:03:43,658 #  saddrsub 22:68:31:23:9D:F1:96:3
2020-03-05 14:03:43,659 # usage: saddrsub <address string>
```

For your convenience a python script (see `RIOT > examples > interference > capture.py`) is provided that creates a logfile from the serial output passed to it (via a pipe). Creating a logfile with a name of your choice is done as follows:

```
$ make term BOARD=openmote-b PORT=/dev/ttyUSB1 | python3 capture.py -f <name of logfile>
Created logfile "<name of logfile>.log"
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

The logfiles can then be analyzed with `analyzer.py`. The script may be called by providing the name of the logfile to be analyzed and the csv file to which log-derived information must be appended: `$ python3 analyzer.py <name of logfile>.log <name of csv file>.csv`. Each change of PHY configuration is indicated in the specified logfile. Since, the analyzer script has no clue at which PHY configuration index you where when the logfile was created, it assumes that you cycled through all combinations of the same configurations as specified in the `phy_cfg_sub_ghz[]` and `phy_cfg_2_4_ghz[]`arrays (defined in `examples > interference > interference_constants.h`). This came to be because the legacy interference testing application **did** effectively cycle through all available PHY config combinations by means of pin interrupts triggered by the timing controller (see `examples > timing_control`). This resulted in a single logfile containing all necessary information to calculate the PRR for each combination of PHY configurations (of the transmitter/receiver and interferer respectively).

>**Note:** make sure debugging is enabled in `RIOT > examples > interference > main.c`, otherwise the analyzer script won't work properly.

There is an easy way around this issue though. You can force the analyzer script to cycle through a list of PHY configurations (for both transmitter/receiver and interferer respectively) with just a single, user-specified PHY configuration string. For this purpose, the optional `-i` and `-t` arguments where created:
```
$ python3 analyzer.py -h
usage: analyzer.py [-h] [-a] [-i INTERFERER] [-t TRANSMITTER] [-n N]
                   [-l IFR_LOGFILE]
                   rx_logfile csvfile

positional arguments:
  rx_logfile            The rx_logfile to be analyzed.
  csvfile               The csv file to which log-derived info must be written
                        / appended.

optional arguments:
  -h, --help            show this help message and exit
  -a, --append          Append to the csv file, overwrite it otherwise.
  -i INTERFERER, --interferer INTERFERER
                        The interferer PHY in case there's only one.
  -t TRANSMITTER, --transmitter TRANSMITTER
                        The transmitter PHY in case there's only one.
  -n N, --numtx N       Amount of messages transmitted, set to 100 otherwise.
  -l IFR_LOGFILE, --ifr_logfile IFR_LOGFILE
                        The ifr_logfile to be analyzed.
```

Using both options at the same time, the analyzer script calculates the PRR for that specific combination by:
1. counting the messages received before the first PHY reconfig indication in the given logfile and;
2. dividing by the number of messages transmitted. 

The number of transmitted messages is set through the `-n` argument. Also, if the specified csv file already exists in the current directory, the `-a` flag may be usefull if you wish to append the results of the call to `analyzer.py` to said csv file instead of overwriting them (which is the default behavior).
```
$ python3 analyzer.py <name of logfile>.log <name of csv file>.csv -i <interferer PHY> -t <transmitter/receiver PHY> -n <#messages>
```

>**Note:** depending on whether the corresponding pin from the timing controller is connected to the PHY reconfiguration pin of the receiving node, the PHY reconfiguration indication might not appear in the logfile. In case you unplugged the wire, as you should, you have to write the reconfiguration indication to the logfile manually in order for the analyzer script to work properly.

>**Note:** the format of the strings passed to the `-t` and `-i` options is important. All legal values are specified in the `TRX_PHY_CONFIGS` and `IF_PHY_CONFIGS` lists (see `examples > interference > analyzer.py`) respectively:
>```py
>TRX_PHY_CONFIGS = ["SUN-FSK 863-870MHz OM1", "SUN-FSK 863-870MHz OM2",
>               "SUN-OFDM 863-870MHz O4 MCS2", "SUN-OFDM 863-870MHz O4 MCS3",
>               "SUN-OFDM 863-870MHz O3 MCS1", "SUN-OFDM 863-870MHz O3 MCS2"]
>IF_PHY_CONFIGS = ["SUN-FSK 863-870MHz OM1", "SUN-FSK 863-870MHz OM2",
>               "SUN-OFDM 863-870MHz O4 MCS2", "SUN-OFDM 863-870MHz O4 MCS3",
>               "SUN-OFDM 863-870MHz O3 MCS1", "SUN-OFDM 863-870MHz O3 MCS2"]
>```

It is highly advisable that you follow a certain naming scheme when specifying the csv filenames (this will become clear further on). Specifically, you should format it as follows: `IF_<IF bytes>_TX_<TX bytes>B_OF_<offset><prefix>S_SIR_<sir>DB.csv`. Wherein `<IF bytes>` is the amount of bytes in the interfering transmission's PHY payload (i.e., L2 payload size + L2 header), `<TX bytes>` is the amount of bytes in the data transmission's PHY payload (i.e., L2 payload size + L2 header), `<offset>` is a quantifier for the amount of time units between start of transmission of the interferer (IF) and the transmitter (TX) (have a look at `RIOT > examples > timing_control > README.md` for further explanation), `<prefix>` can be either capital U (for micro) or capital M (for mili) and specifies the base time-unit for the TX <-> IF offset, and finally, `<sir>` specifies the difference in signal strength between TX and IF (in dB). Some correctly formatted examples include: `IF_20B_TX_40B_OF_1MS_SIR_10DB.csv` and `IF_30B_TX_70B_OF_1350US_SIR_0DB.csv`.

After running the analyzer script, the output written to the csv file has the following format (just an example):

|TX / RX PHY configuration  |IF PHY configuration       |TRX PRR|
|---------------------------|---------------------------|-------|
|SUN-OFDM 863-870MHz O4 MCS3|SUN-OFDM 863-870MHz O4 MCS3|0.00   |
|SUN-OFDM 863-870MHz O3 MCS2|SUN-OFDM 863-870MHz O4 MCS3|0.92   |
|...                        |...                        |...    |

### Interference PRR
A newer feature of the analyzer script is that it has an option (`-l`) to specify an additional logfile containing all packets received by a node listening to interfering transmissions. Since this feature is quite new, it could output useless data if you specify an interference logfile wherein the amount of *next experiment* indications isn't equal to the amount of PHY reconfig indications in the receiving node's logfile. However, if you're forcing the analyzer script as previously explained, this is a non-issue.

>**Note:** once again, by *receiving node* we mean the node intended to receive the DATA transmission, not the node that's supposed to receive the interfering transmission.

If you connect the transmitter/interferer PHY reconfig pin of the timing controller to PA7 of an openmote-b node that's receiving/logging interfering transmissions, similarly to the node receiving data transmissions, it will write "NEXT_EXP" (i.e., a *next experiment* indication) to the terminal (and therefore to its logfile) each time the transmitter/receiver PHY changes. This feature ensures backward compatibility with the original analyzer functionality. 

>**Note:** Otherwise, the analyzer script wouldn't know which logged interfering transmissions occured during which transmitter/receiver PHY configuration, because the interferer PHY only changes (thus causing a PHY reconfiguration indication to be logged) when the transmitter/receiver has cycled through all possible PHY configurations.

Anyway, when using the `-l` option, an additional column is written to the specified csv file. As you might expect, this column contains the PRR of the interfering transmissions.

|TX / RX PHY configuration  |IF PHY configuration       |TRX PRR|IF PRR|
|---------------------------|---------------------------|-------|------|
|SUN-OFDM 863-870MHz O4 MCS3|SUN-OFDM 863-870MHz O4 MCS3|0.00   |0.00  |
|SUN-OFDM 863-870MHz O3 MCS2|SUN-OFDM 863-870MHz O4 MCS3|0.92   |0.00  |
|...                        |...                        |...    |...   |

### Controller Script
I admit that it's quite difficult to keep track of every possible feature previously discussed. Hence, the `examples > interference > controller.py` script was included as an example. For now, this script doesn't work with the two FSK PHY configs from `phy_cfg_sub_ghz[]` and `phy_cfg_2_4_ghz[]`. This is because the drivers used for the openmote-b (see [#12128](https://github.com/RIOT-OS/RIOT/pull/12128)) are still experimental and, in the commit on which this example was based, FSK wasn't working properly.

Before you can really understand what this script does, you should really read `examples > timing_control > README.md`. In short, the timing controller triggers a number of rising edges on its GPIO pins. Two of those pins are configured to trigger interrupts on the send pins of a transmitter and interferer node respectively. The time offset between these triggers, and hence (in theory) the time offset between DATA and interference transmission, is user configurable via a timing controller shell command (the `offset` command). However, in practice, the time between triggering a send pin interrupt and actually sending out data is a function of the size of the packet to be transmitted. Hence, the offset needs to be compensated, which can be done with the `comp` timing controller shell command. 

The offset compensation is therefore a function of 2 variables, i.e., the size of both the data and interference transmissions. Luckily for you, we've measured the required compensation for 11 different combinations of packet sizes and for every other combination the required compensation is calculated via 2-dimensional interpolation as follows:

```py
x = np.ogrid[20:130:10]
x = np.tile(x,11)
y = np.ogrid[20:130:10]
y = np.repeat(y,11)
z = np.array([20,-28,-70,-118,-154,-208,-256,-298,-352,-388,-442,
              62,20,-28,-70,-118,-154,-208,-256,-298,-352,-388,
              104,62,20,-28,-70,-118,-154,-208,-256,-298,-352,
              152,104,62,20,-28,-70,-118,-154,-208,-256,-298,
              194,152,104,62,20,-28,-70,-118,-154,-208,-256,
              242,194,152,104,62,20,-28,-70,-118,-154,-208,
              284,242,194,152,104,62,20,-28,-70,-118,-154,
              332,284,242,194,152,104,62,20,-28,-70,-118,
              380,332,284,242,194,152,104,62,20,-28,-70,
              428,380,332,284,242,194,152,104,62,20,-28,
              476,428,380,332,284,242,194,152,104,62,20])
rbf = interpolate.Rbf(x,y,z)

def get_compensation(size_tuple):
    # NOTE size_tuple = (if_payload_size,trx_payload_size)
    compensation = int(round(rbf(size_tuple[0],size_tuple[1]).tolist()))
    return compensation
```

Now, the controller script cycles through every combination of a list of transmitter/receiver PHY configs (`trx_phy_cfg`), a list of interferer PHY configs (`if_phy_cfg`), a list of interference payload sizes (L2 payload + 15 byte MHR + 4 byte MFR)(`if_payload_sizes`), and a single data transmission payload size (`trx_payload_size`). For each combination, all nodes are configured with the `physub` command and the payload size of the 2 transmitting nodes is set through the `numbytesub` command, after which the timing controller is configured to generate `num_of_tx` rising edges on the pins connected to the transmitter and interferer send pins (at a certain offset + compensation). In addition, the destination address of the data and interference transmissions is set to `trx_dest_addr` and `if_dest_addr` respectively via the `saddrsub` shell command.
```py
trx_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
               (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
if_phy_cfg = [(2,"SUN-OFDM 863-870MHz O4 MCS2"), (4,"SUN-OFDM 863-870MHz O3 MCS1"),
              (3,"SUN-OFDM 863-870MHz O4 MCS3"), (5,"SUN-OFDM 863-870MHz O3 MCS2")]
trx_payload_size = 120 # in bytes
if_payload_sizes = [50,70,90] # in bytes
trx_dest_addr = "22:68:31:23:9D:F1:96:37"
if_dest_addr = "22:68:31:23:14:F1:99:37"
num_of_tx = 100
```

After all transmissions (for a given combination) are done, all nodes are rebooted via shell, the logging streams are closed, and the newly created logfiles are passed to the analyzer script. Since the logfiles only contain the received messages for a single PHY combination, the `-i` and `-t` arguments were used (in conjunction with the `-n` and `-l` arguments). 

>**Note:** Assuming no pins are connected to the timing controller (other than a send pin), the PHY reconfiguration and next experiment indication strings were writen to each logfile manually.

>**Note:** this script requires installation of the scipy python library: `pip3 install scipy`

## Visualizing Results
Coming soon

## Recommended Reads
- [**IEEE 802.15.4-2015**](https://standards.ieee.org/standard/802_15_4-2015.html): The currently active PHY + MAC layer standard for 802.15.4 networks. Although this is the official standard, many developers seem to have a total disregard for certain aspects of it. Especially on the Sub-GHz PHY layers, there seems to be a lot of confusion as to what is actually standardised and what is not. The fact that IEEE standards are very expensive to obtain doesn't help this confusion either.
- [**Jump Start Git**](https://www.sitepoint.com/premium/books/jump-start-git) *by Shaumik Daityari*: If you don't speak Git, this book is where you might want to start your journey.
- [**C in a Nutshell**](http://shop.oreilly.com/product/0636920033844.do) *by Peter Prinz & Tony Crawford*: Although C is an incredibly forgiving language when it comes to getting what you want out of it (hence the abundance of terribly written but otherwise "functional" code), some parts of our source code contain more advanced features and contstructs from the C99 and later C11 specification. We make no mistake about it, seasoned embedded developers would probably have a heart-attack looking at parts of our code, but the important thing to remember is that we're well on our way to write better source code and this book is what's getting us there.
- [**The Quick Python Book**](https://www.manning.com/books/the-quick-python-book-third-edition) *by Naomi Ceder*: Tis book offers a clear introduction to the Python programming language and its famously easy-to-read syntax. Written for programmers new to Python, the latest edition includes new exercises throughout. It covers features common to other languages concisely, while introducing Python's comprehensive standard functions library and unique features in detail.
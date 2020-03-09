# Timing controller application for interference testing
This document describes the usage and configuration of the timing controller application in conjunction with the interference testing application for RIOT-OS. This application was written for the remote-revb platform but it could easily be adapted to run on many other platforms supported by RIOT.

## Table of contents
- [Timing controller application for interference testing](#timing-controller-application-for-interference-testing)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
  - [Tips & Tricks](#tips--tricks)
  - [Usage](#usage)
  - [Recommended Reads](#recommended-reads)

## Getting started

In order to get started, follow the getting started section in `examples > interference > README.md`.

## Tips & Tricks
An invaluable command is `make list-ttys`. What it does is displaying all available USB devices for flashing like so for example:
```
$ make list-ttys
/sys/bus/usb/devices/1-1: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB1, ttyUSB0
/sys/bus/usb/devices/2-2: Silicon Labs Zolertia RE-Mote platform, serial: 'ZOL-RM02-B0240000002', tty(s): ttyUSB4
/sys/bus/usb/devices/1-2: FTDI Dual RS232-HS, serial: '', tty(s): ttyUSB2, ttyUSB3
```

Flashing a remote-revb node with this example is fairly straightforward:
```
$ make flash BOARD=remote-revb PORT_BSL=/dev/ttyUSB4
```

After flashing you can access the node's terminal by issuing the following command (make sure to specify the correct ttyUSB device):
```
$ make term BOARD=remote-revb PORT=/dev/ttyUSB4
/home/relsas/RIOT-benpicco/dist/tools/pyterm/pyterm -p "/dev/ttyUSB4" -b "115200" 
Twisted not available, please install it if you want to use pyterm's JSON capabilities
2020-03-09 11:51:04,163 # Connect to serial port /dev/ttyUSB4
Welcome to pyterm!
Type '/exit' to exit.
```

You can now list all available shell commands by issuing the `help` command in the terminal window.
```
> help
2020-03-09 11:51:12,941 #  help
2020-03-09 11:51:12,942 # Command              Description
2020-03-09 11:51:12,943 # ---------------------------------------
2020-03-09 11:51:12,944 # start                starts the interference experiment
2020-03-09 11:51:12,945 # offset               sets an IF/TX offset (in microseconds)
2020-03-09 11:51:12,964 # comp                 sets an IF/TX offset compensation (in microseconds)
2020-03-09 11:51:12,965 # numtx                sets the number of messages transmitted for each IF/TX PHY combination
2020-03-09 11:51:12,966 # ifnumphy             sets the number of IF PHY reconfigurations
2020-03-09 11:51:12,966 # trxnumphy            sets the number of TRX PHY reconfigurations
2020-03-09 11:51:12,980 # ifphy                set the IF phy configuration
2020-03-09 11:51:12,982 # trxphy               set the TRX phy configuration
2020-03-09 11:51:12,982 # reboot               Reboot the node
2020-03-09 11:51:12,985 # ps                   Prints information about running threads.
```

## Usage
In short, the timing controller generates rising edges on its GPIO pins. Two of those pins (PA7 and PA5) are configured to trigger interrupts on the send pins of a transmitter and interferer node respectively (when used in conjunction with `examples > interference`). The time offset (in microseconds) between these rising edges, and hence (in theory) the time offset between DATA and interference transmission, is user configurable via the `offset` shell command:
```
> offset 1000
2020-03-09 12:13:44,151 #  offset 1000
```

However, in practice, the time between triggering a send pin interrupt and actually sending out data is a function of the size of the packet to be transmitted. Hence, the offset needs to be compensated, which can be done with the `comp` shell command (in microseconds):
 ```
 > comp 100
2020-03-09 12:14:34,720 #  comp 100
 ```

The number of consecutive rising edges on PA7 and PA5 can in turn be set with the `numtx` shell command (100 when unspecified). After the specified amount of rising edges was generated, a rising edge is generated on a (sub-)set of pins originally intended to trigger PHY reconfigurations on the openmote-b nodes in an interference test. The complete set of PHY reconfig pins can be found in `examples > timing_control > timing_control_constants.h`:
```c
#define TX_PHY_CFG_PIN      GPIO_PIN(PORT_A, 4)
#define RX_PHY_CFG_PIN      GPIO_PIN(PORT_C, 1)
#define IF_PHY_CFG_PIN      GPIO_PIN(PORT_C, 0)
```

Notice that we specifically said (sub-)set. That's because a rising edge on `IF_PHY_CFG_PIN` only occurs when `numtx` consecutive rising edges on PA7 and PA5 and a subsequent rising edge on PA4 and PC1 have occured for a user-specified amount of times.

>**Note:** by default, when you don't specify the number via `trxnumphy`, it equals 1. In that case, a rising edge occurs on **all** PHY reconfig pins after **every** `numtx` rising edges on PA7 and PA5.

## Recommended Reads
- [**C in a Nutshell**](http://shop.oreilly.com/product/0636920033844.do) *by Peter Prinz & Tony Crawford*: Although C is an incredibly forgiving language when it comes to getting what you want out of it (hence the abundance of terribly written but otherwise "functional" code), some parts of our source code contain more advanced features and contstructs from the C99 and later C11 specification. We make no mistake about it, seasoned embedded developers would probably have a heart-attack looking at parts of our code, but the important thing to remember is that we're well on our way to write better source code and this book is what's getting us there.
- [**The Quick Python Book**](https://www.manning.com/books/the-quick-python-book-third-edition) *by Naomi Ceder*: Tis book offers a clear introduction to the Python programming language and its famously easy-to-read syntax. Written for programmers new to Python, the latest edition includes new exercises throughout. It covers features common to other languages concisely, while introducing Python's comprehensive standard functions library and unique features in detail.
- [**RIOT online course**](https://github.com/RIOT-OS/riot-course): This project provides a learning course for RIOT, an operating system for constrained IoT devices. The course is divided in 5 sections, of which section 2, 3 and 4 are certainly worth a read if you wish to better understand the inner workings of this application.
- [**The Zolertia RE-Mote wiki**](https://github.com/Zolertia/Resources/wiki/RE-Mote): The RE-Mote is a hardware development platform designed jointly with universities and industrial partners, in the frame of the European research project RERUM. It aims to fill the gap of existing IoT platforms lacking an industrial-grade design and ultra-low power consumption, yet allowing makers and researchers alike to develop IoT applications and connected products. This wiki is particularly usefull if you keep forgetting the RE-Mote pin layout.
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
Coming soon

## Recommended Reads
- [**IEEE 802.15.4-2015**](https://standards.ieee.org/standard/802_15_4-2015.html): The currently active PHY + MAC layer standard for 802.15.4 networks. Although this is the official standard, many developers seem to have a total disregard for certain aspects of it. Especially on the Sub-GHz PHY layers, there seems to be a lot of confusion as to what is actually standardised and what is not. The fact that IEEE standards are very expensive to obtain doesn't help this confusion either.
- [**Jump Start Git**](https://www.sitepoint.com/premium/books/jump-start-git) *by Shaumik Daityari*: If you don't speak Git, this book is where you might want to start your journey.
- [**C in a Nutshell**](http://shop.oreilly.com/product/0636920033844.do) *by Peter Prinz & Tony Crawford*: Although C is an incredibly forgiving language when it comes to getting what you want out of it (hence the abundance of terribly written but otherwise "functional" code), some parts of our source code contain more advanced features and contstructs from the C99 and later C11 specification. We make no mistake about it, seasoned embedded developers would probably have a heart-attack looking at parts of our code, but the important thing to remember is that we're well on our way to write better source code and this book is what's getting us there.
- [**The Quick Python Book**](https://www.manning.com/books/the-quick-python-book-third-edition) *by Naomi Ceder*: Tis book offers a clear introduction to the Python programming language and its famously easy-to-read syntax. Written for programmers new to Python, the latest edition includes new exercises throughout. It covers features common to other languages concisely, while introducing Python's comprehensive standard functions library and unique features in detail.
- [**RIOT online course**](https://github.com/RIOT-OS/riot-course): This project provides a learning course for RIOT, an operating system for constrained IoT devices. The course is divided in 5 sections, of which section 2, 3 and 4 are certainly worth a read if you wish to better understand the inner workings of this application.
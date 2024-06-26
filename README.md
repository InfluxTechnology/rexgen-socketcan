# Influx Technology LTD - ReXgen SocketCAN Driver for linux

## About SocketCAN Driver

The SocketCAN driver allows you to use [ReXgen ](https://app.gitbook.com/o/HhTfJJOHHF3lfqYUgSrl/s/TCRfbN5hBLeUzIaKHbJ7/)devices as CAN interfaces within your socketCAN-based applications. For further information on SocketCAN, see [<mark style="color:blue;">https://www.kernel.org/doc/Documentation/networking/can.txt</mark>](https://www.kernel.org/doc/Documentation/networking/can.txt)&#x20;

## Getting started

Prerequisites: Make sure the following is installed before proceeding:

1. make
2. gcc
3. can-utils is required to use commands like candump/cansend etc

Make sure the can-dev module is loaded properly. You can check this with "lsmod | grep can-dev." If it is not loaded yet, you can do this with

"sudo modprobe can-dev"

Building and installing the driver while starting the script

"sudo ./do\_make.sh"

## Using SocketCAN

1.  Before you can send and receive data, you need to configure the interface. To do this, you can use the 'ip' tool. For example, if you want to set the bitrate of 500Kbit (500000 bit) to the can0 interface, you can do this with\
    "sudo ip link set can0 type can bitrate 500000"

    If you need to set CAN FD with a bitrate of 500Kbit and data bitrate of 8Mbit\
    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on"

    We support loopback mode and listen-only mode. Typical socket can syntax is\
    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on loopback on|off"\
    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on listen-only on|off"

    We also support CANFD non-ISO mode via fd-non-iso on|off
2.  Now the bitrate is set, and you can start the interface by typing

    "sudo ip link set can0 up"
3.  After this, you can send and receive data on CAN0. At the beginning, you may want to check some small communications. To do this, you could use the tools from "can-utils". For example, you could try to send data with

    "cansend can0 123#112233" - Simple CAN message\
    "cansend can0 00000123#112233" - CAN message with ID Extended (IDE)\
    "cansend can0 123##0112233" - Simple CANFD message\
    "cansend can0 123##1112233" - CANFD message (EDL) with BRS flag\
    "cansend can0 00000123##1112233" - CANFD message (EDL) with BRS flag and IDE

    and receive it with

    "candump can0"\
    or\
    "candump can0 -x" for extra details

    A prerequisite for this is a connected and properly configured bus with at least two communication partners.

For more details, visit our [<mark style="color:blue;">GitHub</mark>](https://github.com/InfluxTechnology/rexgen-socketcan).

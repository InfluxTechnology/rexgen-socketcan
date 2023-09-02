# SocketCAN Installation on Raspberry PI OS

1.  Clone the repository [<mark style="color:blue;">https://github.com/InfluxTechnology/rexgen-socketcan.git</mark>](https://github.com/InfluxTechnology/rexgen-socketcan.git)

    _git clone_ [_<mark style="color:blue;">https://github.com/InfluxTechnology/rexgen-socketcan.git</mark>_](https://github.com/InfluxTechnology/rexgen-socketcan.git)
2. Install raspberrypi-kernal-headers

```bash
sudo apt-get install raspberrypi-kernel-headers
```

3.  Install can-utils

    This will install the necessary tools to acquire CAN data using SocketCAN

```bash
sudo apt-get install can-utils
```

&#x20;4\. Open the _rexgen-socketcan_ folder

5. Load the can-dev module

```bash
sudo modprobe can-dev
```

&#x20;6\.  Run the do\_make.sh script

```bash
sudo ./domake.sh
```

&#x20;_7._ Ignore any Warnings

8. Plugin the ReXgen Device
9. Reboot the device

## Using Socket CAN

1.  Before you can send and receive data, you need to configure the interface. To do this, you can use the 'ip' tool. For example, if you want to set the bitrate of 500Kbit (500000 bit) to the can0 interface, you can do this with&#x20;

    "sudo ip link set can0 type can bitrate 500000"

    If you need to set CAN FD with a bitrate of 500Kbit and data bitrate of 8Mbit&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on"

    We support loopback mode and listen-only mode. Typical socket can syntax is&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on loopback on|off"&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on listen-only on|off"

    We also support CANFD non-ISO mode via fd-non-iso on|off
2.  Now the bitrate is set, and you can start the interface by typing

    &#x20;"sudo ip link set can0 up"
3.  After this, you can send and receive data on CAN0. At the beginning, you may want to check some small communications. To do this, you could use the tools from "can-utils". For example, you could try to send data with

    "cansend can0 123#112233" - Simple CAN message&#x20;

    "cansend can0 00000123#112233" - CAN message with ID Extended (IDE)&#x20;

    "cansend can0 123##0112233" - Simple CANFD message&#x20;

    "cansend can0 123##1112233" - CANFD message (EDL) with BRS flag&#x20;

    "cansend can0 00000123##1112233" - CANFD message (EDL) with BRS flag and IDE&#x20;

    and receive it with

    "candump can0"&#x20;

    or&#x20;

    "candump can0 -x" for extra details

    As a prerequisite for this is a connected and properly configured bus with at least two communication partners

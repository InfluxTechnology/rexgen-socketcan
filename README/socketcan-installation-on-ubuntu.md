# SocketCAN Installation on Ubuntu

1. Install git

```bash
sudo apt-get install git
```

2.  Clone the repository [<mark style="color:blue;">https://github.com/InfluxTechnology/rexgen-socketcan.git</mark>](https://github.com/InfluxTechnology/rexgen-socketcan.git)

    &#x20;_git clone_ [_<mark style="color:blue;">https://github.com/InfluxTechnology/rexgen-socketcan.git</mark>_](https://github.com/InfluxTechnology/rexgen-socketcan.git)
3. Install gcc

<pre class="language-bash"><code class="lang-bash"><strong>sudo apt-get install gcc
</strong></code></pre>

4. Install make

```bash
sudo apt-get install make    
```

5.  Install can-utils

    This will install the necessary tools to acquire CAN data using SocketCAN

```bash
sudo apt-get install can-utils
```

6. Open the _rexgen-socketcan_ folder
7. Load the can-dev module

```bash
sudo modprobe can-dev
```

8. Run the do\_make.sh script

```bash
sudo ./domake.sh
```

9. Ignore any Warnings
10. Plugin the ReXgen Device
11. Reboot the device

## Using SocketCAN

1.  Before you can send and receive data, you need to configure the interface. To do this, you can use the 'ip' tool. For example, if you want to set the bitrate of 500Kbit (500000 bit) to the can0 interface, you can do this with&#x20;

    "sudo ip link set can0 type can bitrate 500000"

    If you need to set CAN FD with a bitrate of 500Kbit and a data bitrate of 8Mbit&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on"

    We support loopback mode and listen-only mode. Typical socket can syntax is&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on loopback on|off"&#x20;

    "sudo ip link set can0 type can bitrate 500000 dbitrate 8000000 fd on listen-only on|off"

    We also support CANFD non-ISO mode via fd-non-iso on|off
2.  Now the bitrate is set, and you can start the interface by typing

    "sudo ip link set can0 up"
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

    A prerequisite for this is a connected and properly configured bus with at least two communication partners.

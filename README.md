# Influx Technology LTD - ReXgen SocketCAN Driver for linux
============================================================


About SocketCAN Driver
----------------
The SocketCAN driver allows you to use ReXgen device as CAN interfaces within your own socketCAN based applications. For further informations on SocketCAN see 
https://www.kernel.org/doc/Documentation/networking/can.txt

Getting started
Make sure the can-dev module is loaded properly. 
   You can check this with "lsmod | grep can-dev"
   If it is not loaded yet you can do this with 

   "sudo modprobe can-dev"

Using SocketCAN
------------------------------------
1) Before you can send and receive data you need to configure 
   the interface. To do this you can use the 'ip' tool. For example if you 
   want to set the bitrate of 1Mbit (1000000 bit) to the can0 interface
   you can do this with

   "sudo ip link set can0 type can bitrate 1000000"

2) Now the bitrate is set and you can start the interface by typing

   "sudo ip link set can0 up"

3) After this you can send and receive data on can0. At the beginning you may
   just want to check some small communications. To do this you could use the
   tools from "can-utils". For example you could try to send data with

   "cansend can0 123#112233"
   
   and receive it with 
   
   "candump can0"
   
   As a prerequisite for this, is a connected and properly configured bus with
   at least two communication partners.

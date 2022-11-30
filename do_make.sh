#!/bin/sh
cp ../rexgen-socketcan/rex* kernel/drivers/net/can/usb/rexgen_usb/

sudo make uninstall 
sudo make
sudo make install 
sudo dmesg -C
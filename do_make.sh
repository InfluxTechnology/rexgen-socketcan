#!/bin/sh
sudo make uninstall 
sudo make
sudo make install 
sudo dmesg -C

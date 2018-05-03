#!/bin/sh

sudo apt-get update
sudo apt-get upgrade
sudo apt-get install g++ cmake git libssl1.0-dev libcurl4-openssl-dev

# Create a local swap file of 1GB, needed for compilation
dd if=/dev/zero of=~/myswapfile bs=1M count=1024
chmod 600 ~/myswapfile
sudo mkswap ~/myswapfile
sudo swapon ~/myswapfile

# Optional install PHP server
sudo apt-get install lighttpd
sudo apt-get install php7.0-cgi php7.0-curl

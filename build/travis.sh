#!/bin/bash

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq

sudo apt-get install -qq g++-7
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-7 90

sudo apt-get install -qq -y libasound2-dev

sudo apt-get install -qq build-essential xorg-dev libc++-dev

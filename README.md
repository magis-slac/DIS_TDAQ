# DIS_TDAQ
Trigger and Data Acquisition codes for MAGIS-100 Distributed Imaging System (DIS)

These codes are supposed to run on a Raspberry Pi (RPi) with `Spinnaker` to configure and read the cameras and `pigpio` to switch power and trigger the cameras.

## RPi Set-up

The standard set-up is:

* Hardware: Raspberry Pi 4B (8GB RAM)
* OS: 64-bit Raspberrian OS (Debian GNU/Linux 11, Bullseye)
* Software:
	* Spinnaker SDK: 2.7.0.128-arm64
	* pigpio: 2.52

These codes might run fine on a different set-up, but it is not guaranteed.
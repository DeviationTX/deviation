**IMPORTANT:**\
**You must install the filesystem along with the new firmware.  Make sure to save your models first!**\
**The configuration of hardware mods has changed since the previous 4.0.1 version. Instead of the tx.ini, they are in a new hardware.ini file, just like they were in the last nightly builds.**


## Installation

    Follow the instructions in the User Manual that applies to your transmitter


## Critical bug fixes
  * Fix mixer ordering and mixer loop depdency.
  * Significant reduce stick-to-air latency in FrskyX, Devo, and SFHSS protocols.


## Major Features
  * Support new transimitters:
    - Deviation 12E, F4, F7, F12E,
    - iRangeX IR8M
    - Jumper T8SG, V2, V2 Plus
    - RadioLink AT9
  * Introduce a new format of localiztion file to save Flash space and speed up UI rendering.
  * Scanner to check the RF enviroment around.
  * Enhance telemetry support by adding some new fields.
  * Support Japanese language in manual and Tx
  * F12e-XMS supports CN/JA/RU language
  * Improved Tx Power interface

## Minor Features / Bug Fixes
  * Reduce RAM requirement by optimizing stack and heap usage
  * Reduce ROM size for AT9
  * New bigger font for AT9
  * Support of Beken BK2421 / BK2423 support is removed.
  * Fix USBHID under OSX
  * Tx power setting shows power values of radio in use
  * Support Walkera QR-X350 telemetry format

## New protocols
    * PXX protocol
    * GD00X protocol
    * Air Hogs "Star Trek" NCC-1701 protocol
    * WFLY protocol
    * Hitec protocol
    * protocol for MJX Bugs 3, 6, 8
    * SBUS, CRSF
    * Flydream V3 protocol
    * Corona protocol
    * LOLI protocol
    * Bugs3Mini protocol: Bugs 3H format
    * Bayang protocol: DHD D4 and H8S 3D formats

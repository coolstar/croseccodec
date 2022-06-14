Chrome EC Audio Codec Driver

* NOTE: This driver requires the https://github.com/coolstar/crosecbus driver to be present, as well as the correct ACPI tables set up by coreboot

Initializes the Chrome EC Audio Codec to use 16-bit 48 Khz Audio Streams

* Wake on Voice is not currently implemented

Note:
* Intel SST and AMD ACP proprietary drivers do NOT have documented interfaces, so this driver will not work with them.
* Using this driver on chromebooks with this audio codec will require using CoolStar ACP Audio or CoolStar SOF Audio
* Most chromebooks with Chrome EC Codec use it for DMIC only. This driver waits 2 seconds before initialization to allow the main codec to start generating a clock signal

Tested on HP Chromebook 14b (Ryzen 3 3250C)
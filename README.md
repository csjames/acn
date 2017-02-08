#acn cwk

![Pinout](https://farm4.staticflickr.com/3818/10585364014_df2e1604bc_o.png)

## Libs

[Board Manager](https://github.com/MCUdude/MiniCore#how-to-install)
[RF69 Library](https://github.com/LowPowerLab/RFM69)
[Flash Chip Library -- Required for board](https://github.com/LowPowerLab/SPIFlash)

## FAQ

*I can't put a bootloader on the fucking thing?*
- Add a 10K pullup between D10 and VCC (SS of RFM69)
*Everything is fine, however the radio module cannot transmit?*
- The breadboards seem to stop transmission being possible. This could be useful when testing mesh drops

# 3DPE Hardware Configuration

This document describes the hardware setup for the 3DPE (3D Print ePaper) variant of OpenEPaperLink.

## Hardware Components

| Component | Model | Link |
|-----------|-------|------|
| MCU | Seeed Studio XIAO ESP32-C3 | [Product Page](https://www.seeedstudio.com/Seeed-XIAO-ESP32C3-p-5431.html) |
| Driver Board | ePaper Driver Board for XIAO V2 | [Product Page](https://www.seeedstudio.com/ePaper-breakout-Board-for-XIAO-V2-p-6374.html) |
| Display | 2.9" Monochrome ePaper (296x128) | SSD1680 controller |

## Pin Mapping

The ePaper Driver Board V2 connects the display to the XIAO using the following pin assignments:

| Function | XIAO Pin | ESP32-C3 GPIO | Description |
|----------|----------|---------------|-------------|
| RST | D0 | GPIO 2 | Display reset |
| CS | D1 | GPIO 3 | SPI chip select |
| BUSY | D2 | GPIO 4 | Display busy signal |
| DC | D3 | GPIO 5 | Data/Command select |
| SCK | D8 | GPIO 8 | SPI clock |
| MOSI | D10 | GPIO 10 | SPI data out |

**Important:** The BUSY pin must be correctly configured for the display to function. If the display flickers but shows no content, verify the BUSY pin is set to D2 (GPIO 4).

## Display Driver

The firmware uses the GxEPD2 library with the `GxEPD2_290_BS` driver class (SSD1680 controller).

If your display doesn't work with this driver, try these alternatives in `display_3dpe.h`:
- `GxEPD2_290_T5` - DEPG0290BS (SSD1680)
- `GxEPD2_290_T5D` - DEPG0290BS (UC8151D)
- `GxEPD2_290` - GDEH029A1
- `GxEPD2_290_T94` - GDEM029T94

## Building and Flashing

### Build
```bash
cd ESP32_AP-Flasher
pio run -e XIAO_ESP32C3_3DPE
```

### Flash
```bash
pio run -e XIAO_ESP32C3_3DPE -t upload --upload-port /dev/cu.usbmodem*
```

On macOS, the port is typically `/dev/cu.usbmodem1101`. On Linux, it's usually `/dev/ttyACM0`.

### Serial Monitor
```bash
pio device monitor -p /dev/cu.usbmodem1101 -b 115200
```

## Troubleshooting

### Display flickers but shows nothing
- **Cause:** BUSY pin misconfigured
- **Solution:** Verify `EPD_BUSY` is set to `D2` in `display_3dpe.h`

### Display doesn't respond at all
- Check all SPI connections
- Verify the display ribbon cable is properly seated
- Try a different display driver class

### Upload fails with "No upload port found"
- Ensure the XIAO is connected via USB
- On XIAO ESP32-C3, you may need to hold BOOT while pressing RESET to enter bootloader mode
- Specify the port manually with `--upload-port`

## References

- [Seeed XIAO ESP32-C3 Wiki](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/)
- [ePaper Driver Board V2 Wiki](https://wiki.seeedstudio.com/xiao_eink_expansion_board_v2/)
- [GxEPD2 Library](https://github.com/ZinggJM/GxEPD2)
- [XIAO ePaper Panel Driver Board (Example Code)](https://github.com/EpaperPix/XIAOePaperPanelDriverBoard)

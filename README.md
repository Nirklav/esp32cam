# esp32cam

## Firmware for esp32cam that allows:
* Using high resolution (with UXGA 1600x1200 ~10 fps)
* Multistream on several devices
* Write records on sd card
* Attach it to [Home Assistant](https://github.com/Nirklav/esp32cam_ha)

## How-to: flash
Repository contains already built binaries, you can falsh it with help of `espflash`.
1. Install `espflash` [guide](https://docs.rs/espflash/latest/espflash/).
2. Flash build/camera.elf with help of `espflash`.

## How-to: build
1. Install VS Code
2. Install ESP-IDF [guide](https://github.com/espressif/vscode-esp-idf-extension/blob/master/docs/tutorial/install.md)
3. Clone this repo
4. Open it with VS Code
5. Open 'ESP-IDF SDK Configuration Editor'
   1. Set Wi-Fi password and ssid (You can search fields with 'wifi')
   2. Set Server password and port (You can search fields with 'server configuration')
6. Connect your camera to computer
7. Click 'ESP-IDF Build, Flash and Monitor'

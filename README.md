# Firmware for MatchX LoRa node

This is the firmware for the MatchX products LPWAN DevKit and MatchX Core with DA14680 BLE 4.2 SoC from Dialog Semiconductor.

## Installation

In order to use this code you need the SDK provided by Dialog [here](https://www.dialog-semiconductor.com/products/connectivity/bluetooth-low-energy/smartbond-da14680-and-da14681). Download the latest SDK (DA1468x SDK1.0.14.1081) from there by registering to their website and unzip it to your workspace. It is important to clone this repo under the same folder with the SDK for smooth development as shown below.

```
    .
    ├── DA1468x_DA15xxx_SDK_1.0.14.1081     # SDK folder
    └── node-prod-firmware                  # Firmware folder
```

## Usage

You can start developing your application and use the given Makefile with command **make** to build the code. This Makefile uses the [custom_config.h](https://gitlab.com/matchx/node-prod-firmware/blob/master/custom_config.h). If there are no errors during compiling, a binary will be generated under the "obj" folder. This binary can be flashed with the scripts provided by Dialog SDK. This application is using the BLE SUOTA(Software Updates Over The Air) feature for firmware updates. Therefore, you need to run the script **initial_flash** given by Dialog under the SDK folder "/utilities/scripts/suota/v11/" to flash the binary generated before. Please refer to the User Guide of your product for further information.

You can also use the Eclipse based SmartSnippets IDE for development. Download the latest version from the [website](https://www.dialog-semiconductor.com/products/connectivity/bluetooth-low-energy/smartbond-da14680-and-da14681) under "Development Tools". After installing, choose the SDK folder as your workspace and go to "File->Import->General->Existing Projects into Workspace". Browse and select the firmware folder to find the project, then click finish to import it. You can use the build configuration "MatchX" to build with the given Makefile. You can also use other build configurations by Dialog but be aware that those configurations are using different custom_config_xxx.h files under the folder [config](https://gitlab.com/matchx/node-prod-firmware/tree/master/config) and generate the output under other folders with different names. Please refer to the user manual of SmartSnippets Studio [UM-B-057](https://www.dialog-semiconductor.com/sites/default/files/user_manual_um-b-057_0.pdf) for further details on how to use this IDE.

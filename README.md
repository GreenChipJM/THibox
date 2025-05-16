# THibox AI Voice Weather Station

## AI Voice Weather Station

This is an AI Voice Weather Station that retrieves real-time weather data via the Internet and displays it on a TFT screen, while supporting voice weather broadcasting triggered by a touch button.

## Features

- Real-time display of current temperature, humidity, pressure, wind speed, and other weather information
- Display temperature trend chart for the past 12 hours
- Support weather information broadcasting in both Chinese and English
- Automatically synchronize network time via NTP server
- Easily configure WiFi connection using WiFiManager

## Hardware Requirements

- T-Display-S3 development board
- I2S audio output device (e.g., MAX98357A)
- speaker (4~8Ω)
- Touch sensor chip (QH8233)

## Hardware Connection

![image 1](https://github.com/user-attachments/assets/161b5961-6b4e-4e8c-8d2a-023c4b80c7ef)




## Software Dependencies

- Arduino IDE
- ESP32 support package
- The following Arduino libraries:
    - WiFiManager (version 2.0.17)
    - TFT_eSPI (version 2.5.43)
    - ArduinoJson (version 7.1.0)
    - HTTPClient (version 0.6.1)
    - ESP32Time (version 2.0.6)
    - ESP32-audioI2S-master (version 3.0.13)
    - UrlEncode (version 1.0.1)
- API keys:
    - OpenWeatherMap API key
    - Baidu TTS API key

## Installation Steps

1. **Install Arduino IDE**
Download and install Arduino IDE from the official website.
2. **Install ESP32 Support Package**
In Arduino IDE, go to "Tools" -> "Board" -> "Boards Manager," search for and install ESP32 (version 3.2.0).
3. **Install Library Files**
In Arduino IDE, go to "Tools" -> "Manage Libraries," search for and install the libraries listed above.
4. **Obtain API Keys**
    - Register an OpenWeatherMap account and obtain an API key.
    - Register a Baidu AI Open Platform account and obtain the API Key and Secret Key for TTS API.
5. **Configure TFT_eSPI Library**
Edit the TFT_eSPI/User_Setup_Select.h file to use #include <User_Setups/Setup206_LilyGo_T_Display_S3.h>.
6. **Configure Development Board Options**
Refer to the image for board settings:
    
    ![image](https://github.com/user-attachments/assets/98f886ec-7420-4e96-8627-f7733e115070)

    

## API Key Application Methods

### Applying for OpenWeatherMap API Key

1. Visit the OpenWeatherMap official website [Members](https://home.openweathermap.org/users/sign_up).
2. Register a new account or log in with an existing account.
3. After logging in, click on "API keys" in the navigation bar.
4. In the "Create key" section, enter a name (e.g., "weatherStation") and click "Generate".
5. Copy the generated API key and paste it into the kOpenWeatherApiKey variable in the code.

**Note**: New accounts may need to wait for a period (usually a few hours) before the API key becomes active.

### Applying for Baidu TTS API Key (Optional, for Chinese Voice Broadcasting)

1. Visit the Baidu AI Open Platform [Speech Technology - Baidu Smart Cloud Console](https://console.bce.baidu.com/ai-engine/old/#/ai/speech/app/list).
2. Click "Log in" or "Register" to create a Baidu account.
3. After logging in, click "Console" to enter the management page.
4. In the left menu, select "Speech Technology" -> "Speech Synthesis".
5. Click "Create Application," fill in the application name and description, and click "Create Now".
6. In the application list, find the newly created application and click "View Details".
7. On the details page, copy the "API Key" and "Secret Key" and paste them into the kApiKey and kSecretKey variables in the code.

**Important**: Please keep your API keys secure and do not disclose them to others.

For more details, refer to:

- [OpenWeatherMap API Documentation](https://openweathermap.org/api)
- [Baidu TTS API Documentation](https://ai.baidu.com/ai-doc/SPEECH/mlciskuqn)

## Configuration Options

In the "USER CONFIGURATION" section of the code, you can modify the following options:

- kTimeZone: Time zone, default is 2
- kTown: City name, default is "zegbre"
- kOpenWeatherApiKey: OpenWeatherMap API key
- kUnits: Weather units, metric for metric system, imperial for imperial system
- kApiKey and kSecretKey: API Key and Secret Key for Baidu TTS API
- kSpeakChinese: Whether to use Chinese broadcasting, true for Chinese, false for English

## Usage

1. **Upload the Code**
Upload the project code to the device.
2. **Configure WiFi**
On first startup, the device will create a WiFi hotspot named "GCWifiConf". Connect to this hotspot, open a browser and visit 192.168.4.1, then enter your WiFi name and password to configure.
3. **Trigger Voice Broadcasting**
Press the touch button on the development board to broadcast the current weather information. It will also automatically broadcast once on startup.
4. **View Weather Information**
The TFT screen will display real-time weather information and time.

## Factory Reset

1. Factory Firmware: [THibox.bin](https://github.com/GreenChipJM/THibox/blob/main/THibox/build/esp32.esp32.esp32s3/THibox.ino.bin)
2. For esp32 firmware burning, please refer to[Flash Download Tool User Guide - ESP32 - — ESP Test Tools latest documentation](https://docs.espressif.com/projects/esp-test-tools/en/latest/esp32/production_stage/tools/flash_download_tool.html)

## Troubleshooting

- **WiFi Connection Failure**
Check if the WiFi name and password are correct and ensure the device is within WiFi range.
- **Weather Data Retrieval Failure**
Check if the OpenWeatherMap API key is correct and the city name is spelled correctly.
- **Voice Broadcasting Failure**
Check if the Baidu TTS API key is correct, verify if the token is obtained correctly via serial print, and ensure the network connection is normal.
- **Screen Display Abnormality**
Check if the TFT_eSPI library configuration matches your screen model.

## Contribution Guidelines

Contributions, issue reports, feature suggestions, or code contributions are welcome. Please participate in the project through GitHub's Issues and Pull Requests features.

## License

This project is licensed under the MIT License.

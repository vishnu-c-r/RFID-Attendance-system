RFID-Based Attendance System
============================

An ESP32 and RFID-based solution for efficient attendance management, integrated with Google Sheets via Google Apps Script.

Overview
--------

This project implements an attendance system using an **ESP32** microcontroller and an **MFRC522** RFID reader. It allows students or employees to scan their RFID tags to mark attendance. The system communicates with a Google Apps Script to log attendance data in a Google Sheets spreadsheet.

Features
--------

*   **RFID Scanning**: Reads RFID tags/cards to identify users.
    
*   **Wi-Fi Connectivity**: ESP32 connects to a Wi-Fi network to communicate with Google Sheets.
    
*   **Web Interface**: Hosts a web server for administrative tasks like starting/stopping attendance sessions and registering/removing users.
    
*   **Google Sheets Integration**: Attendance data is stored and managed in Google Sheets.
    
*   **LED Indicators**: Provides visual feedback for system status.
    

Getting Started
---------------

### Hardware Requirements

*   **ESP32 Development Board**
    
*   **MFRC522 RFID Reader Module**
    
*   **RFID Tags/Cards** (13.56 MHz)
    
*   **LEDs** and **Resistors** for indicators
    
*   **Jumper Wires** and **Breadboard/PCB** for assembly
    

### Software Requirements

*   **Arduino IDE**
    
*   **Required Arduino Libraries**:
    
    *   WiFi.h
        
    *   WebServer.h
        
    *   WiFiClientSecure.h
        
    *   HTTPClient.h
        
    *   ArduinoJson.h
        
    *   MFRC522v2.h (and associated driver libraries)
        
    *   SPI.h
        
    *   ESPmDNS.h
        
*   **Google Apps Script** deployed as a web app
    
*   **Google Sheets** for data storage
    

### Setup Instructions

#### 1\. Hardware Setup

Connect the ESP32, RFID reader, and LEDs according to the wiring diagram provided in the documentation.

**ESP32 to MFRC522 RFID Reader:**

**MFRC522 PinESP32 GPIO PinDescription**VCC3.3VPower SupplyGNDGNDGroundRSTGPIO 21Reset PinNSS/SDA/SSGPIO 22Slave Select (SS)MOSIGPIO 23Master Out Slave In (SPI)MISOGPIO 19Master In Slave Out (SPI)SCKGPIO 18Serial Clock (SPI)

**ESP32 to LEDs:**

*   **Ready to Scan LED:**
    
    *   **Anode (+):** GPIO 5 (LED\_READY\_PIN) via a 220Ω resistor.
        
    *   **Cathode (-):** Connected to GND.
        
*   **Marking Attendance LED:**
    
    *   **Anode (+):** GPIO 4 (LED\_MARK\_PIN) via a 220Ω resistor.
        
    *   **Cathode (-):** Connected to GND.
        

#### 2\. Software Setup

*   **Arduino IDE Configuration:**
    
    *   Install the ESP32 Board Support:
        
        *   In the Arduino IDE, go to **File** → **Preferences**.
            
        *   arduinoCopy codehttps://dl.espressif.com/dl/package\_esp32\_index.json
            
        *   Go to **Tools** → **Board** → **Boards Manager**, search for **ESP32**, and install it.
            
    *   Select your specific ESP32 board under **Tools** → **Board**.
        
*   **Install Required Libraries:**
    
    *   Use the Library Manager (**Sketch** → **Include Library** → **Manage Libraries...**) to install:
        
        *   **MFRC522v2**
            
        *   **WiFi**
            
        *   **WebServer**
            
        *   **WiFiClientSecure**
            
        *   **HTTPClient**
            
        *   **ArduinoJson**
            
        *   **SPI**
            
        *   **ESPmDNS**
            
*   **Configure the Code:**
    
    *   cppCopy codeconst char\* ssid = "your\_SSID";const char\* password = "your\_PASSWORD";const char\* googleScriptURL = "your\_Google\_Apps\_Script\_URL";
        
    *   Replace "your\_SSID" and "your\_PASSWORD" with your Wi-Fi network credentials.
        
    *   Replace "your\_Google\_Apps\_Script\_URL" with the URL of your deployed Google Apps Script.
        
*   **Upload the Code:**
    
    *   Connect your ESP32 to your computer via USB.
        
    *   Select the correct port under **Tools** → **Port**.
        
    *   Upload the code to the ESP32.
        

#### 3\. Google Apps Script Setup

*   **Create a New Google Apps Script:**
    
    *   Go to [**script.google.com**](https://script.google.com/) and create a new project.
        
*   **Script Code:**
    
    *   Write the Google Apps Script code to handle HTTP requests and interact with Google Sheets.
        
    *   The script should handle actions like markAttendance, register, remove, and resetAttendance.
        
*   **Deploy the Script as a Web App:**
    
    *   Click on **Deploy** → **New Deployment**.
        
    *   Choose **Web app** as the deployment type.
        
    *   Set **Who has access** to **Anyone** or **Anyone with the link**.
        
    *   Copy the **Web App URL** for use in the ESP32 code.
        

#### 4\. Google Sheets Setup

*   **Create a New Google Sheet:**
    
    *   Structure the sheet with columns like **RFID**, **Name**, **Admission Number**, **On Bus**, **Time**.
        
*   **Link the Sheet to the Google Apps Script:**
    
    *   In the Apps Script editor, link the spreadsheet by its ID.
        

Documentation
-------------

For detailed instructions, including hardware setup, software configuration, code explanation, and usage guide, please refer to the [**Notion Page**](https://open-swim-7f6.notion.site/RFID-Based-Attendance-System-3351dd3ad6234cd5b2a4330ea7ca32fa) for this project.

Usage
-----

### Starting the System

1.  **Power On:**
    
    *   Connect the ESP32 to a power source.
        
    *   The system will initialize and connect to Wi-Fi.
        
2.  **Access the Web Interface:**
    
    *   Open a web browser on a device connected to the same network.
        
    *   http://log.local/ if mDNS is working.
        

### Registering Students

1.  **Scan RFID Tag:**
    
    *   Go to the **Register/Remove Student** section.
        
    *   Click on **Scan RFID**.
        
    *   Present an RFID tag to the reader.
        
2.  **Fill in Student Details:**
    
    *   After scanning, you will be prompted to enter the student's name and admission number.
        
    *   Submit the form to register the student.
        

### Starting an Attendance Session

1.  **Begin Session:**
    
    *   On the main page, click on **Start Attendance Session**.
        
    *   The **"Ready to Scan"** LED will turn on.
        
2.  **Scanning Tags:**
    
    *   Students can now scan their RFID tags to mark attendance.
        
    *   The system will provide visual feedback through the LEDs.
        

### Stopping an Attendance Session

*   Click on **Stop Attendance Session** to end the session.
    
*   Both LEDs will turn off.
    

### Viewing Attendance Status

*   Click on **View Attendance Status** to see the last scanned tag and status.
    

### Resetting Attendance

*   Click on **Reset Attendance Session** to clear attendance records.
    

Troubleshooting
---------------

### RFID Reader Not Working

*   **Check Wiring:**
    
    *   Ensure all connections between the ESP32 and RFID reader are secure and correct.
        
*   **Power Supply:**
    
    *   Make sure the RFID reader is powered by **3.3V**, not 5V.
        
*   **Library Compatibility:**
    
    *   Verify that the correct libraries are installed and up to date.
        

### Cannot Connect to Wi-Fi

*   **Credentials:**
    
    *   Ensure the SSID and password are correct in the code.
        
*   **Network Issues:**
    
    *   Check if the Wi-Fi network is operational.
        

### Web Interface Not Accessible

*   **IP Address:**
    
    *   Confirm the ESP32's IP address via the Serial Monitor.
        
*   **Firewall Settings:**
    
    *   Ensure that there are no network restrictions blocking access.
        

### LEDs Not Functioning

*   **Check LED Wiring:**
    
    *   Ensure the LEDs are connected correctly, with appropriate resistors.
        
*   **GPIO Pin Conflicts:**
    
    *   Verify that the GPIO pins used are not conflicting with other functions.
        

### Attendance Not Marked

*   **Google Apps Script Issues:**
    
    *   Verify that the script is deployed correctly and accessible.
        
    *   Check the script logs for any errors.
        
*   **Internet Connectivity:**
    
    *   Ensure the ESP32 has internet access to communicate with Google services.
        

License
-------

This project is licensed under the [MIT License](LICENSE).

Contributing
------------

Contributions are welcome! Please open an issue or submit a pull request for any improvements.

Contact
-------

For questions or support, please contact vishnucrajeev9@gmail.com

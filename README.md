# Smart IOT Parking system

A distributed IoT and Computer Vision system that captures vehicle images, extracts license plate numbers using Optical Character Recognition (OCR), logs the data to the cloud, and wirelessly displays the results on a remote OLED screen.

## 🔄 System Flowchart

Below is the visual flowchart of how the system operates. GitHub natively renders this `mermaid` diagram when you view the README!

```mermaid
graph TD
    subgraph Capture Node
    A[ESP32-CAM] -->|1. Captures Image every 15s| B(HTTP POST over WiFi)
    end

    subgraph Processing Server
    B -->|2. Image Data| C{Python Flask Server}
    C -->|3. OpenCV| D[Grayscale & Adaptive Thresholding]
    D -->|4. EasyOCR| E[Text Extraction]
    E --> F{Valid Plate? > 4 chars}
    F -- Yes --> G[Clean & Format Text]
    F -- No --> Z[Drop & Wait for Next]
    G -->|5. Push Record| H[(Firebase Realtime Database)]
    G -->|6. Send UDP Packet| I((UDP Port 4210))
    end

    subgraph Display Node
    I -->|7. Wireless Transmission| J[ESP8266 NodeMCU]
    J -->|8. Update Display| K[OLED Screen]
    end

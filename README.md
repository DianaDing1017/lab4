# TECHIN515 - Magic Wand Final Report  
**Author:** Diana Ding  

---

## 1. Introduction  
The Magic Wand project implements a real-time gesture recognition system using an ESP32-C3 microcontroller and an MPU6050 sensor. The wand recognizes three distinct hand gestures and responds with visual LED feedback. This project demonstrates embedded machine learning using Edge Impulse, sensor data processing, and gesture-based user interaction.

---

## 2. Hardware Setup  

**Components Used:**  
- Seeed Studio XIAO ESP32-C3  
- MPU6050 sensor  
- Tactile button module (5-pin rotary encoder, using SW pin)  
- LED (connected to GPIO5 / D3)  
- Li-Po battery (LP603449)  

**Connections:**  

**MPU6050:**  
- VCC → 3.3V  
- GND → GND  
- SDA → GPIO21  
- SCL → GPIO22  

**Button:**  
- SW → GPIO21 (configured as `INPUT_PULLUP`)  
- + → 3.3V  
- GND → GND  

**LED:**  
- Positive → GPIO5  
- Negative → GND  

---

## 3. Data Collection  

Gesture data was collected using a custom Arduino sketch (`gesture_capture.ino`) and a Python script (`process_gesture_data.py`).  

**Data Summary:**  
- Gesture **"O"** (Reflect Shield): 62 samples  
- Gesture **"V"** (Healing Spell): 26 samples  
- Gesture **"Z"** (Fire Bolt): 26 samples  

Each sample was captured at 100Hz for 1 second and saved as a CSV file containing:  timestamp, x, y, z


---

## 4. Model Design in Edge Impulse  

### **Impulse Configuration**  
- Window size: 1000ms  
- Stride: 200ms  
- Frequency: 100Hz  
- Input Axes: x, y, z (acceleration)

### **Processing Block**  
- **Spectral Features**  
  - Effective for capturing temporal patterns and motion bursts  
  - Suitable for short dynamic gestures  

### **Learning Block**  
- **Classifier (Neural Network)**  
  - Input: Spectral features  
  - Output: 3 labels (O, V, Z)  

**Justification:**  
- Spectral features capture frequency-domain characteristics of motion.  
- Neural network is lightweight and deployable on resource-constrained devices.

---

## 5. Model Training & Evaluation  

- Trained with default architecture and learning rate  
- **Test Accuracy:** 94%  
- **Confusion Matrix Insight:**  
  - High confidence for **"O"**  
  - Slight overlap between **"V"** and **"Z"**

### **Deployment**  
- Quantized (int8) model  
- Exported as Arduino library and integrated into `wand.ino`  

---

## 6. Inference System Implementation  

- Gesture capture triggered by button press  
- 2-second capture window to ensure sufficient data  
- Inference run using Edge Impulse SDK  
- LED feedback patterns:  
  - **"O"**: Fast blink 3 times  
  - **"V"**: Slow blink 2 times  
  - **"Z"**: LED steady ON for 1 second  

**Example Serial Output:**  
```
Button pressed, starting capture...
Capture complete.
Prediction: V (86.21%)
```

---

## 7. Challenges and Solutions  

| Challenge                  | Solution                              |
|---------------------------|----------------------------------------|
| Button not detected       | Corrected pin to GPIO21 and used `INPUT_PULLUP` |
| LED not lighting          | Fixed by using correct GPIO pin (GPIO5 / D3) |
| Not enough data captured  | Extended capture window from 1s to 2s   |

---

## 8. Battery
- Battery: LP603449
- 3.7V

---

## 9. Discussion  

To further improve the model’s performance and robustness, the following strategies are suggested:

### 1. Expand and Balance the Dataset  
The current dataset contains 62 samples for "O" but only 26 for "V" and "Z". This imbalance may cause bias toward the overrepresented class. Collecting more samples for the underrepresented gestures would enhance generalization and classification accuracy.

### 2. Optimize DSP Parameters and Add More Features  
Tuning window size and stride could help better capture gesture dynamics, especially for shorter or faster movements. Additionally, incorporating gyroscope data (angular velocity) could help distinguish gestures that have similar acceleration but differ in rotation, improving recognition accuracy.

---

## 10. Link to Access the DEMO VIDEO: https://drive.google.com/file/d/1tpyyebiIyXZeqE7kT5wJY5n7OY8KA5Z0/view?usp=sharing


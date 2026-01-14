# Multi-Sensor Test: BH1750 + DS18B20 + DHT11

**Sensors:**
- BH1750 Digital Light Sensor (I2C)
- DS18B20 Digital Temperature Sensor (1-Wire Dallas)
- DHT11 Temperature & Humidity Sensor (1-Wire DHT protocol)

**Framework:** ESP-IDF (native)
**Purpose:** Test I2C and multiple 1-Wire protocols simultaneously

---

## Hardware Setup

### Wiring Diagram

```
BH1750 Module    Waveshare ESP32-C6-Zero    DS18B20 Probe    DHT11 Module
┌──────────┐                                ┌──────────┐     ┌──────────┐
│  BH1750  │     ┌──────────────┐           │ DS18B20  │     │  DHT11   │
│          │     │              │           │  Probe   │     │          │
│  VCC  ───┼────►│ 3.3V ◄───────┼───────────│ RED      │◄────┤ VCC      │
│  GND  ───┼────►│ GND  ◄───────┼───────────│ BLACK    │◄────┤ GND      │
│  SCL  ───┼────►│ GPIO2 (SCL)  │           │          │     │          │
│  SDA  ───┼────►│ GPIO1 (SDA)  │           │          │     │          │
│  ADDR ───┼─────┤ GND or Float │  [4.7kΩ]  │          │     │          │
│          │     │ GPIO5 ◄──────┴───┬───────│ YELLOW   │     │          │
└──────────┘     │              │   │       │          │     │          │
                 │ 3.3V ─────────────┘       │          │     │          │
                 │ GPIO4 ◄───────────────────────────────────┤ DATA     │
                 └──────────────┘            └──────────┘     └──────────┘
```

**Notes:**
- DS18B20 requires 4.7kΩ pull-up resistor between DATA and 3.3V
- DHT11 typically has built-in pull-up resistor on module (no external resistor needed)

### Pin Connections

**BH1750 (I2C):**

| Pin | ESP32-C6 Pin | Notes |
|-----|--------------|-------|
| VCC | 3.3V | **NOT 5V!** ESP32-C6 is 3.3V only |
| GND | GND | Ground |
| SCL | GPIO2 | I2C Clock (4.7kΩ pull-up usually on module) |
| SDA | GPIO1 | I2C Data (4.7kΩ pull-up usually on module) |
| ADDR | GND or Float | GND = 0x23, Float = 0x5C (default 0x23) |

**DS18B20 (1-Wire Dallas):**

| Wire Color | ESP32-C6 Pin | Notes |
|------------|--------------|-------|
| RED | 3.3V | Power |
| BLACK | GND | Ground |
| YELLOW | GPIO5 | Data (requires 4.7kΩ pull-up to 3.3V) |

**Important:** DS18B20 requires external 4.7kΩ pull-up resistor between DATA and 3.3V!

**DHT11 (1-Wire DHT Protocol):**

| Pin | ESP32-C6 Pin | Notes |
|-----|--------------|-------|
| VCC | 3.3V | Power (can use 3.3V or 5V, using 3.3V for ESP32-C6) |
| GND | GND | Ground |
| DATA | GPIO4 | Data (module usually has built-in pull-up) |

**Note:** DHT11 modules typically include a built-in pull-up resistor. No external resistor needed.

---

## Build and Upload

```bash
# Navigate to test directory
cd tests/multi-sensor-test

# Build firmware
pio run

# Upload to ESP32-C6
pio run --target upload

# Monitor serial output
pio device monitor
```

**Keyboard shortcut to exit monitor:** `Ctrl+C`

---

## Expected Output

```
========================================
Multi-Sensor Test: BH1750 + DS18B20 + DHT11
Waveshare ESP32-C6-Zero
========================================
BH1750: ✓ BH1750 ready (Address: 0x23)
DS18B20: ✓ DS18B20 detected on 1-Wire bus
DHT11: ✓ DHT11 initialized
========================================
Starting measurements (every 2 seconds)...

BH1750: Light:    53.3 lux | Dim Indoor       | #
DS18B20: Temp:   17.50 °C  (63.50 °F)  [Outdoor]
DHT11: Temp:   22.0 °C  (71.6 °F)  [Indoor]
DHT11: Humid:  45.0 %
---
BH1750: Light:    54.2 lux | Dim Indoor       | #
DS18B20: Temp:   17.44 °C  (63.39 °F)  [Outdoor]
DHT11: Temp:   22.0 °C  (71.6 °F)  [Indoor]
DHT11: Humid:  45.0 %
---
```

---

## Testing Procedure

### Test 1: Sensor Detection

**Check:** Does it initialize?
- ✅ `BH1750 initialized successfully`
- ❌ `ERROR: BH1750 not found!` → Check wiring

### Test 2: Normal Room Light

**Expected:** 200-500 lux
**Action:** Place sensor on desk with normal room lighting
**Result:** Should read stable lux value

### Test 3: Cover Sensor

**Expected:** < 1 lux
**Action:** Cover sensor completely with your hand
**Result:** Should drop to near 0 lux (pitch dark)

### Test 4: Flashlight

**Expected:** 1000-10000+ lux
**Action:** Shine flashlight or phone torch directly at sensor
**Result:** Should spike dramatically

### Test 5: Window/Sunlight

**Expected:** 10,000-50,000+ lux
**Action:** Place near window with daylight or direct sun
**Result:** Very high lux reading

---

## Lux Reference Values

| Condition | Typical Lux |
|-----------|-------------|
| Pitch Black | 0-1 lux |
| Moonlight | 0.1-1 lux |
| Dark Room | 1-50 lux |
| Indoor (Dim) | 50-200 lux |
| Indoor (Normal) | 200-500 lux |
| Indoor (Bright) | 500-1000 lux |
| Overcast Day | 1,000-10,000 lux |
| Full Daylight (Shade) | 10,000-25,000 lux |
| Direct Sunlight | 32,000-100,000 lux |

---

## Troubleshooting

### ERROR: BH1750 not found!

**Cause:** I2C communication failure

**Checks:**
1. **Power:** Verify VCC to 3.3V (**not 5V!**)
2. **Ground:** Verify GND to GND
3. **SDA:** Verify GPIO1 connection
4. **SCL:** Verify GPIO2 connection
5. **Pull-ups:** Check 4.7kΩ resistors on SDA/SCL (usually built-in)
6. **Address:** Try ADDR pin to GND (address 0x23)

**Test I2C Bus:**
```cpp
// Add to setup():
Wire.begin(1, 2);  // SDA=GPIO1, SCL=GPIO2
byte error, address;
Serial.println("Scanning I2C bus...");
for(address = 1; address < 127; address++ ) {
  Wire.beginTransmission(address);
  error = Wire.endTransmission();
  if (error == 0) {
    Serial.printf("I2C device found at 0x%02X\n", address);
  }
}
```

**Expected:** Should find device at `0x23` or `0x5C`

---

### Readings Stuck at 0 or 65535

**Cause:** Sensor saturated or misconfigured

**Solutions:**
- **Stuck at 0:** Check if sensor is covered or module defective
- **Stuck at 65535:** Too bright, sensor saturated (direct sun overload)
- **Try different resolution mode:** `BH1750::ONE_TIME_LOW_RES_MODE`

---

### Readings Fluctuate Wildly

**Cause:** Sensor movement or flickering light

**Solutions:**
- Place sensor on stable surface
- Check for fluorescent lights (50/60Hz flicker)
- Average multiple readings:
  ```cpp
  float sum = 0;
  for(int i = 0; i < 10; i++) {
    sum += lightMeter.readLightLevel();
    delay(100);
  }
  float avg_lux = sum / 10.0;
  ```

---

## Temperature Calibration

**Problem:** Temperature sensors may read higher than actual room temperature due to self-heating from ESP32, voltage regulator, and other components.

**Symptoms:**
- Both DS18B20 and DHT11 read similar temperatures (e.g., 23°C)
- But reference thermometer shows lower temperature (e.g., 16.5°C)
- Offset is consistent over time

**Solutions:**

### Option 1: Physical Separation (Recommended)
- **DS18B20:** Extend cable to 1-2 meters away from ESP32
- **DHT11:** Add wire extensions, move 20-30cm from board
- **Benefit:** More accurate readings, no software changes needed

### Option 2: Software Calibration Offsets
Edit `src/main.c` and adjust these values:

```c
// Temperature Calibration Offsets (°C)
#define DS18B20_OFFSET_C    -1    // Adjust based on reference thermometer
#define DHT11_OFFSET_C      -1    // Positive = sensor high, negative = sensor low
```

**How to calibrate:**
1. Place reference thermometer near sensors
2. Wait 30 minutes for thermal equilibrium
3. Calculate offset: `OFFSET = actual_temp - sensor_reading`
4. Update defines in `main.c`
5. Rebuild and upload firmware

**Example:**
- Sensor reads: 23.0°C
- Reference thermometer: 16.5°C
- Offset: 16.5 - 23.0 = **-6.5°C**

---

## Technical Specifications

### BH1750

| Parameter | Value |
|-----------|-------|
| Interface | I2C (TWI) |
| I2C Address | 0x23 (ADDR→GND) or 0x5C (ADDR→Float) |
| Supply Voltage | 3.0V - 5.0V (use 3.3V for ESP32-C6) |
| Current Draw | 0.12 mA (active) |
| Resolution | 1 lux (H-Resolution Mode) |
| Measurement Range | 0-65535 lux |
| Accuracy | ±20% typical |
| Response Time | ~120ms (H-Resolution), ~16ms (L-Resolution) |

### DS18B20

| Parameter | Value |
|-----------|-------|
| Interface | 1-Wire (Dallas) |
| Supply Voltage | 3.0V - 5.5V |
| Current Draw | 1.0 mA (active) |
| Resolution | 9-12 bits (0.0625°C at 12-bit) |
| Measurement Range | -55°C to +125°C |
| Accuracy | ±0.5°C (-10°C to +85°C) |
| Conversion Time | 750ms (12-bit resolution) |

### DHT11

| Parameter | Value |
|-----------|-------|
| Interface | 1-Wire (DHT Protocol) |
| Supply Voltage | 3.0V - 5.5V |
| Current Draw | 0.5-2.5 mA |
| Temperature Range | 0°C to 50°C |
| Temperature Accuracy | ±2°C |
| Humidity Range | 20% to 90% RH |
| Humidity Accuracy | ±5% RH |
| Sampling Period | 1 second minimum |

---

## Next Steps

Once all sensors are working:

1. ✅ **Document calibration offsets** for your specific setup
2. ✅ **Take baseline readings** in different locations
3. **Move to Zigbee integration** - create production firmware with native ESP-IDF + Zigbee stack
4. **Design PCB** for permanent installation

---

**Test Status:** ✅ All Sensors Passed
- BH1750: 91.7 lux ✅
- DS18B20: 21.38°C (outdoor) ✅
- DHT11: 30.0°C + 32% humidity (indoor) ✅

**Last Updated:** 2026-01-14

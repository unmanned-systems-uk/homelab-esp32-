# Multi-Sensor Test: BH1750 + DS18B20

**Sensors:**
- BH1750 Digital Light Sensor (I2C)
- DS18B20 Digital Temperature Sensor (1-Wire)

**Framework:** ESP-IDF (native)
**Purpose:** Test I2C and 1-Wire sensors simultaneously

---

## Hardware Setup

### Wiring Diagram

```
BH1750 Module          Waveshare ESP32-C6-Zero    DS18B20 Probe
┌──────────┐                                      ┌──────────┐
│  BH1750  │           ┌──────────────┐           │ DS18B20  │
│          │           │              │           │  Probe   │
│  VCC  ───┼──────────►│ 3.3V ◄───────┼───────────│ RED      │
│  GND  ───┼──────────►│ GND  ◄───────┼───────────│ BLACK    │
│  SCL  ───┼──────────►│ GPIO2 (SCL)  │           │          │
│  SDA  ───┼──────────►│ GPIO1 (SDA)  │           │          │
│  ADDR ───┼───────────┤ GND or Float │  [4.7kΩ]  │          │
│          │           │ GPIO5 ◄──────┴───┬───────│ YELLOW   │
└──────────┘           │              │   │       │          │
                       │ 3.3V ─────────────┘       │          │
                       └──────────────┘            └──────────┘
```

**Note:** DS18B20 requires 4.7kΩ pull-up resistor between DATA and 3.3V

### Pin Connections

**BH1750 (I2C):**

| Pin | ESP32-C6 Pin | Notes |
|-----|--------------|-------|
| VCC | 3.3V | **NOT 5V!** ESP32-C6 is 3.3V only |
| GND | GND | Ground |
| SCL | GPIO2 | I2C Clock (4.7kΩ pull-up usually on module) |
| SDA | GPIO1 | I2C Data (4.7kΩ pull-up usually on module) |
| ADDR | GND or Float | GND = 0x23, Float = 0x5C (default 0x23) |

**DS18B20 (1-Wire):**

| Wire Color | ESP32-C6 Pin | Notes |
|------------|--------------|-------|
| RED | 3.3V | Power |
| BLACK | GND | Ground |
| YELLOW | GPIO5 | Data (requires 4.7kΩ pull-up to 3.3V) |

**Important:** DS18B20 requires external 4.7kΩ pull-up resistor between DATA and 3.3V!

---

## Build and Upload

```bash
# Navigate to test directory
cd tests/bh1750-test

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
=================================
BH1750 Illuminance Sensor Test
=================================
BH1750 initialized successfully
=================================

Light: 245.3 lux (Normal Indoor)
Light: 248.1 lux (Normal Indoor)
Light: 892.7 lux (Bright Indoor)
Light: 15234.5 lux (Direct Sunlight)
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

## Next Steps

Once BH1750 is working:

1. **Take photos/videos** of sensor responding to light changes
2. **Record lux values** for different locations in your home
3. **Move to next test:** `ds18b20-test/` (outdoor temperature)

---

## Technical Specifications

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

---

## Library Documentation

**PlatformIO Library:** `claws/BH1750@^1.3.0`

**Modes:**
- `CONTINUOUS_HIGH_RES_MODE` - Continuous 1 lux resolution (default)
- `CONTINUOUS_HIGH_RES_MODE_2` - Continuous 0.5 lux resolution
- `CONTINUOUS_LOW_RES_MODE` - Continuous 4 lux resolution (faster)
- `ONE_TIME_HIGH_RES_MODE` - One-shot high resolution
- `ONE_TIME_LOW_RES_MODE` - One-shot low resolution

**GitHub:** https://github.com/claws/BH1750

---

**Test Status:** ⏳ Pending
**Last Updated:** 2026-01-14

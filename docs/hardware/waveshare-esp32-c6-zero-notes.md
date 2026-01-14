# Waveshare ESP32-C6-Zero vs ESP32-C6-DevKitC-1

**User's Hardware:** Waveshare ESP32-C6-Zero (confirmed)

---

## Processor Comparison

✅ **SAME PROCESSOR**: Both use ESP32-C6

| Specification | ESP32-C6 (Both Boards) |
|--------------|------------------------|
| CPU | RISC-V 32-bit @ 160MHz |
| RAM | 512 KB SRAM |
| Flash | 4 MB (external) |
| Zigbee | 3.0 (IEEE 802.15.4) |
| WiFi | 802.11 b/g/n (2.4 GHz) |
| Bluetooth | BLE 5.0 |
| GPIO | 30 pins (not all exposed) |

**Conclusion:** Software and functionality are **100% identical** - same Zigbee stack, same libraries, same capabilities.

---

## Board Differences (Hardware Only)

| Feature | ESP32-C6-DevKitC-1 | Waveshare ESP32-C6-Zero |
|---------|-------------------|-------------------------|
| **Form Factor** | Development board (~50mm) | Tiny module (~25mm) |
| **Size** | Large with headers | Thumb-sized |
| **USB Connector** | USB-C | USB-C (same) |
| **Exposed Pins** | ~30 GPIO on headers | ~22 GPIO (castellated edges) |
| **Antenna** | PCB antenna + U.FL connector | PCB antenna only |
| **Boot Button** | Yes (GPIO9) | Yes (BOOT) |
| **Reset Button** | Yes | Yes |
| **LED** | RGB LED (WS2812) | Single LED (GPIO15) |
| **Power Regulator** | 3.3V LDO | 3.3V LDO (same) |
| **Breadboard Friendly** | Yes (headers) | No (castellated/SMT) |
| **Pin Pitch** | 2.54mm (standard) | 1.27mm (castellated) |

---

## Key Implications for This Project

### ✅ Advantages of ESP32-C6-Zero

1. **Smaller size** - Better for compact enclosure
2. **Lower cost** - Cheaper per unit for production
3. **Professional look** - Cleaner final product
4. **Same capabilities** - All sensors and Zigbee work identically

### ⚠️ Challenges of ESP32-C6-Zero

1. **Soldering required** - Castellated edges need soldering to breakout board or PCB
2. **Fewer exposed pins** - But we only need ~10 pins (plenty available)
3. **No breadboard prototyping** - Need to solder to adapter board first
4. **Harder to reflash** - No easy USB access if mounted

---

## Pin Mapping for Waveshare ESP32-C6-Zero

### Available GPIO Pins (Waveshare Specific)

The Waveshare ESP32-C6-Zero exposes these GPIO pins via castellated edges:

| GPIO | Function | Available | Notes |
|------|----------|-----------|-------|
| GPIO0 | Strapping | ✅ | Boot button (hold LOW to enter bootloader) |
| GPIO1 | I2C SDA | ✅ | Use for BH1750 |
| GPIO2 | I2C SCL | ✅ | Use for BH1750 |
| GPIO3 | UART0 RX | ⚠️ | Used by USB serial (avoid) |
| GPIO4 | GPIO | ✅ | Use for DHT11 |
| GPIO5 | GPIO | ✅ | Available |
| GPIO6 | GPIO | ✅ | Available |
| GPIO7 | GPIO | ✅ | Available |
| GPIO8 | Strapping | ⚠️ | Boot mode (avoid) |
| GPIO9 | Strapping | ⚠️ | Boot mode (avoid) |
| GPIO10 | GPIO | ✅ | Available |
| GPIO11 | GPIO | ✅ | Available |
| GPIO12 | SPI MISO | ✅ | Available |
| GPIO13 | SPI MOSI | ✅ | Available |
| GPIO14 | SPI CLK | ✅ | Available |
| GPIO15 | Built-in LED | ⚠️ | Status LED (already connected) |
| GPIO16 | UART1 RX | ✅ | Use for LD2450 RX |
| GPIO17 | UART1 TX | ✅ | Use for LD2450 TX |
| GPIO18 | USB D- | ❌ | Reserved for USB |
| GPIO19 | USB D+ | ❌ | Reserved for USB |
| GPIO20 | UART0 TX | ⚠️ | Used by USB serial (avoid) |
| GPIO21 | UART0 RX | ⚠️ | Used by USB serial (avoid) |
| GPIO22 | GPIO | ✅ | Available |
| GPIO23 | GPIO | ✅ | Available |

---

## Updated Pin Allocation for This Project

### Recommended GPIO Assignment (Waveshare ESP32-C6-Zero)

| GPIO | Function | Sensor/Peripheral | Notes |
|------|----------|-------------------|-------|
| **GPIO1** | I2C SDA | BH1750 Illuminance | Pull-up required (4.7kΩ) |
| **GPIO2** | I2C SCL | BH1750 Illuminance | Pull-up required (4.7kΩ) |
| **GPIO4** | DHT11 Data | DHT11 Temp/Humidity | 10kΩ pull-up (usually on module) |
| **GPIO16** | UART RX | HLK-LD2450 TX | 115200 baud |
| **GPIO17** | UART TX | HLK-LD2450 RX | 115200 baud |
| **GPIO15** | LED Status | Built-in LED | Active HIGH (pre-wired on board) |
| **GPIO0** | Boot Button | Factory Reset | Hold 10s for reset |

**Pins Used:** 6 GPIOs (plenty remaining for future expansion)

**Pins Available:** GPIO5, 6, 7, 10, 11, 12, 13, 14, 22, 23 (10+ pins free)

---

## PlatformIO Configuration Update

### platformio.ini for Waveshare ESP32-C6-Zero

```ini
[env:waveshare-esp32-c6-zero]
platform = espressif32
board = esp32-c6-devkitc-1   ; Use DevKitC-1 profile (same chip)
framework = arduino
monitor_speed = 115200

; Board-specific settings
board_build.mcu = esp32c6
board_build.f_cpu = 160000000L
board_build.f_flash = 80000000L
board_build.flash_mode = qio

; USB CDC for serial communication
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DWAVESHARE_ESP32_C6_ZERO=1
    -DLED_BUILTIN=15

; Libraries
lib_deps =
    adafruit/DHT sensor library@^1.4.4
    claws/BH1750@^1.3.0

; Upload settings
upload_speed = 921600
upload_protocol = esptool
```

**Note:** We use `board = esp32-c6-devkitc-1` because PlatformIO doesn't have a specific Waveshare profile yet, but the ESP32-C6 chip is identical.

---

## Development Workflow Changes

### Prototyping Strategy (No Breadboard)

Since the ESP32-C6-Zero has castellated edges (not breadboard-friendly), you have 3 options:

#### Option 1: Breakout Adapter (Recommended for Development)
**What:** Solder ESP32-C6-Zero to a DIP adapter board (2.54mm pitch)
**Pros:** Breadboard-compatible, easy sensor swapping, reusable
**Cons:** Extra $5-10, requires soldering
**Where:** AliExpress/Amazon: "ESP32-C6-Zero breakout board" or generic castellated-to-DIP adapter

#### Option 2: Direct PCB Prototyping
**What:** Design PCB immediately, skip breadboarding
**Pros:** Final form factor from start, professional
**Cons:** Harder to debug, longer iteration cycle
**Good for you:** You're an electronics engineer - PCB design is your strength!

#### Option 3: Dead-Bug Prototyping (Hacky)
**What:** Solder wires directly to castellated pads
**Pros:** No adapter needed
**Cons:** Fragile, messy, hard to change
**Not recommended:** Unless you're desperate

**Recommendation:** **Option 2** (Direct PCB) since you're comfortable with PCB design. This saves time in the long run.

---

## PCB Design Considerations (Waveshare ESP32-C6-Zero)

### Mounting the Module

The Waveshare ESP32-C6-Zero mounts via:
1. **Castellated holes** - Solder paste + reflow (professional)
2. **SMT pads** - Hand-solder with fine-pitch iron (easier)

**Footprint:**
- Module size: 25.4mm × 20.8mm (approx)
- Pin pitch: 1.27mm (0.05")
- Castellated holes: 0.5mm diameter

**KiCad Footprint:** Search for "ESP32-C6-Zero" in footprint library or create custom from Waveshare datasheet.

### PCB Layout Tips

1. **USB Access:** Keep USB-C connector accessible for programming
2. **Antenna Clearance:** 5mm keepout zone around PCB antenna (no copper, no components)
3. **Power Decoupling:** 10µF + 100nF caps near VDD pins
4. **ESD Protection:** TVS diodes on USB data lines
5. **Programming Header:** Add UART header (TX, RX, GND) as backup if USB fails

---

## Testing Without Breakout Board

If you don't have a breakout adapter yet, you can still test:

### USB Serial Test (No Soldering Needed)
1. Plug ESP32-C6-Zero into USB-C cable
2. Upload "Hello World" via USB
3. Verify serial output works

**This confirms:**
- ✅ Board is functional
- ✅ USB serial works
- ✅ PlatformIO can flash firmware

**You CANNOT test sensors yet** without soldering connections.

---

## Advantages for This Project

The Waveshare ESP32-C6-Zero is actually **better** for your multi-sensor project:

### ✅ Professional Production Device
- Compact size fits in small enclosure
- Cleaner PCB layout (no oversized dev board)
- Lower cost per unit (important for multiple devices)

### ✅ Forces Good PCB Design
- Since you need a PCB anyway, start with final design
- Skip breadboard → PCB migration step
- Faster to production

### ✅ Built-in Status LED
- GPIO15 LED already connected
- No external LED needed (saves BOM cost)

---

## Updated Bill of Materials (BOM)

### Hardware Changes for Waveshare ESP32-C6-Zero

| Component | Quantity | Notes |
|-----------|----------|-------|
| **Waveshare ESP32-C6-Zero** | 2 | You already have these |
| **BH1750 Module** | 2 | Ordered (arriving soon) |
| **DHT11 Module** | 2 | Assume you have these |
| **HLK-LD2450** | 1-2 | mmWave radar sensor |
| **Custom PCB** | 1 | Design and fabricate (your strength!) |
| **5V 2A USB-C PSU** | 2 | Quality power supply |

**Optional for Development:**
- Castellated-to-DIP adapter (if you want breadboard testing)
- OR jump straight to PCB design

---

## Development Timeline Adjustment

### Original Timeline (DevKitC-1)
1. Week 1: Breadboard prototyping
2. Week 2-3: Software development
3. Week 4: PCB design
4. Week 5: PCB fabrication & assembly

### Updated Timeline (ESP32-C6-Zero)
1. Week 1: Software development (USB serial testing only)
2. Week 2: PCB design (parallel with sensor integration planning)
3. Week 3: PCB fabrication (5-day turnaround)
4. Week 4: PCB assembly & sensor integration testing
5. Week 5: Software finalization & deployment

**Net change:** Similar timeline, but PCB happens earlier (which suits your electronics background).

---

## Updated Task #1: Install VS Code + PlatformIO

### Modified Success Criteria

**Original:** Upload to ESP32-C6 and read sensors
**Updated:** Upload to ESP32-C6-Zero via USB, verify serial output (sensors come later on PCB)

### Test Firmware for Waveshare ESP32-C6-Zero

```cpp
// Test firmware - blink built-in LED on GPIO15
#define LED_BUILTIN 15

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println("\n\n=================================");
  Serial.println("Waveshare ESP32-C6-Zero Test");
  Serial.println("=================================");
  Serial.printf("CPU Frequency: %d MHz\n", ESP.getCpuFreqMHz());
  Serial.printf("Flash Size: %d MB\n", ESP.getFlashChipSize() / 1024 / 1024);
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.println("=================================\n");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  Serial.println("LED ON");
  delay(1000);

  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("LED OFF");
  delay(1000);
}
```

**Expected Output:**
- Serial monitor shows ESP32-C6 info
- Built-in LED blinks (you can see it on the module)

---

## CCPM Task Updates Needed

The following tasks need minor updates for Waveshare board:

1. **[ESP32-P1] Install VS Code + PlatformIO** - Change board to `esp32-c6-devkitc-1` in config (works for both)
2. **[ESP32-P2] Sensor integration tasks** - Note: Requires PCB or breakout adapter
3. **[ESP32-P9] PCB design** - **PRIORITY INCREASED** - This becomes essential, not optional

**No other software tasks change** - same processor, same code!

---

## Recommendation: Development Approach

Given that you:
1. ✅ Are an electronics engineer (PCB design is your strength)
2. ✅ Have Waveshare ESP32-C6-Zero (requires soldering anyway)
3. ✅ Want to build multiple devices (PCB is necessary)

**Recommended Approach:**

### Phase 1A: Software Setup (This Week)
- Install VS Code + PlatformIO
- Upload blink test to ESP32-C6-Zero via USB
- Verify USB serial works
- Read ESP-IDF docs, start learning framework

### Phase 1B: PCB Design (This Week - Parallel)
- Design PCB with ESP32-C6-Zero footprint
- Add sensor connections (I2C, UART, DHT11)
- Add USB-C connector, power regulation
- Send to fabrication (JLCPCB 5-day turnaround)

### Phase 2: PCB Assembly (Week 2)
- Receive PCBs
- Assemble (solder ESP32-C6-Zero, connectors, sensors)
- Test hardware (continuity, power)

### Phase 3: Software Integration (Week 3+)
- Now you can test sensors on real hardware
- Continue with software tasks as planned

**This approach plays to your strengths** (electronics) and avoids breadboard workarounds.

---

## Questions?

1. **Do you have a castellated-to-DIP adapter?** If not, want to order one for quick testing?
2. **Prefer to jump to PCB design now?** (I can help with schematic/layout review)
3. **Need Waveshare datasheet/pinout?** I can fetch the official docs

Let me know how you want to proceed!

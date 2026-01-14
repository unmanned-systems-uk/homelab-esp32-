# ESP32 Zigbee Development Skill

## Overview

This skill provides expert guidance for ESP32-C6 Zigbee device development using:
- **PlatformIO** build system
- **ESP-IDF** native C framework
- **ESP-Zigbee-SDK** 1.0.9+ APIs
- **Home Assistant ZHA** integration

## Purpose

Capture hard-won knowledge from real-world ESP32-C6 Zigbee multi-sensor development, including:
- Critical project structure requirements (src/ vs main/)
- ESP-Zigbee-SDK 1.0.9 API breaking changes
- Component dependency management
- Build system troubleshooting
- Zigbee network pairing procedures
- Multi-sensor integration patterns

## When to Use This Skill

Use this skill when:
- Setting up new ESP32-C6 Zigbee projects
- Troubleshooting build errors
- Implementing Zigbee sensors for Home Assistant
- Debugging network pairing issues
- Migrating from older ESP-Zigbee-SDK versions
- Integrating multiple sensors (I2C, 1-Wire, DHT)

## How to Load This Skill

### Method 1: Startup Command (Recommended)
```bash
/start-esp32-zigbee-dev
```

### Method 2: Direct Reference
When spawning an agent, include in prompt:
```
Load the ESP32 Zigbee development skill from /home/homelab/homelab-esp32-/.claude/skills/esp32-zigbee-dev/
```

### Method 3: Explicit Tool Call
```
Use the Skill tool with skill="esp32-zigbee-dev"
```

## Skill Contents

### SKILL.md Sections

1. **Critical Knowledge**
   - PlatformIO project structure (src/ not main/)
   - Component dependencies (esp-zigbee-lib + esp-zboss-lib)
   - Zigbee configuration (sdkconfig.defaults)
   - ESP-Zigbee-SDK 1.0.9 API changes
   - FreeRTOS task coordination

2. **Common Build Errors**
   - Component resolution failures
   - Missing header files
   - Project recognition issues
   - Linking errors

3. **Zigbee Network Integration**
   - Home Assistant ZHA pairing
   - Network join troubleshooting
   - Entity configuration

4. **Technical Implementation Patterns**
   - Illuminance logarithmic encoding
   - Attribute reporting modes
   - Location descriptions
   - Multi-sensor integration

5. **Hardware Reference**
   - Waveshare ESP32-C6-Zero specs
   - GPIO assignments
   - Sensor wiring

6. **Quick Reference**
   - Build commands
   - Troubleshooting steps
   - Resource links

## Related Files

| File | Purpose |
|------|---------|
| `SKILL.md` | Main skill knowledge base |
| `README.md` | This file - skill documentation |
| `../../zigbee-multi-sensor/README.md` | Production firmware lessons learned |
| `../../docs/` | ESP32 project documentation |

## Knowledge Source

This skill is derived from real development experience documented in:
- `/home/homelab/homelab-esp32-/zigbee-multi-sensor/README.md`
- Production firmware: `/home/homelab/homelab-esp32-/zigbee-multi-sensor/src/main.c`
- Testing code: `/home/homelab/homelab-esp32-/tests/multi-sensor-test/`

## Maintenance

### Updating This Skill

When you discover new knowledge:
1. Update `SKILL.md` with new information
2. Update version number and "Last Updated" date
3. Commit to homelab-esp32- repo
4. Optionally copy to cc-share for shared agent access

### Version History

- **1.0.0** (2026-01-14) - Initial skill creation
  - PlatformIO structure requirements
  - ESP-Zigbee-SDK 1.0.9 API changes
  - Multi-sensor integration patterns
  - Production deployment lessons

## Future Enhancements

Planned additions:
- HLK-LD2450 mmWave radar integration patterns
- OTA firmware update procedures
- Custom Zigbee cluster development
- Power optimization techniques
- Production hardening best practices

---

**Skill Type:** Project-specific (ESP32 Zigbee)
**Repository:** unmanned-systems-uk/homelab-esp32-
**Last Updated:** 2026-01-14

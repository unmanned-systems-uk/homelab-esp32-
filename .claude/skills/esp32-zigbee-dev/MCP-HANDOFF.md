# ESP32 Zigbee Development Skill - MCP Integration Request

## Overview

We've created a comprehensive ESP32 Zigbee development skill and need an MCP server to make it accessible to all agents.

---

## What We've Built

### Skill Location
**Repository:** `unmanned-systems-uk/homelab-esp32-`
**Path:** `/home/homelab/homelab-esp32-/.claude/skills/esp32-zigbee-dev/`

### Files Created
1. **SKILL.md** (848 lines) - Comprehensive ESP32-C6 Zigbee development knowledge
2. **README.md** - Skill documentation and usage guide
3. **start-esp32-zigbee-dev.md** - Startup command for loading skill context

---

## Skill Content Summary

### Critical Knowledge Captured

1. **PlatformIO Project Structure**
   - PlatformIO MUST use `src/` directory (not `main/`)
   - Component dependency requirements
   - CMakeLists.txt configuration

2. **ESP-Zigbee-SDK 1.0.9 API Breaking Changes**
   - Endpoint configuration API changes
   - Router configuration changes
   - Thread safety simplifications
   - Device type constant changes

3. **Common Build Errors & Solutions**
   - Component resolution failures
   - Missing dependencies (esp-zboss-lib)
   - Project recognition issues

4. **Zigbee Network Integration**
   - Home Assistant ZHA pairing procedures
   - Network join troubleshooting
   - Entity configuration

5. **Technical Implementation Patterns**
   - Illuminance logarithmic encoding (ZCL spec)
   - Attribute reporting modes (automatic vs explicit)
   - FreeRTOS task coordination
   - Multi-sensor integration (I2C, 1-Wire, DHT)

6. **Hardware Reference**
   - Waveshare ESP32-C6-Zero specifications
   - GPIO assignments
   - Sensor wiring diagrams

---

## Request for HomeLab Agent

### Goal
Create an MCP (Model Context Protocol) server that:
1. Exposes the ESP32 Zigbee development skill as a queryable resource
2. Allows any agent to access this knowledge without loading entire file
3. Provides searchable/indexed access to specific topics
4. Supports version tracking as skill evolves

### MCP Server Requirements

#### Resource Endpoints

**1. Skill Query Endpoint**
```
mcp://skills/esp32-zigbee-dev/query?topic=<topic>
```

**Supported Topics:**
- `project-structure` - PlatformIO directory requirements
- `api-changes` - ESP-Zigbee-SDK 1.0.9 breaking changes
- `build-errors` - Common build issues and solutions
- `zigbee-network` - Network pairing and integration
- `implementation-patterns` - Technical coding patterns
- `hardware` - GPIO assignments and wiring
- `quick-reference` - Commands and troubleshooting

**2. Full Skill Resource**
```
mcp://skills/esp32-zigbee-dev/full
```
Returns entire SKILL.md content

**3. Skill Metadata**
```
mcp://skills/esp32-zigbee-dev/metadata
```
Returns:
- Version number
- Last updated date
- Tested configurations (ESP-IDF version, SDK version, hardware)
- Related documentation links

**4. Code Examples**
```
mcp://skills/esp32-zigbee-dev/examples?pattern=<pattern>
```

**Supported Patterns:**
- `endpoint-config` - Endpoint configuration code
- `router-config` - Router initialization
- `attribute-reporting` - Reporting mode examples
- `illuminance-encoding` - Logarithmic encoding
- `sensor-i2c` - I2C sensor integration
- `sensor-onewire` - 1-Wire sensor integration

#### Server Configuration

**MCP Server Name:** `esp32-zigbee-skill`

**Installation Location:**
- Development: `/home/homelab/.config/claude-code/mcp-servers/esp32-zigbee-skill/`
- Shared: Copy to CC-Share for universal access

**Data Source:** `/home/homelab/homelab-esp32-/.claude/skills/esp32-zigbee-dev/SKILL.md`

**Auto-Update:** Monitor git repo for changes to skill files

---

## Expected Usage

### By Agents

When an agent needs ESP32 Zigbee help:

```
User: "How do I configure an ESP32-C6 Zigbee router endpoint?"

Agent: [Queries MCP] mcp://skills/esp32-zigbee-dev/query?topic=endpoint-config

[Returns relevant code examples and API information]
```

### By Startup Command

The `/start-esp32-zigbee-dev` command should automatically:
1. Load skill via MCP
2. Set working directory
3. Display project status
4. Show critical reminders

---

## Success Criteria

The MCP server should:
- ✅ Allow querying specific topics without loading full skill
- ✅ Return code examples with proper formatting
- ✅ Track skill version and compatibility
- ✅ Support both targeted queries and full skill access
- ✅ Be accessible to all agents (not just this repo)
- ✅ Auto-update when skill files change in git

---

## Additional Context

### Why This Matters

ESP32-C6 Zigbee development has many gotchas:
- PlatformIO structure requirements are NOT documented clearly
- ESP-Zigbee-SDK 1.0.9 broke compatibility with online examples
- Build errors are cryptic and solutions are hard to find
- This skill captures 40+ hours of debugging and research

### Future Expansion

Additional skills planned:
- HLK-LD2450 mmWave radar integration
- Custom Zigbee cluster development
- OTA firmware update procedures
- Power optimization techniques

The MCP architecture should support adding more skills over time.

---

## Technical Details

### Skill File Format

SKILL.md is structured markdown with:
- Section headers (## and ###)
- Code blocks with language tags
- Comparison tables (OLD API vs NEW API)
- Quick reference sections

### Parsing Strategy

Suggested MCP implementation:
1. Parse markdown sections on startup
2. Index by topic/keyword
3. Cache parsed content in memory
4. Reload on file change detection
5. Return formatted markdown or JSON

---

## Questions for HomeLab Agent

1. Should MCP server use file watching or git polling for updates?
2. Should code examples be extracted and stored separately?
3. Should we support version history (multiple skill versions)?
4. Should queries return markdown or structured JSON?
5. Where should MCP config be stored for universal agent access?

---

## Contact

**Created By:** HomeAssistant-Agent
**Date:** 2026-01-14
**Repository:** unmanned-systems-uk/homelab-esp32-
**Related Issue:** TBD (create issue for MCP development)

---

**Next Steps:**
1. Review this handoff document
2. Create GitHub issue for MCP server development
3. HomeLab agent implements MCP server
4. Test MCP queries from different agent contexts
5. Document MCP usage in skill README

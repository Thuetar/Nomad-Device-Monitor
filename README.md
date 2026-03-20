# Nomad Device Monitor

Nomad Device Monitor is an open-source ESP32-based telemetry platform for RV and mobile systems. It currently targets ESP32-C3 and ESP32-S3 boards and provides web diagnostics, OTA support, current and environment telemetry, and a configurable tank monitoring subsystem with persisted JSON configuration.

## Status

This project is currently experimental. Interfaces and config formats are still evolving as the feature services stabilize.

## Features

- ESP32-C3 and ESP32-S3 target support
- Wi-Fi networking and OTA updates
- Web diagnostics dashboard
- Current sensor telemetry
- Temperature and humidity telemetry
- Tank monitor subsystem with persisted JSON config
- Native unit tests for config and service validation

## Supported Hardware

| Target | Status |
|--------|--------|
| ESP32-C3 | Supported |
| ESP32-S3 | Supported |

Board-specific pin mappings are defined under [board_config.h](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/c3__mini/src/targets/board_config.h).

## Getting Started

### Requirements

- [PlatformIO](https://platformio.org/)
- An ESP32-C3 or ESP32-S3 board
- Sensor hardware appropriate for the enabled subsystems

### Build

```bash
pio run
```

### Upload

```bash
pio run -t upload
```

### Serial Monitor

```bash
pio device monitor
```

## Testing

Run the native unit tests with:

```bash
pio test -e native_test
```

Native tests currently cover config persistence, validation, and tank service CRUD behavior. Hardware-specific sensor IO still requires device-level validation.

## Configuration

- Wi-Fi credentials are provided through local project secrets.
- Tank configuration is stored as a JSON blob in `Preferences`.
- Maximum supported tank sensors: `4`.
- Sensor IDs must be unique and use lowercase letters, digits, `_`, or `-`.
- Tank sensor pins must be unique.

## Documentation

- [API Reference](docs/api.md)

## Project Layout

- [c3__mini/src](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/c3__mini/src)
- [c3__mini/lib/system](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/c3__mini/lib/system)
- [c3__mini/lib/tank](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/c3__mini/lib/tank)
- [test](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/test)
- [docs](/Users/justin/Documents/PlatformIO/Projects/Nomad%20Device%20Monitor/docs)

## Contributing

- Keep changes scoped and board-safe.
- Add or update tests when changing config, validation, or service logic.
- Avoid coupling API handlers directly to low-level runtime components when a service layer exists.

## Roadmap

- Expanded tank alerts and fault detection
- Calibration workflow improvements
- Auth for write endpoints
- Additional subsystem config services on the shared JSON blob pattern

## License

This project is open source. Add the repository license file and update this section to reference the exact license name once it is committed.

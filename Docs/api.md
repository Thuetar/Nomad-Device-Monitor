# API Reference

This document describes the current HTTP API exposed by Nomad Device Monitor.

## Base URL

Examples below assume the device is reachable at one of:

- `http://nomad-device.local`
- `http://192.168.1.42`

## Notes

- Tank configuration is stored as a JSON blob in `Preferences`.
- Maximum supported tank sensors: `4`.
- Sensor IDs must be unique.
- Allowed sensor ID characters: lowercase letters, digits, `_`, `-`.
- Tank sensor pins must be unique.
- Calibration requires `empty_value < full_value`.
- Threshold values must be monotonic.
- Write endpoints are currently unauthenticated.

## Read Endpoints

### `GET /api/status`

Returns system-level runtime telemetry.

#### curl

```bash
curl http://nomad-device.local/api/status
```

#### wget

```bash
wget -qO- http://nomad-device.local/api/status
```

#### Example Response

```json
{
  "wifi_connected": true,
  "ip": "192.168.1.42",
  "amp_voltage_v": 0.12,
  "current_a": 1.84,
  "aht_valid": true,
  "temp_c": 21.5,
  "humidity_pct": 38.2,
  "tank_enabled": true,
  "tank_sensor_count": 2,
  "tank_healthy_sensor_count": 2,
  "tank_last_poll_ms": 184223,
  "sample_count": 941,
  "last_sample_ms": 184220,
  "uptime_s": 184,
  "cpu_mhz": 160,
  "main_loop_busy_pct": 3.4
}
```

### `GET /api/tank`

Returns live tank runtime state.

#### curl

```bash
curl http://nomad-device.local/api/tank
```

#### wget

```bash
wget -qO- http://nomad-device.local/api/tank
```

#### Example Response

```json
{
  "enabled": true,
  "version": 1,
  "poll_interval_ms": 1000,
  "units": "percent",
  "last_poll_ms": 184223,
  "sensor_count": 2,
  "healthy_sensor_count": 2,
  "config_loaded": true,
  "config_degraded": false,
  "sensors": [
    {
      "id": "fresh_1",
      "type": "fresh",
      "custom_name": "Fresh Tank",
      "enabled": true,
      "pin": 10,
      "status": "ok",
      "reading": {
        "last_read_ms": 184223,
        "raw": 2451,
        "filtered": 62.34,
        "percent": 62.34,
        "volume_gallons": 24.94,
        "valid": true
      },
      "history": {
        "lowest_percent": 61.8,
        "highest_percent": 63.1,
        "samples_taken": 941,
        "read_error_count": 0
      }
    },
    {
      "id": "gray_1",
      "type": "gray",
      "custom_name": "Gray Tank",
      "enabled": true,
      "pin": 11,
      "status": "ok",
      "reading": {
        "last_read_ms": 184223,
        "raw": 3010,
        "filtered": 87.75,
        "percent": 87.75,
        "volume_gallons": 29.84,
        "valid": true
      },
      "history": {
        "lowest_percent": 84.2,
        "highest_percent": 88.1,
        "samples_taken": 941,
        "read_error_count": 0
      }
    }
  ]
}
```

### `GET /api/tank/config`

Returns the persisted tank configuration.

#### curl

```bash
curl http://nomad-device.local/api/tank/config
```

#### wget

```bash
wget -qO- http://nomad-device.local/api/tank/config
```

#### Example Response

```json
{
  "enabled": true,
  "version": 1,
  "poll_interval_ms": 1000,
  "units": "percent",
  "restore_last_state_on_boot": false,
  "clear_history_on_boot": true,
  "sensors": [
    {
      "id": "fresh_1",
      "type": "fresh",
      "custom_name": "Fresh Tank",
      "enabled": true,
      "pin": 10,
      "capacity_gallons": 40,
      "usable_capacity_gallons": 38,
      "geometry_type": "linear",
      "offset_from_bottom_mm": 0,
      "calibration": {
        "mode": "two_point",
        "empty_value": 1800,
        "full_value": 3200,
        "min_valid": 1500,
        "max_valid": 3400,
        "invert": false
      },
      "filter": {
        "mode": "ema",
        "enabled": true,
        "ema_alpha": 0.25,
        "sample_count": 8,
        "sample_interval_ms": 25,
        "deadband": 1.0,
        "spike_threshold": 10.0
      },
      "thresholds": {
        "empty_percent": 5,
        "low_percent": 15,
        "medium_percent": 50,
        "high_percent": 85,
        "full_percent": 95,
        "overflow_percent": 99
      },
      "faults": {
        "stuck_timeout_ms": 300000,
        "noise_limit": 8.0,
        "max_change_percent_per_min": 60.0,
        "fault_holdoff_ms": 5000
      }
    }
  ]
}
```

## Write Endpoints

### `POST /api/tank/config`

Replaces the full tank configuration.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/config \
  -H 'Content-Type: application/json' \
  --data '{
    "enabled": true,
    "version": 1,
    "poll_interval_ms": 1000,
    "units": "percent",
    "restore_last_state_on_boot": false,
    "clear_history_on_boot": true,
    "sensors": [
      {
        "id": "fresh_1",
        "type": "fresh",
        "custom_name": "Fresh Tank",
        "enabled": true,
        "pin": 10,
        "capacity_gallons": 40,
        "usable_capacity_gallons": 38,
        "geometry_type": "linear",
        "offset_from_bottom_mm": 0,
        "calibration": {
          "mode": "two_point",
          "empty_value": 1800,
          "full_value": 3200,
          "min_valid": 1500,
          "max_valid": 3400,
          "invert": false
        },
        "filter": {
          "mode": "ema",
          "enabled": true,
          "ema_alpha": 0.25,
          "sample_count": 8,
          "sample_interval_ms": 25,
          "deadband": 1.0,
          "spike_threshold": 10.0
        },
        "thresholds": {
          "empty_percent": 5,
          "low_percent": 15,
          "medium_percent": 50,
          "high_percent": 85,
          "full_percent": 95,
          "overflow_percent": 99
        },
        "faults": {
          "stuck_timeout_ms": 300000,
          "noise_limit": 8.0,
          "max_change_percent_per_min": 60.0,
          "fault_holdoff_ms": 5000
        }
      }
    ]
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "enabled": true,
    "version": 1,
    "poll_interval_ms": 1000,
    "units": "percent",
    "restore_last_state_on_boot": false,
    "clear_history_on_boot": true,
    "sensors": [
      {
        "id": "fresh_1",
        "type": "fresh",
        "custom_name": "Fresh Tank",
        "enabled": true,
        "pin": 10,
        "capacity_gallons": 40,
        "usable_capacity_gallons": 38,
        "geometry_type": "linear",
        "offset_from_bottom_mm": 0,
        "calibration": {
          "mode": "two_point",
          "empty_value": 1800,
          "full_value": 3200,
          "min_valid": 1500,
          "max_valid": 3400,
          "invert": false
        },
        "filter": {
          "mode": "ema",
          "enabled": true,
          "ema_alpha": 0.25,
          "sample_count": 8,
          "sample_interval_ms": 25,
          "deadband": 1.0,
          "spike_threshold": 10.0
        },
        "thresholds": {
          "empty_percent": 5,
          "low_percent": 15,
          "medium_percent": 50,
          "high_percent": 85,
          "full_percent": 95,
          "overflow_percent": 99
        },
        "faults": {
          "stuck_timeout_ms": 300000,
          "noise_limit": 8.0,
          "max_change_percent_per_min": 60.0,
          "fault_holdoff_ms": 5000
        }
      }
    ]
  }' \
  http://nomad-device.local/api/tank/config
```

#### Example Response

```json
{
  "ok": true
}
```

### `POST /api/tank/sensors`

Creates a new sensor.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensors \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "gray_1",
    "type": "gray",
    "custom_name": "Gray Tank",
    "enabled": true,
    "pin": 11,
    "capacity_gallons": 36,
    "usable_capacity_gallons": 34,
    "geometry_type": "linear",
    "offset_from_bottom_mm": 0,
    "calibration": {
      "mode": "two_point",
      "empty_value": 1750,
      "full_value": 3150,
      "min_valid": 1500,
      "max_valid": 3400,
      "invert": false
    },
    "filter": {
      "mode": "ema",
      "enabled": true,
      "ema_alpha": 0.25,
      "sample_count": 8,
      "sample_interval_ms": 25,
      "deadband": 1.0,
      "spike_threshold": 10.0
    },
    "thresholds": {
      "empty_percent": 5,
      "low_percent": 15,
      "medium_percent": 50,
      "high_percent": 85,
      "full_percent": 95,
      "overflow_percent": 99
    },
    "faults": {
      "stuck_timeout_ms": 300000,
      "noise_limit": 8.0,
      "max_change_percent_per_min": 60.0,
      "fault_holdoff_ms": 5000
    }
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "gray_1",
    "type": "gray",
    "custom_name": "Gray Tank",
    "enabled": true,
    "pin": 11,
    "capacity_gallons": 36,
    "usable_capacity_gallons": 34,
    "geometry_type": "linear",
    "offset_from_bottom_mm": 0,
    "calibration": {
      "mode": "two_point",
      "empty_value": 1750,
      "full_value": 3150,
      "min_valid": 1500,
      "max_valid": 3400,
      "invert": false
    },
    "filter": {
      "mode": "ema",
      "enabled": true,
      "ema_alpha": 0.25,
      "sample_count": 8,
      "sample_interval_ms": 25,
      "deadband": 1.0,
      "spike_threshold": 10.0
    },
    "thresholds": {
      "empty_percent": 5,
      "low_percent": 15,
      "medium_percent": 50,
      "high_percent": 85,
      "full_percent": 95,
      "overflow_percent": 99
    },
    "faults": {
      "stuck_timeout_ms": 300000,
      "noise_limit": 8.0,
      "max_change_percent_per_min": 60.0,
      "fault_holdoff_ms": 5000
    }
  }' \
  http://nomad-device.local/api/tank/sensors
```

### `POST /api/tank/sensor/update`

Updates an existing sensor by `id`.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensor/update \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "fresh_1",
    "type": "fresh",
    "custom_name": "Fresh Water Tank",
    "enabled": true,
    "pin": 10,
    "capacity_gallons": 42,
    "usable_capacity_gallons": 39
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "fresh_1",
    "type": "fresh",
    "custom_name": "Fresh Water Tank",
    "enabled": true,
    "pin": 10,
    "capacity_gallons": 42,
    "usable_capacity_gallons": 39
  }' \
  http://nomad-device.local/api/tank/sensor/update
```

### `POST /api/tank/sensor/enabled`

Enables or disables a sensor.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensor/enabled \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "gray_1",
    "enabled": false
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "gray_1",
    "enabled": false
  }' \
  http://nomad-device.local/api/tank/sensor/enabled
```

### `POST /api/tank/sensor/delete`

Deletes a sensor by `id`.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensor/delete \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "gray_1"
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "gray_1"
  }' \
  http://nomad-device.local/api/tank/sensor/delete
```

### `POST /api/tank/sensor/calibration`

Updates calibration values for a sensor.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensor/calibration \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "fresh_1",
    "mode": "two_point",
    "empty_value": 1820,
    "full_value": 3190,
    "min_valid": 1500,
    "max_valid": 3400,
    "invert": false
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "fresh_1",
    "mode": "two_point",
    "empty_value": 1820,
    "full_value": 3190,
    "min_valid": 1500,
    "max_valid": 3400,
    "invert": false
  }' \
  http://nomad-device.local/api/tank/sensor/calibration
```

### `POST /api/tank/sensor/clear-history`

Clears history for a single sensor.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/sensor/clear-history \
  -H 'Content-Type: application/json' \
  --data '{
    "id": "fresh_1"
  }'
```

#### wget

```bash
wget -qO- \
  --header='Content-Type: application/json' \
  --post-data='{
    "id": "fresh_1"
  }' \
  http://nomad-device.local/api/tank/sensor/clear-history
```

### `POST /api/tank/clear-history`

Clears history for all tank sensors.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/clear-history
```

#### wget

```bash
wget -qO- \
  --method=POST \
  http://nomad-device.local/api/tank/clear-history
```

### `POST /api/tank/force-read`

Forces an immediate tank sample cycle.

#### curl

```bash
curl -X POST http://nomad-device.local/api/tank/force-read
```

#### wget

```bash
wget -qO- \
  --method=POST \
  http://nomad-device.local/api/tank/force-read
```

## Error Responses

Validation and parse failures return `400` with a body similar to:

```json
{
  "ok": false,
  "message": "duplicate sensor pin"
}
```

## Validation Rules

- `sensors.length <= 4`
- sensor `id` is required
- sensor `id` must be lowercase alphanumeric, `_`, or `-`
- sensor `pin` values must be unique
- sensor `type` must be `fresh`, `gray`, `black`, or `custom`
- `capacity_gallons >= 0`
- `usable_capacity_gallons >= 0`
- `empty_value < full_value`
- `min_valid <= empty_value`
- `full_value <= max_valid`
- `filter.mode` currently supports `ema`
- `ema_alpha` must be in `[0, 1]`
- thresholds must be monotonic

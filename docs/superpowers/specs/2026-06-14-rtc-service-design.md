# DS3231 RTC Service Design

## Goal
Implement a service to interface with the DS3231 Real-Time Clock module over I2C to retrieve current time (hour, minute, second).

## Architecture
The service will utilize the centralized `i2c_manager` to communicate with the DS3231. It will provide an initialization function and a function to read the time.

## Components
- `rtc_service.h`: Public interface defining the `rtc_time_t` struct and service functions.
- `rtc_service.c`: Implementation using ESP-IDF I2C master driver via `i2c_manager`.

## Data Flow
1. `rtc_service_init()` adds the DS3231 device to the I2C bus.
2. `rtc_get_time()` performs an I2C transmit-receive operation to read 3 bytes from register 0x00 (seconds, minutes, hours).
3. Data is converted from BCD to decimal and returned in a `rtc_time_t` struct.

## Error Handling
Returns `esp_err_t` from I2C operations.

## Testing
A simple unit test or a manual verification in `main.c` (if requested) can be used. For now, I'll add a test file `main/test_rtc_service.c` to verify the logic if possible, or just follow the user's exact steps.
Given the user's specific steps, I will stick to them but ensure validation.

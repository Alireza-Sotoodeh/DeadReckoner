# To Do list - DeadReckoner

update: 2026-06-12

---

## improving data logger

- [ ] SD card  `Runtime Failure` (after boot)
  
  - SD Dynamic Recovery

- [ ]  MPU9250 Dynamic Recovery
  
  - should have create a file name which stand out and let the user know the MPU problem latter!
  
  - [ ] `mpu_critical_error` during calibration

- [ ] boot menu update

- [ ] live view update menu (make it better)
  
  - [ ] dose i have to be worry about battery icon?

- [ ] safe shut down
  
  - Safe Shutdown (software solution)

- [ ] Queue Overflow
  
  - use `frame_sequence_number instead` of `timestamp`  in`LogFrame`

- [ ] GPS! (should we consider adding it right now before main tests?)
  
  - [ ]  remove 100HZ writing and fix it for 1HZ (memory usage issue)

- [ ]  do we need **watchdog**?

- [ ] **BMP280** for Z-Axis deficit

- [ ] change GPIO 9 and GPIO 10 due to being `Internal Flash/PSRAM pins` or `Strapping Pins`

---

## improving Software

- [ ] find a algorithm for making data logger work (find all options)

```
- Machine Learning
- LSTM
- ZUPT
- SHS
```

---

## Hardware challenges

- [ ] Brownout due to high SD card current flow
  
  - 10µF or 47µF parallel to 0.1µF

- [ ]  pull up resistor for i2c especially for MPU9250
  
  - Pull-up 4.7k

- [ ]  for denouncing key add a 0.1µF parallel to the key



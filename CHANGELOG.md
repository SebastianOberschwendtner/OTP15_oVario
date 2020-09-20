# Changelog for oVario
## [V0.2](https://github.com/knuffel-v2/OTP15_oVario/releases/tag/v0.2) *(2020-09-13)*

>Released by `SO`

**Release Notes:**

A lot happened in the mean time. This is the first release of the software with the new **scheduler** and **arbiter**.

The core functionality of the vario works, but the *igc-logging* and *bms_task()* are not active yet.


**Implemented Enhancements:**

- Introduced an **arbiter** to properly handle the commands within one task. See the `README`for more information about that.
- Improved the use of the **IPC Queues**. Now tasks use them to call other tasks and wait until other tasks are finished. See the `README`for more information about that.
- The **scheduler** of the `main.c` now is interrupt based. &rightarrow; The timing is now consistent for each *task group*, but there can be a small jitter for "fast" tasks. **&rightarrow; `Closes #3`**
- The **sdio_task()** is completely non-blocking. **&rightarrow; `Closes #1`**
- The **igc_task()** is completely non-blocking and waits for commands before doing anything.
- The **md5_task()** is completely non-blocking and computes the hash more efficiently.
- The **i2c_task()** is completely non-blocking and running in the background. It can serve many different callers and executes each call step-by-step. **&rightarrow; `Closes #2`**
- The **ms5611_task()** is completely non-blocking and uses the new *i2c*-driver.

**Known Issues:**

- *#4* :black_square_button: [BMS Task for Arbiter](https://github.com/knuffel-v2/OTP15_oVario/issues/4):

    *bms_task()* not functional yet.

- *#5* :black_square_button: [IGC not Logging](https://github.com/knuffel-v2/OTP15_oVario/issues/5):

    *igc_task()* does not log the gps data yet.

- *#6* :black_square_button: [Faulty Sensors with I2C](https://github.com/knuffel-v2/OTP15_oVario/issues/6):

    *i2c_task()* does not recognize faulty sensors.

**Fixed Issues:**

- *#1* :white_check_mark:  [SDIO Blocking](https://github.com/knuffel-v2/OTP15_oVario/issues/1)
- *#2* :white_check_mark: [I2C Blocking](https://github.com/knuffel-v2/OTP15_oVario/issues/2)
- *#3* :white_check_mark: [Main Loop Timing](https://github.com/knuffel-v2/OTP15_oVario/issues/1)

## [V0.1](https://github.com/knuffel-v2/OTP15_oVario/releases/tag/v0.1) *(2020-06-26)*

>Released by `SO`

**Release Notes:**

Initial release with a `tag` of the oVario software.

**Implemented Enhancements:**

- Initial Release

**Known Issues:**

- *#1* :black_square_button:  [SDIO Blocking](https://github.com/knuffel-v2/OTP15_oVario/issues/1):

    SDIO access blocks the whole system.

- *#2* :black_square_button: [I2C Blocking](https://github.com/knuffel-v2/OTP15_oVario/issues/2):

    I2C communication waits for a transfer to be finished.

- *#3* :black_square_button: [Main Loop Timing](https://github.com/knuffel-v2/OTP15_oVario/issues/3):

    The overall timing of the main.c is not optimal.

**Fixed Issues:**

- n/a
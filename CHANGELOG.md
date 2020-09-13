# Changelog for oVario
## [V0.2](https://github.com/knuffel-v2/OTP15_oVario/releases/tag/v0.2) *(2020-09-13)*

>Released by `SO`

**Release Notes:**

A lot happened in the mean time. This is the first release of the software with the new **scheduler** and **arbiter**.

The core functionality of the vario works, but the *igc-logging* and *bms_task()* are not active yet.


**Implemented Enhancements:**

- Introduced an **arbiter** to properly handle the commands within one task. See the `README`for more information about that.
- Improved the use of the **IPC Queues**. Now tasks use them to call other tasks and wait until other tasks are finished. See the `README`for more information about that.
- The **scheduler** of the `main.c` now is interrupt based. &rightarrow; The timing is now consistent for each *task group*, but there can be a small jitter for "fast" tasks. **&rightarrow; `Closes OVI-003`**
- The **sdio_task()** is completely non-blocking. **&rightarrow; `Closes OVI-001`**
- The **igc_task()** is completely non-blocking and waits for commands before doing anything.
- The **md5_task()** is completely non-blocking and computes the hash more efficiently.
- The **i2c_task()** is completely non-blocking and running in the background. It can serve many different callers and executes each call step-by-step. **&rightarrow; `Closes OVI-002`**
- The **ms5611_task()** is completely non-blocking and uses the new *i2c*-driver.

**Known Issues:**

- :white_check_mark: **OVI-001**:

    SDIO access blocks the whole system.

- :white_check_mark:  **OVI-002**:

    I2C communication waits for a transfer to be finished.

- :white_check_mark: **OVI-003**:

    The overall timing of the main.c is not optimal.

- :black_square_button: **OVI-004**:

    *bms_task()* not functional yet.

- :black_square_button: **OVI-005**:

    *igc_task()* does not log the gps data yet.

**Fixed bugs:**

- n/a

## [V0.1](https://github.com/knuffel-v2/OTP15_oVario/releases/tag/v0.1) *(2020-06-26)*

>Released by `SO`

**Release Notes:**

Initial release with a `tag` of the oVario software.

**Implemented Enhancements:**

- Initial Release

**Known Issues:**

- :black_square_button: **OVI-001**:

    SDIO access blocks the whole system.

- :black_square_button: **OVI-002**:

    I2C communication waits for a transfer to be finished.

- :black_square_button: **OVI-003**:

    The overall timing of the main.c is not optimal.

**Fixed bugs:**

- n/a
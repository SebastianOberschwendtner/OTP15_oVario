# Changelog for Iron Bird Lab

## [V0.1](https://gitlab.lrz.de/ga76lut/IronBird_Lab/-/tags/v0.1) *(2020-07-14)*

>Released by `SO`

**Release Notes:**

Initial release of the `Static` version of the Iron Bird Software. This release is intended as a starting point for the further development and cannot yet measure dynamic behaviour. The functionality will be increased in future releases.

**Implemented enhancements:**

- Initial Release

**Known Issues:**

- [IBI-001](https://gitlab.lrz.de/ga76lut/IronBird_Lab/-/issues/1):  When the state machine goes to the error state after a limit error occurred, the state machine can only be re-initialized when the reason for the limit errors is resolved before re-initializing the Iron Bird. Otherwise the error cannot be cleared. (Only by the debug control)

- [IBI-002](https://gitlab.lrz.de/ga76lut/IronBird_Lab/-/issues/2): The data acquisition rate is not high enough the catch transient behaviour.

- [IBI-003](https://gitlab.lrz.de/ga76lut/IronBird_Lab/-/issues/3): The data acquisition is noisy and needs some additional filtering, maybe directly via the FPGA.

- [IBI-004](https://gitlab.lrz.de/ga76lut/IronBird_Lab/-/issues/4): The Iron Bird cannot measure or produce dynamic behaviour.

**Fixed bugs:**

- n/a
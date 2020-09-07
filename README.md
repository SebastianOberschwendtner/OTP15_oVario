![oVario Header](.img/header.jpg)

# OTP15_oVario
### Branch: `VSCode`

## Open Issues:
- :white_check_mark: Implement proper task behaviour to *sdio task*. 
- :white_check_mark: Make *scheduler* interrupt-based.
- :black_square_button: Adjust timing of *scheduler*.
- :black_square_button: Improve **IPC** for *I2C* communication.
- :black_square_button: Register and get every **IPC** item sperately.
- :black_square_button: Make the assignment of the loop rates in the *scheduler* more modular with `#define LOOP_RATE`.

## IPC Register Overview

The follwing tables give an overview of which memory and queues are registered for **IPC**.
Memory items additionally have the information where they are called by **IPC**.

### Registered Memory
Memory which is registered but not used is crossed out.
|dID|Type|Registered in|Referenced by|
|---|---|---|---|
|did_SYS|`SYS_T`|*oVario_Framework.c*|*sdio.c* <br/> *gps.c*|
|did_MS5611|`ms5611_T`|*ms5611.c*|*datafusion.c* <br/> *gui.c*|
|~~did_LCD~~|~~`uint32_t`~~|~~*DOGXL240.c*~~||
|did_DATAFUSION|`datafusion_T`|*datafusion.c*|*gui.c* <br/> *vario.c* <br/> *igc.c*|
|did_BMS|`BMS_T`|*bms.c*|*oVario_Framework.h* <br/> *gui.c*|
|did_GPS_DMA|`uint32_t`|*gps.c*||
|did_GPS|`GPS_T`|*gps.c*|*datafusion.c* <br/> *gui.c* <br/> *igc.c*|
|did_SDIO|`SDIO_T`|*sdio.c*|*oVario_Framework.h* <br/> *gui.c* <br/> *igc.c*|
|~~did_DIRFILE~~|~~`FILE_T`~~|~~*sdio.c*~~|  |
|did_FILEHANDLER|`FILE_T`|*sdio.c*| *igc.c* (Pointer address is called once...) |



### Registered Queues

|dID|Elements|Type|Registered in|
|---|:---:|---|---|
|did_SOUND|5|`T_command`| *sound.c*|
|did_BMS|20|`T_command` | *bms.c*|
|did_SDIO|5|`T_command` | *sdio.c*|
|did_GUI|20|`T_command` | *gui.c*|
|did_VARIO|20|`T_command`| *vario.c*|
|did_IGC|5|`T_command`| *igc.c*|
|did_MD5|5|`T_command`|*md5.c*|




# OTP15_oVario

In Properties -> C/C++ General -> Paths and Symbols -> Symbols
Following Symbols have to be defined:
  - ARM_MATH_CM4
  - DEBUG
  - STM32
  - STM32F4
  - STM32F405RGTx
  - STM32F40_41xxx
  - USE_STDPERIPH_DRIVER
    
In Properties -> C/C++ General -> Paths and Symbols -> Inludes
Following Folders have to be defined:
  - /inc
  - /CMSIS/core
  - /CMSIS/device
  - /StdPeriph_Driver/inc
  - /driver
  
In Properties -> C/C++ General -> Paths and Symbols -> Source Location
Following Folders have to be added:
  - /StdPeriph_Driver
  - /driver
  - /igc
  - /inc
  - /src
  - /startup

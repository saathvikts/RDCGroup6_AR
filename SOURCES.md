# Source and attribution

This project starts from the UST Robotics Team RDC 2026 software skeleton:
https://github.com/UST-Robotics-Team/RDC-2026-SW-Skeleton

The original README files, source copyright notices and component licence files
are retained. No new licence is assigned to the upstream code here. No top-level
licence file was present in the supplied skeleton copy.

Controller dependencies were generated with STM32CubeMX 6.18.1 using the
existing Controller.ioc and STM32Cube FW_F1 V1.8.7. The generated HAL and CMSIS
sources retain their supplied licences under Controller/Drivers.

MainBoard is retained as supplied; its missing generated dependencies have not
yet been prepared. The verified build currently covers Controller only.

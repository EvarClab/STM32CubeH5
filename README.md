# STM32CubeH5 MCU Firmware Package

![latest tag](https://img.shields.io/github/v/tag/STMicroelectronics/STM32CubeH5.svg?color=brightgreen)

> [!IMPORTANT]
> This repository has been created using the `git submodule` command. Please refer to the ["How to use"](README.md#how-to-use) section for more details.

## Overview

**STM32Cube** is an STMicroelectronics original initiative to ease developers' life by reducing efforts, time and cost.

**STM32Cube** covers the overall STM32 products portfolio. It includes a comprehensive embedded software platform delivered for each STM32 series.
   * The CMSIS modules (core and device) corresponding to the ARM(tm) core implemented in this STM32 product.
   * The STM32 HAL-LL drivers, an abstraction layer offering a set of APIs ensuring maximized portability across the STM32 portfolio.
   * The BSP drivers of each evaluation, demonstration or nucleo board provided for this STM32 series.
   * A consistent set of middlewares libraries such as ThreadX, FileX, USBX, NetDuoX, OpenBootloader, USBPD, secure_manager_api, mbed-crypto...
   * A full set of software projects (basic examples, applications, and demonstrations) for each board provided for this STM32 series and developed with **three** toolchains
     * IAR Embedded Workbench for ARM (EWARM)
     * RealView Microcontroller Development Kit (MDK-ARM)
     * STM32CubeIDE

The **STM32CubeH5 MCU Package** projects are directly running on the STM32H5 series boards. You can find in each Projects/*Board name* directory a set of software projects (Applications/Demonstration/Examples).

> [!NOTE]
> ## Some middleware libraries and projects are unavailable in this repository
> 
> In this repository, the middleware libraries listed below **along with** [this](Projects/README.md#list-of-unavailable-projects) list of projects (demos, applications, and examples) using them, are **not available** as they (the middleware libraries) are subject to some restrictive license terms requiring the user's approval via a "click thru" procedure.
> * `./Middlewares/ST/STM32_Audio`
> 
> If needed, they can be found inside the full firmware package available on our website `st.com` and downloadable from [here](https://www.st.com/content/st_com/en/products/embedded-software/mcu-mpu-embedded-software/stm32-embedded-software/stm32cube-mcu-mpu-packages/stm32cubeh5.html#get-software). You will be prompted to login or to register in case you have no account.

## Release note

Details about the content of this release are available in the release note [here](https://htmlpreview.github.io/?https://github.com/STMicroelectronics/STM32CubeH5/blob/main/Release_Notes.html).

## How to use

This repository has been created using the `git submodule` command. Please check the instructions below for proper use. Please check also **the notes at the end of this section** for further information.

1. To **clone** this repository along with the linked submodules, option `--recursive` has to be specified as shown below.

```bash
git clone --recursive https://github.com/STMicroelectronics/STM32CubeH5.git
```

2. To get the **latest updates**, in case this repository is **already** on your local machine, issue the following **two** commands (with this repository as the **current working directory**).

```bash
git pull
git submodule update --init --recursive
```

3. To use the **same firmware version** as the one available on [st.com](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html), issue the command below **after** specifying the targeted `vX.Y.Z` version. This should be done **after** the command(s) indicated in instruction (1) or in instruction (2) above have been successfully executed.

```bash
git checkout vX.Y.Z # Specify the targeted vX.Y.Z version
```

4. To **avoid** going through the above instructions and **directly** clone the same firmware version as the one available on [st.com](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html), issue the command below **after** specifying the targeted `vX.Y.Z` version.

```bash
git clone --recursive  --depth 1 --branch vX.Y.Z https://github.com/STMicroelectronics/STM32CubeH5.git
```

> [!NOTE]
> * The latest version of this firmware available on GitHub may be **ahead** of the one available on [st.com](https://www.st.com/en/microcontrollers-microprocessors/stm32-32-bit-arm-cortex-mcus.html) or via [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html). This is due to the **rolling release** process deployed on GitHub. Please refer to [this](https://github.com/STMicroelectronics/STM32Cube_MCU_Overall_Offer/discussions/21) post for more details.
> * Option `--depth 1` specified in instruction (4) above is **not** mandatory. It may be useful to skip downloading all previous commits up to the one corresponding to the targeted version.
> * If GitHub "Download ZIP" option is used instead of the `git clone` command, then the different submodules have to be collected and added **manually**.

## Boards available

  * STM32H5
    * [NUCLEO-H563ZI](https://www.st.com/en/product/nucleo-h563zi.html)
    * [NUCLEO-H503RB](https://www.st.com/en/product/nucleo-h503rb.html)
    * [STM32H573I-DK](https://www.st.com/en/product/stm32h573i-dk.html)

## Troubleshooting

Please refer to the [CONTRIBUTING.md](CONTRIBUTING.md) guide.

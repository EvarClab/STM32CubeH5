## <b>Nx_Network_Basics_wifi Application Description</b>

This application demonstrates WiFi connectivity on MXCHIP EMW3080 module for the STM32H573I-DK board from STMicroelectronics.

- At first the application performs a scan function and displays the list of access points, then it displays the help command list on the console other the USART1.

- End-user can enter any of the proposed commands:

  - ping <hostname> : ping a remote host and display time to perform ping request
  - echo : send 10 x [1000..2000] bytes to an echo server (armMBED@ one) and check that return data are identical to the one sent
  - scan : list available WiFi access points
  - http : download a file from an HTTP server, http <url> (default url is `http://public.st.free.fr/500MO.bin`)


#### <b>Expected success behavior</b>

- Output of the command is displayed over the USART1 console.
- Depending on the command, bit rates or time duration can be displayed.

#### <b>Error behaviors</b>

- An error message is displayed over the USART1 console.

#### <b>Assumptions if any</b>

- The application is using the DHCP to acquire IP address, thus a DHCP server should be reachable by the board in the LAN used to test the application.


#### <b>ThreadX usage hints</b>

- ThreadX uses the Systick as time base, thus it is mandatory that the HAL uses a separate time base through the TIM IPs.
- ThreadX is configured with 1000 ticks/sec by default, this should be taken into account when using delays or timeouts at application. It is always possible to reconfigure it, by updating the "TX_TIMER_TICKS_PER_SECOND" define in the "tx_user.h" file. The update should be reflected in "tx_initialize_low_level.S" file too.
- ThreadX is disabling all interrupts during kernel start-up to avoid any unexpected behavior, therefore all system related calls (HAL, BSP) should be done either at the beginning of the application or inside the thread entry functions.
- ThreadX offers the `tx_application_define()` function, that is automatically called by the tx_kernel_enter() API.
  It is highly recommended to use it to create all applications ThreadX related resources (threads, semaphores, memory pools...) but it should not in any way contain a system API call (HAL or BSP).
- Using dynamic memory allocation requires to apply some changes to the linker file.
  ThreadX needs to pass a pointer to the first free memory location in RAM to the `tx_application_define()` function, using the `first_unused_memory` argument.
  This requires changes in the linker files to expose this memory location.
    - For EWARM add the following section into the .icf file:
     ```
        place in RAM_region    { last section FREE_MEM };
     ```
    - For MDK-ARM:
    either define the RW_IRAM1 region in the ".sct" file
    or modify the line below in `tx_initialize_low_level.S` to match the memory region being used
    ```
        LDR r1, =|Image$$RW_IRAM1$$ZI$$Limit|
    ```
    - For STM32CubeIDE add the following section into the .ld file:
    ```
        ._threadx_heap :
        {
         . = ALIGN(8);
         __RAM_segment_used_end__ = .;
         . = . + 64K;
         . = ALIGN(8);
        } >RAM_D1 AT> RAM_D1
    ```

    The simplest way to provide memory for ThreadX is to define a new section, see ._threadx_heap above.
    In the example above the ThreadX heap size is set to 64KBytes.
    The `._threadx_heap` must be located between the `.bss` and the `._user_heap_stack sections` in the linker script.
    Caution: Make sure that ThreadX does not need more than the provided heap memory (64KBytes in this example).
    Read more in STM32CubeIDE User Guide, chapter: "Linker script".

    - The `tx_initialize_low_level.S` should be also modified to enable the `USE_DYNAMIC_MEMORY_ALLOCATION` compilation flag.

### <b>Keywords</b>

RTOS, Network, ThreadX, NetXDuo, WiFi, Station mode, Ping, Scan, Echo, HTTP, MXCHIP, SPI, STMod+

### <b>Hardware and Software environment</b>

 - To use the EMW3080B MXCHIP Wi-Fi module functionality, 2 software components are required:
   1. The module driver running on the STM32 device
   2. The module firmware running on the EMW3080B Wi-Fi module

 - The STM32H573I-DK Discovery board RevC connects to the EMW3080B MXCHIP Wi-Fi module which contains the firmware V2.1.11; to upgrade your board with the required version V2.3.4, please visit [X-WIFI-EMW3080B](https://www.st.com/en/development-tools/x-wifi-emw3080b.html), using the “EMW3080update_STM32H573I-DK-RevA-MB1400-SPI_V2.3.4” file under the V2.3.4/SPI folder.
   The module driver is available under [/Drivers/BSP/Components/mx_wifi](../../../../../Drivers/BSP/Components/mx_wifi/), and its version is indicated in the [Release_Notes.html](../../../../../Drivers/BSP/Components/mx_wifi/Release_Notes.html) file.

 - This application runs on STM32H573xx devices without security enabled (TZEN=B4).

 - This application has been tested with STMicroelectronics STM32H573I-DK (MB1677-H573I-C01)
   board and can be easily tailored to any other supported device and development board.

 - A daughter board with the WiFi module is to be plugged into the STMod+ connector CN3 of the STM32H573I-DK board.

 - The daughterboard that was used is made up of:
   - A STMicroelectronics STMod+ adapter (MB1400-C01), delivered with the EMW3080B MXCHIP Wi-Fi module firmware V2.1.11

   > Please note that module firmware version V2.1.11 is not backwards compatible with the driver V2.3.4 (the V2.1.11 module firmware is compatible with the driver versions from V2.1.11 to V2.1.13).

   To upgrade your board with the required version V2.3.4, please visit [X-WIFI-EMW3080B](https://www.st.com/en/development-tools/x-wifi-emw3080b.html),
   using the `EMW3080update_STM32H573I-DK-RevA-MB1400-SPI_V2.3.4.bin` file under the V2.3.4/SPI folder.

 - This application requires a WiFi access point to connect to
   - With a transparent Internet connectivity: No proxy, no firewall blocking the outgoing traffic.
   - Running a DHCP server delivering the IP to the board.

 - This application uses USART1 to provide a console for commands, the hyperterminal configuration is as follows:
   - BaudRate = 115200 baud
   - Word Length = 8 Bits
   - Stop Bit = 1
   - Parity = None
   - Flow control = None
   - Line endings set to LF (transmit) and LF (receive).

 - This application requires a WiFi access point to connect to:
   - With a transparent Internet connectivity: no proxy, no firewall blocking the outgoing traffic.
   - Running a DHCP server delivering the IP and DNS configuration to the board.

### <b>How to use it?</b>

In order to make the program work, you must do the following:

 - WARNING: Before opening the project with any toolchain be sure your folder installation path is not too in-depth since the toolchain may report errors after building.

 - Open your preferred toolchain

 - Edit the file `Core/Inc/mx_wifi_conf.h` to enter the name of your WiFi access point (`WIFI_SSID`) to connect to and its password (`WIFI_PASSWORD`).

 - Build all files and load your image into target memory

 - Run the application

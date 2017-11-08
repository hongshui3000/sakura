.. _ti_tm4c123gxl_launchpad:

TM4C123GXL Launchpad
####################

Overview
********
The TM4C123G LaunchPad Evaluation Kit is a low-cost evaluation platform for ARM 
Cortex-M4F based microcontrollers from Texas Instruments. The design of the 
TM4C123G LaunchPad highlights the TM4C123GH6PM microcontroller with a USB 2.0 
device interface and hibernation module.

.. image:: img/TITivaLaunchpad.jpg
  :width:  450px
  :height: 600px
  :align:  center
  :alt:    TI Tiva Launchpad

See the `TI TM4C123GXL Product Page`_ for details.

Features:
=========

* High Performance TM4C123GH6PM MCU:

    + 80MHz 32-bit ARM Cortex-M4-based microcontrollers CPU
    + 256KB Flash, 32KB SRAM, 2KB EEPROM
    + Two Controller Area Network (CAN) modules
    + USB 2.0 Host/Device/OTG + PHY
    + Dual 12-bit 2MSPS ADCs, motion control PWMs
    + 8 UART, 6 I2C, 4 SPI
    
* On-board In-Circuit Debug Interface (ICDI)
* USB Micro-B plug to USB-A plug cable
* Preloaded RGB quick-start application

Details on the TM4C123G LaunchPad development board can be found in the
`TM4C123G LaunchPad User's Guide`_.

Supported Features
==================

Zephyr has been ported to the Applications MCU, with basic peripheral
driver support.

+-----------+------------+-----------------------+
| Interface | Controller | Driver/Component      |
+===========+============+=======================+
| UART      | on-chip    | serial port-interrupt |
+-----------+------------+-----------------------+
| GPIO      | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| LED_RGB   | on-chip    | gpio                  |
+-----------+------------+-----------------------+
| SW        | on-chip    | gpio                  |
+-----------+------------+-----------------------+

Connections and IOs
====================

Peripherals on the TM4C123G LaunchPad are mapped to the following pins in
the file :file:`boards/arm/ti_tm4c123gxl_lauchpad/pinmux.c`.

+------------+-----+------+
| Function   | PIN | GPIO |
+============+=====+======+
| UART0_TX   | --  | PA0  |
+------------+-----+------+
| UART0_RX   | --  | PA1  |
+------------+-----+------+
| LED D7 (R) | --  | PF1  |
+------------+-----+------+
| LED D6 (B) | --  | PF2  |
+------------+-----+------+
| LED D5 (G) | --  | PF3  |
+------------+-----+------+
| Switch SW1 | --  | PF4  |
+------------+-----+------+
| Switch SW2 | --  | PF0  |
+------------+-----+------+

The default configuration can be found in the Kconfig file at
:file:`boards/arm/ti_tm4c123gxl_lauchpad/tm4c123gxl_launchxl_defconfig`.


Programming and Debugging
*************************

Flashing
========

Flashing Command:
-----------------

.. code-block:: console

  $ sudo -E make -C samples/hello_world BOARD=ti_tm4c123gxl_launchpad flash

To see program output from UART0, one can execute in a separate terminal
window:

.. code-block:: console

  % screen /dev/ttyACM0 115200 8N1

Debugging
=========

Debugging Command
-----------------

.. code-block:: console

  $ sudo -E make -C samples/hello_world BOARD=ti_tm4c123gxl_launchpad debug

References
**********

.. _TI TM4C123GXL Product Page:
    http://www.ti.com/tool/ek-tm4c123gxl

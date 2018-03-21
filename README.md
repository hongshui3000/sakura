# Sakura

基于 TI ek-tm4c123gxl launchpad 开发板移植[zephyr](https://www.zephyrproject.org/)操作系统。

实现以下基础驱动

1. 板载功能实现

    + uart
    + gpio
    + spi
    + usb

2. 传感器驱动

    + dw1000
    + mpu9250

## 进度说明

**完成**

1. 串口工作正常（poll 和 中断）
2. GPIO可以使用（RGB，开关按键）

**进行中**

1. SPI驱动的移植
2. DW1000驱动移植

## 联系方式

hackin.zhao@qq.com

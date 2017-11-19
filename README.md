# Sakura
Iot &amp;  High precision indoor positioning

项目基于[zephyr](https://www.zephyrproject.org/)开发，硬件采用DWM1000定位芯片融合MPU92509轴传感器，力争达到动态厘米级定位效果。

## master 分支版本说明

**v1.1.0**

1. 串口工作正常（samples/hello_world例程可用）
2. 未完善工作：
    + 时钟系统写死，没有Kconfigure文件及相应驱动
    + 串口中断API未配置
    + 文档未更新

ifdef CONFIG_HAS_TM4C12X_SDK
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/ek-tm4c12x/source
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/ek-tm4c12x/source/inc
ZEPHYRINCLUDE +=-I$(srctree)/ext/hal/ti/ek-tm4c12x/source/driverlib

ifdef CONFIG_SOC_TM4C123GH6PM
KBUILD_CFLAGS += -DTARGET_IS_TM4C123_RB1
KBUILD_CFLAGS += -DPART_TM4C123GH6PM
endif

endif # CONFIG_HAS_TM4C12X_SDK

add_subdirectory(source/soc)

if(CONFIG_HAS_TIVAWARE_SDK)
  zephyr_include_directories(
    source
    source/driverlib
    source/inc
    )

  zephyr_library()
  zephyr_library_compile_definitions(${COMPILER})
  zephyr_library_sources(
      source/driverlib/gpio.c
      )

endif()

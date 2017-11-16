add_subdirectory(source/soc)

if(CONFIG_HAS_TIVAWARE_SDK)
  zephyr_include_directories(
    source
    source/driverlib
    source/inc
		)

endif()

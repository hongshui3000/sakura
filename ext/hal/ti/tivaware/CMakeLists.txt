
if(CONFIG_HAS_TIVAWARE_SDK)
  zephyr_include_directories(
    source
    )

	if(CONFIG_SOC_TM4C123GH6PM)
	  zephyr_compile_definitions(
	    TARGET_IS_TM4C123_RB1
	    PART_TM4C123GH6PM
	    )
	endif()
endif()

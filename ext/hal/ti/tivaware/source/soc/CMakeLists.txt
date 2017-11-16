if(CONFIG_SOC_SERIES_TM4C123)
  zephyr_include_directories(
    TM4C123
    )

    zephyr_sources(TM4C123/system_TM4C123.c)

  if(CONFIG_SOC_TM4C123GH6PM)
    zephyr_compile_definitions(
      TM4C123GH6PM
      TARGET_IS_TM4C123_RB1
      PART_TM4C123GH6PM
      )
  endif()
endif()

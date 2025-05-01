if (DEFINED BRP2350TOUCH2_PATH)
	message("Using Given BSP_PATH '${BRP2350TOUCH2_PATH}')")
else ()
	set(BRP2350TOUCH2_PATH "${CMAKE_CURRENT_LIST_DIR}lib/")
    message("Using local BSP_PATH '${BRP2350TOUCH2_PATH}')")
endif ()

add_library(rp2350touch2 STATIC)
target_sources(rp2350touch2 PUBLIC
	${BRP2350TOUCH2_PATH}/Config/DEV_Config.c
    ${BRP2350TOUCH2_PATH}/LCD/LCD_2in.c
    ${BRP2350TOUCH2_PATH}/QMI8658/QMI8658.c
    ${BRP2350TOUCH2_PATH}/Touch/CST816D.c
)

# Add include directory
target_include_directories(rp2350touch2 PUBLIC 
    ${BRP2350TOUCH2_PATH}/Config
    ${BRP2350TOUCH2_PATH}/LCD
    ${BRP2350TOUCH2_PATH}/QMI8658
    ${BRP2350TOUCH2_PATH}/Touch
)




target_link_libraries(rp2350touch2 PUBLIC 
    pico_stdlib 
    hardware_spi 
    hardware_i2c 
    hardware_pwm 
    hardware_adc 
    hardware_dma
    hardware_pio
    hardware_irq
    )
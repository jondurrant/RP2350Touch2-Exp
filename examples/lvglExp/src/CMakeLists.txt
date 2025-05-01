
add_executable(${NAME}
        main.cpp
        Widgets.c
        LCD_2in_LVGL_test.c
		LVGL_example.c
        )

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(${NAME} 
	 pico_stdlib
     lvgl
     rp2350touch2
     hardware_clocks 
     hardware_adc 
     hardware_dma 
     hardware_spi
     
	)

   
# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# enable usb output, disable uart output
pico_enable_stdio_usb(${NAME} 1)
pico_enable_stdio_uart(${NAME} 0)

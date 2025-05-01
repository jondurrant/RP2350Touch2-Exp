#include <stdio.h>
#include "pico/stdlib.h"

extern "C"{
#include "LCD_test.h"  //example
#include "Widgets.h"
}





int main(void){
	 stdio_init_all();
    LCD_2in_LVGL_Test();

    return 0;
}


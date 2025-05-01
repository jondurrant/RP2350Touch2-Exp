/*****************************************************************************
* | File      	:   LCD_1in28_LVGL_test.c
* | Author      :   Waveshare team
* | Function    :   1.28inch LCD  test demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2024-06-27
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/

#include "LVGL_example.h" 
#include "src/core/lv_obj.h"
#include "src/misc/lv_area.h"

// LVGL
static lv_disp_drv_t disp_drv;
static lv_disp_draw_buf_t disp_buf;
static lv_color_t buf0[DISP_HOR_RES * DISP_VER_RES/2];
static lv_color_t buf1[DISP_HOR_RES * DISP_VER_RES/2];

static lv_obj_t *tv;
static lv_obj_t *tile1;
static lv_obj_t *tile2;
static lv_obj_t *tile3;

static lv_obj_t *table_imu_data;
static lv_obj_t *roller;
static lv_obj_t *lable;

// Touch
static uint16_t ts_x;
static uint16_t ts_y;
static lv_indev_state_t ts_act;
static uint8_t gesture = 0;
static lv_indev_drv_t indev_ts;

// Timer 
static struct repeating_timer lvgl_timer;
static struct repeating_timer imu_data_update_timer;
static struct repeating_timer bat_data_update_timer;
 
static void disp_flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p);
static void touch_callback(uint gpio, uint32_t events);
static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data);
static void dma_handler(void);
static void scroll_begin_event_cb(lv_event_t * eevent);
static void roller_event_cb(lv_event_t * event);
static bool repeating_lvgl_timer_callback(struct repeating_timer *t); 


/********************************************************************************
function:	Initializes LVGL and enbable timers IRQ and DMA IRQ
parameter:
********************************************************************************/
void LVGL_Init(void)
{
    /*1.Init Timer*/ 
    add_repeating_timer_ms(5,    repeating_lvgl_timer_callback,            NULL, &lvgl_timer);
    
    /*2.Init LVGL core*/
    lv_init();

    /*3.Init LVGL display*/
    lv_disp_draw_buf_init(&disp_buf, buf0, buf1, DISP_HOR_RES * DISP_VER_RES / 2); 
    lv_disp_drv_init(&disp_drv);    
    disp_drv.flush_cb = disp_flush_cb;
    disp_drv.draw_buf = &disp_buf;        
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    if(Scan_dir != VERTICAL)
        disp_drv.rotated = LV_DISP_ROT_90;
    lv_disp_t *disp= lv_disp_drv_register(&disp_drv);   

#if INPUTDEV_TS
    /*4.Init touch screen as input device*/ 
    lv_indev_drv_init(&indev_ts); 
    indev_ts.type = LV_INDEV_TYPE_POINTER;    
    indev_ts.read_cb = ts_read_cb;            
    lv_indev_t * ts_indev = lv_indev_drv_register(&indev_ts);
    //Enable touch IRQ
    DEV_IRQ_SET(Touch_INT_PIN, GPIO_IRQ_EDGE_RISE, &touch_callback);
#endif

    /*5.Init DMA for transmit color data from memory to SPI*/
    dma_channel_set_irq0_enabled(dma_tx, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}





/********************************************************************************
function:	Disable scroll animations when a tab button is clicked in a tabview
parameter:
********************************************************************************/
static void scroll_begin_event_cb(lv_event_t * event)
{
    lv_anim_t * a = lv_event_get_param(event);
    if (a) a->time = 0; 
}

/********************************************************************************
function:	Refresh image by transferring the color data to the SPI bus by DMA
parameter:
********************************************************************************/
static void disp_flush_cb(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
    LCD_2IN_SetWindows(area->x1, area->y1, area->x2+1 , area->y2+1);  // Set the LVGL interface display position
    DEV_Digital_Write(LCD_DC_PIN, 1);
    DEV_Digital_Write(LCD_CS_PIN, 0);
    dma_channel_configure(dma_tx,   
                          &c,
                          &spi_get_hw(SPI_PORT)->dr, 
                          color_p, // read address
                          ((area->x2 + 1 - area-> x1)*(area->y2 + 1 - area -> y1))*2,
                          true);// Start DMA transfer
}

/********************************************************************************
function:   Touch interrupt handler
parameter:
********************************************************************************/
static void touch_callback(uint gpio, uint32_t events)
{
    if (gpio == Touch_INT_PIN)
    {
        CST816D_Get_Point(); // Get coordinate data
        gesture = CST816D_Get_Gesture(); // Get gesture data
        ts_x = Touch_CTS816.x_point;
        ts_y = Touch_CTS816.y_point;
        ts_act = LV_INDEV_STATE_PRESSED;
    }
}

/********************************************************************************
function:   Update touch screen input device status
parameter:
********************************************************************************/
static void ts_read_cb(lv_indev_drv_t * drv, lv_indev_data_t*data)
{
    data->point.x = ts_x;
    data->point.y = ts_y; 
    data->state = ts_act;
    ts_act = LV_INDEV_STATE_RELEASED;
}

/********************************************************************************
function:   Indicate ready with the flushing when DMA complete transmission
parameter:
********************************************************************************/
static void dma_handler(void)
{
    if (dma_channel_get_irq0_status(dma_tx)) 
    {
        dma_channel_acknowledge_irq0(dma_tx);
        DEV_Digital_Write(LCD_CS_PIN, 1);
        lv_disp_flush_ready(&disp_drv); // Indicate you are ready with the flushing
    }
}

/********************************************************************************
function:   Check if the page needs to be updated
parameter:
********************************************************************************/
static bool update_check(lv_obj_t *tv,lv_obj_t *tilex) 
{
    uint8_t ret = true; 

    lv_obj_t *active_tile = lv_tileview_get_tile_act(tv); // Get the current active interface
    if (active_tile != tilex) 
    {
        ret = false;
    }

    if(gesture != CST816D_Gesture_None && gesture != CST816D_Gesture_Click) // The current gesture is not empty and is not a click
    {
        gesture = CST816D_Gesture_None;
        ret = false;
    }

    return ret;
}



/********************************************************************************
function:   Report the elapsed time to LVGL each 5ms
parameter:
********************************************************************************/
static bool repeating_lvgl_timer_callback(struct repeating_timer *t) 
{
    lv_tick_inc(5);
    return true;
}


/********************************************************************************
function:   Change LCD brightness
parameter:
********************************************************************************/
static void roller_event_cb(lv_event_t * event) 
{
    lv_obj_t * obj = lv_event_get_target(event); // Get the object that triggered the event
    int selected_index = lv_roller_get_selected(obj); // Get the index of the currently selected item of the scroll bar
    pwm_set_chan_level(slice_num, PWM_CHAN_B, (selected_index+1)*10); // Set the output level of the PWM channel according to the selected item index
}

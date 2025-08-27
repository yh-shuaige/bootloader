#define LOG_TAG    "main"
#include "main.h"

ElogErrCode elog_port_init(void);

static bool button_trap_boot(void)
{
    if(bl_button_pressed())
    {
        bl_delay_ms(40);
        return bl_button_pressed();
    }
    return false;
}

static void button_wait_release(void)
{
    while (bl_button_pressed())
    {
        bl_delay_ms(20);
    }

}
void bl_button_wait_release(void)
{
    while(bl_button_pressed())
    {
        bl_delay_ms(20);
    }
}
extern void bootloder(uint32_t boot_delay);
extern bool verify_application(void);
int main(void)
{
    //时钟初始化
    bl_lowlevel_init();


    usart1_init();

    elog_init();
    /* set EasyLogger log format */
    elog_set_fmt(ELOG_LVL_ASSERT, ELOG_FMT_ALL);
    elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_DEBUG, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_fmt(ELOG_LVL_VERBOSE, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    /* start EasyLogger */
    elog_start();

    bl_button_init();
    bl_delay_init();
    bl_usart_init();
    bl_led_init();
    led_off();
    bool trap_boot = false;

    if(button_trap_boot())
    {
        log_w("button pressed turn to bootloder!");
        trap_boot = true;
    }
    else if (!verify_application())
    {
        log_w("application verify failed, trap into boot");
        trap_boot = true;
    }
    
    if(trap_boot)
    {
        led_on();
        button_wait_release();
    }

    bootloder(trap_boot ? 0 : 3);

    return 0;
}


void *_sbrk(ptrdiff_t incr) {
    (void)incr;
    return (void*)-1;
}


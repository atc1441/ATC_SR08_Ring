#include "rwip_config.h"
#include "gattc_task.h"
#include "gapc_task.h"
#include "user_periph_setup.h"
#include "wkupct_quadec.h"
#include "app_easy_msg_utils.h"
#include "gpio.h"
#include "app_security.h"
#include "user_proxr.h"
#include "arch.h"
#include "arch_api.h"
#include "lld_evt.h"
#include "app_task.h"
#include "app_proxr.h"

#include "timer0.h"
#include "timer0_2.h"
#if (BLE_SUOTA_RECEIVER)
#include "app_suotar.h"
#endif

#if defined (CFG_SPI_FLASH_ENABLE)
#include "spi_flash.h"
#endif

#include "arch_console.h"

#if defined(__IS_SDK6_COMPILER_GCC__) && !defined(__clang__)
#pragma message("Please note that SDK6 GCC support will be deprecated in the next SDK6 release")
#endif


typedef struct led_gpios
{
        GPIO_PORT port;
        GPIO_PIN pin;
}led_gpios;

static led_gpios theLedGpiosLow[] = {
        {GPIO_PORT_2, GPIO_PIN_1},
        {GPIO_PORT_1, GPIO_PIN_3},
        {GPIO_PORT_2, GPIO_PIN_0},
        {GPIO_PORT_1, GPIO_PIN_2},
        {GPIO_PORT_1, GPIO_PIN_0},
        {GPIO_PORT_1, GPIO_PIN_1},
        {GPIO_PORT_0, GPIO_PIN_2},
};

static led_gpios theLedGpiosHigh[] = {
        {GPIO_PORT_2, GPIO_PIN_8},
        {GPIO_PORT_2, GPIO_PIN_7},
        {GPIO_PORT_2, GPIO_PIN_6},
        {GPIO_PORT_2, GPIO_PIN_5},
        {GPIO_PORT_2, GPIO_PIN_2},
        {GPIO_PORT_2, GPIO_PIN_9},
};


const uint8_t numbers[] = {0x77,0x24,0x5D,0x6D,0x2E,0x6B,0x7B,0x25,0x7F,0x6F,0x3F,0x3A,0x53,0x7C,0x5B,0x1B};

uint8_t LED_Buffer[6] = {0x00,0x00,0x00,0x00,0x00,0x00};

uint32_t counter_time = 0;
uint8_t counter_ms = 0;
uint8_t current_line = 0;
uint32_t realUnix = 0;
uint32_t lastTime = 0;

uint8_t LED_Display_state = 0;
uint32_t turnOnTime = 0;

// Neue Variable für die Menüsteuerung
uint8_t current_menu = 0;
#define MAX_MENUS 3 // Anzahl der verfügbaren Menü-Seiten

void calcTime()
{
           uint32_t currentTime = lld_evt_time_get();
           if(currentTime == 0)
                   return;
           currentTime/=1000;
           if (currentTime < lastTime) {
               realUnix += (UINT32_MAX / 1000) + 1;
           }
           realUnix += (currentTime - lastTime);
           lastTime = currentTime;
}

void LED_GPIO_mode(uint8_t mode)
{
        arch_printf("LED_GPIO_mode: %i\r\n", mode);
        if(mode == 1)
        {
                if(LED_Display_state != 1)
                {
                        LED_Display_state = 1;
                        calcTime();
                        turnOnTime = realUnix;
                        for (int i = 0; i < (sizeof(theLedGpiosLow)/sizeof(led_gpios)); ++i )
                                GPIO_ConfigurePin(theLedGpiosLow[i].port, theLedGpiosLow[i].pin, OUTPUT, PID_GPIO, 0);
                        for (int i = 0; i < (sizeof(theLedGpiosHigh)/sizeof(led_gpios)); ++i )
                                GPIO_ConfigurePin(theLedGpiosHigh[i].port, theLedGpiosHigh[i].pin, OUTPUT, PID_GPIO, 1);
                        // Here we enable the Timer
                        arch_force_active_mode();
                        timer0_enable_irq();
                        timer0_start();
                }
        }else{
                if(LED_Display_state != 0)
                {
                        LED_Display_state = 0;
                        for (int i = 0; i < (sizeof(theLedGpiosLow)/sizeof(led_gpios)); ++i )
                                GPIO_ConfigurePin(theLedGpiosLow[i].port, theLedGpiosLow[i].pin, INPUT_PULLDOWN, PID_GPIO, 0);
                        for (int i = 0; i < (sizeof(theLedGpiosHigh)/sizeof(led_gpios)); ++i )
                                GPIO_ConfigurePin(theLedGpiosHigh[i].port, theLedGpiosHigh[i].pin, INPUT_PULLDOWN, PID_GPIO, 1);
                        // Here we disable the Timer
                        timer0_disable_irq();
                        timer0_stop();
                        arch_restore_sleep_mode();
                }
        }
}

void LED_Buff_setInt(uint32_t inputNum, unsigned char *LED_Buf, int lenInt) {
    uint8_t v19[5] = {0};
    int i;
    uint32_t v4 = 1;
    if (lenInt > 5) lenInt = 5;
    for (i = lenInt - 1; i >= 0; i--) {
        v19[i] = (inputNum / v4) % 10;
        v4 *= 10;
    }
    for (i = 0; i < lenInt; i++) {
        LED_Buf[i] = numbers[v19[i]];
    }
}

void refreshMenu()
{
        calcTime();

        // Switch-Case für verschiedene Menüs
        switch(current_menu)
        {
            case 0: // Menü 1: Systemzeit (Original)
                LED_Buff_setInt(lld_evt_time_get()/100, LED_Buffer, 5);
                break;

            case 1: // Menü 2: Eigener counter_time
                LED_Buff_setInt(counter_time, LED_Buffer, 5);
                break;
                //LED_Buff_setInt(realUnix, LED_Buffer, 5);

            case 2:// __ATC
                    // A:0x01, F:0x02, B:0x04, G:0x08, E:0x10, C:0x20, D:0x40
                    static const uint8_t anim8[] = {
                        0x01, // Segment A
                        0x04, // Segment B
                        0x08, // Segment G
                        0x10, // Segment E
                        0x40, // Segment D
                        0x20, // Segment C
                        0x08, // Segment G
                        0x02  // Segment F
                    };
                    uint8_t anim_index = counter_time % 8;

                    LED_Buffer[0] = anim8[anim_index];
                    LED_Buffer[1] = 0x00;
                    LED_Buffer[2] = 0x3F;
                    LED_Buffer[3] = 0x5A;
                    LED_Buffer[4] = 0x58;
                break;
            case 99:// __OTA
                    LED_Buffer[0] = 0x00;
                    LED_Buffer[1] = 0x00;
                    LED_Buffer[2] = 0x77;
                    LED_Buffer[3] = 0x5A;
                    LED_Buffer[4] = 0x3F;
                break;
            case 100:// _DONE
                    LED_Buffer[0] = 0x00;
                    LED_Buffer[1] = 0x7C;
                    LED_Buffer[2] = 0x78;
                    LED_Buffer[3] = 0x38;
                    LED_Buffer[4] = 0x5B;
                break;
            case 101:// BLE 0
                    LED_Buffer[0] = 0x7A;
                    LED_Buffer[1] = 0x52;
                    LED_Buffer[2] = 0x5B;
                    LED_Buffer[3] = 0x00;
                    LED_Buffer[4] = 0x77;
                break;
            case 102:// BLE 1
                    LED_Buffer[0] = 0x7A;
                    LED_Buffer[1] = 0x52;
                    LED_Buffer[2] = 0x5B;
                    LED_Buffer[3] = 0x00;
                    LED_Buffer[4] = 0x24;
                break;

            default:
                current_menu = 0;
                break;
        }
}

static void timer_cb(void)
{
        if(LED_Display_state){
                GPIO_SetActive( theLedGpiosHigh[current_line].port, theLedGpiosHigh[current_line].pin );
                current_line++;
                current_line%=6;
                if(current_line == 0)
                {
                        counter_ms++;
                        if(counter_ms >=55)// every 495ms
                        {
                                counter_ms = 0;
                                counter_time++;
                                arch_printf("Menu: %i, Time %i\r\n", current_menu, realUnix);
                        }
                        memset(LED_Buffer,0x00,sizeof(LED_Buffer));
                        refreshMenu();

                }
                for(int i =0;i<7;i++)
                {
                        if((LED_Buffer[current_line] >> i) & 1)
                               GPIO_SetActive( theLedGpiosLow[i].port, theLedGpiosLow[i].pin );
                        else
                               GPIO_SetInactive( theLedGpiosLow[i].port, theLedGpiosLow[i].pin );
                }
                GPIO_SetInactive( theLedGpiosHigh[current_line].port, theLedGpiosHigh[current_line].pin );
        }
}

void start_refresh_timer(void)
{
        static tim0_2_clk_div_config_t clk_div_config =
        {
            .clk_div  = TIM0_2_CLK_DIV_8
        };
        timer0_2_clk_enable();
        timer0_2_clk_div_set(&clk_div_config);
        timer0_set_pwm_high_counter(0);
        timer0_set_pwm_low_counter(0);
        // Set timer with 2MHz source clock divided by 10 so Fclk = 2MHz/10 = 200kHz
        timer0_init(TIM0_CLK_FAST, PWM_MODE_ONE, TIM0_CLK_DIV_BY_10);
        // reload value for 100ms (T = 1/200kHz * RELOAD_100MS = 0,000005 * 20000 = 100ms)
        timer0_set_pwm_on_counter(300);
        timer0_register_callback(timer_cb);
}

static void app_wakeup_cb(void)
{
    // If state is not idle, ignore the message
    if (ke_state_get(TASK_APP) == APP_CONNECTABLE)
    {
        default_advertise_operation();
    }
}

static void app_resume_system_from_sleep(void)
{
    if (GetBits16(SYS_STAT_REG, PER_IS_DOWN))
    {
        periph_init();
    }

    if (arch_ble_ext_wakeup_get())
    {
        arch_set_sleep_mode(app_default_sleep_mode);
        arch_ble_force_wakeup();
        arch_ble_ext_wakeup_off();
        app_easy_wakeup();
    }
}


void user_app_on_init(void)
{

     default_app_on_init();
     arch_printf("Booted now\r\n");
     LED_GPIO_mode(1);
}

static void app_button_press_cb(void)
{
    app_resume_system_from_sleep();

    app_button_enable();
    if(GPIO_GetPinStatus(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN))
            return;

    arch_printf("Button pressed\r\n");

    if(!LED_Display_state)
    {
        // Wenn aus: Einschalten und erstes Menü zeigen
        current_menu = 0;
        LED_GPIO_mode(1);
    }
    else
    {
        // Wenn an: Nächstes Menü wählen
        current_menu++;
        if(current_menu >= MAX_MENUS)
        {
            // Wenn alle Menüs durch sind: Ausschalten
            LED_GPIO_mode(0);
            current_menu = 0;
        }
    }
}

void app_button_enable(void)
{
    app_easy_wakeup_set(app_wakeup_cb);
    wkupct_register_callback(app_button_press_cb);

    if (!GPIO_GetPinStatus(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN))
    {
        wkupct_enable_irq(WKUPCT_PIN_SELECT(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN), // select pin (GPIO_BUTTON_PORT, GPIO_BUTTON_PIN)
                          WKUPCT_PIN_POLARITY(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, WKUPCT_PIN_POLARITY_HIGH), // polarity low
                          1, // 1 event
                          0); // debouncing time = 0
    }else{
            wkupct_enable_irq(WKUPCT_PIN_SELECT(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN), // select pin (GPIO_BUTTON_PORT, GPIO_BUTTON_PIN)
                              WKUPCT_PIN_POLARITY(GPIO_BUTTON_PORT, GPIO_BUTTON_PIN, WKUPCT_PIN_POLARITY_LOW), // polarity low
                              1, // 1 event
                              0); // debouncing time = 0
    }
}

#if (BLE_SUOTA_RECEIVER)
void on_suotar_status_change(const uint8_t suotar_event)
{
    arch_printf("BLE on_suotar_status_change %u\r\n", suotar_event);
#if (!SUOTAR_SPI_DISABLE)
    uint8_t dev_id;

    // Release the SPI flash memory from power down
    spi_flash_release_from_power_down();

    // Try to auto-detect the SPI flash memory
    spi_flash_auto_detect(&dev_id);

    // Disable the SPI flash memory protection (unprotect all sectors)
    spi_flash_configure_memory_protection(SPI_FLASH_MEM_PROT_NONE);

    if(suotar_event == SUOTAR_START)
    {
        current_menu = 99;
        LED_GPIO_mode(1);
    }
    if (suotar_event == SUOTAR_END)
    {
        // Power down the SPI flash memory
        spi_flash_power_down();
        current_menu = 100;
        LED_GPIO_mode(1);
    }
#endif
}
#endif
void user_app_on_disconnect(struct gapc_disconnect_ind const *param)
{
    arch_printf("BLE Disconnected\r\n");
    default_app_on_disconnect(NULL);
    current_menu = 101;
    LED_GPIO_mode(1);

#if (BLE_BATT_SERVER)
    app_batt_poll_stop();
#endif

#if (BLE_SUOTA_RECEIVER)
    // Issue a platform reset when it is requested by the suotar procedure
    if (suota_state.reboot_requested)
    {
        // Reboot request will be served
        suota_state.reboot_requested = 0;

        // Platform reset
        platform_reset(RESET_AFTER_SUOTA_UPDATE);
    }
#endif
}

void user_app_on_connect(uint8_t conidx, struct gapc_connection_req_ind const *param)
{
    default_app_on_connection(conidx, param);
    arch_printf("BLE Connected\r\n");
    current_menu = 102;
    LED_GPIO_mode(1);
}

void app_advertise_complete(const uint8_t status)
{
    if ((status == GAP_ERR_NO_ERROR) || (status == GAP_ERR_CANCELED))
    {

    }

    if (status == GAP_ERR_CANCELED)
    {
        arch_ble_ext_wakeup_on();
        app_button_enable();
    }
}

void user_catch_rest_hndl(ke_msg_id_t const msgid,
                          void const *param,
                          ke_task_id_t const dest_id,
                          ke_task_id_t const src_id)
{
    switch(msgid)
    {
        case GATTC_EVENT_REQ_IND:
        {
            // Confirm unhandled indication to avoid GATT timeout
            struct gattc_event_ind const *ind = (struct gattc_event_ind const *) param;
            struct gattc_event_cfm *cfm = KE_MSG_ALLOC(GATTC_EVENT_CFM, src_id, dest_id, gattc_event_cfm);
            cfm->handle = ind->handle;
            KE_MSG_SEND(cfm);
        } break;

        default:
            break;
    }
}

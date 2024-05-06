#include "adapter_includes.h"

#define LED_PIN 25

#define CLK_PIN 28
#define P1_IN_PIN 27

#define PULSE_COUNT 66
// Pulse time is 650us divided into 65 chunks, then divided by two
#define PULSE_TIME_US (((7000) / PULSE_COUNT) / 2)

#define REVERSE_BITS(x) ((unsigned char)(((x & 0x01) << 7) | \
                                          ((x & 0x02) << 5) | \
                                          ((x & 0x04) << 3) | \
                                          ((x & 0x08) << 1) | \
                                          ((x & 0x10) >> 1) | \
                                          ((x & 0x20) >> 3) | \
                                          ((x & 0x40) >> 5) | \
                                          ((x & 0x80) >> 7)))

joybus_input_s _port_joybus[4] = {0, 0, 0, 0};

typedef struct
{
    int lx_offset;
    int ly_offset;
    int rx_offset;
    int ry_offset;
    int lt_offset;
    int rt_offset;
} analog_offset_s;


typedef struct
{
    union
    {
        struct
        {
            uint8_t b_a : 1;
            uint8_t b_b : 1;
            uint8_t b_z : 1;
            uint8_t b_start : 1;
            uint8_t d_up : 1;
            uint8_t d_down: 1;
            uint8_t d_left: 1;
            uint8_t d_right: 1;
        };
        uint8_t buttons_1;
    };

    union
    {
        struct
        {
            uint8_t reserved_1 : 2;
            uint8_t b_l : 1;
            uint8_t b_r : 1;
            uint8_t b_y : 1;
            uint8_t b_x : 1;
            uint8_t reserved_2 : 2;
        };
        uint8_t buttons_2;
    };

    uint8_t analog_left_x : 8;
    uint8_t analog_left_y : 8;
    uint8_t analog_right_x : 8; 
    uint8_t analog_right_y : 8;
    uint8_t analog_trigger_l : 8;
    uint8_t analog_trigger_r : 8;

} lodgenet_s;

bool cb_adapter_hardware_test()
{
    return true;
}

void joybus_itf_poll(joybus_input_s **out)
{

    *out = _port_joybus;
    static lodgenet_s p1 = {0};

    p1.buttons_1 = 0;
    p1.buttons_2 = 0;
    p1.analog_trigger_l = 0;
    p1.analog_trigger_r = 0;
    p1.analog_left_x = 0;
    p1.analog_left_y = 0;
    p1.analog_right_x = 0;
    p1.analog_right_y = 0;

    // Check if controller is connected
    if(!gpio_get(P1_IN_PIN))
    {
        p1.analog_left_x = 127;
        p1.analog_left_y = 127;
        p1.analog_right_x = 127;
        p1.analog_right_y = 127;
    }
    else
    {
        // Retrieve and set data
        for(uint i = 0; i < PULSE_COUNT; i++)
        {
            int gi = i-1;
            int a = gi % 8;

            gpio_put(CLK_PIN, 0);
            sleep_us(PULSE_TIME_US);
            gpio_put(CLK_PIN, 1);
            sleep_us(5);
            bool b = !gpio_get(P1_IN_PIN);
            switch(gi)
            {   
                default:
                break;

                // Buttons 1
                case 0 ... 7:
                    p1.buttons_1 |= (b<<a);
                break;

                // Buttons 2
                case 8 ... (8+7):
                    p1.buttons_2 |= (b<<a);
                break;

                // LX
                case 16 ... (16+7):
                    p1.analog_left_x |= (b<<(7-a));
                break;

                // LY
                case 24 ... (24+7):
                    p1.analog_left_y |= (b<<(7-a));
                break;

                // RX
                case 32 ... (32+7):
                    p1.analog_right_x |= (b<<(7-a));
                break;

                // RY
                case 40 ... (40+7):
                    p1.analog_right_y |= (b<<(7-a));
                break;

                // TL
                case 48 ... (48+7):
                    p1.analog_trigger_l |= (b<<(7-a));
                break;

                // TR
                case 56 ... (56+7):
                    p1.analog_trigger_r |= (b<<(7-a));
                break;
            }
            sleep_us(PULSE_TIME_US); 
        }
    }

    _port_joybus[0].port_itf = 0;

    
    _port_joybus[0].button_a            = p1.b_a;
    _port_joybus[0].button_b            = p1.b_b;
    _port_joybus[0].button_x            = p1.b_x;
    _port_joybus[0].button_y            = p1.b_y;
    _port_joybus[0].button_z            = p1.b_z;
    _port_joybus[0].button_r            = p1.b_r;
    _port_joybus[0].button_l            = p1.b_l;
    _port_joybus[0].button_start        = p1.b_start;
    _port_joybus[0].dpad_up             = p1.d_up;
    _port_joybus[0].dpad_down           = p1.d_down;
    _port_joybus[0].dpad_left           = p1.d_left;
    _port_joybus[0].dpad_right          = p1.d_right;
    _port_joybus[0].stick_left_y        = 255-p1.analog_left_y;
    _port_joybus[0].stick_left_x        = 255-p1.analog_left_x;
    _port_joybus[0].stick_right_y       = 255-p1.analog_right_y;
    _port_joybus[0].stick_right_x       = 255-p1.analog_right_x;

    _port_joybus[0].analog_trigger_l    = 255-p1.analog_trigger_l;
    // Copy data
    _port_joybus[0].analog_trigger_r    = 255-p1.analog_trigger_r;

}

void joybus_itf_enable_rumble(uint8_t interface, bool enable)
{
    // Lodgenet does not support rumble
    (void) interface;
    (void) enable;
}

void rgb_itf_update(rgb_s *leds)
{
    // nothing
}

void joybus_itf_init()
{
    gpio_init(CLK_PIN);
    gpio_pull_up(CLK_PIN);
    gpio_set_drive_strength(CLK_PIN, GPIO_DRIVE_STRENGTH_4MA);
    gpio_set_dir(CLK_PIN, GPIO_OUT);
    gpio_put(CLK_PIN, 1);

    gpio_init(P1_IN_PIN);
    gpio_pull_down(P1_IN_PIN);
    gpio_set_dir(P1_IN_PIN, GPIO_IN);
}

void rgb_itf_init()
{
    gpio_init(LED_PIN);
    gpio_pull_up(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_put(LED_PIN, 1);
}

void main()
{
    adapter_main_init();
    adapter_main_loop();
}
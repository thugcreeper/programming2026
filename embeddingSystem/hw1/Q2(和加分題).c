#include <reg52.h>
#define LED P2
#define us250 (256 - 250)

sbit key0 = P0 ^ 0;

bit single_click = 0;
bit double_click = 0;
bit long_press = 0;
bit need_shift = 0; // ISR 通知 rotate() 該移一格

unsigned char key_state = 0;
unsigned int press_time = 0;
unsigned int click_wait = 0;

unsigned char led_state = 0xFE;
unsigned char toggle_index = 0;

unsigned int blink_cnt = 0;
unsigned int shift_cnt = 0;
bit blink_show = 1; // 1=glow, 0=led off

typedef enum
{
    MODE_IDLE = 0,
    MODE_LEFT,
    MODE_RIGHT
} Mode;
Mode currentMode = MODE_IDLE;

void Init_Timer1(void);
void rotate(int direction);

void main()
{
    LED = led_state;
    Init_Timer1();

    while (1)
    {
        if (long_press)
        {
            long_press = 0;
            currentMode = MODE_IDLE;
            toggle_index = 0;
            led_state = 0xFE;
            blink_show = 1;
            LED = led_state;
        }
        if (double_click)
        {
            double_click = 0;
            rotate(1);
        }
        if (single_click)
        {
            single_click = 0;
            rotate(0);
        }
    }
}

void rotate(int direction)
{
    led_state = ~(1 << toggle_index); // ex index=2->~(0000 0100)=1111 1011 led3 glow
    currentMode = (direction == 0) ? MODE_LEFT : MODE_RIGHT;

    while (!long_press)
    {
        if (need_shift)
        {
            need_shift = 0;
            if (direction == 1)
            { // left shift
                toggle_index = (toggle_index + 1) % 8;
            }
            else
            { // right shift
                toggle_index = (toggle_index == 0) ? 7 : toggle_index - 1;
            }

            led_state = ~(1 << toggle_index);
        }
    }
    single_click = 0;
    double_click = 0;
    currentMode = MODE_IDLE;
}
/* interrupt X
編號 0：對應 外部中斷 0 (INT0)
。
編號 1：對應 計時器 0 (Timer 0 / T0)
。
編號 2：對應 外部中斷 1 (INT1)
。
編號 3：對應 計時器 1 (Timer 1 / T1)
。
編號 4：對應 序列埠 (UART)*/
void Timer1_isr(void) interrupt 3
{ // timer 1's interrupt service routine
    if (currentMode != MODE_IDLE)
    {
        shift_cnt++;
        if (shift_cnt >= 2000)
        { // shift every 2000 × 250us =500ms
            shift_cnt = 0;
            need_shift = 1;
        }
    }

    /* ── Toggle：每 1 秒，把 toggle_index 那顆 LED 閃一下 ── */
    blink_cnt++;
    if (blink_cnt >= 4000)
    { // 4000 × 250us = 1s
        blink_cnt = 0;
        blink_show = !blink_show;
        if (blink_show)
            LED = led_state; // 顯示亮點
        else
            LED = led_state | (1 << toggle_index); // 只熄 toggle 那顆
    }
    else
    {
        /* 非 toggle 時機，正常顯示 led_state */
        if (blink_show)
            LED = led_state;
    }

    /* ── 按鍵狀態機 ── */
    switch (key_state)
    {
    case 0:
        if (key0 == 0)
        {
            press_time = 0;
            key_state = 1;
        }
        break;
    case 1:
        if (key0 == 0)
        {
            press_time++;
            if (press_time >= 4000)
            {
                long_press = 1;
                key_state = 3;
            }
        }
        else
        {
            click_wait = 0;
            key_state = 2;
        }
        break;
    case 2:
        click_wait++;
        if (key0 == 0)
        {
            double_click = 1;
            key_state = 3;
        }
        else if (click_wait >= 2000)
        {
            single_click = 1;
            key_state = 0;
        }
        break;
    case 3:
        if (key0 == 1)
            key_state = 0;
        break;
    }
}

void Init_Timer1(void)
{ // TMOD has 8bit:first 4 bit is for timer1(GATE,C/T,M1,M0)
    TMOD &= 0x0F;
    TMOD |= 0x20; // set timer1 mode 2,8bit autoreload timer
    TH1 = us250;  // timer1's high bits
    TL1 = us250;  // timer1's low bits
    ET1 = 1;      // enable timer1's interrupt
    EA = 1;       // open main switch
    TR1 = 1;      // open timer1
}

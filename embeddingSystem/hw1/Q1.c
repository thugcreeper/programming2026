#include <reg52.h>
#define ms10 65536 - 10000

sbit key0 = P0 ^ 0; // 定義key0是P0第一個按鈕
sbit led0 = P2 ^ 0;

unsigned int press_time = 0;
unsigned int led_time = 0;
bit pressing = 0;
bit lighting = 0;

void delay(unsigned int k)
{
	unsigned int j, i;
	for (i = 0; i < k; i++)
		for (j = 0; j < 200; j++)
			;
}

void Init_Timer1(void)
{
	TMOD |= 0x10;	  // 使用模式1，16位定時器，使用"|"符號可以在使用多個定時器時不受影響  0001xxxx
	TH1 = ms10 / 256; // 給定初值，這裡使用定時器最大值從0開始計數一直到65535溢出
	TL1 = ms10 % 256;
	TR1 = 1; // 定時器開關打開
}

Init_Timer_Int()
{
	EA = 1;	 // 總中斷打開
	ET1 = 1; // Timer 1的中斷打開
	// ET0=1;			  //Timer 0的中斷
}

void main()
{
	Init_Timer1();
	Init_Timer_Int();
	while (1)
	{
		if (key0 == 0)
		{ // 按住按鈕
			pressing = 1;
		}
		else
		{
			if (pressing)
			{
				delay(2); // 避免按鈕彈跳
				if (key0 == 1)
				{
					pressing = 0;
					led_time = press_time;
					press_time = 0;
					lighting = 1;
					led0 = 0; // LED發光
				}
			}
		}
	}
}
// timer 1 interrupt service routine
void Timer1_isr(void) interrupt 3 using 1
{
	TH1 = ms10 / 256; // 給定初值，這裡使用定時器最大值從0開始計數一直到65535溢出
	TL1 = ms10 % 256;
	if (pressing)
	{
		press_time++;
	}
	if (lighting)
	{
		if (led_time > 0)
		{
			led_time--;
		}
		else
		{
			lighting = 0;
			led0 = 1; // led熄滅
		}
	}
}

#include <reg52.h>
#define us250 (256 - 250)
#define sevenSegment P0
sbit probe = P1 ^ 0;      // 用於檢測信號並輸出到7段顯示器
sbit wave = P2 ^ 0;       // T=25ms
sbit waveMedium = P2 ^ 1; // T=125ms
sbit waveLong = P2 ^ 2;   // T=625ms
sbit high = P2 ^ 3;
sbit low = P2 ^ 4;

void Init_Timer0(void);
void Timer0_isr(void);
//"code" represent this is a part of code,use less memory than a array
unsigned char code segCode[3] = {0x89, 0xC7, 0x8C}; // show 7 segment code high(H) low(L) pulse(P) 0 is glow

unsigned char lastProbe = 0;
unsigned int halfPeriod = 200;
unsigned int timeoutCnt = 0;
bit isWave = 0;

// if a wave didn't change in 400*250us=0.125sec,then the output is not a wave,7seg will show"L" or "H"
#define TIMEOUT_THRESHOLD 500
void main()
{
    Init_Timer0();
    while (1)
    {
        high = 1;
        low = 0;
        if (isWave)
            sevenSegment = segCode[2];
        else
        {
            if (probe)
                sevenSegment = segCode[0]; // show H
            else
                sevenSegment = segCode[1]; // show L
        }
    }
}
void Init_Timer0(void)
{
    TMOD |= 0x02; // 使用模式2，8位定時器，使用"|"符號可以在使用多個定時器時不受影響
    TH0 = us250;
    TL0 = us250;
    EA = 1;  // 總中斷打開
    ET0 = 1; // 定時器中斷打開
    TR0 = 1; // 定時器開關打開
}

void Timer0_isr(void) interrupt 1
{
    static unsigned int count = 0;
    static unsigned int edgeTick = 0;
    unsigned int elapsed;
    count++;
    if (count % 50 == 0)
    {
        wave = ~wave;
    }
    if (count % 250 == 0)
    {
        waveMedium = ~waveMedium;
    }
    if (count % 1250 == 0)
    {
        waveLong = ~waveLong;
    }

    if (probe != lastProbe)
    {
        elapsed = count - edgeTick;
        edgeTick = count;
        halfPeriod = elapsed;
        timeoutCnt = 0;
        lastProbe = probe;
        isWave = 1;
    }
    else
    {
        timeoutCnt++;
        if (isWave && (timeoutCnt > (halfPeriod + (halfPeriod >> 2))))
        {
            isWave = 0;
        }
        if (timeoutCnt >= TIMEOUT_THRESHOLD)
        {
            isWave = 0;
        }
    }
}

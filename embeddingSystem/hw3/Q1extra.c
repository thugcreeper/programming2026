#include <reg52.h>
// 矩陣按鈕按4個數字後按set(12)七段顯示器要暗掉，表示記住密碼了
// 之後再次輸入四個數字並按enter(15)來比對密碼

#define DataPort P0
#define KeyPort P1
sbit LATCH1 = P2 ^ 2;
sbit LATCH2 = P2 ^ 3;

unsigned char code sevensegWord[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
                                     0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71};
unsigned char code sevensegPos[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
unsigned char TempData[4];
unsigned char password[4] = {0, 0, 0, 0};
bit PasswordSet = 0;

// 狀態定義
#define STATE_IDLE 0 // 等待同時按下四鍵
#define STATE_GOT4 1 // 已取得四鍵，等待SET或ENTER

unsigned char state = STATE_IDLE;
unsigned char captured[4] = {0, 0, 0, 0}; // 暫存按下的四個鍵值

void DelayUs2x(unsigned char t);
void DelayMs(unsigned char t);
void Display(unsigned char FirstBit, unsigned char Num);
void Init_Timer0(void);
unsigned char KeyScanMulti(void);
unsigned char keyProMulti(unsigned char *keys);
bit hasKey(unsigned char keys[], unsigned char count, unsigned char target);
unsigned char KeyScanSingle(void); // 等待單一按鍵放開後回傳

/*------------------------------------------------
                    主函數
------------------------------------------------*/
void main(void)
{
    unsigned char keys[16];
    unsigned char count;
    unsigned char i, j;
    unsigned char used[4];
    unsigned char filtered[16];
    unsigned char fcount = 0;
    bit Flag;

    Init_Timer0();

    // 清空顯示
    for (i = 0; i < 4; i++)
        TempData[i] = 0;

    while (1)
    {
        if (state == STATE_IDLE)
        {
            // 偵測是否有四個鍵同時按下（不含SET/ENTER）
            count = keyProMulti(keys);

            if (count < 4)
                continue;

            // 過濾掉 SET(12) 和 ENTER(15)，只留一般數字鍵
            for (i = 0; i < 16; i++)
                filtered[i] = 0;
            fcount = 0;
            for (i = 0; i < count; i++)
            {
                if (keys[i] != 12 && keys[i] != 15)
                    filtered[fcount++] = keys[i];
            }

            if (fcount != 4)
                continue;

            // 消抖：等待所有鍵放開
            DelayMs(20);
            while (KeyScanMulti() != 0)
                ;
            DelayMs(20);

            // 儲存四個鍵值
            for (i = 0; i < 4; i++)
                captured[i] = filtered[i];

            // 顯示四個數字（由左到右）
            for (i = 0; i < 4; i++)
                TempData[i] = sevensegWord[captured[i]];

            state = STATE_GOT4;
        }
        else if (state == STATE_GOT4)
        {
            // 等待單一按鍵（SET 或 ENTER）
            unsigned char singleKey = KeyScanSingle();

            if (singleKey == 0xff)
                continue;

            if (singleKey == 12) // SET：儲存密碼，清空顯示
            {
                for (i = 0; i < 4; i++)
                    password[i] = captured[i];
                PasswordSet = 1;

                // 清空顯示
                for (i = 0; i < 4; i++)
                    TempData[i] = 0;

                state = STATE_IDLE;
            }
            else if (singleKey == 15) // ENTER：比對密碼
            {
                if (!PasswordSet)
                {
                    // 尚未設定密碼，顯示 FAIL
                    TempData[0] = 0x71; // F
                    TempData[1] = 0x77; // A
                    TempData[2] = 0x30; // I
                    TempData[3] = 0x38; // L
                }
                else
                {
                    // 比對（順序不重要，用過的密碼位不重複配對）
                    for (j = 0; j < 4; j++)
                        used[j] = 0;
                    Flag = 1;
                    for (i = 0; i < 4; i++)
                    {
                        bit found = 0;
                        for (j = 0; j < 4; j++)
                        {
                            if (!used[j] && captured[i] == password[j])
                            {
                                used[j] = 1;
                                found = 1;
                                break;
                            }
                        }
                        if (!found)
                        {
                            Flag = 0;
                            break;
                        }
                    }

                    if (Flag)
                    {
                        TempData[0] = 0x73; // P
                        TempData[1] = 0x77; // A
                        TempData[2] = 0x5B; // S
                        TempData[3] = 0x5B; // S
                    }
                    else
                    {
                        TempData[0] = 0x71; // F
                        TempData[1] = 0x77; // A
                        TempData[2] = 0x30; // I
                        TempData[3] = 0x38; // L
                    }
                }

                // 顯示結果後等一段時間，再清空回 IDLE
                DelayMs(200);
                for (i = 0; i < 4; i++)
                    TempData[i] = 0;

                state = STATE_IDLE;
            }
            else
            {
                // 按到其他鍵，清空顯示回 IDLE
                for (i = 0; i < 4; i++)
                    TempData[i] = 0;
                state = STATE_IDLE;
            }
        }
    }
}

/*------------------------------------------------
 uS 延時
------------------------------------------------*/
void DelayUs2x(unsigned char t)
{
    while (--t)
        ;
}

/*------------------------------------------------
 mS 延時
------------------------------------------------*/
void DelayMs(unsigned char t)
{
    while (t--)
    {
        DelayUs2x(245);
        DelayUs2x(245);
    }
}

/*------------------------------------------------
 顯示函數（動態掃瞄）
------------------------------------------------*/
void Display(unsigned char FirstBit, unsigned char Num)
{
    static unsigned char i = 0;

    DataPort = 0;
    LATCH1 = 1;
    LATCH1 = 0;

    DataPort = sevensegPos[i + FirstBit];
    LATCH2 = 1;
    LATCH2 = 0;

    DataPort = TempData[i];
    LATCH1 = 1;
    LATCH1 = 0;

    i++;
    if (i == Num)
        i = 0;
}

/*------------------------------------------------
 定時器初始化
------------------------------------------------*/
void Init_Timer0(void)
{
    TMOD |= 0x01;
    TH0 = (65536 - 2000) / 256;
    TL0 = (65536 - 2000) % 256;
    EA = 1;
    ET0 = 1;
    TR0 = 1;
}

/*------------------------------------------------
 定時器中斷（每 2ms 掃瞄一次顯示）
------------------------------------------------*/
void Timer0_isr(void) interrupt 1
{
    TH0 = (65536 - 2000) / 256;
    TL0 = (65536 - 2000) % 256;
    Display(0, 4);
}

/*------------------------------------------------
 多鍵掃瞄：回傳 key_state，每個 bit 代表一顆鍵是否按下
------------------------------------------------*/
unsigned char KeyScanMulti(void)
{
    unsigned char row, col;
    unsigned char key_state = 0;

    for (row = 0; row < 4; row++)
    {
        KeyPort = ~(1 << (row + 4));
        col = KeyPort & 0x0f;

        if (col != 0x0f)
        {
            if (!(col & 0x01))
                key_state |= (1 << (row * 4 + 0));
            if (!(col & 0x02))
                key_state |= (1 << (row * 4 + 1));
            if (!(col & 0x04))
                key_state |= (1 << (row * 4 + 2));
            if (!(col & 0x08))
                key_state |= (1 << (row * 4 + 3));
        }
    }

    return key_state;
}

/*------------------------------------------------
 將 key_state 轉成 keys[] 陣列，回傳按下的鍵數
------------------------------------------------*/
unsigned char keyProMulti(unsigned char *keys)
{
    unsigned char state_val;
    unsigned char i, count = 0;

    state_val = KeyScanMulti();
    for (i = 0; i < 16; i++)
    {
        if (state_val & (1 << i))
            keys[count++] = i;
    }
    return count;
}

/*------------------------------------------------
 等待單一按鍵按下並放開，回傳鍵值
 用於 STATE_GOT4 階段等待 SET 或 ENTER
------------------------------------------------*/
unsigned char KeyScanSingle(void)
{
    unsigned char state_val;
    unsigned char i;

    state_val = KeyScanMulti();
    if (state_val == 0)
        return 0xff; // 沒有按鍵

    DelayMs(20); // 消抖
    state_val = KeyScanMulti();
    if (state_val == 0)
        return 0xff;

    // 找出按下的是哪一鍵（只取第一個）
    for (i = 0; i < 16; i++)
    {
        if (state_val & (1 << i))
        {
            // 等待放開
            while (KeyScanMulti() != 0)
                ;
            DelayMs(20);
            return i;
        }
    }

    return 0xff;
}

/*------------------------------------------------
 hasKey：檢查 keys[] 中是否含有 target
------------------------------------------------*/
bit hasKey(unsigned char keys[], unsigned char count, unsigned char target)
{
    unsigned char i;
    for (i = 0; i < count; i++)
    {
        if (keys[i] == target)
            return 1;
    }
    return 0;
}
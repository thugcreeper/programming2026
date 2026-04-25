#include <reg52.h>
// 矩陣按鈕按4個數字後按set(12)七段顯示器要暗掉，表示記住密碼了
// 之後再次輸入四個數字並按enter(15)來比對密碼

#define DataPort P0 // 定義數據端口 程序中遇到DataPort 則用P0 替換
#define KeyPort P1
sbit LATCH1 = P2 ^ 2; // 定義鎖存使能端口 段鎖存
sbit LATCH2 = P2 ^ 3; //                 位鎖存

unsigned char code sevensegWord[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07,
                                     0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; // 顯示段碼值0~F
unsigned char code sevensegPos[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};  // 分別對應相應的數碼管點亮,即位碼
unsigned char TempData[10];                                                           // 存儲顯示值的全局變量
unsigned char password[4] = {0, 0, 0, 0};                                             // 運行中可更新的密碼
bit PasswordSet = 0;                                                                  // 是否已設定密碼

void DelayUs2x(unsigned char t);                         // us級延時函數聲明
void DelayMs(unsigned char t);                           // ms級延時
void Display(unsigned char FirstBit, unsigned char Num); // 數碼管顯示函數
unsigned char KeyScan(void);                             // 鍵盤掃瞄
unsigned char KeyPro(void);                              // 根據鍵盤回傳對應的值
void Init_Timer0(void);                                  // 定時器初始化

/*------------------------------------------------
                    主函數
------------------------------------------------*/
void main(void)
{
    unsigned char num = 0, i = 0, j = 0;
    unsigned char input[4] = {0, 0, 0, 0};
    bit Flag;
    bit afterCheck = 0;
    Init_Timer0();

    for (j = 0; j < 4; j++)
        TempData[j] = 0;

    while (1) // 主循環
    {
        num = KeyPro();
        if (num == 0xff)
            continue;

        if (num == 12) // set鍵：設定密碼並全暗
        {
            if (i == 4)
            {
                for (j = 0; j < 4; j++)
                    password[j] = input[j];
                PasswordSet = 1;
                i = 0;
                for (j = 0; j < 4; j++)
                {
                    input[j] = 0;
                    TempData[j] = 0;
                }
            }
            continue;
        }

        if (num == 15) // enter鍵：比對密碼
        {
            for (j = 0; j < 4; j++)
                TempData[j] = 0;

            if (PasswordSet && i == 4)
            {
                Flag = 1;
                for (j = 0; j < 4; j++)
                    Flag = Flag && (input[j] == password[j]);

                if (Flag)
                {
                    TempData[0] = 0x73; // "P"
                    TempData[1] = 0x77; // "A"
                    TempData[2] = 0x5B; // "S"
                    TempData[3] = 0x5B; // "S"
                    afterCheck = 1;
                }
                else
                {
                    TempData[0] = 0x71; // "F"
                    TempData[1] = 0x77; // "A"
                    TempData[2] = 0x30; // "I"
                    TempData[3] = 0x38; // "L"
                    afterCheck = 1;
                }
            }
            else
            {
                TempData[0] = 0x71; // "F"
                TempData[1] = 0x77; // "A"
                TempData[2] = 0x30; // "I"
                TempData[3] = 0x38; // "L"
            }

            i = 0;
            for (j = 0; j < 4; j++)
                input[j] = 0;
            continue;
        }
        if (afterCheck)
        {
            for (j = 0; j < 4; j++)
            {
                TempData[j] = 0;
            }
            afterCheck = 0;
        }
        // 一般按鍵輸入(保留0~F中除了12/15的值)
        if (i < 4)
        {
            for (j = 0; j < 4; j++)
            {
                TempData[j] = 0;
            }
            input[i] = num;
            for (j = 0; j <= i; j++)
                TempData[3 - i + j] = sevensegWord[input[j]]; // 從右往左輸入
            i++;
        }
    }
}

/*------------------------------------------------
 uS延時函數，含有輸入參數 unsigned char t，無返回值
 unsigned char 是定義無符號字符變量，其值的範圍是
 0~255 這裡使用晶振12M，精確延時請使用彙編,大致延時
 長度如下 T=tx2+5 uS
------------------------------------------------*/
void DelayUs2x(unsigned char t)
{
    while (--t)
        ;
}

/*------------------------------------------------
 mS延時函數，含有輸入參數 unsigned char t，無返回值
 unsigned char 是定義無符號字符變量，其值的範圍是
 0~255 這裡使用晶振12M，精確延時請使用彙編
------------------------------------------------*/
void DelayMs(unsigned char t)
{
    while (t--)
    {
        // 大致延時1mS
        DelayUs2x(245);
        DelayUs2x(245);
    }
}

/*------------------------------------------------
 顯示函數，用於動態掃瞄七段顯示器
 輸入參數 FirstBit 表示需要顯示的第一位，如賦值2表示從第三個數碼管開始顯示
 如輸入0表示從第一個顯示。
 Num表示需要顯示的位數，如需要顯示99兩位數值則該值輸入2
------------------------------------------------*/
void Display(unsigned char FirstBit, unsigned char Num)
{
    static unsigned char i = 0;

    DataPort = 0; // 清空數據，防止有交替重影
    LATCH1 = 1;   // 段鎖存
    LATCH1 = 0;

    DataPort = sevensegPos[i + FirstBit]; // 取位碼
    LATCH2 = 1;                           // 位鎖存
    LATCH2 = 0;

    DataPort = TempData[i]; // 取顯示數據，段碼
    LATCH1 = 1;             // 段鎖存
    LATCH1 = 0;

    i++;
    if (i == Num)
        i = 0;
}

/*------------------------------------------------
                    定時器初始化子程序
------------------------------------------------*/
void Init_Timer0(void)
{
    TMOD |= 0x01; // 使用模式1，16位定時器，使用"|"符號可以在使用多個定時器時不受影響
    TH0 = (65536 - 2000) / 256;
    TL0 = (65536 - 2000) % 256;
    EA = 1;  // 總中斷打開
    ET0 = 1; // 定時器中斷打開
    TR0 = 1; // 定時器開關打開
}

/*------------------------------------------------
                 定時器中斷子程序
------------------------------------------------*/
void Timer0_isr(void) interrupt 1
{
    TH0 = (65536 - 2000) / 256; // 重新賦值 2ms
    TL0 = (65536 - 2000) % 256;

    Display(0, 4); // 調用七段顯示器掃瞄，從第一位開始顯示總共4位數值
}

/*------------------------------------------------
按鍵掃瞄函數，返回掃瞄鍵值
------------------------------------------------*/
unsigned char KeyScan(void) // 鍵盤掃瞄函數，使用行列反轉掃瞄法
{
    unsigned char cord_h, cord_l; // 行列值中間變量
    KeyPort = 0x0f;               // 行線輸出全為0
    cord_h = KeyPort & 0x0f;      // 讀入列線值
    if (cord_h != 0x0f)           // 先檢測有無按鍵按下
    {
        DelayMs(10); // 去抖
        if ((KeyPort & 0x0f) != 0x0f)
        {
            cord_h = KeyPort & 0x0f; // 讀入列線值
            KeyPort = cord_h | 0xf0; // 輸出當前列線值
            cord_l = KeyPort & 0xf0; // 讀入行線值

            while ((KeyPort & 0xf0) != 0xf0)
                ; // 等待鬆開並輸出

            return (cord_h + cord_l); // 鍵盤最後組合碼值
        }
    }
    return (0xff); // 返回該值
}

/*------------------------------------------------
按鍵值處理函數，返回掃鍵值
------------------------------------------------*/
unsigned char KeyPro(void)
{
    switch (KeyScan())
    {
    case 0x7e:
        return 0;
        break; // 0
    case 0x7d:
        return 1;
        break; // 1
    case 0x7b:
        return 2;
        break; // 2
    case 0x77:
        return 3;
        break; // 3
    case 0xbe:
        return 4;
        break; // 4
    case 0xbd:
        return 5;
        break; // 5
    case 0xbb:
        return 6;
        break; // 6
    case 0xb7:
        return 7;
        break; // 7
    case 0xde:
        return 8;
        break; // 8
    case 0xdd:
        return 9;
        break; // 9
    case 0xdb:
        return 10;
        break; // A
    case 0xd7:
        return 11;
        break; // B
    case 0xee: // 設定這個當set鍵
        return 12;
        break; // C
    case 0xed:
        return 13;
        break; // D
    case 0xeb:
        return 14;
        break; // E
    case 0xe7: // 設定這個當enter鍵
        return 15;
        break; // F
    default:
        return 0xff;
        break;
    }
}

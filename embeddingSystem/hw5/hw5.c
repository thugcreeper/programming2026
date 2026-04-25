#include <reg52.h>
#include <intrins.h>

#define us250 (256 - 250)
#define KEY P3 

unsigned char segout[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}; 
unsigned char code tab[] = {
    0x00, 0x40, 0x26, 0x10, 0x10, 0x26, 0x40, 0x00, // Sad
    0x00, 0x10, 0x16, 0x10, 0x10, 0x16, 0x10, 0x00, // Neutral
    0x00, 0x08, 0x16, 0x20, 0x20, 0x16, 0x08, 0x00, // Smile
};

unsigned int changeFaceCounter = 0;
unsigned char faceIndex = 0; 
signed char x_move = 0;     // 水平移動量，正值向右，負值向左 
signed char y_move = 0;      // 垂直移動量，正值向上，負值向下
unsigned char key_delay_cnt = 0; // 用於按鍵去抖動與速度控制的計數器
unsigned char scanIdx = 0; // 記錄目前掃描到哪一列
sbit LATCH = P1 ^ 0;
sbit SRCLK = P1 ^ 1; // 74HC595 的時鐘腳
sbit SER = P1 ^ 2;
sbit LATCH_B = P2 ^ 2;
sbit SRCLK_B = P2 ^ 1;
sbit SER_B = P2 ^ 0;

// --- 拿掉 DelayMs，改用底層延時保證掃描速度 ---
void DelayUs2x(unsigned char t) { while (--t); }

void SendByte(unsigned char dat) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        //& 0x80表示取最高位元
        SRCLK = 0; 
        SER = dat & 0x80; 
        dat <<= 1; //左移1bit，準備送下一位
        SRCLK = 1;
    }
}
/*------------------------------------------------
                   595鎖存程序
		  595級聯發送數據後，鎖存有效
------------------------------------------------*/
void Out595(void) { 
    LATCH = 1; 
    _nop_(); 
    LATCH = 0; 
}
/*------------------------------------------------
                發送位碼字節程序
               使用另外一片單獨595
------------------------------------------------*/
void SendSeg(unsigned char dat) {
    unsigned char i;
    for (i = 0; i < 8; i++) {
        SRCLK_B = 0;
        SER_B = dat & 0x80;
        dat <<= 1;
        SRCLK_B = 1;
    }
    LATCH_B = 1;
    _nop_(); 
    LATCH_B = 0;
}

void Init_Timer1(void) {
    TMOD &= 0x0F; // 清除 Timer1 的控制位
    TMOD |= 0x20; // mode2 8-bit auto-reload
    TH1 = us250; 
    TL1 = us250;
    ET1 = 1;     // enable Timer1 interrupt
    EA = 1;      //總開關
    TR1 = 1;     // start Timer1
}

void main() { 
    Init_Timer1();
    while (1) {
        // 利用計數器來控制移動靈敏度
        if (KEY != 0xFF) { 
            key_delay_cnt++;
            if (key_delay_cnt > 25) { // 調整這個數值可以改變按鈕靈敏度
                if (KEY == 0xFE) {if(y_move < 7) y_move++; } // 上
                if (KEY == 0xFD) {if(y_move > -7) y_move--; } // 下
                if (KEY == 0xF7) {if(x_move < 7) x_move++; } // 左
                if (KEY == 0xFB) {if(x_move > -7) x_move--; } // 右
                key_delay_cnt = 0; 
            }
        } 
        else {
            key_delay_cnt = 0; // 放開按鍵時重置計數
        }
    }
}
//isr不要出現loop，應該盡可能的快結束，此isr只負責掃描和變換表情，
//按鍵的處理放在main裡面，避免按按鈕時畫面閃爍
void timer1_isr(void) interrupt 3 {
    unsigned char dat, shift;
    signed int col_idx;
    changeFaceCounter++;
    //每0.5秒換一張臉
    if (changeFaceCounter >= 2000) { 
        changeFaceCounter = 0;
        faceIndex += 1;
        if (faceIndex >= 3) faceIndex = 0;
    }
    //--- 用中斷掃描顯示---//
    SendByte(0xFF); 
    Out595();

    scanIdx = (scanIdx + 1) % 8;
    
    //水平移動
    if ((scanIdx + x_move) >= 0 && (scanIdx + x_move) < 8) {
        dat = tab[faceIndex * 8 + scanIdx + x_move];
    } 
    //超出邊界就不顯示了
    else {dat = 0; }
    //垂直移動，這裡用左右移而不是查表，是因為tab存的資料是垂直顯示的，
    //因此移動y軸只需要左右移
    if (y_move > 0) dat >>= y_move; 
    else if (y_move < 0) dat <<= -y_move;
    SendSeg(segout[scanIdx]);

    SendByte(~dat); 
    SendByte(0xFF); 
    Out595();
}
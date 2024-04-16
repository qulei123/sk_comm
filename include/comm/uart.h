
#ifndef __UART_H__
#define __UART_H__

#include "deftypes.h"


/* uart --> 接收，收到数据有xs左右的延时 */
#define UART_RX_TIMEOUT_S       1
#define UART_RX_TIMEOUT_MS      0     /* 最小值100ms */

#define UART_BPS_00300      300
#define UART_BPS_00600      600
#define UART_BPS_01200      1200
#define UART_BPS_02400      2400
#define UART_BPS_04800      4800
#define UART_BPS_09600      9600
#define UART_BPS_19200      19200
#define UART_BPS_57600      57600
#define UART_BPS_115200     115200


typedef enum    
{               
    DATABITS_5 = 0x5,
    DATABITS_6 = 0x6, 
    DATABITS_7 = 0x7,
    DATABITS_8 = 0x8
} E_UartDataBits;

typedef enum
{
    ONE_STOPBIT  = 0x1,
    TWO_STOPBITS = 0x2
} E_UartStopBits;

typedef enum
{
    NOPARITY_BIT    = 0x0,
    EVEN_PARITY_BIT = 0x1,
    ODD_PARITY_BIT  = 0x2
} E_UartParityBit;

typedef struct UartPara
{
    U32             u32Bps;         /* UART_BPS_09600 */
    E_UartDataBits  eDataBit;   
    E_UartStopBits  eStopBit;
    E_UartParityBit eParityBit;
} T_UartPara, *PT_UartPara;


INT Uart_Open(const CHAR *pcDevUart);
VOID Uart_Close(INT iFd);
INT Uart_Set_Para(INT iFd, PT_UartPara ptUartPara);
INT Uart_Set_Timeout(INT iFd, U32 vtime, U32 vmin);
INT Uart_Recv(INT iFd, CHAR *pcRecvBuf, U32 u32BufLen, INT iFlag);
INT Uart_Send(INT iFd, CHAR *pcSendBuf, U32 u32SendLen);
VOID Uart_Dummy(INT iFd);

#endif /* __UART_H__ */


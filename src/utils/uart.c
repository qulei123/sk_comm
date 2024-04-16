
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include "uart.h"

/* 选择一个最合适的bps */
static INT Get_Bps_Para(U32 u32Bps) 
{
    INT iBpsVal, iBpsFlag;
    
#define SETBAUD(x) (iBpsFlag = B ## x, iBpsVal = x)
#define CHECKBAUD(x) do { if (u32Bps >= x) SETBAUD(x); } while (0)
    SETBAUD(50);
#ifdef B75
    CHECKBAUD(75);
#endif
#ifdef B110
    CHECKBAUD(110);
#endif
#ifdef B134
    CHECKBAUD(134);
#endif
#ifdef B150
    CHECKBAUD(150);
#endif
#ifdef B200
    CHECKBAUD(200);
#endif
#ifdef B300
    CHECKBAUD(300);
#endif
#ifdef B600
    CHECKBAUD(600);
#endif
#ifdef B1200
    CHECKBAUD(1200);
#endif
#ifdef B1800
    CHECKBAUD(1800);
#endif
#ifdef B2400
    CHECKBAUD(2400);
#endif
#ifdef B4800
    CHECKBAUD(4800);
#endif
#ifdef B9600
    CHECKBAUD(9600);
#endif
#ifdef B19200
    CHECKBAUD(19200);
#endif
#ifdef B38400
    CHECKBAUD(38400);
#endif
#ifdef B57600
    CHECKBAUD(57600);
#endif
#ifdef B76800
    CHECKBAUD(76800);
#endif
#ifdef B115200
    CHECKBAUD(115200);
#endif
#ifdef B153600
    CHECKBAUD(153600);
#endif
#ifdef B230400
    CHECKBAUD(230400);
#endif
#ifdef B307200
    CHECKBAUD(307200);
#endif
#ifdef B460800
    CHECKBAUD(460800);
#endif
#ifdef B500000
    CHECKBAUD(500000);
#endif
#ifdef B576000
    CHECKBAUD(576000);
#endif
#ifdef B921600
    CHECKBAUD(921600);
#endif
#ifdef B1000000
    CHECKBAUD(1000000);
#endif
#ifdef B1152000
    CHECKBAUD(1152000);
#endif
#ifdef B1500000
    CHECKBAUD(1500000);
#endif
#ifdef B2000000
    CHECKBAUD(2000000);
#endif
#ifdef B2500000
    CHECKBAUD(2500000);
#endif
#ifdef B3000000
    CHECKBAUD(3000000);
#endif
#ifdef B3500000
    CHECKBAUD(3500000);
#endif
#ifdef B4000000
    CHECKBAUD(4000000);
#endif
#undef CHECKBAUD
#undef SETBAUD

    log_info("cfg bps %d\n", iBpsVal);
    return iBpsFlag;
} 


INT Uart_Open(const CHAR *pcDevUart)
{
    assert(NULL != pcDevUart);
    INT iFd;
    
    iFd = open(pcDevUart, O_RDWR | O_NOCTTY);   /* O_NONBLOCK O_NDELAY */
    if (iFd < 0)
    {
        log_eno("open %s\n", pcDevUart);
        return -1;
    }

    return iFd;
}

VOID Uart_Close(INT iFd)
{
    close(iFd);
}


INT Uart_Set_Para(INT iFd, PT_UartPara ptUartPara)
{
    assert(NULL != ptUartPara);
    INT iRet;
    U32 u32Bps;
    INT iBpsFlag;
    struct termios tTermios;
    
#if 0
    iRet = tcgetattr(iFd, &tTermios);
    if (-1 == iRet)
    {
        log_eno("tcgetattr");
        return -1;
    }
#endif

    memset(&tTermios, 0, sizeof(tTermios));

    /* 1.并选择一个最合适的bps, 并设置bps */
    u32Bps = ptUartPara->u32Bps;
    iBpsFlag = Get_Bps_Para(u32Bps);
    
    cfsetispeed(&tTermios, iBpsFlag);
    cfsetospeed(&tTermios, iBpsFlag);

    /* 2. 设置数据位 */
    tTermios.c_cflag &= ~CSIZE;
    switch (ptUartPara->eDataBit)
    {
        case DATABITS_5:
            tTermios.c_cflag |= CS5;
            break;
        case DATABITS_6:
            tTermios.c_cflag |= CS6;
            break;
        case DATABITS_7:
            tTermios.c_cflag |= CS7;
            break;
        case DATABITS_8:
            tTermios.c_cflag |= CS8;
            break;
        default:
            tTermios.c_cflag |= CS8;
            log_war("DataBit %d Invaild, default bit8\n", ptUartPara->eDataBit);
    }

    /* 3. 设置停止位 */
    if (TWO_STOPBITS == ptUartPara->eStopBit)
    {
        tTermios.c_cflag |= CSTOPB;
    }
    else
    {
        tTermios.c_cflag &= ~CSTOPB;
    }
    
    /* 4. 设置校验位 */
    if (ODD_PARITY_BIT == ptUartPara->eParityBit)
    {
        tTermios.c_cflag |= PARENB;
        tTermios.c_cflag |= PARODD;
        //tTermios.c_iflag |= (INPCK | ISTRIP);    /* 将奇偶校验设置为有效同时从接收字串中脱去奇偶校验位  */
    } 
    else if (EVEN_PARITY_BIT == ptUartPara->eParityBit)
    {
        tTermios.c_cflag |= PARENB;
        tTermios.c_cflag &= ~PARODD;
        //tTermios.c_iflag |= (INPCK | ISTRIP);
    }
    else
    {
        tTermios.c_cflag &= ~PARENB;
    }

    tTermios.c_cflag |= CLOCAL | CREAD;
    /* RAW 模式 */
    tTermios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tTermios.c_oflag &= ~OPOST;
    /* 禁止软件流控制 */
    tTermios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* 设置超时阻塞:
       1. 当VTIME>0，VMIN=0时。read调用读到数据则立即返回，否则将为每个字符最多等待VTIME时间 */
    tTermios.c_cc[VTIME] = UART_RX_TIMEOUT_S * 10 + UART_RX_TIMEOUT_MS / 100;
    tTermios.c_cc[VMIN]  = 0;

    /* 清空接收缓冲区 */
    tcflush(iFd, TCIFLUSH);

    iRet = tcsetattr(iFd, TCSANOW, &tTermios);
    if (-1 == iRet)
    {
        log_eno("tcgetattr\n");
        return -1;
    }
    
    return 0;
}

INT Uart_Set_Timeout(INT iFd, U32 vtime, U32 vmin)
{
    struct termios tTermios;
    INT iRet;
    
    iRet = tcgetattr(iFd, &tTermios);
    if (-1 == iRet)
    {
        log_eno("tcgetattr");
        return -1;
    }
    
    /* 设置超时阻塞:
       1. 当VTIME>0，VMIN=0时。read调用读到数据则立即返回，否则将为每个字符最多等待VTIME时间 */
    tTermios.c_cc[VTIME] = vtime;
    tTermios.c_cc[VMIN]  = vmin;

    iRet = tcsetattr(iFd, TCSANOW, &tTermios);
    if (-1 == iRet)
    {
        log_eno("tcgetattr\n");
        return -1;
    }

    return 0;
}

/* 接收不到n个数据, 会阻塞VTIME时间 */
static ssize_t readn(int fd, void *ptr, size_t n)
{
    size_t   nleft;
    ssize_t  nread;

    nleft = n;
    while (nleft > 0)
    {
        nread = read(fd, ptr, nleft);
        if (nread < 0)
        {
            log_eno("uart readn\n");
            if (nleft == n)
            {
                return -1; /* error, return -1 */
            }
            else
            {
                break;      /* error, return amount read so far */
            }
        }
        else if (nread == 0)
        {
            break;          /* EOF, timeout走这里 */
        }

        nleft -= nread;
        ptr   += nread;
    }
    
    return (n - nleft);      /* return >= 0 */
}


/* 读到数据则立即返回，否则将为每个字符最多等待VTIME时间 */
static ssize_t read_once(int fd, void *ptr, size_t n)
{
    ssize_t  nread;

    nread = read(fd, ptr, n);
    if (nread > 0)
    {
        return nread;
    }
    else if (errno == EAGAIN)
    {
        return 0;
    }
    else if (nread == 0)
    {
        return -EIO;
    }
    
    log_eno("read_once\n");
    return -errno;
}


/* iFlag说明:
 *   NONE_BLOCKING, 没有接收到数据前阻塞VTIME，读到数据则立即返回(并非真实的非阻塞)
 *   BLOCKING，接收到n个数据则返回, 否则会阻塞VTIME时间
 *   差异：非阻塞方式，接收到数据后立刻返回
 */
INT Uart_Recv(INT iFd, CHAR *pcRecvBuf, U32 u32BufLen, INT iFlag)
{
    assert(NULL != pcRecvBuf);
    INT iReadNum;
        
    if (NONE_BLOCKING == iFlag)
    {
        iReadNum = read_once(iFd, pcRecvBuf, u32BufLen);
    }
    else
    {
        iReadNum = readn(iFd, pcRecvBuf, u32BufLen);
    }
    
    if(iReadNum <= 0)
    {
        log_err("uart readn err\n");
        return -1;
    }
    
    return iReadNum;
}

static ssize_t writen(int fd, const void *ptr, size_t n)
{
    size_t  nleft;
    ssize_t nwritten;
    
    nleft = n;
    while (nleft > 0)
    {
        nwritten = write(fd, ptr, nleft);
        if (nwritten < 0)
        {
            log_eno("uart write\n");
            if (nleft == n)
            {
                return (-1);        /* error, return -1 */
            }
            else
            {
                break;              /* error, return amount written so far */
            }
        } 
        else if (nwritten == 0)
        {
            break;
        }
            
        nleft -= nwritten;
        ptr   += nwritten;
    }
    
    return (n - nleft); /* return >= 0 */
}

INT Uart_Send(INT iFd, CHAR *pcSendBuf, U32 u32SendLen)
{
    assert(NULL != pcSendBuf);
    INT iRet;

    iRet = writen(iFd, pcSendBuf, u32SendLen);
    if(-1 == iRet)
    {
        log_err("uart writen err\n");
        return -1;
    }

    return iRet;
}


#if 0
int generic_write(int fd, const char *buf, int n, void *unused)
{
    int err;

    err = write(fd, buf, n);
    if (err > 0)
        return err;
    else if (errno == EAGAIN)
        return 0;
    else if (err == 0)
        return -EIO;
    return -errno;
    }
#endif


VOID Uart_Dummy(INT iFd)
#if 1
{
    tcflush(iFd, TCIFLUSH);
}
#else
{
    INT iRet;
    CHAR acTmpBuf[128];
    INT iletfLen, iReadLen;

    iRet = ioctl(iFd, FIONREAD, &iletfLen);
    if((iRet == -1) || (iletfLen <= 0))
    {
        /* 没有数据需要读取 */
        return;
    }

    while (iletfLen)
    {
        if (iletfLen > sizeof(acTmpBuf))
        {
            iReadLen = sizeof(acTmpBuf);
        }
        else
        {
            iReadLen = iletfLen;
        }

        iReadLen = read(iFd, acTmpBuf, iReadLen);
        if(-1 == iReadLen)
        {
            MsgPrintf("DummyRead error\n");
            return;
        }
        
        iletfLen -= iReadLen;
    }

    return;
}
#endif


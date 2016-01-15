#include "stm32f10x.h"
#include "config.h"
#include "W5500\w5500.h"
#include "W5500\socket.h"
#include "util.h"
#include "APPs\loopback.h"
#include <stdio.h>
#include <string.h>
#include <zc_protocol_controller.h>
#include <zc_timer.h>
#include <zc_module_interface.h>
#include <zc_adpter.h>
#include <stdlib.h>
#include <stdio.h> 
#include <stdarg.h>
#include <ac_cfg.h>
#include <ac_api.h>

#define tick_second 1

extern uint8 ch_status[MAX_SOCK_NUM];
extern CHCONFIG_TYPE_DEF Chconfig_Type_Def;
extern CONFIG_MSG Config_Msg;
extern uint32_t presentTime;
uint16 any_port = 1000;

typedef struct{
  unsigned int start;
  unsigned int interval;
}struZctimer;

struZctimer g_struZcTimer[ZC_TIMER_MAX_NUM];


extern PTC_ProtocolCon  g_struProtocolController;
PTC_ModuleAdapter g_struHfAdapter;

MSG_Buffer g_struRecvBuffer;
MSG_Buffer g_struRetxBuffer;
MSG_Buffer g_struClientBuffer;


MSG_Queue  g_struRecvQueue;
MSG_Buffer g_struSendBuffer[MSG_BUFFER_SEND_MAX_NUM];
MSG_Queue  g_struSendQueue;

u8 g_u8MsgBuildBuffer[MSG_BULID_BUFFER_MAXLEN];
u8 g_u8ClientSendLen = 0;


u16 g_u16TcpMss;
u16 g_u16LocalPort;


u8 g_u8recvbuffer[HF_MAX_SOCKET_LEN];
ZC_UartBuffer g_struUartBuffer;

u8  g_u8BcSendBuffer[100];
u32 g_u32BcSleepCount = 800;
u8 g_u8RemoteAddr[4];

void loopback_tcpc(SOCKET s)
{
        uint16 RSR_len;
        uint16 received_len;
	uint8 * data_buf = TX_BUF;
   uint16 port = 9100;
        u32 u32Timer = 0;
   switch (getSn_SR(s))
   {
   case SOCK_ESTABLISHED:                 /* if connection is established */
      if(ch_status[s]!=2)
      {
         printf("\r\n%d : Connected",s);
         ch_status[s] = 2;
         ZC_Rand(g_struProtocolController.RandMsg);
         PCT_SendCloudAccessMsg1(&g_struProtocolController);
      }
      if (( received_len=getSn_RX_RSR(s)) > 0)        /* check Rx data */
      {
       if (received_len > TX_RX_MAX_BUF_SIZE) received_len = HF_MAX_SOCKET_LEN;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
       received_len = recv(g_struProtocolController.struCloudConnection.u32Socket, g_u8recvbuffer, received_len); 
      
      if(received_len > 0) 
      {
          ZC_Printf("recv data len = %d\n", received_len);
          MSG_RecvDataFromCloud(g_u8recvbuffer, received_len);
      }
      }


     MSG_SendDataToCloud((u8*)&g_struProtocolController.struCloudConnection);


      break;
   case SOCK_CLOSE_WAIT:                                 /* If the client request to close */
      if ((RSR_len = getSn_RX_RSR(s)) > 0)         /* check Rx data */
      {
         if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = TX_RX_MAX_BUF_SIZE;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
                                                                                    /* the data size to read is MAX_BUF_SIZE. */
         received_len = recv(s, data_buf, RSR_len);         /* read the received data */
      }
      disconnect(s);
      u32Timer = rand();
      u32Timer = (PCT_TIMER_INTERVAL_RECONNECT) * (u32Timer % 10 + 1);
      ZC_Printf("disconnect, timer = %d\n", g_struProtocolController.u8MainState, u32Timer);
      PCT_ReconnectCloud(&g_struProtocolController, u32Timer);
      ch_status[s] = 0;
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */

      break;
        case SOCK_INIT:     /* if a socket is initiated */

                break;
        default:
                break;
        }
}

void loopback_udp(SOCKET s, uint16 port)
{
   uint16 RSR_len;
        uint16 received_len;
	uint8 * data_buf = TX_BUF;
   uint32 destip = 0;
   uint16 destport;

   switch (getSn_SR(s))
   {
   case SOCK_UDP:
      if ((RSR_len = getSn_RX_RSR(s)) > 0)         /* check Rx data */
      {
         if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = TX_RX_MAX_BUF_SIZE;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */

         /* the data size to read is MAX_BUF_SIZE. */
         received_len = recvfrom(s, data_buf, RSR_len, (uint8*)&destip, &destport);       /* read the received data */
         if(sendto(s, data_buf, received_len,(uint8*)&destip, destport) == 0) /* send the received data */
         {
            printf("\a\a\a%d : System Fatal Error.", s);
         }
      }
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */
      printf("\r\n%d : Loop-Back UDP Started. port :%d", s, port);
      if(socket(s,Sn_MR_UDP,port,0x00)== 0)    /* reinitialize the socket */
         printf("\a%d : Fail to create socket.",s);
      break;
   }
}

/*************************************************
* Function: timer_set
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void timer_set(struZctimer *t, u32 interval)
{
  t->interval = interval;
  t->start = time_return();
}


/*************************************************
* Function: timer_expired
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
int timer_expired(struZctimer *t)
{
  return time_return() >= t->start + t->interval;
}

/*************************************************
* Function: MX_TimerExpired
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void ZC_TimerExpired()
{
    u8 u8Index;
    u8 u8Status;
    for (u8Index = 0; u8Index < ZC_TIMER_MAX_NUM; u8Index++)
    {   
        TIMER_GetTimerStatus(u8Index, &u8Status);
        if (ZC_TIMER_STATUS_USED == u8Status)
        {
            if (timer_expired(&g_struZcTimer[u8Index]))
            {
                TIMER_StopTimer(u8Index);
                TIMER_TimeoutAction(u8Index);
            }
        }
    }
}

/*************************************************
* Function: HF_ReadDataFormFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_ReadDataFormFlash(void) 
{
    #if 0
    u32 u32MagicFlag = 0xFFFFFFFF;
#ifdef __LPT200__
    hffile_userbin_read(0, (char *)(&u32MagicFlag), 4);
    if (ZC_MAGIC_FLAG == u32MagicFlag)
    {   
        hffile_userbin_read(0, (char *)(&g_struZcConfigDb), sizeof(ZC_ConfigDB));
    }
    else
    {
        ZC_Printf("no para, use default\n");
    }
#else
    hfuflash_read(0, (char *)(&u32MagicFlag), 4);
    if (ZC_MAGIC_FLAG == u32MagicFlag)
    {   
        hfuflash_read(0, (char *)(&g_struZcConfigDb), sizeof(ZC_ConfigDB));
    }
    else
    {
        ZC_Printf("no para, use default\n");
    }
#endif    
#endif
    if (ZC_MAGIC_FLAG == *((u32 *)(0x40000-sizeof(ZC_ConfigDB))))
    {   
         memcpy((char *)(&g_struZcConfigDb),(u8 *)(0x80000000+0x40000-sizeof(ZC_ConfigDB)),sizeof(ZC_ConfigDB));
    }
    else
    {
        ZC_Printf("no para, use default\n");
    }
}

/*************************************************
* Function: HF_WriteDataToFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_WriteDataToFlash(u8 *pu8Data, u16 u16Len)
{
#if 0
#ifdef __LPT200__
    hffile_userbin_write(0, (char*)pu8Data, u16Len);
#else
    hfuflash_erase_page(0,1); 
    hfuflash_write(0, (char*)pu8Data, u16Len);
#endif
#endif
    int i = 0;
    for( i = 0;i<sizeof(ZC_ConfigDB)/4;i++)
    FLASH_ProgramWord(0x80000000+0x40000-sizeof(ZC_ConfigDB) +4*i,*((u32 *)(pu8Data +4*i)));

}

/*************************************************
* Function: HF_StopTimer
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_StopTimer(u8 u8TimerIndex)
{

}

/*************************************************
* Function: HF_SetTimer
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SetTimer(u8 u8Type, u32 u32Interval, u8 *pu8TimeIndex)
{
    u8 u8TimerIndex;
    u32 u32Retval;

    u32Retval = TIMER_FindIdleTimer(&u8TimerIndex);
    if (ZC_RET_OK == u32Retval)
    {
        TIMER_AllocateTimer(u8Type, u8TimerIndex, (u8*)&g_struZcTimer[u8TimerIndex]);
        timer_set(&g_struZcTimer[u8TimerIndex], u32Interval);
        *pu8TimeIndex = u8TimerIndex;

    }
    
    return u32Retval;
}
/*************************************************
* Function: HF_FirmwareUpdateFinish
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_FirmwareUpdateFinish(u32 u32TotalLen)
{
    #if 0
    int retval;
    retval = hfupdate_complete(HFUPDATE_SW, u32TotalLen);
    if (HF_SUCCESS == retval)
    {
        return ZC_RET_OK;
    }
    else
    {
        return ZC_RET_ERROR;    
    }
    #endif
}


/*************************************************
* Function: HF_FirmwareUpdate
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_FirmwareUpdate(u8 *pu8FileData, u32 u32Offset, u32 u32DataLen)
{
#if 0
    int retval;
    if (0 == u32Offset)
    {
        hfupdate_start(HFUPDATE_SW);
    }
    
    retval = hfupdate_write_file(HFUPDATE_SW, u32Offset, (char *)pu8FileData, u32DataLen); 
    if (retval < 0)
    {
        return ZC_RET_ERROR;
    }
#endif    
    return ZC_RET_OK;
}
/*************************************************
* Function: HF_SendDataToMoudle
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SendDataToMoudle(u8 *pu8Data, u16 u16DataLen)
{
#ifdef ZC_MODULE_DEV 
    AC_RecvMessage((ZC_MessageHead *)pu8Data);
#else
    u8 u8MagicFlag[4] = {0x02,0x03,0x04,0x05};
    hfuart_send(HFUART0,(char*)u8MagicFlag,4,1000); 
    hfuart_send(HFUART0,(char*)pu8Data,u16DataLen,1000); 
#endif
    return ZC_RET_OK;
}

/*************************************************
* Function: HF_Rest
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Rest(void)
{
    
    g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;            
    HF_WriteDataToFlash((u8 *)&g_struZcConfigDb, sizeof(ZC_ConfigDB));

}
/*************************************************
* Function: HF_SendTcpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SendTcpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    send(u32Fd, pu8Data, u16DataLen, 0);
}
/*************************************************
* Function: HF_SendUdpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SendUdpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    if(SOCK_UDP==getSn_SR(u32Fd))
    {
    sendto(u32Fd,(const u8 *)pu8Data,u16DataLen,pstruParam->pu8AddrPara,ZC_MOUDLE_BROADCAST_PORT); 
    }
}

/*************************************************
* Function: HF_GetMac
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_GetMac(u8 *pu8Mac)
{
    ZC_HexToString(pu8Mac,Config_Msg.Mac,6);
}

/*************************************************
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Reboot(void)
{
    //hfsys_reset();
}

/*************************************************
* Function: HF_ConnectToCloud
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ConnectToCloud(PTC_Connection *pstruConnection)
{
    int fd = SOCK_TCPC; 
    //u8 ip[4] = {120, 132, 77, 0}; for test
    u8 ip[4] = {123, 57, 206, 235};
    u32 temp;
    int retval;
    u16 port;
    u16 lport;
    if (1 == g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig)
    {
        port = g_struZcConfigDb.struSwitchInfo.u16ServerPort;
        temp = ZC_HTONL(g_struZcConfigDb.struSwitchInfo.u32ServerIp);
        memcpy(ip,&temp,4);
        retval = RET_SUCCESS;
    }
    else
    {
        port = ZC_CLOUD_PORT;
        retval = Dns_Task((const char *)g_struZcConfigDb.struCloudInfo.u8CloudAddr,ip);
        //struIp = ZC_HTONL(0x78844D00);
    }


    if (RET_SUCCESS != retval)
    {
        return ZC_RET_ERROR;
    }
    lport = rand();
    if(SOCK_CLOSED==(getSn_SR(fd)))
    {
    retval = socket(fd,Sn_MR_TCP,lport,0x00);
    }
    if (RET_SUCCESS != retval)
    {
        return ZC_RET_ERROR;
    }
    if(SOCK_INIT==(getSn_SR(fd)))
    {
    if(time_return() - presentTime >= (10*tick_second))
    {  /* For TCP client's connection request delay : 1 sec */
        connect(fd, ip, port); /* Try to connect to TCP server(Socket, DestIP, DestPort) */
        presentTime = time_return();
        if(g_struProtocolController.struCloudConnection.u32ConnectionTimes++>20)
        {
           g_struZcConfigDb.struSwitchInfo.u32ServerAddrConfig = 0;
        }
    }
        return ZC_RET_ERROR;
    }


    

    g_struProtocolController.struCloudConnection.u32Socket = fd;

    
    ZC_Rand(g_struProtocolController.RandMsg);

    return ZC_RET_ERROR;
}
/*************************************************
* Function: HF_ListenClient
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ListenClient(PTC_Connection *pstruConnection)
{
    #if 0
    int fd; 
    struct sockaddr_in servaddr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0)
        return ZC_RET_ERROR;

    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
    servaddr.sin_port = htons(pstruConnection->u16Port);
    if(bind(fd,(struct sockaddr *)&servaddr,sizeof(servaddr))<0)
    {
        close(fd);
        return ZC_RET_ERROR;
    }
    
    if (listen(fd, TCP_DEFAULT_LISTEN_BACKLOG)< 0)
    {
        close(fd);
        return ZC_RET_ERROR;
    }

    ZC_Printf("Tcp Listen Port = %d\n", pstruConnection->u16Port);
    g_struProtocolController.struClientConnection.u32Socket = fd;
#endif
    return ZC_RET_OK;
}

/*************************************************
* Function: HF_BcInit
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_BcInit()
{
    #if 0
    int tmp=1;
    struct sockaddr_in addr; 

    addr.sin_family = AF_INET; 
    addr.sin_port = htons(ZC_MOUDLE_PORT); 
    addr.sin_addr.s_addr=htonl(INADDR_ANY);

    g_Bcfd = socket(AF_INET, SOCK_DGRAM, 0); 

    tmp=1; 
    setsockopt(g_Bcfd, SOL_SOCKET,SO_BROADCAST,&tmp,sizeof(tmp)); 

    hfnet_set_udp_broadcast_port_valid(ZC_MOUDLE_PORT, ZC_MOUDLE_PORT + 1);

    bind(g_Bcfd, (struct sockaddr*)&addr, sizeof(addr)); 
    g_struProtocolController.u16SendBcNum = 0;

    memset((char*)&struRemoteAddr,0,sizeof(struRemoteAddr));
    struRemoteAddr.sin_family = AF_INET; 
    struRemoteAddr.sin_port = htons(ZC_MOUDLE_BROADCAST_PORT); 
    struRemoteAddr.sin_addr.s_addr=inet_addr("255.255.255.255"); 
    g_pu8RemoteAddr = (u8*)&struRemoteAddr;
    g_u32BcSleepCount = 250000;
#endif
    if(SOCK_CLOSED==(getSn_SR(g_Bcfd))||(SOCK_INIT==(getSn_SR(g_Bcfd))))
    {
       if(RET_SUCCESS==socket(SOCK_UDPC,Sn_MR_UDP,ZC_MOUDLE_PORT,0x00))
        {
            g_Bcfd = SOCK_UDPC;
            g_pu8RemoteAddr = g_u8RemoteAddr;
            memset(g_pu8RemoteAddr,0xff,4);
        }
    }
    g_u32BcSleepCount = 12000;
    return;
}

/*************************************************
* Function: HF_Cloudfunc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Cloudfunc() 
{
    u32 u32Timer = 0;
    g_struProtocolController.struCloudConnection.u32Socket = SOCK_TCPC;
    HF_BcInit();
    ZC_TimerExpired();

    PCT_Run();

    loopback_tcpc(SOCK_TCPC);

    ZC_SendBc();

}


void ZC_Init()
{
    //网络通信接口
    g_struHfAdapter.pfunConnectToCloud = HF_ConnectToCloud;
    g_struHfAdapter.pfunListenClient = HF_ListenClient;
    g_struHfAdapter.pfunSendTcpData = HF_SendTcpData; 
    g_struHfAdapter.pfunSendUdpData = HF_SendUdpData;     
    g_struHfAdapter.pfunUpdate = HF_FirmwareUpdate;  
    //设备内部通信接口
    g_struHfAdapter.pfunSendToMoudle = HF_SendDataToMoudle; 
    //定时器类接口
    g_struHfAdapter.pfunSetTimer = HF_SetTimer;   
    g_struHfAdapter.pfunStopTimer = HF_StopTimer;        
    //存储类接口
    g_struHfAdapter.pfunUpdateFinish = HF_FirmwareUpdateFinish;
    g_struHfAdapter.pfunWriteFlash = HF_WriteDataToFlash;
    //系统类接口    
    g_struHfAdapter.pfunRest = HF_Rest;    
    g_struHfAdapter.pfunGetMac = HF_GetMac;
    g_struHfAdapter.pfunReboot = HF_Reboot;
    g_struHfAdapter.pfunMalloc = (pFunMalloc)malloc;
    g_struHfAdapter.pfunFree = free;
    g_struHfAdapter.pfunPrintf = (pFunPrintf)printf;
    g_u16TcpMss = 1000;
    PCT_Init(&g_struHfAdapter);
    
    ZC_Printf("MT Init\n");
    
    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;

}

/*************************************************
* Function: HF_WakeUp
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_WakeUp()
{
    PCT_WakeUp();
}

/*************************************************
* Function: HF_Sleep
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Sleep()
{
    u32 u32Index;
    
    close(g_Bcfd);

    if (PCT_INVAILD_SOCKET != g_struProtocolController.struClientConnection.u32Socket)
    {
        close(g_struProtocolController.struClientConnection.u32Socket);
        g_struProtocolController.struClientConnection.u32Socket = PCT_INVAILD_SOCKET;
    }

    if (PCT_INVAILD_SOCKET != g_struProtocolController.struCloudConnection.u32Socket)
    {
        close(g_struProtocolController.struCloudConnection.u32Socket);
        g_struProtocolController.struCloudConnection.u32Socket = PCT_INVAILD_SOCKET;
    }
    
    for (u32Index = 0; u32Index < ZC_MAX_CLIENT_NUM; u32Index++)
    {
        if (0 == g_struClientInfo.u32ClientVaildFlag[u32Index])
        {
            close(g_struClientInfo.u32ClientFd[u32Index]);
            g_struClientInfo.u32ClientFd[u32Index] = PCT_INVAILD_SOCKET;
        }
    }

    PCT_Sleep();
    
    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;
}

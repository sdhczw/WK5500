#include "stm32f10x.h"
#include "config.h"
#include "W5500\w5500.h"
#include "W5500\socket.h"
#include "util.h"
#include "dns.h"
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
#ifdef ZC_HTTPOTA 
#include <zc_http.h>
#endif

#define tick_second 1
#define LINK_ON  1
#define LINK_OFF 0
extern uint8 ch_status[MAX_SOCK_NUM];
extern CHCONFIG_TYPE_DEF Chconfig_Type_Def;
extern CONFIG_MSG Config_Msg;
extern uint32_t presentTime;
extern int g_NtpFd;

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
#ifdef ZC_HTTPOTA
vu8 g_vu8HttpConnectFlag = 0;
vu8 g_vu8HttpDnsOk = 0;
#endif
struct sockaddr_in
{
	u8 addr[4];
	unsigned short port;
};
struct sockaddr_in g_struNtpAddr;
struct sockaddr_in g_struRemoteAddr;
/*************************************************
* Function: loopback_tcpc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void loopback_tcpc(SOCKET s)
{
 uint16 RSR_len;
    uint16 received_len;
	uint8 * data_buf = g_u8recvbuffer;
    u32 u32Timer = 0;
    s32 s32LeftLen;
    u16 u16PacketLen = 0;
    ZC_SecHead struSecHead;
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
      if (( RSR_len=getSn_RX_RSR(s)) > 0)        /* check Rx data */
      {
       if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = HF_MAX_SOCKET_LEN;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
       received_len = recv(g_struProtocolController.struCloudConnection.u32Socket, g_u8recvbuffer, RSR_len); 
        u8 *data = g_u8recvbuffer;
          s32LeftLen = received_len; 
         while (s32LeftLen > sizeof(ZC_SecHead) - 1)
        {
            memcpy((u8 *)&struSecHead, data, sizeof(ZC_SecHead));
            u16PacketLen = ZC_HTONS(struSecHead.u16TotalMsg) + sizeof(ZC_SecHead);
            if (s32LeftLen <  u16PacketLen)
            {
                ZC_Printf("recv data not enough,data len = %d,packet len =%d\n",s32LeftLen,u16PacketLen);
                break; 
            }
            ZC_Printf("recv data len = %d\n", u16PacketLen);
            MSG_RecvDataFromCloud(data, u16PacketLen);
            s32LeftLen -= u16PacketLen;
            data = data + u16PacketLen;
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
         if(received_len > 0) 
         {
              ZC_Printf("recv data len = %d\n", received_len);
              MSG_RecvDataFromCloud(g_u8recvbuffer, received_len);
          }
      }
      disconnect(s);
      u32Timer = rand();
      u32Timer = (PCT_TIMER_INTERVAL_RECONNECT) * (u32Timer % 10 + 1);
      ZC_Printf("CLOSE_WAIT:disconnect, timer = %d\r\n", u32Timer);
      PCT_ReconnectCloud(&g_struProtocolController, u32Timer);
      ch_status[s] = 0;
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */
   {
      // static
        //printf("\r\n%d : Closed",s);
       uint8 Linkstatus;
       static uint32 i = 0;
       getPHYCFGR(&Linkstatus);
       if(i++>10000)
       {
          printf("\r\nPHYCFGR = %x",Linkstatus);
           i = 0;
       }
       if(Linkstatus&LINK_ON)
       {               
           if(ch_status[s] == 2)//主动断链，重连
           {
               u32Timer = rand();
               u32Timer = (PCT_TIMER_INTERVAL_RECONNECT) * (u32Timer % 10 + 1);
               ZC_Printf("CLOSED:disconnect, timer = %d\r\n", u32Timer);
               PCT_ReconnectCloud(&g_struProtocolController, u32Timer);
               ch_status[s] = 0;
           } 
       }
       else
       {
          HF_Sleep(); 
       }

      break;
   }
        case SOCK_INIT:     /* if a socket is initiated */

                break;
        default:
                break;
        }
}
#ifdef ZC_HTTPOTA 
/*************************************************
* Function: loopback_tcpc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void loopback_tcphttp(SOCKET s)
{
   uint16 RSR_len;
    uint16 received_len;
	uint8 * data_buf = g_u8recvbuffer;
   switch (getSn_SR(s))
   {
   case SOCK_ESTABLISHED:                 /* if connection is established */
      if(ch_status[s]!=2)
      {
         printf("\r\n%d : Connected",s);
         ch_status[s] = 2;
         g_vu8HttpConnectFlag = 1;
      }
      if (( RSR_len=getSn_RX_RSR(s)) > 0)        /* check Rx data */
      {
       if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = HF_MAX_SOCKET_LEN;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
       received_len = recv(s, g_u8recvbuffer, RSR_len); 
      
      if(received_len > 0) 
      {
            ZC_Printf("recv http data len = %d\n", received_len);
            ZC_HandleHttpOta(g_u8recvbuffer, received_len);;
      }
      }

      break;
   case SOCK_CLOSE_WAIT:                                 /* If the client request to close */
      if ((RSR_len = getSn_RX_RSR(s)) > 0)         /* check Rx data */
      {
         if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = TX_RX_MAX_BUF_SIZE;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */
                                                                                    /* the data size to read is MAX_BUF_SIZE. */
         received_len = recv(s, data_buf, RSR_len);         /* read the received data */
         if(received_len > 0) 
         {
			 ZC_Printf("recv http data len = %d\n", received_len);
			 ZC_HandleHttpOta(g_u8recvbuffer, received_len);;
          }
      }
      //disconnect(s);
      g_vu8HttpConnectFlag = 0;
      g_vu8HttpDnsOk = 0;
      // ZC_Printf(" http closed wait");
	  //g_struServerInfo.socketFd = HTTP_INVALID_FD;
	  //ZC_HttpOtaRelease(HTTP_DEFAULT_RELEASE_SOURCE);
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */
   {
      //printf("\r\n HTTP CLOSED");
              uint8 Linkstatus;
       static uint32 i = 0;
       getPHYCFGR(&Linkstatus);
       if(i++>10000)
       {
          //printf("\r\nPHYCFGR = %x",Linkstatus);
           ZC_Printf(" http closed");
           i = 0;
       }
       if(Linkstatus&LINK_ON)
       {               
           if(ch_status[s] == 2)//主动断链，重连
           {
             //ZC_HttpOtaRelease(HTTP_DEFAULT_RELEASE_SOURCE);;
           } 
       }
	  g_vu8HttpConnectFlag = 0; 
      break;
   }
        case SOCK_INIT:     /* if a socket is initiated */
         printf("\r\n HTTP INIT");
                break;
        default:
                break;
        }
}
#endif
/*************************************************
* Function: loopback_tcps
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void loopback_tcps(SOCKET s)
{
    uint16 RSR_len;
    uint16 received_len;
	uint8 * data_buf = g_u8recvbuffer;
     switch (getSn_SR(s))
   {
   case SOCK_ESTABLISHED:              /* if connection is established */
      if(ch_status[s]==1)
      {
         printf("\r\n%d : Connected",s);
         printf("\r\n Peer IP : %d.%d.%d.%d", IINCHIP_READ(Sn_DIPR0(s)),  IINCHIP_READ(Sn_DIPR1(s)), IINCHIP_READ(Sn_DIPR2(s)), IINCHIP_READ(Sn_DIPR3(s)) );
         printf("\r\n Peer Port : %d", ( (uint16)(IINCHIP_READ(Sn_DPORT0(s)))<<8) +(uint16)IINCHIP_READ( Sn_DPORT1(s)) ) ;
         ch_status[s] = 2;
         ZC_ClientConnect(s);
      }

      if ((RSR_len = getSn_RX_RSR(s)) > 0)        /* check Rx data */
      {
         if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = TX_RX_MAX_BUF_SIZE;   /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */                                                                           /* the data size to read is MAX_BUF_SIZE. */
         received_len = recv(s, data_buf, RSR_len);      /* read the received data */
         ZC_RecvDataFromClient(s, data_buf, received_len);
      }

      break;
   case SOCK_CLOSE_WAIT:                              /* If the client request to close */
      printf("\r\n%d : CLOSE_WAIT", s);
        if ((RSR_len = getSn_RX_RSR(s)) > 0)     /* check Rx data */
      {
                   if (RSR_len > TX_RX_MAX_BUF_SIZE) RSR_len = TX_RX_MAX_BUF_SIZE;  /* if Rx data size is lager than TX_RX_MAX_BUF_SIZE */                                                                     /* the data size to read is MAX_BUF_SIZE. */
                   received_len = recv(s, data_buf, RSR_len);     /* read the received data */
                   ZC_RecvDataFromClient(s, data_buf, received_len);
      }
      disconnect(s);
      ZC_ClientDisconnect(s);
      ch_status[s] = 0;
      break;
   case SOCK_CLOSED:                                       /* if a socket is closed */
      if(!ch_status[s])
      {
                   printf("\r\n%d : Loop-Back TCP Server Started. port : %d", s, ZC_SERVER_PORT);
                   ch_status[s] = 1;
      }
      if(socket(s,(Sn_MR_ND|Sn_MR_TCP),ZC_SERVER_PORT,0x00) == 0)    /* reinitialize the socket */
      {
                   printf("\r\n%d : Fail to create socket.",s);
                   ch_status[s] = 0;
      }
      else
      {
        g_struProtocolController.struClientConnection.u32Socket = s;
        ZC_StartClientListen();
      }
      break;
        case SOCK_INIT:   /* if a socket is initiated */

                  listen(s);
                  printf("\r\n%x :LISTEN socket %d ",getSn_SR(s), s);
                break;
        default:
                break;
   }
}
/*************************************************
* Function: loopback_udp
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void loopback_udp(SOCKET s)
{
   uint16 RSR_len;
   uint16 received_len;
   uint8 * data_buf = g_u8BcSendBuffer;
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
         ZC_SendClientQueryReq(data_buf, (u16)received_len);

      }
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */
      printf("\r\n%d : Loop-Back Discovery UDP Started. port :%d", s, ZC_MOUDLE_PORT);
      if(socket(s,Sn_MR_UDP,ZC_MOUDLE_PORT,0x00)== 0)    /* reinitialize the socket */
         printf("\a%d : Fail to create socket.",s);
      else
      {
          g_Bcfd = SOCK_UDPC;
          memset(g_struRemoteAddr.addr,0xff,4);
          g_struRemoteAddr.port = ZC_MOUDLE_BROADCAST_PORT;
          g_pu8RemoteAddr = (u8*)&g_struRemoteAddr;
          memset(g_pu8RemoteAddr,0xff,4);
      }
      break;
   }
}
/*************************************************
* Function: loopback_udp
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void loopback_udpntp(SOCKET s)
{
   uint16 RSR_len;
   uint16 received_len;
   uint8 * data_buf = g_u8BcSendBuffer;
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
         ZC_RecvNtpTime(data_buf, received_len);

      }
      break;
   case SOCK_CLOSED:                                               /* if a socket is closed */
      printf("\r\n%d : Loop-Back NTP UDP Started. port :%d", s, ZC_CLIENT_NTP_PORT);
      if(socket(s,Sn_MR_UDP,ZC_CLIENT_NTP_PORT,0x00)== 0)    /* reinitialize the socket */
         printf("\a%d : Fail to create socket.",s);
      else
      {
          g_NtpFd = SOCK_UDPNTP;
      }
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
u32 HF_ReadDataFromFlash(u8 *pu8Data, u16 u16Len) 
{
    FLASH_Unlock(); 
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPTERR | FLASH_FLAG_WRPRTERR | FLASH_FLAG_PGERR);
 
    memcpy((char *)(&g_struZcConfigDb),(void *)(0x08000000+0x40000-sizeof(ZC_ConfigDB)),sizeof(ZC_ConfigDB));

    FLASH_Lock();
    return ZC_RET_OK;
}

/*************************************************
* Function: HF_WriteDataToFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_WriteDataToFlash(u8 *pu8Data, u16 u16Len)
{
    int i = 0;
    FLASH_Unlock();
    FLASH_ClearFlag(FLASH_FLAG_EOP| FLASH_FLAG_OPTERR|FLASH_FLAG_PGERR|FLASH_FLAG_WRPRTERR);
    FLASH_ErasePage(0x08000000+0x40000-sizeof(ZC_ConfigDB));
    for( i = 0;i<sizeof(ZC_ConfigDB)/4;i++)
    FLASH_ProgramWord(0x08000000+0x40000-sizeof(ZC_ConfigDB) +4*i,*((u32 *)(pu8Data +4*i)));
    FLASH_Lock();
    return ZC_RET_OK;
}
/*************************************************
* Function: HF_ReadLicenseFromFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ReadLicenseFromFlash(u8 *pu8Data, u16 u16Len) 
{
    return ZC_RET_OK;	
}
/*************************************************
* Function: HF_WriteLicenseToFlash
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_WriteLicenseToFlash(u8 *pu8Data, u16 u16Len)
{
    return ZC_RET_OK;
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
    return ZC_RET_OK;
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

}
/*************************************************
* Function: HF_SendTcpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SendTcpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    send(u32Fd, pu8Data, u16DataLen, 0);
    return ZC_RET_OK;
}
/*************************************************
* Function: HF_SendUdpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_SendUdpData(u32 u32Fd, u8 *pu8Data, u16 u16DataLen, ZC_SendParam *pstruParam)
{
    struct sockaddr_in *addr;
   	addr = (struct sockaddr_in *)pstruParam->pu8AddrPara;
    if(SOCK_UDP==getSn_SR(u32Fd))
    {
       sendto(u32Fd,(const u8 *)pu8Data,u16DataLen,pstruParam->pu8AddrPara,addr->port); 
    }
    return ZC_RET_OK;
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
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SleepOn(void)
{
    //hfsys_reset();
}
/*************************************************
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SleepOff(void)
{
    //hfsys_reset();
}
/*************************************************
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SetSoftAp(u8 *pu8Data, u16 u16Len)
{
    //hfsys_reset();
}
/*************************************************
* Function: HF_Reboot
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ConfigWifi(char *chssid, char *chPassword)
{
    //hfsys_reset();
    return ZC_RET_OK;
}
/*************************************************
* Function: HF_SetStation
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_SetStation()
{
    //hfsys_reset();
}

 u16 HF_GetApList(u8 *pu8ApList)
{
    return 1;
}
/*************************************************
* Function: HF_GetApList
* Description: not support wep
* Author: cxy 
* Returns: 
* Parameter: 0:easylink,1:ap
* History:
*************************************************/
u32 HF_GetWifiMode()
{
    ZC_Printf("STA mode\r\n");
    return  ZC_WMODE_STATION;       
}

/*************************************************
* Function: HF_GetLinkQuality
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
s8 HF_GetLinkQuality()
{

   return  100;

}

/*************************************************
* Function: HF_ConnectToCloud
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ConnectToCloud(PTC_CloudConnection *pstruConnection)
{
    int fd = SOCK_TCPC; 
    //u8 ip[4] = {120, 132, 77, 0}; for test
    u8 ip[4] = {0};
    u32 temp;
    int retval;
    u16 port;
    u16 lport;
    if (1 == g_struZcConfigDb.struSwitchInfo.u8ServerAddrConfig)
    {
        port = g_struZcConfigDb.struSwitchInfo.u16ServerPort;
        temp = ZC_HTONL(g_struZcConfigDb.struSwitchInfo.u32ServerIp);
        memcpy(ip,&temp,4);
        g_struNtpAddr.port = NTP_Port;
        memcpy(g_struNtpAddr.addr,&ip,4);
        g_pu8NtpAddr = (u8*)&g_struNtpAddr; 
        retval = RET_SUCCESS;
    }
    else
    {
        port = ZC_CLOUD_PORT;
        retval = Dns_Task(g_struProtocolController.struCloudConnection.u8CloudAddr,ip);
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
    if(time_return() - presentTime >= (1000*tick_second))
    {  /* For TCP client's connection request delay : 1 sec */
        connect(fd, ip, port); /* Try to connect to TCP server(Socket, DestIP, DestPort) */
        presentTime = time_return();
        if(g_struProtocolController.struCloudConnection.u32ConnectionTimes++>20)
        {
           g_struZcConfigDb.struSwitchInfo.u8ServerAddrConfig = 0;
        }
    }
        return ZC_RET_ERROR;
    }


    

    g_struProtocolController.struCloudConnection.u32Socket = fd;

    
    ZC_Rand(g_struProtocolController.RandMsg);

    return ZC_RET_ERROR;
}


#ifdef ZC_HTTPOTA 
/*************************************************
* Function: HF_HttpConnectServer
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
s32 HF_HttpConnectServer(s8 *ps8Host, void *pvInfo)
{
 
 int fd = SOCK_TCPHTTPOTA; 
  //u8 ip[4] = {120, 132, 77, 0}; for test
  u8 static ip[4] = {0};
  int retval;
  u16 port;
  u16 lport;
   if(time_return() - presentTime <(3000*tick_second))
   {
          
       return HTTP_OTA_STATUS_ERROR;
    }
    
    presentTime = time_return();
      /* For TCP client's connection request delay : 1 sec */
  if(1==g_vu8HttpConnectFlag)
  {
	  ZC_Printf("connect http server ok!\n");
	  return fd;
  }

    ZC_OtaApp *pOtaApp = (ZC_OtaApp *)pvInfo;
    port = pOtaApp->port;
	if(0==g_vu8HttpDnsOk)
	{
		retval = Dns_Task((u8 *)ps8Host,ip);
	    if (RET_SUCCESS != retval)
		{
		   ZC_Printf("%d : dns error.\n",fd);
		  return HTTP_OTA_STATUS_ERROR;
		}
        g_vu8HttpDnsOk = 1;
	}
 

    lport = rand();
    if(SOCK_CLOSED==(getSn_SR(fd)))
    {
    retval = socket(fd,Sn_MR_TCP,lport,0x00);
    }
    if (RET_SUCCESS != retval)
    {
        ZC_Printf("%d : Fail to create socket.\n",fd);
      return HTTP_OTA_STATUS_ERROR;
    }
    if(SOCK_INIT==(getSn_SR(fd)))
    {
    //if(time_return() - presentTime >= (10*tick_second))
    {  /* For TCP client's connection request delay : 1 sec */
      connect(fd, ip, port); /* Try to connect to TCP server(Socket, DestIP, DestPort) */
      //presentTime = time_return();
    }
      return HTTP_OTA_STATUS_ERROR;
    }

  return HTTP_OTA_STATUS_ERROR;
}
/*************************************************
* Function: HF_SendTcpData
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_CloseSocket(s32 s32Fd)
{
    close(s32Fd);
}

#endif

/*************************************************
* Function: HF_ListenClient
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
u32 HF_ListenClient(PTC_ClientConnection *pstruConnection)
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
* Function: HF_Cloudfunc
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_Cloudfunc() 
{
    g_struProtocolController.struCloudConnection.u32Socket = SOCK_TCPC;
    //HF_BcInit();
    ZC_TimerExpired();

    PCT_Run();
#ifdef ZC_HTTPOTA 
        ZC_HttpOtaRun();
#endif
    loopback_tcpc(SOCK_TCPC);
    loopback_tcps(SOCK_TCPS);
#if ZC_HTTPOTA
    loopback_tcphttp(SOCK_TCPHTTPOTA);
#endif
    loopback_udp(SOCK_UDPC);
    loopback_udpntp(SOCK_UDPNTP);
    ZC_SendBc();

}
#ifdef ZC_HTTPOTA 
/*************************************************
* Function: HF_LockMutex
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_LockMutex(void)
{
    //hfthread_mutext_lock(g_struOtaMutex);
}
/*************************************************
* Function: HF_UnlockMutex
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void HF_UnlockMutex(void)
{
    //hfthread_mutext_unlock(g_struOtaMutex);
}

/*************************************************
* Function: HF_UnlockMutex
* Description: 
* Author: cxy 
* Returns: 
* Parameter: 
* History:
*************************************************/
void AC_UartSend(u8 *pu8Buf, u32 u32Len)
{
    //hfthread_mutext_unlock(g_struOtaMutex);
}
#endif


void ZC_Init()
{
    //网络通信接口
    g_struHfAdapter.pfunConnectToCloud = HF_ConnectToCloud;
    g_struHfAdapter.pfunListenClient = HF_ListenClient;
    g_struHfAdapter.pfunSendTcpData = HF_SendTcpData; 
    g_struHfAdapter.pfunSendUdpData = HF_SendUdpData;     
    //设备内部通信接口
    g_struHfAdapter.pfunSendToMoudle = HF_SendDataToMoudle; 
    //定时器类接口
    g_struHfAdapter.pfunSetTimer = HF_SetTimer;   
    g_struHfAdapter.pfunStopTimer = HF_StopTimer;        
    //存储类接口
    g_struHfAdapter.pfunUpdate = HF_FirmwareUpdate;  
    g_struHfAdapter.pfunUpdateFinish = HF_FirmwareUpdateFinish;
    g_struHfAdapter.pfunWriteFlash = HF_WriteDataToFlash;
    g_struHfAdapter.pfunReadFlash = HF_ReadDataFromFlash;
    g_struHfAdapter.pfunWriteLicense = HF_WriteLicenseToFlash;
    g_struHfAdapter.pfunReadLicense = HF_ReadLicenseFromFlash;
    //系统类接口    
    g_struHfAdapter.pfunRest = HF_Rest;    
    g_struHfAdapter.pfunGetMac = HF_GetMac;
    g_struHfAdapter.pfunReboot = HF_Reboot;
    g_struHfAdapter.pfunSleepOn = HF_SleepOn;
    g_struHfAdapter.pfunSleepOff = HF_SleepOff;
    g_struHfAdapter.pfunSetSoftAp = HF_SetSoftAp;
    g_struHfAdapter.pfunSetStation = HF_SetStation;
    g_struHfAdapter.pfunConfigWifi = HF_ConfigWifi;
    g_struHfAdapter.pfunGetApList = HF_GetApList;
    g_struHfAdapter.pfunGetWifiMode = HF_GetWifiMode;
	g_struHfAdapter.pfunGetLinkQuality = HF_GetLinkQuality;
    g_struHfAdapter.pfunGetMsTime = time_return;
    g_struHfAdapter.pfunMalloc = (pFunMalloc)malloc;
    g_struHfAdapter.pfunFree = free;
    g_struHfAdapter.pfunPrintf = (pFunPrintf)printf;
#ifdef ZC_HTTPOTA 
    g_struHfAdapter.pfunHttpConnectServer = HF_HttpConnectServer;
    g_struHfAdapter.pfunCloseSocket = HF_CloseSocket;
    g_struHfAdapter.pfunLockMutex = HF_LockMutex;
    g_struHfAdapter.pfunUnlockMutex = HF_UnlockMutex;
    g_struHfAdapter.pfunUartSend = AC_UartSend;
#endif
    g_u16TcpMss = 1000;
    PCT_Init(&g_struHfAdapter);
    
    ZC_Printf("MT Init\n");
    
    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;
    PCT_SetDeviceType(PCT_DEVICE_WIFI,(u8 *)"W5500");

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

    if (PCT_INVALID_SOCKET != g_struProtocolController.struClientConnection.u32Socket)
    {
        close(g_struProtocolController.struClientConnection.u32Socket);
        g_struProtocolController.struClientConnection.u32Socket = PCT_INVALID_SOCKET;
    }

    if (PCT_INVALID_SOCKET != g_struProtocolController.struCloudConnection.u32Socket)
    {
        close(g_struProtocolController.struCloudConnection.u32Socket);
        g_struProtocolController.struCloudConnection.u32Socket = PCT_INVALID_SOCKET;
    }
    
    for (u32Index = 0; u32Index < ZC_MAX_CLIENT_NUM; u32Index++)
    {
        if (0 == g_struClientInfo.u32ClientVaildFlag[u32Index])
        {
            close(g_struClientInfo.u32ClientFd[u32Index]);
            g_struClientInfo.u32ClientFd[u32Index] = PCT_INVALID_SOCKET;
        }
    }

    PCT_Sleep();
    
    g_struUartBuffer.u32Status = MSG_BUFFER_IDLE;
    g_struUartBuffer.u32RecvLen = 0;
}

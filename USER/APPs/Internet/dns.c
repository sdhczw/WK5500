/**
******************************************************************************
* @file    			DNS.c
* @author  			WIZnet Software Team 
* @version 			V1.0
* @date    			2015-02-14
* @brief   			域名解析客户端函数 通过解析域名Domain_name可获得其IP地址  
******************************************************************************
*/

#include <stdio.h>
#include <string.h>	

#include "w5500.h"
#include "socket.h"

#include "dhcp.h"
#include "dns.h"
#include "util.h"
//#include "config.h"
/* 局部变量  ----------------------------------------------------------*/
uint8   dns_get_ip[4];
uint16  MSG_ID = 0x1122;

uint8 dns_ok=0;
uint8 dns_wait_time = 0;
uint8 dns_retry_cnt=0;


/* 局部函数  ----------------------------------------------------------*/
int     Dns_makequery(uint16 op, uint8 * name, uint8 * buf, uint16 len);
int     Dns_parse_name(uint8 * msg, uint8 * compressed, /*char * buf,*/ uint16 len);
uint8*  Dns_question(uint8 * msg, uint8 * cp);
uint8*  Dns_answer(uint8 * msg, uint8 * cp);
uint8   Dns_parseMSG(struct dhdr * pdhdr, uint8 * pbuf);
uint8   Dns_query(uint8 s, uint8 * name);

/*******************************************************************************
* 名称: do_dns
* 功能: 查询DNS报文信息，解析来自DNS服务器的回复
* 形参: 域名 *domain_name    
* 返回: 成功: 返回1, 失败 :返回 -1
* 说明: 
*******************************************************************************/
uint8 Dns_Task(uint8* name,uint8* rip)
{
  struct dhdr dhp;																												/*定义一个结构体用来包含报文头信息*/
  uint8 ip[4];
  uint16 len, port;
  uint8 BUFPUB[MAX_DNS_BUF_SIZE];
  
  switch(getSn_SR(SOCK_DNS))																											/*获取socket状态*/
  {
    case SOCK_CLOSED:
      socket(SOCK_DNS, Sn_MR_UDP, 3000, 0);																			/*打开W5500的socket的3000端口，并设置为UDP模式*/
      break;     
    case SOCK_UDP:
      /*socket已打开*/
      if(dns_wait_time==0)
      {
        if(++dns_retry_cnt==DNS_RETRY)
        {
          dns_retry_cnt = 0;
          dns_ok=DNS_RET_FAIL;
          close(SOCK_DNS);
          printf("dns count out\r\n");
          return DNS_RET_FAIL;
        }          
        dns_wait_time = DNS_RESPONSE_TIMEOUT;
        len = Dns_makequery(0, name, BUFPUB, MAX_DNS_BUF_SIZE);	/*接收DNS请求报文并存入BUFPUB*/
        sendto(SOCK_DNS, BUFPUB, len, EXTERN_DNS_SERVERIP, IPPORT_DOMAIN);	/*发送DNS请求报文给DNS服务器*/
        
      }
      if ((len = getSn_RX_RSR(SOCK_DNS)) > 0)
      {
        if (len > MAX_DNS_BUF_SIZE) 
          len = MAX_DNS_BUF_SIZE;
        len = recvfrom(SOCK_DNS, BUFPUB, len, ip, &port);												/*接收UDP传输的数据并存入BUFPUB*/
        if(Dns_parseMSG(&dhp, BUFPUB))																				/*解析DNS响应信息*/
        {
          dns_ok=1;
          memcpy(rip,dns_get_ip,4);
          close(SOCK_DNS);	/*关闭socket*/
          printf("Get [%s]'s IP address [%d.%d.%d.%d] from %d.%d.%d.%d\r\n",
          name,
          rip[0],rip[1],rip[2],rip[3],
          Config_Msg.DNS_Server_IP[0],Config_Msg.DNS_Server_IP[1],Config_Msg.DNS_Server_IP[2],Config_Msg.DNS_Server_IP[3]); 
          return DNS_RET_SUCCESS;																					/*返回DNS解析成功域名信息*/          
        }
        else 
          dns_wait_time = 0;														/*等待响应时间设置为超时*/
      }
      break;
      
    }
//  printf("dns fail\r\n");
  return DNS_RET_FAIL;
    
}

/*******************************************************************************
* 名称: Dns_makequery
* 功能: 生成DNS查询报文
* 形参: len   - buf的长度 
        buf  - 接收DNS信息的
        name - 指向域名的指针
        op   - 期望的数据
* 返回: 返回DNS报文指针
* 说明: 
*******************************************************************************/
int Dns_makequery(uint16 op, uint8 * name, uint8 * buf, uint16 len)
{
  uint8  *cp;
  uint8   *cp1;
  uint8  *dname;
  uint16 p;
  uint16 dlen;
  
  cp = buf;
  
  MSG_ID++;
  *(uint16*)&cp[0] = htons(MSG_ID);
  p = (op << 11) | 0x0100;
  *(uint16*)&cp[2] = htons(p);
  *(uint16*)&cp[4] = htons(1);
  *(uint16*)&cp[6] = htons(0);
  *(uint16*)&cp[8] = htons(0);
  *(uint16*)&cp[10]= htons(0);
  
  cp += sizeof(uint16)*6;
  //	strcpy(sname, name);
  dname = name;
  dlen = strlen((char*)dname);
  for (;;)
  {
    /* Look for next dot */
    cp1 = (unsigned char*)strchr((char*)dname, '.');
    
    if (cp1) len = cp1 - dname;																						/* More to come */
    else len = dlen;																											/* Last component */
    
    *cp++ = len;																													/* Write length of component */
    if (len == 0) break;
    
    strncpy((char *)cp, (char*)dname, len);																/* Copy component up to (but not including) dot */
    cp += len;
    if (!cp1)
    {
      *cp++ = 0;																													/* Last one; write null and finish */
      break;
    }
    dname += len+1;
    dlen -= len+1;
  }
  
  cp[1] = htons(0x0001)>>8;																				/* type */
  cp[0] = htons(0x0001)&0x00ff;
  cp[3] = htons(0x0001)>>8;																				/* class */
  cp[2] = htons(0x0001)&0x00ff;
  cp += sizeof(uint16)*2;
  
  return ((int)((uint32)(cp) - (uint32)(buf)));
}


/*******************************************************************************
* 名称: Dns_parse_name
* 功能: 将压缩域名信息转化为可读的形式
* 形参: msg        - 指向回复信息的指针
        compressed - 指向回复信息中域名的指针
        buf        - 存放可读的域名信息
        len        - buf的长度
* 返回: 返回压缩域名信息长度
* 说明: 
*******************************************************************************/
int Dns_parse_name(uint8 * msg, uint8 * compressed, /*char * buf,*/ uint16 len)
{
  uint16 slen;																													/* Length of current segment */
  uint8  * cp;
  int16  clen = 0;																											/* Total length of compressed name */
  int16  indirect = 0;																									/* Set if indirection encountered */
  int16  nseg = 0;																											/* Total number of segments in name */
  int8   name[MAX_DNS_BUF_SIZE];
  int8   *buf;
  
  buf = name;
  
  cp = compressed;
  
  for (;;)
  {
    slen = *cp++;																												/* Length of this segment */
    
    if (!indirect) clen++;
    
    if ((slen & 0xc0) == 0xc0)
    {
      if (!indirect)
        clen++;
      indirect = 1;
      /* Follow indirection */
      cp = &msg[((slen & 0x3f)<<8) + *cp];
      slen = *cp++;
    }
    
    if (slen == 0)																											/* zero length == all done */
      break;
    
    len -= slen + 1;
    
    if (len <= 0) return -1;
    
    if (!indirect) clen += slen;
    
    while (slen-- != 0) *buf++ = (int8)*cp++;
    *buf++ = '.';
    nseg++;
  }
  
  if (nseg == 0)                                                    	 /* Root name; represent as single dot */
  {
    *buf++ = '.';
    len--;
  }
  
  *buf++ = '\0';
  len--;
  
  return clen;																												/* Length of compressed message */
}

/*******************************************************************************
* 名称: Dns_question
* 功能: 解析回复消息的问询记录
* 形参: msg - 指向回复信息的指针
        cp  - 指向问询记录的指针
* 返回: 返回下一个记录指针
* 说明: 
*******************************************************************************/
uint8 * Dns_question(uint8 * msg, uint8 * cp)
{
  int16 len;
  //	int8  xdata name[MAX_DNS_BUF_SIZE];
  
  len = Dns_parse_name(msg, cp, /*name,*/ MAX_DNS_BUF_SIZE);
  
  if (len == -1) return 0;
  
  cp += len;
  cp += 2;		/* type */
  cp += 2;		/* class */
  
  return cp;
}

/*******************************************************************************
* 名称:   Dns_answer
* 功能:   解析回复信息的应答记录
* 形参:   msg - 指向回复信息的指针
          cp  - 接收应答记录的指针
* 返回:   返回下一个应答记录指针
* 说明: 
*******************************************************************************/
uint8 * Dns_answer(uint8 * msg, uint8 * cp)
{
  int16 len, type;
  
  len = Dns_parse_name(msg, cp, /*name,*/ MAX_DNS_BUF_SIZE);
  
  if (len == -1) return 0;
  
  cp += len;
  type = cp[0]<<8|cp[1];
  cp += 2;		/* type */
  cp += 2;		/* class */
  cp += 4;		/* ttl */
  cp += 2;		/* len */
  
  switch (type)
  {
  case TYPE_A:
    dns_get_ip[0] = *cp++;
    dns_get_ip[1] = *cp++;
    dns_get_ip[2] = *cp++;
    dns_get_ip[3] = *cp++;
    break;
  case TYPE_CNAME:
  case TYPE_MB:
  case TYPE_MG:
  case TYPE_MR:
  case TYPE_NS:
  case TYPE_PTR:
    /* These types all consist of a single domain name */
    /* convert it to ascii format */
    len = Dns_parse_name(msg, cp, /*name,*/ MAX_DNS_BUF_SIZE);
    if (len == -1) return 0;
    
    cp += len;
    break;
    
  case TYPE_HINFO:
    len = *cp++;
    cp += len;
    len = *cp++;
    cp += len;
    break;
    
  case TYPE_MX:
    cp += 2;
    /* Get domain name of exchanger */
    len = Dns_parse_name(msg, cp,/* name,*/ MAX_DNS_BUF_SIZE);
    if (len == -1) return 0;
    cp += len;
    break;
    
  case TYPE_SOA:
    /* Get domain name of name server */
    len = Dns_parse_name(msg, cp,/* name,*/ MAX_DNS_BUF_SIZE);
    if (len == -1) return 0;
    cp += len;
    /* Get domain name of responsible person */
    len = Dns_parse_name(msg, cp,/* name,*/ MAX_DNS_BUF_SIZE);
    if (len == -1) return 0;
    
    cp += len;
    
    cp += 4;
    cp += 4;
    cp += 4;
    cp += 4;
    cp += 4;
    break;
    
  case TYPE_TXT:
    /* Just stash */
    break;
    
  default:
    /* Ignore */
    break;
  }
  
  return cp;
}

/*******************************************************************************
* 名称:   Dns_parseMSG
* 功能:   解析来自DNS服务器的回复报文
* 形参:   dhdr - 指向DNS信息头的指针
          buf  - 接收回复信息
          len  - 回复信息的长度
* 返回:   返回下一个应答记录指针
* 说明: 
*******************************************************************************/
uint8 Dns_parseMSG(struct dhdr * pdhdr, uint8 * pbuf)
{
  uint16 tmp;
  uint16 i;
  uint8 * msg;
  uint8 * cp;
  
  msg = pbuf;
  memset(pdhdr, 0, sizeof(pdhdr));
  
  pdhdr->id = ntohs(*((uint16*)&msg[0]));
  tmp = ntohs(*((uint16*)&msg[2]));
  if (tmp & 0x8000) pdhdr->qr = 1;
  
  pdhdr->opcode = (tmp >> 11) & 0xf;
  
  if (tmp & 0x0400) pdhdr->aa = 1;
  if (tmp & 0x0200) pdhdr->tc = 1;
  if (tmp & 0x0100) pdhdr->rd = 1;
  if (tmp & 0x0080) pdhdr->ra = 1;
  
  pdhdr->rcode = tmp & 0xf;
  pdhdr->qdcount = ntohs(*((uint16*)&msg[4]));
  pdhdr->ancount = ntohs(*((uint16*)&msg[6]));
  pdhdr->nscount = ntohs(*((uint16*)&msg[8]));
  pdhdr->arcount = ntohs(*((uint16*)&msg[10]));
  
  /* Now parse the variable length sections */
  cp = &msg[12]; 
  /* Question section */
  for (i = 0; i < pdhdr->qdcount; i++)
  {
    cp = Dns_question(msg, cp);
  } 
  /* Answer section */
  for (i = 0; i < pdhdr->ancount; i++)
  {
    cp = Dns_answer(msg, cp);
  }  
  /* Name server (authority) section */
  for (i = 0; i < pdhdr->nscount; i++)
  {
    ;
  }  
  /* Additional section */
  for (i = 0; i < pdhdr->arcount; i++)
  {
    ;
  }
  
  if(pdhdr->rcode == 0) return 1;	
  else return 0;
}
void Dns_Tick(void)
{  
  if(dns_wait_time)
    dns_wait_time--;  
}

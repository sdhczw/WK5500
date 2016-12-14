/**
******************************************************************************
* @file     zc_hf_adpter.h
* @authors  cxy
* @version  V1.0.0
* @date     10-Sep-2014
* @brief    HANDSHAKE
******************************************************************************
*/

#ifndef  __ZC_HF_ADPTER_H__ 
#define  __ZC_HF_ADPTER_H__

#include <zc_common.h>
#include <zc_protocol_controller.h>
#include <zc_module_interface.h>



#define HF_MAX_SOCKET_LEN    (TX_RX_MAX_BUF_SIZE)

#define  RET_FAIL           0
#define  RET_SUCCESS        1


#ifdef __cplusplus
extern "C" {
#endif
void ZC_Init(void);
void HF_WakeUp(void);
void HF_Sleep(void);
u32 HF_WriteDataToFlash(u8 *pu8Data, u16 u16Len);
void HF_Cloudfunc(void); 
#ifdef __cplusplus
}
#endif
#endif
/******************************* FILE END ***********************************/


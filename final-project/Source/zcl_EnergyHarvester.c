/**************************************************************************************************
  Description:    Zigbee Cluster Library - energy harvesting device.


  Copyright 2006-2012 Texas Instruments Incorporated. All rights reserved.

  IMPORTANT: Your use of this Software is limited to those specific rights
  granted under the terms of a software license agreement between the user
  who downloaded the software, his/her employer (which must be your employer)
  and Texas Instruments Incorporated (the "License").  You may not use this
  Software unless you agree to abide by the terms of the License. The License
  limits your use, and you acknowledge, that the Software may not be modified,
  copied or distributed unless embedded on a Texas Instruments microcontroller
  or used solely and exclusively in conjunction with a Texas Instruments radio
  frequency transceiver, which is integrated into your product.  Other than for
  the foregoing purpose, you may not use, reproduce, copy, prepare derivative
  works of, modify, distribute, perform, display or sell this Software and/or
  its documentation for any purpose.

  YOU FURTHER ACKNOWLEDGE AND AGREE THAT THE SOFTWARE AND DOCUMENTATION ARE
  PROVIDED “AS IS” WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, 
  INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY, TITLE, 
  NON-INFRINGEMENT AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL
  TEXAS INSTRUMENTS OR ITS LICENSORS BE LIABLE OR OBLIGATED UNDER CONTRACT,
  NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH OF WARRANTY, OR OTHER
  LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT DAMAGES OR EXPENSES
  INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL, SPECIAL, INDIRECT, PUNITIVE
  OR CONSEQUENTIAL DAMAGES, LOST PROFITS OR LOST DATA, COST OF PROCUREMENT
  OF SUBSTITUTE GOODS, TECHNOLOGY, SERVICES, OR ANY CLAIMS BY THIRD PARTIES
  (INCLUDING BUT NOT LIMITED TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.

  Should you have any questions regarding your right to use this Software,
  contact Texas Instruments Incorporated at www.TI.com. 
**************************************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "ZComDef.h"
#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"

#include "zcl.h"
#include "zcl_general.h"
#include "zcl_ha.h"

#include "zcl_EnergyHarvester.h"

/* HAL */
#include "hal_led.h"
#include "zcl_adc.h"

#include "DebugTrace.h"
#include <stdio.h>
#include <string.h>

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
#define COORDINATOR 0
#define ROUTER 1
#define END_DEVICE 2

/*********************************************************************
 * TYPEDEFS
 */

/*********************************************************************
 * GLOBAL VARIABLES
 */
byte zclEnergyHarvester_TaskID;

/*********************************************************************
 * GLOBAL FUNCTIONS
 */

/*********************************************************************
 * LOCAL VARIABLES
 */
static zAddrType_t dstAddr;
static uint8 childAddr[Z_EXTADDR_LEN];

#define ZCL_BINDINGLIST       1
static cId_t bindingInClusters[ZCL_BINDINGLIST] = {
  ZCL_CLUSTER_ID_MS_ALL
};

// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t testEp = {
  20,                                 // Test endpoint
  &zclEnergyHarvester_TaskID,
  (SimpleDescriptionFormat_t *)NULL,  // No Simple description for this test endpoint
  (afNetworkLatencyReq_t)0            // No Network Latency req
};

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static void zcl_ProcessZdoStateChange( osal_event_hdr_t *pMsg );
static void zcl_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg );
static ZStatus_t zclEnergyHarvester_HdlIncoming( zclIncoming_t *pInMsg );
static void zcl_SendBindRequest( void );
#if DEV_TYPE != COORDINATOR
static void zcl_SendDeviceData( void );
#endif

void zclEnergyHarvester_Init( byte task_id ) {
  zclEnergyHarvester_TaskID = task_id;

  // This app is part of the Home Automation Profile
  zclHA_Init( &zclSampleLight_SimpleDesc );

  // Register for a test endpoint
  afRegister( &testEp );
  
  zcl_registerPlugin( ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,
    ZCL_CLUSTER_ID_MS_ALL,
    zclEnergyHarvester_HdlIncoming );
  
  ZDO_RegisterForZDOMsg( zclEnergyHarvester_TaskID, End_Device_Bind_rsp );
  ZDO_RegisterForZDOMsg( zclEnergyHarvester_TaskID, Device_annce );
    
  adc_Init();
}

uint16 zclEnergyHarvester_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  ( void )task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG ) {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclEnergyHarvester_TaskID )) ) {
      switch ( MSGpkt->hdr.event ) {
        case ZDO_STATE_CHANGE:
          zcl_ProcessZdoStateChange( ( osal_event_hdr_t * )MSGpkt );
          break;
          
        case ZDO_CB_MSG:
          zcl_ProcessZDOMsgs( ( zdoIncomingMsg_t * )MSGpkt );
          break;
      }

      // Release the memory
      osal_msg_deallocate( (uint8 *)MSGpkt );
    }

    // return unprocessed events
    return (events ^ SYS_EVENT_MSG);
  }

  if ( events & SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT ) {
    if ( zclSampleLight_IdentifyTime > 0 ) {
      zclSampleLight_IdentifyTime--;
    }

    return ( events ^ SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT );
  }

  // Discard unknown events
  return 0;
}

/*********************************************************************
 * @fn      zcl_ProcessZdoStateChange
 *
 * @brief   Handles state change OSAL message from ZDO.
 *
 * @param   pMsg - Message data
 *
 * @return  none
 */
static void zcl_ProcessZdoStateChange(osal_event_hdr_t *pMsg) {
  switch( ( devStates_t )pMsg->status ) {
    case DEV_ZB_COORD: case DEV_ROUTER: case DEV_END_DEVICE:
#if DEV_TYPE == COORDINATOR
      debug_str( "Successfully started network." );
#else
      debug_str( "Successfully connected to network." );
      zcl_SendBindRequest();
#endif
      break;
      
    case DEV_NWK_ORPHAN:
      debug_str( "Lost information about parent." );
      break;
  }
}

/*********************************************************************
 * @fn      zcl_ProcessZDOMsgs
 *
 * @brief   Process response messages.
 *
 * @param   inMsg - ZDO response message.
 *
 * @return  none
 */
static void zcl_ProcessZDOMsgs( zdoIncomingMsg_t *inMsg ) {
  uint8 response;
  
  switch( inMsg->clusterID ) {
    case End_Device_Bind_rsp:
      response = ZDO_ParseBindRsp( inMsg );
      if ( response == ZSuccess ) {
        debug_str( "Bind succeeded." );
#if DEV_TYPE != COORDINATOR
        HalLedSet( HAL_LED_4, HAL_LED_MODE_TOGGLE );
        zcl_SendDeviceData();
#endif
      }
      break;

#if DEV_TYPE == COORDINATOR      
    case Device_annce: {
      ZDO_DeviceAnnce_t msg;
      uint8 buffer[50];
      
      ZDO_ParseDeviceAnnce( inMsg, &msg );
      sprintf( ( char* )buffer, "New device joined, address: %d", msg.nwkAddr );
      memcpy( childAddr, msg.extAddr, Z_EXTADDR_LEN );
      zcl_SendBindRequest();
      }
      break;
#endif
  }
}

/*********************************************************************
 * @fn      zclEnergyHarvester_HdlIncoming
 *
 * @brief   Callback from ZCL to process incoming Commands specific
 *          to this cluster library or Profile commands for attributes
 *          that aren't in the attribute list
 *
 * @param   pInMsg - pointer to the incoming message
 *
 * @return  ZStatus_t
 */
static ZStatus_t zclEnergyHarvester_HdlIncoming( zclIncoming_t *pInMsg ) {
  ZStatus_t stat = ZSuccess;
  NLME_LeaveReq_t leaveReq = {childAddr, FALSE, TRUE, TRUE};
  
  debug_str( pInMsg->pData );
  
  HalLedSet( HAL_LED_4, HAL_LED_MODE_TOGGLE );
  
  bindRemoveDev( &dstAddr );
  
  NLME_LeaveReq( &leaveReq );
  
  if ( zcl_ClusterCmd( pInMsg->hdr.fc.type ) )
  {
    
    stat = ZFailure;
    
    /*// Is this a manufacturer specific command?
    if ( pInMsg->hdr.fc.manuSpecific == 0 )
    {
      stat = zclGeneral_HdlInSpecificCommands( pInMsg );
    }
    else
    {
      // We don't support any manufacturer specific command.
      stat = ZFailure;
    }*/
  }
  else
  {
    // Handle all the normal (Read, Write...) commands -- should never get here
    stat = ZFailure;
  }
  
  return ( stat );
}

/*********************************************************************
 * @fn      zcl_SendBindRequest
 *
 * @brief   Send the appropriate bind request based on DEV_TYPE.
 *
 * @param   none
 *
 * @return  none
 */
static void zcl_SendBindRequest( void ) {  
  dstAddr.addrMode = afAddr16Bit;
  dstAddr.addr.shortAddr = 0;   // Coordinator makes the match

#if DEV_TYPE == COORDINATOR
  ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
    ENDPOINT,
    ZCL_HA_PROFILE_ID,
    ZCL_BINDINGLIST, bindingInClusters,
    0, NULL,   // No Outgoing clusters to bind
    TRUE );
#else
  ZDP_EndDeviceBindReq( &dstAddr, NLME_GetShortAddr(),
    ENDPOINT,
    ZCL_HA_PROFILE_ID,
    0, NULL,   // No incoming clusters to bind
    ZCL_BINDINGLIST, bindingInClusters,
    TRUE );
#endif
}

#if DEV_TYPE != COORDINATOR
/*********************************************************************
 * @fn      zcl_SendDeviceData
 *
 * @brief   Process sensor send data to coordinator.
 *
 * @param   none
 *
 * @return  none
 */
static void zcl_SendDeviceData( void ) {  
  afAddrType_t afDstAddr;
  ZStatus_t response;
  uint8 *temperature;
  uint8 dataLength;
  uint8 *data;
  
  afDstAddr.addr.shortAddr = dstAddr.addr.shortAddr;
  afDstAddr.addrMode = ( afAddrMode_t )dstAddr.addrMode;
  afDstAddr.endPoint = ENDPOINT;
 
  temperature = readTemperature();
  dataLength = strlen( ( char* )temperature ) + 1;
  data = ( uint8* )osal_mem_alloc( dataLength );
  strcpy( ( char* )data, ( char* )temperature );
  
  response = zcl_SendCommand( ENDPOINT, &afDstAddr,
    ZCL_CLUSTER_ID_MS_ALL, COMMAND_OFF,
    TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
    FALSE, 0, 0,
    dataLength, data );
  
  if( response == ZSuccess ) {
    debug_str( "Successfully sent message." );
  }
  
  osal_mem_free( data );
}
#endif

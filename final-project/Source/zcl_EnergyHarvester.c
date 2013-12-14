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

#include "DebugTrace.h"


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

#define ZCL_BINDINGLIST       2
static cId_t bindingInClusters[ZCL_BINDINGLIST] =
{
  ZCL_CLUSTER_ID_GEN_ON_OFF,
  ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT
};

// Test Endpoint to allow SYS_APP_MSGs
static endPointDesc_t sampleLight_TestEp =
{
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
static void zcl_BasicResetCB( void );
static void zcl_IdentifyCB( zclIdentify_t *pCmd );
static void zcl_IdentifyQueryRspCB( zclIdentifyQueryRsp_t *pRsp );
static void zcl_OnOffCB( uint8 cmd );
static void zclSampleLight_ProcessIdentifyTimeChange( void );

// Functions to process ZCL Foundation incoming Command/Response messages 
static void zclSampleLight_ProcessIncomingMsg( zclIncomingMsg_t *msg );
#ifdef ZCL_READ
static uint8 zclSampleLight_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg );
#endif
#ifdef ZCL_WRITE
static uint8 zclSampleLight_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg );
#endif
static uint8 zclSampleLight_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg );
#ifdef ZCL_DISCOVER
static uint8 zclSampleLight_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg );
#endif

/*********************************************************************
 * ZCL General Profile Callback table
 */
static zclGeneral_AppCallbacks_t zclSampleLight_CmdCallbacks =
{
  zcl_BasicResetCB,                       // Basic Cluster Reset command
  zcl_IdentifyCB,                         // Identify command
  NULL,                                   // Identify Trigger Effect command
  zcl_IdentifyQueryRspCB,                 // Identify Query Response command
  zcl_OnOffCB,                            // On/Off cluster commands
  NULL,                                   // On/Off cluster enhanced command Off with Effect
  NULL,                                   // On/Off cluster enhanced command On with Recall Global Scene
  NULL,                                   // On/Off cluster enhanced command On with Timed Off
  NULL,                                   // Level Control Move to Level command
  NULL,                                   // Level Control Move command
  NULL,                                   // Level Control Step command
  NULL,                                   // Level Control Stop command
  NULL,                                   // Group Response commands
  NULL,                                   // Scene Store Request command
  NULL,                                   // Scene Recall Request command
  NULL,                                   // Scene Response command
  NULL,                                   // Alarm (Response) commands
  NULL,                                   // RSSI Location command
  NULL                                    // RSSI Location Response command
};

void zclEnergyHarvester_Init( byte task_id )
{
  zclEnergyHarvester_TaskID = task_id;

  // This app is part of the Home Automation Profile
  zclHA_Init( &zclSampleLight_SimpleDesc );
  
  // Register the ZCL General Cluster Library callback functions
  zclGeneral_RegisterCmdCallbacks( ENDPOINT, &zclSampleLight_CmdCallbacks );

  // Register the application's attribute list
  zcl_registerAttrList( ENDPOINT, SAMPLELIGHT_MAX_ATTRIBUTES, zcl_Attrs );

  // Register the Application to receive the unprocessed Foundation command/response messages
  //zcl_registerForMsg( zclEnergyHarvester_TaskID );

  // Register for a test endpoint
  afRegister( &sampleLight_TestEp );
  
  zcl_registerPlugin( ZCL_CLUSTER_ID_MS_ILLUMINANCE_MEASUREMENT,
    ZCL_CLUSTER_ID_MS_OCCUPANCY_SENSING,
    zclEnergyHarvester_HdlIncoming );
  
  ZDO_RegisterForZDOMsg( zclEnergyHarvester_TaskID, End_Device_Bind_rsp );
#if DEV_TYPE == COORDINATOR
  ZDO_RegisterForZDOMsg( zclEnergyHarvester_TaskID, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT );
#endif
}

uint16 zclEnergyHarvester_event_loop( uint8 task_id, uint16 events )
{
  afIncomingMSGPacket_t *MSGpkt;
  
  ( void )task_id;  // Intentionally unreferenced parameter

  if ( events & SYS_EVENT_MSG ) {
    while ( (MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( zclEnergyHarvester_TaskID )) ) {
      switch ( MSGpkt->hdr.event ) {
        case ZCL_INCOMING_MSG:
          // Incoming ZCL Foundation command/response messages
          zclSampleLight_ProcessIncomingMsg( ( zclIncomingMsg_t * )MSGpkt );
          break;
 
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
    zclSampleLight_ProcessIdentifyTimeChange();

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
#endif
      zcl_SendBindRequest();
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
#if DEV_TYPE == COORDINATOR
        // Try to bind another device immediately.
        zcl_SendBindRequest();
#else
        zcl_SendDeviceData();
#endif
      } else {
        debug_str( "Bind failed; trying again." );
        zcl_SendBindRequest();
      }
      break;
    
#if DEV_TYPE == COORDINATOR
    case ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT:
      debug_str( "Got a 'Sensor Response.' Whatever we decide that to be" );
      HalLedSet( HAL_LED_4, HAL_LED_MODE_TOGGLE );
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
  zAddrType_t dstAddr;
  
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
  afAddrType_t dstAddr;
  zcl_SendCommand( ENDPOINT, &dstAddr, ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, COMMAND_TOGGLE, TRUE, ZCL_FRAME_CLIENT_SERVER_DIR, FALSE, 0, 0, 0, NULL );
  
  /*zcl_SendCommand( ENDPOINT, &dstAddr,
    ZCL_CLUSTER_ID_MS_TEMPERATURE_MEASUREMENT, COMMAND_OFF,
    TRUE, ZCL_FRAME_CLIENT_SERVER_DIR,
    FALSE, 0, 0, 0, NULL );*/
  
  /*zclWriteRec_t zcl_WriteRec = {0, ZCL_DATATYPE_CHAR_STR, "\x04Test"};
  zclWriteCmd_t *zcl_WriteCmd = ( zclWriteCmd_t * )osal_mem_alloc( sizeof ( zclWriteCmd_t ) + 1 * sizeof( zclWriteRec_t ) );;
  zcl_WriteCmd->numAttr = 1;
  zcl_WriteCmd->attrList[0] = zcl_WriteRec;
  
  ZStatus_t response;
  response = zcl_SendWrite( ENDPOINT, &dstAddr, 
    ZCL_CLUSTER_ID_SE_SIMPLE_METERING, zcl_WriteCmd, 
    0, TRUE, 
    0 );
  if( response == ZSuccess ) {
    debug_str( "Successfully Sent Message." );
  }
  
  osal_mem_free( zcl_WriteCmd );*/
}
#endif

/*********************************************************************
 * @fn      zclSampleLight_ProcessIdentifyTimeChange
 *
 * @brief   Called to process any change to the IdentifyTime attribute.
 *
 * @param   none
 *
 * @return  none
 */
static void zclSampleLight_ProcessIdentifyTimeChange( void )
{
  if ( zclSampleLight_IdentifyTime > 0 )
  {
    osal_start_timerEx( zclEnergyHarvester_TaskID, SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT, 1000 );
    HalLedBlink ( HAL_LED_4, 0xFF, HAL_LED_DEFAULT_DUTY_CYCLE, HAL_LED_DEFAULT_FLASH_TIME );
  }
  else
  {
    if ( zclSampleLight_OnOff )
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_ON );
    else
      HalLedSet ( HAL_LED_4, HAL_LED_MODE_OFF );
    osal_stop_timerEx( zclEnergyHarvester_TaskID, SAMPLELIGHT_IDENTIFY_TIMEOUT_EVT );
  }
}

/*********************************************************************
 * @fn      zcl_BasicResetCB
 *
 * @brief   Callback from the ZCL General Cluster Library
 *          to set all the Basic Cluster attributes to default values.
 *
 * @param   none
 *
 * @return  none
 */
static void zcl_BasicResetCB( void )
{
  // Reset all attributes to default values
}

/*********************************************************************
 * @fn      zcl_IdentifyCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Command for this application.
 *
 * @param   srcAddr - source address and endpoint of the response message
 * @param   identifyTime - the number of seconds to identify yourself
 *
 * @return  none
 */
static void zcl_IdentifyCB( zclIdentify_t *pCmd )
{
  zclSampleLight_IdentifyTime = pCmd->identifyTime;
  zclSampleLight_ProcessIdentifyTimeChange();
}

/*********************************************************************
 * @fn      zcl_IdentifyQueryRspCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an Identity Query Response Command for this application.
 *
 * @param   srcAddr - requestor's address
 * @param   timeout - number of seconds to identify yourself (valid for query response)
 *
 * @return  none
 */
static void zcl_IdentifyQueryRspCB(  zclIdentifyQueryRsp_t *pRsp )
{
  // Query Response (with timeout value)
  (void)pRsp;
}

/*********************************************************************
 * @fn      zcl_OnOffCB
 *
 * @brief   Callback from the ZCL General Cluster Library when
 *          it received an On/Off Command for this application.
 *
 * @param   cmd - COMMAND_ON, COMMAND_OFF or COMMAND_TOGGLE
 *
 * @return  none
 */
static void zcl_OnOffCB( uint8 cmd )
{
  // Turn on the light
  if ( cmd == COMMAND_ON )
    zclSampleLight_OnOff = LIGHT_ON;

  // Turn off the light
  else if ( cmd == COMMAND_OFF )
    zclSampleLight_OnOff = LIGHT_OFF;

  // Toggle the light
  else
  {
    if ( zclSampleLight_OnOff == LIGHT_OFF )
      zclSampleLight_OnOff = LIGHT_ON;
    else
      zclSampleLight_OnOff = LIGHT_OFF;
  }

  // In this sample app, we use LED4 to simulate the Light
  if ( zclSampleLight_OnOff == LIGHT_ON )
    HalLedSet( HAL_LED_4, HAL_LED_MODE_ON );
  else
    HalLedSet( HAL_LED_4, HAL_LED_MODE_OFF );
}


/****************************************************************************** 
 * 
 *  Functions for processing ZCL Foundation incoming Command/Response messages
 *
 *****************************************************************************/

/*********************************************************************
 * @fn      zclSampleLight_ProcessIncomingMsg
 *
 * @brief   Process ZCL Foundation incoming message
 *
 * @param   pInMsg - pointer to the received message
 *
 * @return  none
 */
static void zclSampleLight_ProcessIncomingMsg( zclIncomingMsg_t *pInMsg)
{
  switch ( pInMsg->zclHdr.commandID )
  {
#ifdef ZCL_READ
    case ZCL_CMD_READ_RSP:
      zclSampleLight_ProcessInReadRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_WRITE    
    case ZCL_CMD_WRITE_RSP:
      zclSampleLight_ProcessInWriteRspCmd( pInMsg );
      break;
#endif
#ifdef ZCL_REPORT
    // See ZCL Test Applicaiton (zcl_testapp.c) for sample code on Attribute Reporting
    case ZCL_CMD_CONFIG_REPORT:
      //zclSampleLight_ProcessInConfigReportCmd( pInMsg );
      break;
    
    case ZCL_CMD_CONFIG_REPORT_RSP:
      //zclSampleLight_ProcessInConfigReportRspCmd( pInMsg );
      break;
    
    case ZCL_CMD_READ_REPORT_CFG:
      //zclSampleLight_ProcessInReadReportCfgCmd( pInMsg );
      break;
    
    case ZCL_CMD_READ_REPORT_CFG_RSP:
      //zclSampleLight_ProcessInReadReportCfgRspCmd( pInMsg );
      break;
    
    case ZCL_CMD_REPORT:
      //zclSampleLight_ProcessInReportCmd( pInMsg );
      break;
#endif   
    case ZCL_CMD_DEFAULT_RSP:
      zclSampleLight_ProcessInDefaultRspCmd( pInMsg );
      break;
#ifdef ZCL_DISCOVER     
    case ZCL_CMD_DISCOVER_RSP:
      zclSampleLight_ProcessInDiscRspCmd( pInMsg );
      break;
#endif  
    default:
      break;
  }
  
  if ( pInMsg->attrCmd )
    osal_mem_free( pInMsg->attrCmd );
}

#ifdef ZCL_READ
/*********************************************************************
 * @fn      zclSampleLight_ProcessInReadRspCmd
 *
 * @brief   Process the "Profile" Read Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleLight_ProcessInReadRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclReadRspCmd_t *readRspCmd;
  uint8 i;

  readRspCmd = (zclReadRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < readRspCmd->numAttr; i++)
  {
    // Notify the originator of the results of the original read attributes 
    // attempt and, for each successfull request, the value of the requested 
    // attribute
  }

  return TRUE; 
}
#endif // ZCL_READ

#ifdef ZCL_WRITE
/*********************************************************************
 * @fn      zclSampleLight_ProcessInWriteRspCmd
 *
 * @brief   Process the "Profile" Write Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleLight_ProcessInWriteRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclWriteRspCmd_t *writeRspCmd;
  uint8 i;

  writeRspCmd = (zclWriteRspCmd_t *)pInMsg->attrCmd;
  for (i = 0; i < writeRspCmd->numAttr; i++)
  {
    // Notify the device of the results of the its original write attributes
    // command.
  }

  return TRUE; 
}
#endif // ZCL_WRITE

/*********************************************************************
 * @fn      zclSampleLight_ProcessInDefaultRspCmd
 *
 * @brief   Process the "Profile" Default Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleLight_ProcessInDefaultRspCmd( zclIncomingMsg_t *pInMsg )
{
  // zclDefaultRspCmd_t *defaultRspCmd = (zclDefaultRspCmd_t *)pInMsg->attrCmd;
   
  // Device is notified of the Default Response command.
  (void)pInMsg;
  
  return TRUE; 
}

#ifdef ZCL_DISCOVER
/*********************************************************************
 * @fn      zclSampleLight_ProcessInDiscRspCmd
 *
 * @brief   Process the "Profile" Discover Response Command
 *
 * @param   pInMsg - incoming message to process
 *
 * @return  none
 */
static uint8 zclSampleLight_ProcessInDiscRspCmd( zclIncomingMsg_t *pInMsg )
{
  zclDiscoverRspCmd_t *discoverRspCmd;
  uint8 i;
  
  discoverRspCmd = (zclDiscoverRspCmd_t *)pInMsg->attrCmd;
  for ( i = 0; i < discoverRspCmd->numAttr; i++ )
  {
    // Device is notified of the result of its attribute discovery command.
  }
  
  return TRUE;
}
#endif // ZCL_DISCOVER


/****************************************************************************
****************************************************************************/



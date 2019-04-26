/**

  *          ===================================================================
  *                                     MIDI Class
  *          ===================================================================

*/

#include "usbh_midi.h"
#include "string.h"

#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
    #pragma data_alignment=4
  #endif
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
__ALIGN_BEGIN MIDI_Machine_TypeDef        MIDI_Machine __ALIGN_END ;

//MIDI_Machine_TypeDef        MIDI_Machine;
//#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
//  #if defined ( __ICCARM__ ) /*!< IAR Compiler */
//    #pragma data_alignment=4
//  #endif
//#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */
//__ALIGN_BEGIN USB_Setup_TypeDef          MIDI_Setup __ALIGN_END ;


static USBH_Status 	USBH_MIDI_InterfaceInit(USB_OTG_CORE_HANDLE *pdev , void *phost);
static void 		USBH_MIDI_InterfaceDeInit(USB_OTG_CORE_HANDLE *pdev , void *phost);
static USBH_Status 	USBH_MIDI_ClassRequest(USB_OTG_CORE_HANDLE *pdev , void *phost);
static USBH_Status 	USBH_MIDI_Handle(USB_OTG_CORE_HANDLE *pdev , void *phost);

USBH_Class_cb_TypeDef  MIDI_cb = {
		USBH_MIDI_InterfaceInit,
		USBH_MIDI_InterfaceDeInit,
		USBH_MIDI_ClassRequest,
		USBH_MIDI_Handle
};

extern USB_OTG_CORE_HANDLE           USB_OTG_HS_dev;

#define SEND_BUFFER_SIZE 128 // (8, 16, 32, 64 ...)
#define SEND_BUFFER_MASK (SEND_BUFFER_SIZE-1)

static struct {
	mios32_midi_package_t event[SEND_BUFFER_SIZE];
	uint8_t read;
	uint8_t write;
} send_buffer;


/*-----------------------------------------------------------------------------------------*/
/**
 * @brief  USBH_MIDI_InterfaceInit
 *         The function init the MIDI class.
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval  USBH_Status :Response for USB MIDI driver intialization
 */
static USBH_Status USBH_MIDI_InterfaceInit(USB_OTG_CORE_HANDLE *pdev, void *phost) {

	USBH_HOST *pphost = phost;
	USBH_Status status = USBH_BUSY;
	MIDI_Machine.state_out = MIDI_ERROR;
	MIDI_Machine.state_in = MIDI_ERROR;

//	#define INTERFACE 1
//
//	DEBUG("class %d-%d\n",
//		pphost->device_prop.Itf_Desc[INTERFACE].bInterfaceClass,
//		pphost->device_prop.Itf_Desc[INTERFACE].bInterfaceSubClass);
	int i = 0;

	for(i=0; i<pphost->device_prop.Cfg_Desc.bNumInterfaces && i < USBH_MAX_NUM_INTERFACES; ++i) {
	    MIOS32_MIDI_SendDebugMessage("InterfaceInit %d %d %d", i, pphost->device_prop.Itf_Desc[i].bInterfaceClass, pphost->device_prop.Itf_Desc[i].bInterfaceSubClass);

		if ((pphost->device_prop.Itf_Desc[i].bInterfaceClass == 1) && \
			(pphost->device_prop.Itf_Desc[i].bInterfaceSubClass == 3))
		{
			if(pphost->device_prop.Ep_Desc[i][0].bEndpointAddress & 0x80)
			{
				MIDI_Machine.MIDIBulkInEp = (pphost->device_prop.Ep_Desc[i][0].bEndpointAddress);
				MIDI_Machine.MIDIBulkInEpSize  = pphost->device_prop.Ep_Desc[i][0].wMaxPacketSize;
			}
			else
			{
				MIDI_Machine.MIDIBulkOutEp = (pphost->device_prop.Ep_Desc[i][0].bEndpointAddress);
				MIDI_Machine.MIDIBulkOutEpSize  = pphost->device_prop.Ep_Desc[i][0].wMaxPacketSize;
			}

			if(pphost->device_prop.Ep_Desc[i][1].bEndpointAddress & 0x80)

			{
				MIDI_Machine.MIDIBulkInEp = (pphost->device_prop.Ep_Desc[i][1].bEndpointAddress);
				MIDI_Machine.MIDIBulkInEpSize  = pphost->device_prop.Ep_Desc[i][1].wMaxPacketSize;
			}
			else
			{
				MIDI_Machine.MIDIBulkOutEp = (pphost->device_prop.Ep_Desc[i][1].bEndpointAddress);
				MIDI_Machine.MIDIBulkOutEpSize  = pphost->device_prop.Ep_Desc[i][1].wMaxPacketSize;
			}

			MIDI_Machine.hc_num_out = USBH_Alloc_Channel(pdev,
					MIDI_Machine.MIDIBulkOutEp);
			MIDI_Machine.hc_num_in = USBH_Alloc_Channel(pdev,
					MIDI_Machine.MIDIBulkInEp);
			DEBUG_MSG("hc_num_out= %d", MIDI_Machine.hc_num_out);
			DEBUG_MSG("hc_num_in= %d", MIDI_Machine.hc_num_in);
			/* Open the new channels */
			USBH_Open_Channel  (pdev,
					MIDI_Machine.hc_num_out,
					pphost->device_prop.address,
					pphost->device_prop.speed,
					EP_TYPE_BULK,
					MIDI_Machine.MIDIBulkOutEpSize);

			USBH_Open_Channel  (pdev,
					MIDI_Machine.hc_num_in,
					pphost->device_prop.address,
					pphost->device_prop.speed,
					EP_TYPE_BULK,
					MIDI_Machine.MIDIBulkInEpSize);

			MIDI_Machine.state_out  = MIDI_DATA;
			MIDI_Machine.state_in  = MIDI_DATA;
			status = USBH_OK;

			send_buffer.read=0;
			send_buffer.write=0;
			return status ;
		}
	}
	pphost->usr_cb->DeviceNotSupported();

	return status ;

}


/*-----------------------------------------------------------------------------------------*/
/**
 * @brief  USBH_MIDI_InterfaceDeInit
 *         The function DeInit the Host Channels used for the MIDI class.
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval None
 */
void USBH_MIDI_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev, void *phost)
{
	if ( MIDI_Machine.hc_num_out)
	{
		USB_OTG_HC_Halt(pdev, MIDI_Machine.hc_num_out);
		USBH_Free_Channel  (pdev, MIDI_Machine.hc_num_out);
		MIDI_Machine.hc_num_out = 0;     /* Reset the Channel as Free */
	}

	if ( MIDI_Machine.hc_num_in)
	{
		USB_OTG_HC_Halt(pdev, MIDI_Machine.hc_num_in);
		USBH_Free_Channel  (pdev, MIDI_Machine.hc_num_in);
		MIDI_Machine.hc_num_in = 0;     /* Reset the Channel as Free */
	}
}
/*-----------------------------------------------------------------------------------------*/
/**
 * @brief  USBH_MIDI_ClassRequest
 *         The function is responsible for handling MIDI Class requests
 *         for MIDI class.
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval  USBH_Status :Response for USB Set Protocol request
 */
static USBH_Status USBH_MIDI_ClassRequest(USB_OTG_CORE_HANDLE *pdev , void *phost)
{
	USBH_Status status = USBH_OK ;

	return status;
}


int MIDI_send(mios32_midi_package_t packet)
{
	uint8_t next = ((send_buffer.write + 1) & SEND_BUFFER_MASK);
	if (send_buffer.read != next)
	{
		send_buffer.event[send_buffer.write & SEND_BUFFER_MASK] = packet;
		send_buffer.write = next;
		return 1;
	}
	return 0;
}


/*-----------------------------------------------------------------------------------------*/
/**
 * @brief  USBH_MIDI_Handle
 *         The function is for managing state machine for MIDI data transfers
 * @param  pdev: Selected device
 * @param  hdev: Selected device property
 * @retval USBH_Status
 */
static USBH_Status USBH_MIDI_Handle(USB_OTG_CORE_HANDLE *pdev , void  *phost)
{
	USBH_HOST *pphost = phost;
	USBH_Status status = USBH_OK;

	//uint8_t appliStatus = 0;
	//USBH_Status status = USBH_BUSY;

	if(HCD_IsDeviceConnected(pdev))
	{
		//appliStatus = pphost->usr_cb->UserApplication(); // this will call USBH_USR_MIDI_Application()


		//todo:
		//
		//two state machines one for send one for receive
		//recive urb_idle is received afer send is issued until a packet is received then urb_done is received


		switch (MIDI_Machine.state_in)
		{

			case MIDI_DATA:

				memset(MIDI_Machine.buff_in,0,USBH_MIDI_MPS_SIZE);
				USBH_BulkReceiveData(&USB_OTG_HS_dev, MIDI_Machine.buff_in, 64, MIDI_Machine.hc_num_in);
				MIDI_Machine.state_in = MIDI_POLL;
				//DEBUG_MSG("Data->poll");
				break;


			case MIDI_POLL:


				if(HCD_GetURB_State(pdev , MIDI_Machine.hc_num_in) == URB_IDLE)
				{
					//DEBUG_MSG("Poll->Idle");
				}
				else if(HCD_GetURB_State(pdev , MIDI_Machine.hc_num_in) == URB_DONE)
				{
				//URB_STATE URB_State = HCD_GetURB_State(pdev, MIDI_Machine.hc_num_in);
			        //if( HCD_GetURB_State(pdev, MIDI_Machine.hc_num_in) == URB_IDLE || HCD_GetURB_State(pdev, MIDI_Machine.hc_num_in) == URB_DONE ) {
					//DEBUG_MSG("Poll->Idle");
					int i = 0;
					while((i < USBH_MIDI_MPS_SIZE) && (MIDI_Machine.buff_in[i] != 0))
					{

						mios32_midi_package_t packet;
						packet.bytes[0] = MIDI_Machine.buff_in[i];
						packet.bytes[1] = MIDI_Machine.buff_in[i+1];
						packet.bytes[2] = MIDI_Machine.buff_in[i+2];
						packet.bytes[3] = MIDI_Machine.buff_in[i+3];
						MIOS32_MIDI_SendPackage(USB0, packet);

						i+=4;
					}
					//DEBUG_MSG("Poll->Data");
					MIDI_Machine.state_in = MIDI_DATA;
				}
				else if(HCD_GetURB_State(pdev, MIDI_Machine.hc_num_in) == URB_STALL) /* IN Endpoint Stalled */
				{

					/* Issue Clear Feature on IN endpoint */
					if( (USBH_ClrFeature(pdev,
									pphost,
									MIDI_Machine.MIDIBulkInEp,
									MIDI_Machine.hc_num_in)) == USBH_OK)
					{
						// Change state to issue next IN token
						MIDI_Machine.state_in = MIDI_DATA;
						//STM_EVAL_LEDToggle(LED_Blue);
					}
				}
				break;

			default:
				break;
		}


		switch (MIDI_Machine.state_out)
		{
			case MIDI_DATA:

				if (send_buffer.read != send_buffer.write)
				{
					MIDI_Machine.buff_out[0] = send_buffer.event[send_buffer.read].bytes[0];
					MIDI_Machine.buff_out[1] = send_buffer.event[send_buffer.read].bytes[1];
					MIDI_Machine.buff_out[2] = send_buffer.event[send_buffer.read].bytes[2];
					MIDI_Machine.buff_out[3] = send_buffer.event[send_buffer.read].bytes[3];
					send_buffer.read = (send_buffer.read+1) & SEND_BUFFER_MASK;

					//printf("send %i %i %i %i\n",MIDI_Machine.buff_out[0],MIDI_Machine.buff_out[1],MIDI_Machine.buff_out[2],MIDI_Machine.buff_out[3]);

					USBH_BulkSendData( &USB_OTG_HS_dev, MIDI_Machine.buff_out, 4, MIDI_Machine.hc_num_out);
					MIDI_Machine.state_out = MIDI_POLL;
				}

				break;
			case MIDI_POLL:

				if(HCD_GetURB_State(pdev , MIDI_Machine.hc_num_out) == URB_DONE)
				{
					MIDI_Machine.state_out = MIDI_DATA;
				}
				else if(HCD_GetURB_State(pdev , MIDI_Machine.hc_num_out) == URB_NOTREADY)
				{
					MIDI_Machine.state_out = MIDI_DATA;
				}
				break;

			default:
				break;
		}

	}

	return status;

}

void MIDI_recv_cb(mios32_midi_package_t packet);



// $Id: mios32_usb_midi.c 1715 2013-03-17 11:14:14Z tk $
//! \defgroup MIOS32_USB_MIDI
//!
//! USB MIDI layer for MIOS32
//! 
//! Applications shouldn't call these functions directly, instead please use \ref MIOS32_MIDI layer functions
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB_MIDI)

#include <usb_lib.h>

#ifdef STM32F10X_CL
extern USB_OTG_CORE_REGS USB_OTG_FS_regs;
#endif

/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void MIOS32_USB_MIDI_TxBufferHandler(void);
static void MIOS32_USB_MIDI_RxBufferHandler(void);


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

// Rx buffer
static u32 rx_buffer[MIOS32_USB_MIDI_RX_BUFFER_SIZE];
static volatile u16 rx_buffer_tail;
static volatile u16 rx_buffer_head;
static volatile u16 rx_buffer_size;
static volatile u8 rx_buffer_new_data;

// Tx buffer
static u32 tx_buffer[MIOS32_USB_MIDI_TX_BUFFER_SIZE];
static volatile u16 tx_buffer_tail;
static volatile u16 tx_buffer_head;
static volatile u16 tx_buffer_size;
static volatile u8 tx_buffer_busy;

// transfer possible?
static u8 transfer_possible = 0;


/////////////////////////////////////////////////////////////////////////////
//! Initializes USB MIDI layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] device number (doesn't matter here)
//! \param[in] connected status (1 if connected)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_ChangeConnectionState(u8 dev, u8 connected)
{
  // in all cases: re-initialize USB MIDI driver
  // clear buffer counters and busy/wait signals again (e.g., so that no invalid data will be sent out)
  rx_buffer_tail = rx_buffer_head = rx_buffer_size = 0;
  rx_buffer_new_data = 0; // no data received yet
  tx_buffer_tail = tx_buffer_head = tx_buffer_size = 0;

  if( connected ) {
    transfer_possible = 1;
    tx_buffer_busy = 0; // buffer not busy anymore
  } else {
    // cable disconnected: disable transfers
    transfer_possible = 0;
    tx_buffer_busy = 1; // buffer busy
  }

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB MIDI interface
//! \param[in] cable number
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_CheckAvailable(u8 cable)
{
#ifdef MIOS32_SYS_ADDR_BSL_INFO_BEGIN
  if( MIOS32_USB_ForceSingleUSB() && cable >= 1 )
    return 0;
#endif

  if( cable >= MIOS32_USB_MIDI_NUM_PORTS )
    return 0;

  return transfer_possible ? 1 : 0;
}


/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: USB not connected
//! \return -2: buffer is full
//!             caller should retry until buffer is free again
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package)
{
  // device available?
  if( !transfer_possible )
    return -1;

  // buffer full?
  if( tx_buffer_size >= (MIOS32_USB_MIDI_TX_BUFFER_SIZE-1) ) {
    // call USB handler, so that we are able to get the buffer free again on next execution
    // (this call simplifies polling loops!)
    MIOS32_USB_MIDI_TxBufferHandler();

    // device still available?
    // (ensures that polling loop terminates if cable has been disconnected)
    if( !transfer_possible )
      return -1;

    // notify that buffer was full (request retry)
    return -2;
  }

  // put package into buffer - this operation should be atomic!
  MIOS32_IRQ_Disable();
  tx_buffer[tx_buffer_head++] = package.ALL;
  if( tx_buffer_head >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
    tx_buffer_head = 0;
  ++tx_buffer_size;
  MIOS32_IRQ_Enable();

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! (blocking function)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: USB not connected
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageSend(mios32_midi_package_t package)
{
  static u16 timeout_ctr = 0;
  // this function could hang up if USB is available, but MIDI port won't be
  // serviced by the host (e.g. windows: no program uses the MIDI IN port)
  // Therefore we time out the polling after 10000 tries
  // Once the timeout value is reached, each new MIDI_PackageSend call will
  // try to access the USB port only a single time anymore. Once the try
  // was successfull (MIDI port will be used by host), timeout value is
  // reset again

  s32 error;

  while( (error=MIOS32_USB_MIDI_PackageSend_NonBlocking(package)) == -2 ) {
    if( timeout_ctr >= 10000 )
      break;
    ++timeout_ctr;
  }

  if( error >= 0 ) // no error: reset timeout counter
    timeout_ctr = 0;

  return error;
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return -1 if no package in buffer
//! \return >= 0: number of packages which are still in the buffer
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_PackageReceive(mios32_midi_package_t *package)
{
  // package received?
  if( !rx_buffer_size )
    return -1;

  // get package - this operation should be atomic!
  MIOS32_IRQ_Disable();
  package->ALL = rx_buffer[rx_buffer_tail];
  if( ++rx_buffer_tail >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
    rx_buffer_tail = 0;
  --rx_buffer_size;
  MIOS32_IRQ_Enable();

  return rx_buffer_size;
}



/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to handle timeout
//! and expire counters.
//!
//! For USB MIDI it also checks for incoming/outgoing USB packages!
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_MIDI_Periodic_mS(void)
{
  // check for received packages
  MIOS32_USB_MIDI_RxBufferHandler();

  // check for packages which should be transmitted
  MIOS32_USB_MIDI_TxBufferHandler();

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// This handler sends the new packages through the IN pipe if the buffer 
// is not empty
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_USB_MIDI_TxBufferHandler(void)
{
  // send buffered packages if
  //   - last transfer finished
  //   - new packages are in the buffer
  //   - the device is configured

  // atomic operation to avoid conflict with other interrupts
  MIOS32_IRQ_Disable();
#ifdef STM32F10X_CL
  if( !tx_buffer_busy && tx_buffer_size && transfer_possible ) {
    u32 ep_num = EP1_IN & 0x7f;
    s16 count = (tx_buffer_size > (MIOS32_USB_MIDI_DATA_IN_SIZE/4)) ? (MIOS32_USB_MIDI_DATA_IN_SIZE/4) : tx_buffer_size;

    USB_OTG_DSTS_TypeDef dsts;  
    USB_OTG_Status status = USB_OTG_OK;

    // Set transfer size
    OTG_FS_DEPTSIZx_TypeDef deptsiz;
    deptsiz.d32 = 0;
    deptsiz.b.xfersize = count * 4;
    deptsiz.b.pktcnt = 1;
    USB_OTG_WRITE_REG32(&USB_OTG_FS_regs.DINEPS[ep_num]->DIEPTSIZx, deptsiz.d32);

    // Enable the Tx FIFO Empty Interrupt for this EP
    uint32_t fifoemptymsk = 1 << ep_num;
    USB_OTG_MODIFY_REG32(&USB_OTG_FS_regs.DEV->DIEPEMPMSK, 0, fifoemptymsk);

    /* EP enable, IN data in FIFO */
    USB_OTG_DEPCTLx_TypeDef depctl;
    depctl.d32 = USB_OTG_READ_REG32(&(USB_OTG_FS_regs.DINEPS[ep_num]->DIEPCTLx));
    depctl.b.cnak = 1;
    depctl.b.epena = 1;
    USB_OTG_WRITE_REG32(&USB_OTG_FS_regs.DINEPS[ep_num]->DIEPCTLx, depctl.d32); 

    // notify that new package is sent
    tx_buffer_busy = 1;

    // send to IN pipe
    tx_buffer_size -= count;

    // copy into EP FIFO
    __IO uint32_t *fifo = USB_OTG_FS_regs.FIFO[ep_num];
    do {
      USB_OTG_WRITE_REG32(fifo, tx_buffer[tx_buffer_tail]);
      if( ++tx_buffer_tail >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
	tx_buffer_tail = 0;
    } while( --count );
  }
#else
  if( !tx_buffer_busy && tx_buffer_size && transfer_possible ) {
    u32 *pma_addr = (u32 *)(PMAAddr + (MIOS32_USB_ENDP1_TXADDR<<1));
    s16 count = (tx_buffer_size > (MIOS32_USB_MIDI_DATA_IN_SIZE/4)) ? (MIOS32_USB_MIDI_DATA_IN_SIZE/4) : tx_buffer_size;

    // notify that new package is sent
    tx_buffer_busy = 1;

    // send to IN pipe
    SetEPTxCount(ENDP1, 4*count);

    tx_buffer_size -= count;

    // copy into PMA buffer (16bit word with, only 32bit addressable)
    do {
      *pma_addr++ = tx_buffer[tx_buffer_tail] & 0xffff;
      *pma_addr++ = (tx_buffer[tx_buffer_tail]>>16) & 0xffff;
      if( ++tx_buffer_tail >= MIOS32_USB_MIDI_TX_BUFFER_SIZE )
	tx_buffer_tail = 0;
    } while( --count );

    // send buffer
    SetEPTxValid(ENDP1);
  }
#endif
  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
// This handler receives new packages if the Tx buffer is not full
/////////////////////////////////////////////////////////////////////////////
static void MIOS32_USB_MIDI_RxBufferHandler(void)
{
  s16 count;

  // atomic operation to avoid conflict with other interrupts
  MIOS32_IRQ_Disable();

  // check if we can receive new data and get packages to be received from OUT pipe
#ifdef STM32F10X_CL
  USB_OTG_EP *ep = PCD_GetOutEP(EP2_OUT & 0x7f);
  if( rx_buffer_new_data && (count=ep->xfer_len>>2) ) {
    // check if buffer is free
    if( count < (MIOS32_USB_MIDI_RX_BUFFER_SIZE-rx_buffer_size) ) {
      u32 *buf_addr = (u32 *)&ep->xfer_buff[0];

      // copy received packages into receive buffer
      // this operation should be atomic
      do {
	mios32_midi_package_t package;
	package.ALL = *buf_addr++;

	if( MIOS32_MIDI_SendPackageToRxCallback(USB0 + package.cable, package) == 0 ) {
	  rx_buffer[rx_buffer_head] = package.ALL;

	  if( ++rx_buffer_head >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
	    rx_buffer_head = 0;
	  ++rx_buffer_size;
	}
      } while( --count > 0 );

      // notify, that data has been put into buffer
      rx_buffer_new_data = 0;

      // configuration for next transfer
      ep->xfer_len = 0; // OTGD_FS_EPStartXfer will set maximum size in this case
      ep->xfer_count = 0; // clear counter to ensure that it will be set by LLD again
      ep->is_in = 0; // out endpoint
      ep->num = EP2_OUT & 0x7F;

      OTGD_FS_EPStartXfer(ep);
    }
  }
#else
  if( rx_buffer_new_data && (count=GetEPRxCount(ENDP2)>>2) ) {

    // check if buffer is free
    if( count < (MIOS32_USB_MIDI_RX_BUFFER_SIZE-rx_buffer_size) ) {
      u32 *pma_addr = (u32 *)(PMAAddr + (MIOS32_USB_ENDP2_RXADDR<<1));

      // copy received packages into receive buffer
      // this operation should be atomic
      do {
	u16 pl = *pma_addr++;
	u16 ph = *pma_addr++;
	mios32_midi_package_t package;
	package.ALL = (ph << 16) | pl;

	if( MIOS32_MIDI_SendPackageToRxCallback(USB0 + package.cable, package) == 0 ) {
	  rx_buffer[rx_buffer_head] = package.ALL;

	  if( ++rx_buffer_head >= MIOS32_USB_MIDI_RX_BUFFER_SIZE )
	    rx_buffer_head = 0;
	  ++rx_buffer_size;
	}
      } while( --count > 0 );

      // notify, that data has been put into buffer
      rx_buffer_new_data = 0;

      // release OUT pipe
      SetEPRxValid(ENDP2);
    }
  }
#endif
  MIOS32_IRQ_Enable();
}


/////////////////////////////////////////////////////////////////////////////
//! Called by STM32 USB driver to check for IN streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
//! \note also: bEP, bEPStatus only relevant for LPC17xx port
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP1_IN_Callback(u8 bEP, u8 bEPStatus)
{
  // package has been sent
  tx_buffer_busy = 0;
  
  // check for next package
  MIOS32_USB_MIDI_TxBufferHandler();
}

/////////////////////////////////////////////////////////////////////////////
//! Called by STM32 USB driver to check for OUT streams
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
//! \note also: bEP, bEPStatus only relevant for LPC17xx port
/////////////////////////////////////////////////////////////////////////////
void MIOS32_USB_MIDI_EP2_OUT_Callback(u8 bEP, u8 bEPStatus)
{
  // put package into buffer
  rx_buffer_new_data = 1;
  MIOS32_USB_MIDI_RxBufferHandler();
}

//! \}

#endif /* MIOS32_DONT_USE_USB_MIDI */

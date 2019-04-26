// $Id: mios32_spi_midi.c 2096 2014-12-05 22:04:15Z tk $
//! \defgroup MIOS32_SPI_MIDI
//!
//! SPI MIDI layer for MIOS32
//! 
//! Applications shouldn't call these functions directly, instead please
//! use \ref MIOS32_MIDI layer functions
//! 
//! \{
/* ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
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
#if !defined(MIOS32_DONT_USE_SPI_MIDI)


/////////////////////////////////////////////////////////////////////////////
// Local definitions
/////////////////////////////////////////////////////////////////////////////

#if !defined(MIOS32_SPI_MIDI_MUTEX_TAKE)
#define MIOS32_SPI_MIDI_USE_MUTEX 0
#define MIOS32_SPI_MIDI_MUTEX_TAKE {}
#define MIOS32_SPI_MIDI_MUTEX_GIVE {}
#else
#define MIOS32_SPI_MIDI_USE_MUTEX 1
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Variables
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SPI_MIDI_NUM_PORTS > 0
// TX double buffer toggles between each scan
static u32 tx_upstream_buffer[2][MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE];
static u8 tx_upstream_buffer_select;

// RX downstream buffer used to temporary store new words from current scan
static u32 rx_downstream_buffer[MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE];

// RX ring buffer
static u32 rx_ringbuffer[MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE];

#if MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE > 255 || MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE > 255
# error "Please adapt size pointers!"
#endif
static u8 tx_buffer_head;

static u8 rx_ringbuffer_tail;
static u8 rx_ringbuffer_head;
static u8 rx_ringbuffer_size;

// indicates ongoing scan
static volatile u8 transfer_done;

#if defined MIOS32_SPI_MIDI_USE_M16
static u16 m16_rx_act;
static u16 m16_tx_act;
static u16 m16_ovl_act;
static mios32_spim_m16_gpio_mode_t  m16_gpio_mode[3];
static u16 m16_gpio_inv[3];
static u16 m16_gpio_val[3];
static u16 rs_optimisation;
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Local Prototypes
/////////////////////////////////////////////////////////////////////////////

#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static s32 MIOS32_SPI_MIDI_InitScanBuffer(u32 *buffer);
static void MIOS32_SPI_MIDI_DMA_Callback(void);
#if defined MIOS32_SPI_MIDI_USE_M16
static s32 MIOS32_SPI_MIDI_M16_StatReceive(u32 word);
static s32 (*m16_stat_callback_func)(mios32_spim_m16_cmd_t stat_cmd, u16 stat_val);
#endif
#endif


/////////////////////////////////////////////////////////////////////////////
// Initializes the SPI Master
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_Init(u32 mode)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  s32 status = 0;

  if( mode != 0 )
    return -1; // currently only mode 0 supported

  if( !MIOS32_SPI_MIDI_Enabled() )
    return -3; // SPI MIDI device hasn't been enabled in MIOS32 bootloader

  // deactivate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // ensure that fast pin drivers are activated
  MIOS32_SPI_IO_Init(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_PIN_DRIVER_STRONG);

  // starting with first half of the double buffer
  tx_upstream_buffer_select = 0;

  // last transfer done (to allow next transfer)
  transfer_done = 1;

  // init buffer pointers
  tx_buffer_head = 0;
  rx_ringbuffer_tail = rx_ringbuffer_head = rx_ringbuffer_size = 0;

  // init double buffers
  MIOS32_SPI_MIDI_InitScanBuffer((u32 *)&tx_upstream_buffer[0]);
  MIOS32_SPI_MIDI_InitScanBuffer((u32 *)&tx_upstream_buffer[1]);

#if defined MIOS32_SPI_MIDI_USE_M16
  m16_stat_callback_func = NULL;
  int i;
  MIOS32_SPIM_M16_RxStatEnable(1); 	// Set RX status On
  MIOS32_SPIM_M16_TxStatEnable(1);	// Set TX status On
  MIOS32_SPIM_M16_OvlStatEnable(1);	// Set TX buffer Overload status Off
  for(i=0;i<3; i++){
	  // All GPIO Groups set to OUT and not inverted by default
	  MIOS32_SPIM_M16_GPIO_Grp_ModeSet(i, M16_GPIO_MODE_OUT);
	  MIOS32_SPIM_M16_GPIO_Grp_InvSet(i, 0x0000);
	  MIOS32_SPIM_M16_GPIO_Grp_Set(i, 0x0000);
  }
  MIOS32_SPIM_M16_SofEnable(0);
#endif

  return status;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! \returns != 0 if SPI MIDI has been enabled in MIOS32 bootloader
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_Enabled(void)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return 0; // SPI MIDI interface not explicitely enabled in mios32_config.h
#else
  u8 *spi_midi_confirm = (u8 *)MIOS32_SYS_ADDR_SPI_MIDI_CONFIRM;
  u8 *spi_midi = (u8 *)MIOS32_SYS_ADDR_SPI_MIDI;
  if( *spi_midi_confirm == 0x42 && *spi_midi < 0x80 )
    return *spi_midi;

  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function checks the availability of a SPI MIDI port as configured
//! with MIOS32_SPI_MIDI_NUM_PORTS
//!
//! \param[in] spi_midi_port module number (0..7)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_CheckAvailable(u8 spi_midi_port)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return 0; // SPI MIDI interface not explicitely enabled in mios32_config.h
#else
  return (spi_midi_port < MIOS32_SPI_MIDI_NUM_PORTS) ? MIOS32_SPI_MIDI_Enabled() : 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Available for m16 interface only
//! This function enables/disables running status optimisation for a given
//! MIDI OUT port to improve bandwidth if MIDI events with the same
//! status byte are sent back-to-back.<BR>
//! Note that the optimisation is enabled by default.
//! \param[in] uart_port UART number (0..15)
//! \param[in] enable 0=optimisation disabled, 1=optimisation enabled
//! \return -1 if port not available
//! \return 0 on success
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_RS_OptimisationSet(u8 spim_port, u8 enable)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // all SPIMs explicitely disabled
#else
#if defined MIOS32_SPI_MIDI_USE_M16
  if( spim_port >= MIOS32_SPI_MIDI_NUM_PORTS )
    return -1; // port not available

  u16 mask = 1 << spim_port;
  rs_optimisation &= ~mask;
  if( enable )rs_optimisation |= mask;
  mios32_midi_package_t p;
  p.cin_cable = 0x01;
  p.evnt0 = 0x10;
  p.evnt1 = (u8)(rs_optimisation>>8);
  p.evnt2 = (u8)(rs_optimisation&0xff);
  MIOS32_SPI_MIDI_PackageSend(p);

  return 0; // no error
#else
  return -1; // feature not available
#endif
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! Available for m16 interface only
//! This function returns the running status optimisation enable/disable flag
//! for the given MIDI OUT port.
//! \param[in] uart_port UART number (0..2)
//! \return -1 if port not available
//! \return 0 if optimisation disabled
//! \return 1 if optimisation enabled
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_RS_OptimisationGet(u8 spim_port)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // all SPIMs explicitely disabled
#else
#if defined MIOS32_SPI_MIDI_USE_M16
  if( spim_port >= MIOS32_SPI_MIDI_NUM_PORTS )
    return -1; // port not available

  return (rs_optimisation & (1 << spim_port)) ? 1 : 0;
#else
  return -1; // feature not available
#endif
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function should be called periodically each mS to initiate a new
//! SPI scan
//!
//! Not for use in an application - this function is called from
//! MIOS32_MIDI_Periodic_mS(), which is called by a task in the programming
//! model!
//! 
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_Periodic_mS(void)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return 0; // SPI MIDI not activated (no error)
#else
  if( !MIOS32_SPI_MIDI_Enabled() ){
	  //DEBUG_MSG("poll,err:-3");
    return -3; // SPI MIDI device hasn't been enabled in MIOS32 bootloader
  }
  if( !transfer_done ){
	  //DEBUG_MSG("poll,err:-2");
    return -2; // previous transfer not finished yet
  }
  // following operation should be atomic
  MIOS32_IRQ_Disable();

  // last TX buffer
  u32 *last_tx = (u32 *)&tx_upstream_buffer[tx_upstream_buffer_select];

  // next TX buffer
  tx_upstream_buffer_select = tx_upstream_buffer_select ? 0 : 1;
  u32 *next_tx = (u32 *)&tx_upstream_buffer[tx_upstream_buffer_select];

  // init last TX buffer for next words
  MIOS32_SPI_MIDI_InitScanBuffer(last_tx);
  tx_buffer_head = 0;

  // start next transfer
  transfer_done = 0;

  MIOS32_IRQ_Enable();

  // take over access over SPI port
  MIOS32_SPI_MIDI_MUTEX_TAKE;

  // init SPI
  MIOS32_SPI_TransferModeInit(MIOS32_SPI_MIDI_SPI,
			      MIOS32_SPI_MODE_CLK1_PHASE1,
			      MIOS32_SPI_MIDI_SPI_PRESCALER);

  // activate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 0); // spi, rc_pin, pin_value

  // start next transfer
  MIOS32_SPI_TransferBlock(MIOS32_SPI_MIDI_SPI,
			   (u8 *)next_tx, (u8 *)rx_downstream_buffer,
			   4*MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE,
			   MIOS32_SPI_MIDI_DMA_Callback);

#if MIOS32_SPI_MIDI_USE_MUTEX
  // workaround - search for a better way to release mutex from ISR
  // it's currently not possible to release it from MIOS32_SPI_MIDI_DMA_Callback()
  while( !transfer_done );
  MIOS32_SPI_MIDI_MUTEX_GIVE;
#endif

  return 0; // no error
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Called after DMA transfer finished
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static void MIOS32_SPI_MIDI_DMA_Callback(void)
{
  // deactivate CS output
  MIOS32_SPI_RC_PinSet(MIOS32_SPI_MIDI_SPI, MIOS32_SPI_MIDI_SPI_RC_PIN, 1); // spi, rc_pin, pin_value

  // release access over SPI port
  //MIOS32_SPI_MIDI_MUTEX_GIVE;
  // doesn't work - see MIOS32_SPI_MIDI_USE_MUTEX workaround in MIOS32_SPI_MIDI_Periodic_mS

  // transfer RX values into ringbuffer (if possible)
  if( rx_ringbuffer_size < MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE ) {
    int i;

    // atomic operation to avoid conflict with other interrupts
    MIOS32_IRQ_Disable();

    // search for valid MIDI events in downstream buffer, and put them into the receive ringbuffer
    u32 *rx_buffer = (u32 *)&rx_downstream_buffer[0];
    for(i=0; i<MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE; ++i) {
      u32 word = *rx_buffer++;

      if( word != 0xffffffff && word != 0x00000000 ) {

#if defined MIOS32_SPI_MIDI_USE_M16
		if((word &0x0f000000) == 0x01000000){
			// parser
			MIOS32_SPI_MIDI_M16_StatReceive(word);
		}else{
#endif
	    	mios32_midi_package_t p;
			p.cin_cable = word >> 24;
			p.evnt0 = word >> 16;
			p.evnt1 = word >> 8;
			p.evnt2 = word >> 0;
			rx_ringbuffer[rx_ringbuffer_head] = p.ALL;

			if( ++rx_ringbuffer_head >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
			  rx_ringbuffer_head = 0;

			if( ++rx_ringbuffer_size >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
			  break; // ringbuffer full :-( - TODO: add rx error counter
		      }
		    }
#if defined MIOS32_SPI_MIDI_USE_M16
		}
#endif

    MIOS32_IRQ_Enable();
  }

  // transfer finished
  transfer_done = 1;
}
#endif


/////////////////////////////////////////////////////////////////////////////
// This function puts a new MIDI package into the Tx buffer
// \param[in] package MIDI package
// \return 0: no error
// \return -1: SPI not configured
// \return -2: buffer is full
//             caller should retry until buffer is free again
// \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  if( !MIOS32_SPI_MIDI_Enabled() ){
    return -3; // SPI MIDI device hasn't been enabled in MIOS32 bootloader
  }
  // buffer full?
  if( tx_buffer_head >= MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE ) {
    // flush buffer if possible
    // (this call simplifies polling loops!)
    MIOS32_SPI_MIDI_Periodic_mS();

    // notify that buffer was full (request retry)
    return -2;
  }

  // since data will be transmitted bytewise, we've to swap the order
  u32 word = (package.cin_cable << 24) | (package.evnt0 << 16) | (package.evnt1 << 8) | (package.evnt2 << 0);

  // put package into buffer - this operation should be atomic!
  MIOS32_IRQ_Disable();
  u8 next_select = tx_upstream_buffer_select ? 0 : 1;
  tx_upstream_buffer[next_select][tx_buffer_head] = word;
  ++tx_buffer_head;
  MIOS32_IRQ_Enable();

  return 0;
#endif
}

/////////////////////////////////////////////////////////////////////////////
//! This function puts a new MIDI package into the Tx buffer
//! (blocking function)
//! \param[in] package MIDI package
//! \return 0: no error
//! \return -1: SPI not configured
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageSend(mios32_midi_package_t package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  if( !MIOS32_SPI_MIDI_Enabled() )
    return -3; // SPI MIDI device hasn't been enabled in MIOS32 bootloader

  static u16 timeout_ctr = 0;
  // this function could hang up if SPI receive buffer not empty and data
  // should be sent.
  // Therefore we time out the polling after 10000 tries
  // Once the timeout value is reached, each new MIDI_PackageSend call will
  // try to access the SPI only a single time anymore. Once the try
  // was successfull (transfer done and receive buffer empty), timeout value is
  // reset again

  s32 error;

  while( (error=MIOS32_SPI_MIDI_PackageSend_NonBlocking(package)) == -2 ) {
    if( timeout_ctr >= 10000 )
      break;
    ++timeout_ctr;
  }

  if( error >= 0 ) // no error: reset timeout counter
    timeout_ctr = 0;

  return error;
#endif
}


/////////////////////////////////////////////////////////////////////////////
//! This function checks for a new package
//! \param[out] package pointer to MIDI package (received package will be put into the given variable)
//! \return -1 if no package in buffer
//! \return >= 0: number of packages which are still in the buffer
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_PackageReceive(mios32_midi_package_t *package)
{
#if MIOS32_SPI_MIDI_NUM_PORTS == 0
  return -1; // SPI MIDI not activated
#else
  if( !MIOS32_SPI_MIDI_Enabled() )
    return -3; // SPI MIDI device hasn't been enabled in MIOS32 bootloader

  // package received?
  if( !rx_ringbuffer_size )
    return -1;

  // get package - this operation should be atomic!
  MIOS32_IRQ_Disable();
  package->ALL = rx_ringbuffer[rx_ringbuffer_tail];
  if( ++rx_ringbuffer_tail >= MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE )
    rx_ringbuffer_tail = 0;
  --rx_ringbuffer_size;
  MIOS32_IRQ_Enable();

  return rx_ringbuffer_size;
#endif
}



/////////////////////////////////////////////////////////////////////////////
// Invalidates a buffer with all-1
/////////////////////////////////////////////////////////////////////////////
#if MIOS32_SPI_MIDI_NUM_PORTS > 0
static s32 MIOS32_SPI_MIDI_InitScanBuffer(u32 *buffer)
{
  int i;

  for(i=0; i<MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE; ++i) {
    *buffer++ = 0xffffffff;
  }

  return 0; // no error
}
#endif

#if defined MIOS32_SPI_MIDI_USE_M16
/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
//! Installs an optional SysEx callback which is called by
//! MIOS32_SPI_MIDI_M16_StatReceive() to simplify the parsing of m16 statuses.
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_StatCallback_Init(s32 (*callback_m16_stat)(mios32_spim_m16_cmd_t stat_cmd, u16 stat_val))
{
	m16_stat_callback_func = callback_m16_stat;

  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_RxStatEnable(u8 enable){
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = M16_CMD_RX_STAT;
	p.evnt1 = 0x00;
	p.evnt2 = enable;
	return MIOS32_SPI_MIDI_PackageSend(p);
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_TxStatEnable(u8 enable){
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = M16_CMD_TX_STAT;
	p.evnt1 = 0x00;
	p.evnt2 = enable;
	return MIOS32_SPI_MIDI_PackageSend(p);
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_OvlStatEnable(u8 enable){
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = M16_CMD_OVL_STAT;
	p.evnt1 = 0x00;
	p.evnt2 = enable;
	return MIOS32_SPI_MIDI_PackageSend(p);
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPI_MIDI_M16_StatReceive(u32 word){
	u8 cmd = (u8)(word>>16);
	switch(cmd){
	case M16_CMD_RX_STAT:
		if((word & 0x0000ffff) != m16_rx_act){
		  m16_rx_act = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_rx_act);
		  }
		}
		break;
	case M16_CMD_TX_STAT:
		if((word & 0x0000ffff) != m16_tx_act){
		  m16_tx_act = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_tx_act);
		  }
		}
		break;
	case M16_CMD_OVL_STAT:
		if((word & 0x0000ffff) != m16_ovl_act){
		  m16_ovl_act = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_ovl_act);
		  }
		}
		break;
	case M16_CMD_GPIO_BASE:
		if((word & 0x0000ffff) != m16_gpio_val[0]){
		  m16_gpio_val[0] = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_gpio_val[0]);
		  }
		}
		break;
	case (M16_CMD_GPIO_BASE+0x10):
		if((word & 0x0000ffff) != m16_gpio_val[1]){
		  m16_gpio_val[1] = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_gpio_val[1]);
		  }
		}
		break;
	case (M16_CMD_GPIO_BASE+0x20):
		if((word & 0x0000ffff) != m16_gpio_val[2]){
		  m16_gpio_val[2] = (u16)(word & 0x0000ffff);
		  if( m16_stat_callback_func != NULL ) {
			m16_stat_callback_func(cmd, m16_gpio_val[2]);
		  }
		}
		break;
	default:
		break;
	}
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Grp_ModeSet(u8 gpio_grp, mios32_spim_m16_gpio_mode_t mode){
	if(gpio_grp>2)return -1; //GPIO Group not available
	//set value
	m16_gpio_mode[gpio_grp]=mode;
	//send command
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = (gpio_grp<<4)+ M16_CMD_GPIO_BASE + 0x02;
	p.evnt1 = 0x00;
	p.evnt2 = (u8)m16_gpio_mode[gpio_grp];
	MIOS32_SPI_MIDI_PackageSend(p);
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
mios32_spim_m16_gpio_mode_t MIOS32_SPIM_M16_GPIO_Grp_ModeGet(u8 gpio_grp){
	if(gpio_grp>2)return -1; //GPIO Group not available
	return 	m16_gpio_mode[gpio_grp]; // no error
}
/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Grp_InvSet(u8 gpio_grp,u16 value){
	if(gpio_grp>2)return -1; //GPIO Group not available
	//set value
	m16_gpio_inv[gpio_grp]=value;
	//send command
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = (gpio_grp<<4)+ M16_CMD_GPIO_BASE + 0x01;
	p.evnt1 = (u8)(m16_gpio_inv[gpio_grp]>>8);
	p.evnt2 = (u8)(m16_gpio_inv[gpio_grp]&0xff);
	MIOS32_SPI_MIDI_PackageSend(p);
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Grp_InvGet(u8 gpio_grp){
	if(gpio_grp>2)return -1; //GPIO Group not available
	return 	m16_gpio_inv[gpio_grp]; // no error
}
/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
//! \param[in] GPIO Group (0...2) A to C
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Grp_Set(u8 gpio_grp,u16 value){
	if(gpio_grp>2)return -1; //GPIO Group not available
	//set value
	m16_gpio_val[gpio_grp]=value;
	//send command
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = (gpio_grp<<4)+M16_CMD_GPIO_BASE;
	p.evnt1 = (u8)(m16_gpio_val[gpio_grp]>>8);
	p.evnt2 = (u8)(m16_gpio_val[gpio_grp]&0xff);
	MIOS32_SPI_MIDI_PackageSend(p);
	return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Grp_Get(u8 gpio_grp){
	if(gpio_grp>2)return -1; //GPIO Group not available
	return 	m16_gpio_val[gpio_grp]; // no error
}
/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_InvSet(u8 gpio,u8 value){
	if(gpio>48)return -1; //GPIO Pin not available
	u8 gpio_grp = gpio>>4;
	u16 mask = 1 << (gpio &0x0f);
	m16_gpio_inv[gpio_grp] &= ~mask;
	if( value )m16_gpio_inv[gpio_grp] |= mask;
	return MIOS32_SPIM_M16_GPIO_Grp_InvSet(gpio_grp, m16_gpio_inv[gpio_grp]); // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_InvGet(u8 gpio){
	if(gpio>48)return -1; //GPIO Pin not available
	u8 gpio_grp = gpio>>4;
	u16 mask = 1 << (gpio &0x0f);
	return ((m16_gpio_inv[gpio_grp] & mask)? 1 : 0);
}
/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Set(u8 gpio,u8 value){
	if(gpio>48)return -1; //GPIO Pin not available
	u8 gpio_grp = gpio>>4;
	u16 mask = 1 << (gpio &0x0f);
	m16_gpio_val[gpio_grp] &= ~mask;
	if( value )m16_gpio_val[gpio_grp] |= mask;
	return MIOS32_SPIM_M16_GPIO_Grp_InvSet(gpio_grp, m16_gpio_val[gpio_grp]); // no error
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_GPIO_Get(u8 gpio){
	if(gpio>48)return -1; //GPIO Pin not available
	u8 gpio_grp = gpio>>4;
	u16 mask = 1 << (gpio &0x0f);
	return ((m16_gpio_val[gpio_grp] & mask)? 1 : 0);
}

/////////////////////////////////////////////////////////////////////////////
//! m16 specific function
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_SPIM_M16_SofEnable(u8 enable){
	mios32_midi_package_t p;
	p.cin_cable = 0x01;
	p.evnt0 = M16_CMD_SOF_ENA;
	p.evnt1 = 0;
	p.evnt2 = enable;
	return MIOS32_SPI_MIDI_PackageSend(p);
}

#endif
//! \}

#endif /* MIOS32_DONT_USE_SPI_MIDI */

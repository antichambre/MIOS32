// $Id: mios32_spi_midi.h 2097 2014-12-05 22:05:12Z tk $
/*
 * Header file for SPI MIDI functions
 *
 * ==========================================================================
 *
 *  Copyright (C) 2014 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_SPI_MIDI_H
#define _MIOS32_SPI_MIDI_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// how many SPI MIDI ports are available?
// if 0: interface disabled (default)
// other allowed values: 1..16
#ifndef MIOS32_SPI_MIDI_NUM_PORTS
#if defined MIOS32_SPI_MIDI_USE_M16
#define MIOS32_SPI_MIDI_NUM_PORTS 16
#else
#define MIOS32_SPI_MIDI_NUM_PORTS 0
#endif
#endif

// Which SPI peripheral should be used
// allowed values: 0 (J16), 1 (J8/9) and 2 (J19)
#ifndef MIOS32_SPI_MIDI_SPI
#if defined MIOS32_SPI_MIDI_USE_M16
#define MIOS32_SPI_MIDI_SPI 2
#else
#define MIOS32_SPI_MIDI_SPI 0
#endif
#endif

// Which RC pin of the SPI port should be used
// allowed values: 0 or 1 for SPI0 (J16:RC1, J16:RC2), 0 for SPI1 (J8/9:RC), 0 or 1 for SPI2 (J19:RC1, J19:RC2)
#ifndef MIOS32_SPI_MIDI_SPI_RC_PIN
#if defined MIOS32_SPI_MIDI_USE_M16
#define MIOS32_SPI_MIDI_SPI_RC_PIN 1
#else
#define MIOS32_SPI_MIDI_SPI_RC_PIN 1
#endif
#endif

// Which transfer rate should be used?
#ifndef MIOS32_SPI_MIDI_SPI_PRESCALER
#if defined MIOS32_SPI_MIDI_USE_M16
// MIOS32_SPI_PRESCALER_8 typically results into ca. 10 MBit/s
#define MIOS32_SPI_MIDI_SPI_PRESCALER MIOS32_SPI_PRESCALER_8
#else
// MIOS32_SPI_PRESCALER_16 typically results into ca. 5 MBit/s
#define MIOS32_SPI_MIDI_SPI_PRESCALER MIOS32_SPI_PRESCALER_16
#endif
#endif

// DMA buffer size
// Note: should match with transfer rate
// e.g. with MIOS32_SPI_PRESCALER_64 (typically ca. 2 MBit) the transfer of 1 word takes ca. 18 uS
// MIOS32_SPI_MIDI_Tick() is serviced each mS
// Accordingly, we shouldn't send more than 55 words, taking 50 seems to be a good choice.
#ifndef MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE
#if defined MIOS32_SPI_MIDI_USE_M16
#define MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE 48
#else
#define MIOS32_SPI_MIDI_SCAN_BUFFER_SIZE 16
#endif
#endif

// note also, that the resulting RAM consumption will be 4*4*MIOS32_SPI_MIDI_BUFFER_SIZE bytes (Rx/Tx double buffers for 4byte words)

// we've a separate RX ringbuffer which stores received MIDI events until the
// application processes them
#ifndef MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE
#define MIOS32_SPI_MIDI_RX_RINGBUFFER_SIZE 64
#endif



/////////////////////////////////////////////////////////////////////////////
// Global Types
/////////////////////////////////////////////////////////////////////////////
#if defined MIOS32_SPI_MIDI_USE_M16
typedef enum {
  M16_CMD_SPI_DEBUG     = 0x01, 	// Set SPI bus in Loop Mode
  M16_CMD_UART_DEBUG    = 0x02, 	// Set all UARTs in Loop Mode
  M16_CMD_RX_STAT       = 0x03,		// Enable UARTs RX Status reception
  M16_CMD_TX_STAT       = 0x04, 	// Enable UARTs TX Status reception
  M16_CMD_OVL_STAT      = 0x05, 	// Enable UARTs TX Overload Status reception
  M16_CMD_SOF_ENA       = 0x0f,		// Enable m16 Sign of life Led
  M16_CMD_TX_RS       	= 0x10,		// Enable UARTs MIDI Running Status
  M16_CMD_GPIO_BASE    	= 0xa0		// Command for GPIO value
} mios32_spim_m16_cmd_t;

typedef enum {
  M16_GPIO_MODE_RX_STAT     = 0x00,		// Group is MIDI RX Activity
  M16_GPIO_MODE_TX_STAT     = 0x01, 	// Group is MIDI TX Activity
  M16_GPIO_MODE_OVL_STAT    = 0x02, 	// Group is MIDI TX Overoad Activity
  M16_GPIO_MODE_OUT    		= 0x03,		// Group is General Purpose Out
  M16_GPIO_MODE_IN    		= 0x04,		// Group is General Purpose In
} mios32_spim_m16_gpio_mode_t;
#endif

/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_SPI_MIDI_Init(u32 mode);

extern s32 MIOS32_SPI_MIDI_Enabled(void);

extern s32 MIOS32_SPI_MIDI_CheckAvailable(u8 spi_midi_port);
extern s32 MIOS32_SPI_MIDI_RS_OptimisationSet(u8 spim_port, u8 enable);
extern s32 MIOS32_SPI_MIDI_RS_OptimisationGet(u8 spim_port);
extern s32 MIOS32_SPI_MIDI_Periodic_mS(void);

extern s32 MIOS32_SPI_MIDI_PackageSend_NonBlocking(mios32_midi_package_t package);
extern s32 MIOS32_SPI_MIDI_PackageSend(mios32_midi_package_t package);
extern s32 MIOS32_SPI_MIDI_PackageReceive(mios32_midi_package_t *package);

#if defined MIOS32_SPI_MIDI_USE_M16
// m16 status callback install
extern s32 MIOS32_SPIM_M16_StatCallback_Init(s32 (*callback_m16_stat)(mios32_spim_m16_cmd_t stat_cmd, u16 stat_val));
// m16 UARTs qtatuses receive
extern s32 MIOS32_SPIM_M16_RxStatEnable(u8 enable);
extern s32 MIOS32_SPIM_M16_TxStatEnable(u8 enable);
extern s32 MIOS32_SPIM_M16_OvlStatEnable(u8 enable);
// m16 GPIO functions
extern s32 MIOS32_SPIM_M16_GPIO_Grp_ModeSet(u8 gpio_grp,mios32_spim_m16_gpio_mode_t mode);
extern mios32_spim_m16_gpio_mode_t MIOS32_SPIM_M16_GPIO_Grp_ModeGet(u8 gpio_grp);
extern s32 MIOS32_SPIM_M16_GPIO_Grp_InvSet(u8 gpio_grp,u16 value);
extern s32 MIOS32_SPIM_M16_GPIO_Grp_InvGet(u8 gpio_grp);
extern s32 MIOS32_SPIM_M16_GPIO_Grp_Set(u8 gpio_grp,u16 value);
extern s32 MIOS32_SPIM_M16_GPIO_Grp_Get(u8 gpio_grp);
extern s32 MIOS32_SPIM_M16_GPIO_InvSet(u8 gpio,u8 value);
extern s32 MIOS32_SPIM_M16_GPIO_InvGet(u8 gpio);
extern s32 MIOS32_SPIM_M16_GPIO_Set(u8 gpio,u8 value);
extern s32 MIOS32_SPIM_M16_GPIO_Get(u8 gpio);
// m16 options
extern s32 MIOS32_SPIM_M16_SofEnable(u8 enable);
#endif

/////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////



#endif /* _MIOS32_SPI_MIDI_H */

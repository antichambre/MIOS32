/**
 ******************************************************************************
 * @file    usbh_MIDI.h
 * @author  Xavier Halgand
 * @version
 * @date
 * @brief   This file contains all the prototypes for the usbh_MIDI.c
 ******************************************************************************
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* Define to prevent recursive  ----------------------------------------------*/
#ifndef __USBH_MIDI_CORE_H
#define __USBH_MIDI_CORE_H

#include <mios32.h>
#include "usbh_core.h"
#include "usbh_stdreq.h"
#include "usb_bsp.h"
#include "usbh_ioreq.h"
#include "usbh_hcs.h"
//#include "usbh_usr.h"

typedef enum
{
	MIDI_DATA=0,
	MIDI_POLL,
	MIDI_ERROR
}
MIDI_State_t;


typedef struct _MIDI_Process
{
	MIDI_State_t	state_out;
	MIDI_State_t	state_in;
	uint8_t			buff_in[USBH_MIDI_MPS_SIZE];
	uint8_t			buff_out[USBH_MIDI_MPS_SIZE];
	uint8_t			hc_num_in;
	uint8_t 		hc_num_out;
	uint8_t			MIDIBulkOutEp;
	uint8_t			MIDIBulkInEp;
	uint16_t		MIDIBulkInEpSize;
	uint16_t		MIDIBulkOutEpSize;
	//MIDI_cb_TypeDef *cb;
}
MIDI_Machine_TypeDef;

//
//typedef enum {
//	NoteOff       = 0x8,
//	NoteOn        = 0x9,
//	PolyPressure  = 0xa,
//	CC            = 0xb,
//	ProgramChange = 0xc,
//	Aftertouch    = 0xd,
//	PitchBend     = 0xe
//} midi_event_t;

typedef union {
	uint32_t all;
	struct {
		uint8_t cin_cable;
		union {
			uint8_t status;
			struct {
				uint8_t channel:4;
				uint8_t type:4;
			};
		};
		uint8_t data1;
		uint8_t data2;
	};
} MIDI_EventPacket_t;

extern USBH_Class_cb_TypeDef  MIDI_cb;

extern int MIDI_send(mios32_midi_package_t packet);
extern void MIDI_recv_cb(mios32_midi_package_t packet);
/*-------------------------------------------------------------------------------------------*/
#endif /* __USBH_MIDI_CORE_H */


/*****************************END OF FILE*************************************************************/


// $Id: mios32_usb_com.h 1156 2011-03-27 18:45:18Z tk $
/*
 * Header file for USB COM Driver
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIOS32_USB_HID_H
#define _MIOS32_USB_HID_H

/////////////////////////////////////////////////////////////////////////////
// Global definitions
/////////////////////////////////////////////////////////////////////////////

// size of IN/OUT pipe
#ifndef MIOS32_USB_HID_DATA_IN_SIZE
#define MIOS32_USB_HID_DATA_IN_SIZE            64
#endif
#ifndef MIOS32_USB_HID_DATA_OUT_SIZE
#define MIOS32_USB_Hid_DATA_OUT_SIZE           64
#endif

#ifndef MIOS32_USB_HID_INT_IN_SIZE
#define MIOS32_USB_HID_INT_IN_SIZE             64
#endif

  
/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIOS32_USB_HID_Init(u32 mode);

extern s32 MIOS32_USB_HID_CheckAvailable(u8 dev);

extern s32 MIOS32_USB_HID_Process(void);

////////////////////////////////////////////////////////////////////////////
// Export global variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIOS32_USB_HID_H */

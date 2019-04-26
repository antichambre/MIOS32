// $Id: midio_ain.h 1441 2012-03-04 19:50:32Z tk $
/*
 * AIN access functions for MIDIO128 V3
 *
 * ==========================================================================
 *
 *  Copyright (C) 2008 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 * 
 * ==========================================================================
 */

#ifndef _MIDIO_AIN_H
#define _MIDIO_AIN_H

/////////////////////////////////////////////////////////////////////////////
// global definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Type definitions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Prototypes
/////////////////////////////////////////////////////////////////////////////

extern s32 MIDIO_AIN_Init(u32 mode);
extern s32 MIDIO_AIN_NotifyChange(u32 pin, u32 pin_value);
extern s32 MIDIO_AIN_NotifyChange_SER64(u32 module, u32 pin, u32 pin_value);

/////////////////////////////////////////////////////////////////////////////
// Exported variables
/////////////////////////////////////////////////////////////////////////////


#endif /* _MIDIO_AIN_H */
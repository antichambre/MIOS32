// $Id: mios32_usb_com.c 1800 2013-06-02 22:09:03Z tk $
//! \defgroup MIOS32_USB_HS
//!
//! USB HID layer for MIOS32
//! 
//! Not supported for STM32F4 (yet)
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
// Include files & defines
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>

// this module can be optionally disabled in a local mios32_config.h file (included from mios32.h)
#if !defined(MIOS32_DONT_USE_USB_HOST) || !defined(MIOS32_DONT_USE_USB_HS_HOST)
#if !defined(MIOS32_DONT_USE_USB_HID)

#include <usb_conf.h>
#include <usbh_stdreq.h>
#include <usbh_ioreq.h>
#include <usb_bsp.h>
#include <usbh_core.h>
#include <usbh_conf.h>
#include <usb_hcd_int.h>
#include <usbh_hcs.h>

#include <string.h>

#ifndef DEBUG_MSG
#define DEBUG_MSG MIOS32_MIDI_SendDebugMessage
#endif

#define DEBUG_HID_VERBOSE_LEVEL 2

#define  LE32(addr)             (((u32)(*((u8 *)(addr))))\
                                + (((u32)(*(((u8 *)(addr)) + 1))) << 8)\
                                + (((u32)(*(((u8 *)(addr)) + 2))) << 16)\
                                + (((u32)(*(((u8 *)(addr)) + 3))) << 24))

#define  LES32(addr)             (((s32)(*((u8 *)(addr))))\
                                + (((s32)(*(((u8 *)(addr)) + 1))) << 8)\
                                + (((s32)(*(((u8 *)(addr)) + 2))) << 16)\
                                + (((s32)(*(((u8 *)(addr)) + 3))) << 24))

#define USB_HID_BOOT_CODE                                  0x01
#define USB_HID_KEYBRD_BOOT_CODE                           0x01
#define USB_HID_MOUSE_BOOT_CODE                            0x02
#define USB_HID_GAMPAD_BOOT_CODE                           0x05

#define USB_HID_REQ_GET_REPORT       0x01
#define USB_HID_GET_IDLE             0x02
#define USB_HID_GET_PROTOCOL         0x03
#define USB_HID_SET_REPORT           0x09
#define USB_HID_SET_IDLE             0x0A
#define USB_HID_SET_PROTOCOL         0x0B

#define HID_MIN_POLL          10

#define  KBD_LEFT_CTRL                                  0x01
#define  KBD_LEFT_SHIFT                                 0x02
#define  KBD_LEFT_ALT                                   0x04
#define  KBD_LEFT_GUI                                   0x08
#define  KBD_RIGHT_CTRL                                 0x10
#define  KBD_RIGHT_SHIFT                                0x20
#define  KBD_RIGHT_ALT                                  0x40
#define  KBD_RIGHT_GUI                                  0x80

#define  KBR_MAX_NBR_PRESSED                            6

#ifndef MIOS32_USB_HID_QWERTY_KEYBOARD
#define MIOS32_USB_HID_AZERTY_KEYBOARD
#endif

/////////////////////////////////////////////////////////////////////////////
// Local constants
/////////////////////////////////////////////////////////////////////////////
static const uint8_t USB_HID_MouseStatus[]    = "> Mouse connected\n";
static const uint8_t USB_HID_KeybrdStatus[]   = "> Keyboard connected\n";
static const uint8_t USB_HID_GampadStatus[]   = "> GamePad connected\n";

static  const  uint8_t  HID_KEYBRD_Codes[] = {
  0,     0,    0,    0,   31,   50,   48,   33,
  19,   34,   35,   36,   24,   37,   38,   39,       /* 0x00 - 0x0F */
  52,    51,   25,   26,   17,   20,   32,   21,
  23,   49,   18,   47,   22,   46,    2,    3,       /* 0x10 - 0x1F */
  4,    5,    6,    7,    8,    9,   10,   11,
  43,  110,   15,   16,   61,   12,   13,   27,       /* 0x20 - 0x2F */
  28,   29,   42,   40,   41,    1,   53,   54,
  55,   30,  112,  113,  114,  115,  116,  117,       /* 0x30 - 0x3F */
  118,  119,  120,  121,  122,  123,  124,  125,
  126,   75,   80,   85,   76,   81,   86,   89,       /* 0x40 - 0x4F */
  79,   84,   83,   90,   95,  100,  105,  106,
  108,   93,   98,  103,   92,   97,  102,   91,       /* 0x50 - 0x5F */
  96,  101,   99,  104,   45,  129,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x60 - 0x6F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x70 - 0x7F */
  0,    0,    0,    0,    0,  107,    0,   56,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x80 - 0x8F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0x90 - 0x9F */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xA0 - 0xAF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xB0 - 0xBF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xC0 - 0xCF */
  0,    0,    0,    0,    0,    0,    0,    0,
  0,    0,    0,    0,    0,    0,    0,    0,       /* 0xD0 - 0xDF */
  58,   44,   60,  127,   64,   57,   62,  128        /* 0xE0 - 0xE7 */
};

#ifdef QWERTY_KEYBOARD
static  const  int8_t  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',
  '7',  '8',  '9',  '0',  '-',  '=',  '\0', '\r',
  '\t',  'q',  'w',  'e',  'r',  't',  'y',  'u',
  'i',  'o',  'p',  '[',  ']',  '\\',
  '\0',  'a',  's',  'd',  'f',  'g',  'h',  'j',
  'k',  'l',  ';',  '\'', '\0', '\n',
  '\0',  '\0', 'z',  'x',  'c',  'v',  'b',  'n',
  'm',  ',',  '.',  '/',  '\0', '\0',
  '\0',  '\0', '\0', ' ',  '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\r', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '7',  '4',  '1',
  '\0',  '/',  '8',  '5',  '2',
  '0',   '*',  '9',  '6',  '3',
  '.',   '-',  '+',  '\0', '\n', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0'
};

static  const  int8_t  HID_KEYBRD_ShiftKey[] = {
  '\0', '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',
  '_',  '+',  '\0', '\0', '\0', 'Q',  'W',  'E',  'R',  'T',  'Y',  'U',
  'I',  'O',  'P',  '{',  '}',  '|',  '\0', 'A',  'S',  'D',  'F',  'G',
  'H',  'J',  'K',  'L',  ':',  '"',  '\0', '\n', '\0', '\0', 'Z',  'X',
  'C',  'V',  'B',  'N',  'M',  '<',  '>',  '?',  '\0', '\0',  '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0',    '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

#else

static  const  int8_t  HID_KEYBRD_Key[] = {
  '\0',  '`',  '1',  '2',  '3',  '4',  '5',  '6',  '7',  '8',  '9',  '0',
  '-',  '=',  '\0', '\r', '\t',  'a',  'z',  'e',  'r',  't',  'y',  'u',
  'i',  'o',  'p',  '[',  ']', '\\', '\0',  'q',  's',  'd',  'f',  'g',
  'h',  'j',  'k',  'l',  'm',  '\0', '\0', '\n', '\0',  '\0', 'w',  'x',
  'c',  'v',  'b',  'n',  ',',  ';',  ':',  '!',  '\0', '\0', '\0',  '\0',
  '\0', ' ',  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0',  '\0', '\0', '\0', '\0', '\r', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0',  '\0', '7',  '4',  '1','\0',  '/',
  '8',  '5',  '2', '0',   '*',  '9',  '6',  '3', '.',   '-',  '+',  '\0',
  '\n', '\0', '\0', '\0', '\0', '\0', '\0','\0',  '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};

static  const  int8_t  HID_KEYBRD_ShiftKey[] = {
  '\0', '~',  '!',  '@',  '#',  '$',  '%',  '^',  '&',  '*',  '(',  ')',  '_',
  '+',  '\0', '\0', '\0', 'A',  'Z',  'E',  'R',  'T',  'Y',  'U',  'I',  'O',
  'P',  '{',  '}',  '*', '\0', 'Q',  'S',  'D',  'F',  'G',  'H',  'J',  'K',
  'L',  'M',  '%',  '\0', '\n', '\0', '\0', 'W',  'X',  'C',  'V',  'B',  'N',
  '?',  '.',  '/',  '\0',  '\0', '\0','\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',
  '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0'
};
#endif

/////////////////////////////////////////////////////////////////////////////
// Local types
/////////////////////////////////////////////////////////////////////////////
/* States for HID State Machine */
typedef enum
{
  HID_IDLE= 0,
  HID_SEND_DATA,
  HID_BUSY,
  HID_GET_DATA,
  HID_SYNC,
  HID_POLL,
  HID_ERROR,
}
HID_State;

typedef enum
{
  HID_REQ_IDLE = 0,
  HID_REQ_GET_REPORT_DESC,
  HID_REQ_GET_HID_DESC,
  HID_REQ_SET_IDLE,
  HID_REQ_SET_PROTOCOL,
  HID_REQ_SET_REPORT,

}
HID_CtlState;

typedef enum
{
  HID_REPORT_ITEM_In      = 0,
  HID_REPORT_ITEM_Out     = 1,
  HID_REPORT_ITEM_Feature = 2,
}
HID_ReportItemTypes_t;

typedef struct HID_cb
{
  void  (*Init)   (void);
  void  (*Decode) (USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data);

} USB_HID_cb_t;

typedef  struct  _HID_Report
{
    uint8_t   ReportID;
    uint8_t   ReportType;
    uint16_t  UsagePage;
    uint32_t  Usage[2];
    uint32_t  NbrUsage;
    uint32_t  UsageMin;
    uint32_t  UsageMax;
    int32_t   LogMin;
    int32_t   LogMax;
    int32_t   PhyMin;
    int32_t   PhyMax;
    int32_t   UnitExp;
    uint32_t  Unit;
    uint32_t  ReportSize;
    uint32_t  ReportCnt;
    uint32_t  Flag;
    uint32_t  PhyUsage;
    uint32_t  AppUsage;
    uint32_t  LogUsage;
}
USB_HID_Report_t;

/* Structure for HID process */
typedef struct _HID_Process
{
  uint8_t			   transfer_possible;
  volatile uint8_t 	   start_toggle;
  uint8_t              buff[64];
  uint8_t              hc_num_in;
  uint8_t              hc_num_out;
  HID_State            state;
  uint8_t              HIDIntOutEp;
  uint8_t              HIDIntInEp;
  HID_CtlState         ctl_state;
  uint16_t             length;
  uint8_t              ep_addr;
  uint16_t             poll;
  __IO uint16_t        timer;
  USB_HID_cb_t         *cb;
}
USB_HID_machine_t;

typedef struct _HID_MOUSE_Data
{
  uint8_t              x;
  uint8_t              y;
  uint8_t              z;               /* Not Supported */
  uint8_t              button;
}
USB_HID_Mouse_Data_t;

typedef union _HID_KEYBOARD_State{
  struct {
    uint8_t ALL;
  };
  struct
  {
    uint8_t              num_lock:1;
    uint8_t              caps_lock:1;
    uint8_t              scroll_lock:1;
    uint8_t              dummy:5;
  };
}USB_HID_Keyboard_Stat_t;

/////////////////////////////////////////////////////////////////////////////
// Variables
/////////////////////////////////////////////////////////////////////////////

#ifndef MIOS32_DONT_USE_USB_HOST
// imported from mios32_usb.c
extern USB_OTG_CORE_HANDLE  USB_OTG_dev;
extern USBH_HOST USB_Host;
extern USBH_Class_Status USB_Host_Class;
__ALIGN_BEGIN USB_HID_machine_t  	USB_FS_HID_machine __ALIGN_END ;
//__ALIGN_BEGIN USB_HID_Report_t   	USB_FS_HID_Report __ALIGN_END ;
//__ALIGN_BEGIN USB_Setup_TypeDef  	USB_FS_HID_Setup __ALIGN_END ;
__ALIGN_BEGIN USBH_HIDDesc_TypeDef  USB_FS_HID_Desc __ALIGN_END ;
#endif


#ifndef MIOS32_DONT_USE_USB_HS_HOST
// imported from mios32_usb.c
extern USB_OTG_CORE_HANDLE  USB_OTG_HS_dev;
extern USBH_HOST USB_HS_Host;
extern USBH_Class_Status USB_HS_Host_Class;

__ALIGN_BEGIN USB_HID_machine_t  	USB_HS_HID_machine __ALIGN_END ;
//__ALIGN_BEGIN USB_HID_Report_t   	USB_HS_HID_Report __ALIGN_END ;
//__ALIGN_BEGIN USB_Setup_TypeDef  	USB_HS_HID_Setup __ALIGN_END ;
__ALIGN_BEGIN USBH_HIDDesc_TypeDef  USB_HS_HID_Desc __ALIGN_END ;

#endif


/////////////////////////////////////////////////////////////////////////////
// Local prototypes
/////////////////////////////////////////////////////////////////////////////

static void  MIOS32_USB_HID_Mouse_Init (void);
static void  MIOS32_USB_HID_Mouse_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data);

USB_HID_cb_t HID_MOUSE_cb=
{
	MIOS32_USB_HID_Mouse_Init,
	MIOS32_USB_HID_Mouse_Decode
};

USB_HID_Mouse_Data_t Mouse_Data;


static void  MIOS32_USB_HID_Keyboard_Init (void);
static void  MIOS32_USB_HID_Keyboard_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *pbuf);

USB_HID_cb_t HID_KEYBRD_cb=
{
	MIOS32_USB_HID_Keyboard_Init,
	MIOS32_USB_HID_Keyboard_Decode
};

USB_HID_Keyboard_Stat_t Keyboard_State;
static u8 Keyboard_OldState;

static USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev,
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId);
static USBH_Status USBH_Set_Report (USB_OTG_CORE_HANDLE *pdev,
                                    USBH_HOST *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t reportLen,
                                    uint8_t* reportBuff);



/**
 * @brief  USR_MOUSE_Init
 *         Init Mouse window
 * @param  None
 * @retval None
 */
void MIOS32_USB_HID_Mouse_Init	(void)
{
#if DEBUG_HID_VERBOSE_LEVEL >= 2
  DEBUG_MSG((void*)USB_HID_MouseStatus);
  DEBUG_MSG("\n\n\n\n\n\n\n\n");
#endif
}

/**
 * @brief  USR_MOUSE_ProcessData
 *         Process Mouse data
 * @param  data : Mouse data to be displayed
 * @retval None
 */
void MIOS32_USB_HID_Mouse_Decode(USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *data)
{
	Mouse_Data.button = data[0];

	Mouse_Data.x      = data[1];
	Mouse_Data.y      = data[2];
	uint8_t idx = 1;
	static uint8_t b_state[3] = { 0, 0 , 0};

	if ((Mouse_Data.x != 0) && (Mouse_Data.y != 0))
	{
		//HID_MOUSE_UpdatePosition(data->x , data->y);
#if DEBUG_HID_VERBOSE_LEVEL >= 1
    DEBUG_MSG("Mouse position, x:%d, y:%d \n", (s8)Mouse_Data.x, (s8)Mouse_Data.y);
#endif
	
	}

	for ( idx = 0 ; idx < 3 ; idx ++)
	{

		if(Mouse_Data.button & 1 << idx)
		{
			if(b_state[idx] == 0)
			{
				//HID_MOUSE_ButtonPressed (idx);
#if DEBUG_HID_VERBOSE_LEVEL >= 1
        DEBUG_MSG("Mouse button %d pressed \n", idx);
#endif
				b_state[idx] = 1;
			}
		}
		else
		{
			if(b_state[idx] == 1)
			{
				//HID_MOUSE_ButtonReleased (idx);
#if DEBUG_HID_VERBOSE_LEVEL >= 1
        DEBUG_MSG("Mouse button %d pressed \n", idx);
#endif
				b_state[idx] = 0;
			}
		}
	}
}

/**
 * @brief  USR_KEYBRD_Init
 *         Init Keyboard window
 * @param  None
 * @retval None
 */
void  MIOS32_USB_HID_Keyboard_Init (void)
{
  Keyboard_State.ALL=0;
  Keyboard_OldState = 0;
#if DEBUG_HID_VERBOSE_LEVEL >= 1
  DEBUG_MSG((void*)USB_HID_KeybrdStatus);
  
  DEBUG_MSG("> Use Keyboard to tape characters: \n\n");
  DEBUG_MSG("\n\n\n\n\n\n");
#endif

}


/**
 * @brief  USR_KEYBRD_ProcessData
 *         Process Keyboard data
 * @param  data : Keyboard data to be displayed
 * @retval None
 */
void  MIOS32_USB_HID_Keyboard_Decode (USB_OTG_CORE_HANDLE *pdev , void  *phost, uint8_t *pbuf)
{
	static  uint8_t   shift;
	static  uint8_t   keys[KBR_MAX_NBR_PRESSED];
	static  uint8_t   keys_new[KBR_MAX_NBR_PRESSED];
	static  uint8_t   keys_last[KBR_MAX_NBR_PRESSED];
	static  uint8_t   key_newest;
	static  uint8_t   nbr_keys;
	static  uint8_t   nbr_keys_new;
	static  uint8_t   nbr_keys_last;

	uint8_t   ix;
	uint8_t   jx;
	uint8_t   error;
	uint8_t   output;
 
   USBH_Status status = USBH_OK;
   USBH_HOST *pphost = phost;
//   if(pdev->cfg.coreID == USB_OTG_FS_CORE_ID){
// #ifndef MIOS32_DONT_USE_USB_HOST
//     machine = &USB_FS_HID_machine;
//     //MIOS32_MIDI_DebugPortSet(UART0);
// #else
//     return USBH_NOT_SUPPORTED; //
// #endif
//   }else{
// #ifndef MIOS32_DONT_USE_USB_HS_HOST
//     machine = &USB_HS_HID_machine;
//     //MIOS32_MIDI_DebugPortSet(USB0);
// #else
//     return USBH_NOT_SUPPORTED; //
// #endif
//  }
	nbr_keys      = 0;
	nbr_keys_new  = 0;
	nbr_keys_last = 0;
	key_newest    = 0x00;


	/* Check if Shift key is pressed */
	if ((pbuf[0] == KBD_LEFT_SHIFT) || (pbuf[0] == KBD_RIGHT_SHIFT)) {
		shift = 1;
	} else {
		shift = 0;
	}
 
	error = 0;

	/* Check for the value of pressed key */
	for (ix = 2; ix < 2 + KBR_MAX_NBR_PRESSED; ix++) {
		if ((pbuf[ix] == 0x01) ||
				(pbuf[ix] == 0x02) ||
				(pbuf[ix] == 0x03)) {
			error = 1;
		}
	}
	if (error == 1) {
		return;
	}

  
	nbr_keys     = 0;
	nbr_keys_new = 0;
	for (ix = 2; ix < 2 + KBR_MAX_NBR_PRESSED; ix++) {
		if (pbuf[ix] != 0) {
			keys[nbr_keys] = pbuf[ix];
			nbr_keys++;
			for (jx = 0; jx < nbr_keys_last; jx++) {
				if (pbuf[ix] == keys_last[jx]) {
					break;
				}
			}

			if (jx == nbr_keys_last) {
				keys_new[nbr_keys_new] = pbuf[ix];
				nbr_keys_new++;
			}
		}
	}
  
	if (nbr_keys_new == 1) {
		key_newest = keys_new[0];
    
    switch (key_newest) {
      case 0x53:
        // caps_lock
        Keyboard_State.num_lock = ~Keyboard_State.num_lock;
        break;
      case 0x39:
        // caps_lock
        Keyboard_State.caps_lock = ~Keyboard_State.caps_lock;
        break;
      case 0x47:
        // scroll_lock
        Keyboard_State.scroll_lock = ~Keyboard_State.scroll_lock;
        break;
      default:
        if (((shift == 1)&& (Keyboard_State.caps_lock==0)) ||((shift == 0)&& (Keyboard_State.caps_lock==1))) {
          output =  HID_KEYBRD_ShiftKey[HID_KEYBRD_Codes[key_newest]];
        } else {
          output =  HID_KEYBRD_Key[HID_KEYBRD_Codes[key_newest]];
        }
        
        /* call user process handle */ // toDo callback!
        //keyboard_callback(output);
        
#if DEBUG_HID_VERBOSE_LEVEL >= 1
        DEBUG_MSG("KB data: %c", output);
#endif
        break;
    }

  } else {
    key_newest = 0x00;
  }
  if(Keyboard_OldState != Keyboard_State.ALL){
    //      u8 count = 0;
    //      do
    //      {
    //        USBH_Set_Idle (pdev, pphost, 0x01, 0x00);
    status = USBH_Set_Report(pdev, pphost, 0x02, 0x00, 0x01, &Keyboard_State.ALL);
    if(status==0)Keyboard_OldState = Keyboard_State.ALL;
       DEBUG_MSG("USBH_Set_Report stat:%d", (u8)(status));
    //        count++;
    //      }
    //      while((status != 0) && (count<=6));
  }
  nbr_keys_last  = nbr_keys;
  for (ix = 0; ix < KBR_MAX_NBR_PRESSED; ix++) {
    keys_last[ix] = keys[ix];
  }
  
  
}



  

/////////////////////////////////////////////////////////////////////////////
//! Initializes USB HID layer
//! \param[in] mode currently only mode 0 supported
//! \return < 0 if initialisation failed
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_Init(u32 mode)
{
DEBUG_MSG("MIOS32_USB_HID_Init PASSED");
#ifndef MIOS32_DONT_USE_USB_HOST
	USB_FS_HID_machine.start_toggle = 0;
#endif


#ifndef MIOS32_DONT_USE_USB_HS_HOST
	USB_HS_HID_machine.start_toggle = 0;
#endif

  return 0;
}

/////////////////////////////////////////////////////////////////////////////
//! This function is called by the USB driver on cable connection/disconnection
//! \param[in] device number (0 for OTG_FS, 1 for OTG_HS)
//! \return < 0 on errors
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_MIDI layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_ChangeConnectionState(u8 dev, u8 connected)
{
  if(dev>1)return -1;
#ifndef MIOS32_DONT_USE_USB_HOST
  if(dev == 0){
	  if( connected ) {
		  USB_FS_HID_machine.transfer_possible = 1;
	  } else {
		// device disconnected: disable transfers
		  USB_FS_HID_machine.transfer_possible = 0;
	  }
  }
#endif
#ifndef MIOS32_DONT_USE_USB_HS_HOST
  if(dev == 1){
	  if( connected ) {
		  USB_HS_HID_machine.transfer_possible = 1;
	  } else {
		// device disconnected: disable transfers
		  USB_HS_HID_machine.transfer_possible = 0;
	  }
  }
#endif
	return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
//! This function returns the connection status of the USB COM interface
//! \param[in] device number (0 for OTG_FS, 1 for OTG_HS)
//! \return 1: interface available
//! \return 0: interface not available
//! \note Applications shouldn't call this function directly, instead please use \ref MIOS32_COM layer functions
/////////////////////////////////////////////////////////////////////////////
s32 MIOS32_USB_HID_CheckAvailable(u8 dev)
{
#ifndef MIOS32_DONT_USE_USB_HOST
  if(dev == 0){
	  return USB_FS_HID_machine.transfer_possible ? 1 : 0;
  }
#endif
#ifndef MIOS32_DONT_USE_USB_HS_HOST
  if(dev == 1){
	  return USB_HS_HID_machine.transfer_possible ? 1 : 0;
  }
#endif
	return -1; // not available
}



s32 MIOS32_USB_HID_Process(void)
{
	//USBH_Process(&USB_OTG_HS_dev, &USB_HS_Host);
	//if(USB_HS_Host_Class == USBH_IS_HID)USBH_Process(&USB_OTG_HS_dev, &USB_HS_Host);
	return 0;
}



/**
* @brief  USBH_Get_HID_ReportDescriptor
*         Issue report Descriptor command to the device. Once the response
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Report Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_HID_ReportDescriptor (USB_OTG_CORE_HANDLE *pdev,
                                                  USBH_HOST *phost,
                                                  uint16_t length)
{

  USBH_Status status;

  status = USBH_GetDescriptor(pdev,
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,
                                USB_DESC_HID_REPORT,
                                pdev->host.Rx_Buffer,
                                length);

  /* HID report descriptor is available in pdev->host.Rx_Buffer.
  In case of USB Boot Mode devices for In report handling ,
  HID report descriptor parsing is not required.
  In case, for supporting Non-Boot Protocol devices and output reports,
  user may parse the report descriptor*/


  return status;
}

/**
* @brief  USBH_Get_HID_Descriptor
*         Issue HID Descriptor command to the device. Once the response
*         received, parse the report descriptor and update the status.
* @param  pdev   : Selected device
* @param  Length : HID Descriptor Length
* @retval USBH_Status : Response for USB HID Get Report Descriptor Request
*/
static USBH_Status USBH_Get_HID_Descriptor (USB_OTG_CORE_HANDLE *pdev,
                                            USBH_HOST *phost,
                                            uint16_t length)
{

  USBH_Status status;

  status = USBH_GetDescriptor(pdev,
                              phost,
                              USB_REQ_RECIPIENT_INTERFACE
                                | USB_REQ_TYPE_STANDARD,
                                USB_DESC_HID,
                                pdev->host.Rx_Buffer,
                                length);

  return status;
}

/**
* @brief  USBH_Set_Idle
*         Set Idle State.
* @param  pdev: Selected device
* @param  duration: Duration for HID Idle request
* @param  reportID : Targetted report ID for Set Idle request
* @retval USBH_Status : Response for USB Set Idle request
*/
USBH_Status USBH_Set_Idle (USB_OTG_CORE_HANDLE *pdev,
                                  USBH_HOST *phost,
                                  uint8_t duration,
                                  uint8_t reportId)
{

  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_IDLE;
  phost->Control.setup.b.wValue.w = (duration << 8 ) | reportId;

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0 , 0 );
}


/**
* @brief  USBH_Set_Report
*         Issues Set Report
* @param  pdev: Selected device
* @param  reportType  : Report type to be sent
* @param  reportID    : Targetted report ID for Set Report request
* @param  reportLen   : Length of data report to be send
* @param  reportBuff  : Report Buffer
* @retval USBH_Status : Response for USB Set Idle request
*/
USBH_Status USBH_Set_Report (USB_OTG_CORE_HANDLE *pdev,
                                 USBH_HOST *phost,
                                    uint8_t reportType,
                                    uint8_t reportId,
                                    uint8_t reportLen,
                                    uint8_t* reportBuff)
{

  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_REPORT;
  phost->Control.setup.b.wValue.w = (reportType << 8 ) | reportId;

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = reportLen;

  return USBH_CtlReq(pdev, phost, reportBuff , reportLen );
}


/**
* @brief  USBH_Set_Protocol
*         Set protocol State.
* @param  pdev: Selected device
* @param  protocol : Set Protocol for HID : boot/report protocol
* @retval USBH_Status : Response for USB Set Protocol request
*/
static USBH_Status USBH_Set_Protocol(USB_OTG_CORE_HANDLE *pdev,
                                     USBH_HOST *phost,
                                     uint8_t protocol)
{


  phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE |\
    USB_REQ_TYPE_CLASS;


  phost->Control.setup.b.bRequest = USB_HID_SET_PROTOCOL;

  if(protocol != 0)
  {
    /* Boot Protocol */
    phost->Control.setup.b.wValue.w = 0;
  }
  else
  {
    /*Report Protocol*/
    phost->Control.setup.b.wValue.w = 1;
  }

  phost->Control.setup.b.wIndex.w = 0;
  phost->Control.setup.b.wLength.w = 0;

  return USBH_CtlReq(pdev, phost, 0 , 0 );

}

/**
* @brief  USBH_ParseHIDDesc
*         This function Parse the HID descriptor
* @param  buf: Buffer where the source descriptor is available
* @retval None
*/
static void  USBH_ParseHIDDesc (USBH_HIDDesc_TypeDef *desc, uint8_t *buf)
{

  desc->bLength                  = *(uint8_t  *) (buf + 0);
  desc->bDescriptorType          = *(uint8_t  *) (buf + 1);
  desc->bcdHID                   =  LE16  (buf + 2);
  desc->bCountryCode             = *(uint8_t  *) (buf + 4);
  desc->bNumDescriptors          = *(uint8_t  *) (buf + 5);
  desc->bReportDescriptorType    = *(uint8_t  *) (buf + 6);
  desc->wItemLength              =  LE16  (buf + 7);

}




/** @defgroup USBH_HID_CORE_Private_Functions
* @{
*/

/**
* @brief  USBH_HID_InterfaceInit
*         The function init the HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB HID driver intialization
*/
static USBH_Status USBH_InterfaceInit ( USB_OTG_CORE_HANDLE *pdev, void *phost)
{
	uint8_t maxEP;
	USBH_HOST *pphost = phost;

	uint8_t num =0;
	USB_HID_machine_t* machine;
	u8 dev = 0;
	if(pdev->cfg.coreID == USB_OTG_FS_CORE_ID){
#ifndef MIOS32_DONT_USE_USB_HOST
		machine = &USB_FS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(UART0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}else{
#ifndef MIOS32_DONT_USE_USB_HS_HOST
		dev = 1;
		machine = &USB_HS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(USB0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}

	MIOS32_USB_HID_ChangeConnectionState(dev, 0);
	int i;
	machine->state = HID_ERROR;

	for(i=0; i<pphost->device_prop.Cfg_Desc.bNumInterfaces && i < USBH_MAX_NUM_INTERFACES; ++i) {

		if(pphost->device_prop.Itf_Desc[i].bInterfaceSubClass  == USB_HID_BOOT_CODE)
		{
			/*Decode Bootclass Protocl: Mouse or Keyboard*/
			if(pphost->device_prop.Itf_Desc[i].bInterfaceProtocol == USB_HID_KEYBRD_BOOT_CODE)
			{
				machine->cb = &HID_KEYBRD_cb;
			}
			else if(pphost->device_prop.Itf_Desc[i].bInterfaceProtocol  == USB_HID_MOUSE_BOOT_CODE)
			{
				machine->cb = &HID_MOUSE_cb;
			}

			machine->state     = HID_IDLE;
			machine->ctl_state = HID_REQ_IDLE;
			machine->ep_addr   = pphost->device_prop.Ep_Desc[i][0].bEndpointAddress;
			machine->length    = pphost->device_prop.Ep_Desc[i][0].wMaxPacketSize;
			machine->poll      = pphost->device_prop.Ep_Desc[i][0].bInterval ;

			if (machine->poll  < HID_MIN_POLL)
			{
				machine->poll = HID_MIN_POLL;
			}


			/* Check fo available number of endpoints */
			/* Find the number of EPs in the Interface Descriptor */
			/* Choose the lower number in order not to overrun the buffer allocated */
			maxEP = ( (pphost->device_prop.Itf_Desc[i].bNumEndpoints <= USBH_MAX_NUM_ENDPOINTS) ?
					pphost->device_prop.Itf_Desc[i].bNumEndpoints :
					USBH_MAX_NUM_ENDPOINTS);


			/* Decode endpoint IN and OUT address from interface descriptor */
			for (num=0; num < maxEP; num++)
			{
				if(pphost->device_prop.Ep_Desc[i][num].bEndpointAddress & 0x80)
				{
					machine->HIDIntInEp = (pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);
					machine->hc_num_in  =\
							USBH_Alloc_Channel(pdev,
									pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);

					/* Open channel for IN endpoint */
					USBH_Open_Channel  (pdev,
							machine->hc_num_in,
							pphost->device_prop.address,
							pphost->device_prop.speed,
							EP_TYPE_INTR,
							machine->length);
				}
				else
				{
					machine->HIDIntOutEp = (pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);
					machine->hc_num_out  =\
							USBH_Alloc_Channel(pdev,
									pphost->device_prop.Ep_Desc[i][num].bEndpointAddress);

					/* Open channel for OUT endpoint */
					USBH_Open_Channel  (pdev,
							machine->hc_num_out,
							pphost->device_prop.address,
							pphost->device_prop.speed,
							EP_TYPE_INTR,
							machine->length);
				}

			}

		    MIOS32_USB_HID_ChangeConnectionState(dev, 1);
		    machine->start_toggle = 0;
		    break;
		}
	}
	if( MIOS32_USB_HID_CheckAvailable(dev) == 0 ) {
		pphost->usr_cb->DeviceNotSupported();
		return USBH_NOT_SUPPORTED;
	}
	return USBH_OK;

}



/**
* @brief  USBH_HID_InterfaceDeInit
*         The function DeInit the Host Channels used for the HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval None
*/
void USBH_InterfaceDeInit ( USB_OTG_CORE_HANDLE *pdev,
		void *phost)
{
	USB_HID_machine_t* machine;
	u8 dev = 0;
	if(pdev->cfg.coreID == USB_OTG_FS_CORE_ID){
#ifndef MIOS32_DONT_USE_USB_HOST
		machine = &USB_FS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(UART0);
#else
		return; //
#endif
	}else{
#ifndef MIOS32_DONT_USE_USB_HS_HOST
		dev = 1;
		machine = &USB_HS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(USB0);
#else
		return; //
#endif
	}
	if(machine->hc_num_in != 0x00)
	{
		USB_OTG_HC_Halt(pdev, machine->hc_num_in);
		USBH_Free_Channel  (pdev, machine->hc_num_in);
		machine->hc_num_in = 0;     /* Reset the Channel as Free */
	}

	if(machine->hc_num_out != 0x00)
	{
		USB_OTG_HC_Halt(pdev, machine->hc_num_out);
		USBH_Free_Channel  (pdev, machine->hc_num_out);
		machine->hc_num_out = 0;     /* Reset the Channel as Free */
	}
	MIOS32_USB_HID_ChangeConnectionState(dev, 0);
	machine->start_toggle = 0;
}

/**
* @brief  USBH_HID_ClassRequest
*         The function is responsible for handling HID Class requests
*         for HID class.
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval  USBH_Status :Response for USB Set Protocol request
*/
static USBH_Status USBH_ClassRequest(USB_OTG_CORE_HANDLE *pdev ,
		void *phost)
{
	USBH_HOST *pphost = phost;
	USBH_Status status         = USBH_BUSY;
	USBH_Status classReqStatus = USBH_BUSY;
	USB_HID_machine_t* machine;
	USBH_HIDDesc_TypeDef*  desc;
	if(pdev->cfg.coreID == USB_OTG_FS_CORE_ID){
#ifndef MIOS32_DONT_USE_USB_HOST
		machine = &USB_FS_HID_machine;
		desc = &USB_FS_HID_Desc;
		//MIOS32_MIDI_DebugPortSet(UART0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}else{
#ifndef MIOS32_DONT_USE_USB_HS_HOST
		machine = &USB_HS_HID_machine;
		desc = &USB_HS_HID_Desc;
		//MIOS32_MIDI_DebugPortSet(USB0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}

	/* Switch HID state machine */
	switch (machine->ctl_state)
	{
	case HID_IDLE:
	case HID_REQ_GET_HID_DESC:

		/* Get HID Desc */
		if (USBH_Get_HID_Descriptor (pdev, pphost, USB_HID_DESC_SIZE)== USBH_OK)
		{

			USBH_ParseHIDDesc(desc, pdev->host.Rx_Buffer);
			machine->ctl_state = HID_REQ_GET_REPORT_DESC;
		}

		break;
	case HID_REQ_GET_REPORT_DESC:


		/* Get Report Desc */
		if (USBH_Get_HID_ReportDescriptor(pdev , pphost, desc->wItemLength) == USBH_OK)
		{
			machine->ctl_state = HID_REQ_SET_IDLE;
		}

		break;

	case HID_REQ_SET_IDLE:

		classReqStatus = USBH_Set_Idle (pdev, pphost, 0, 0);

		/* set Idle */
		if (classReqStatus == USBH_OK)
		{
			machine->ctl_state = HID_REQ_SET_PROTOCOL;
		}
		else if(classReqStatus == USBH_NOT_SUPPORTED)
		{
			machine->ctl_state = HID_REQ_SET_PROTOCOL;
		}
		break;

	case HID_REQ_SET_PROTOCOL:
		/* set protocol */
		if (USBH_Set_Protocol (pdev ,pphost, 0) == USBH_OK)
		{
			machine->ctl_state = HID_REQ_IDLE;

			/* all requests performed*/
			status = USBH_OK;
		}
		break;

	default:
		break;
	}

	return status;
}


/**
* @brief  USBH_HID_Handle
*         The function is for managing state machine for HID data transfers
* @param  pdev: Selected device
* @param  hdev: Selected device property
* @retval USBH_Status
*/
static USBH_Status USBH_Handle(USB_OTG_CORE_HANDLE *pdev , void   *phost)
{
	USBH_HOST *pphost = phost;
	USBH_Status status = USBH_OK;
	USB_HID_machine_t* machine;
	if(pdev->cfg.coreID == USB_OTG_FS_CORE_ID){
#ifndef MIOS32_DONT_USE_USB_HOST
		machine = &USB_FS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(UART0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}else{
#ifndef MIOS32_DONT_USE_USB_HS_HOST
		machine = &USB_HS_HID_machine;
		//MIOS32_MIDI_DebugPortSet(USB0);
#else
		return USBH_NOT_SUPPORTED; //
#endif
	}

  switch (machine->state)
  {

  case HID_IDLE:
    machine->cb->Init();
    machine->state = HID_SYNC;

  case HID_SYNC:

    /* Sync with start of Even Frame */
    if(USB_OTG_IsEvenFrame(pdev) == TRUE)
    {
      machine->state = HID_GET_DATA;
    }
    break;

  case HID_GET_DATA:

    USBH_InterruptReceiveData(pdev,
        machine->buff,
        machine->length,
        machine->hc_num_in);
    machine->start_toggle = 1;

    machine->state = HID_POLL;
    machine->timer = HCD_GetCurrentFrame(pdev);
    break;

  case HID_POLL:
    if(( HCD_GetCurrentFrame(pdev) - machine->timer) >= machine->poll)
    {
      machine->state = HID_GET_DATA;
    }
    else if(HCD_GetURB_State(pdev , machine->hc_num_in) == URB_DONE)
    {
      if(machine->start_toggle == 1) /* handle data once */
      {
        machine->start_toggle = 0;
        machine->cb->Decode(pdev, phost, machine->buff);
      }
    }
    else if(HCD_GetURB_State(pdev, machine->hc_num_in) == URB_STALL) /* IN Endpoint Stalled */
    {

      /* Issue Clear Feature on interrupt IN endpoint */
      if( (USBH_ClrFeature(pdev,
          pphost,
          machine->ep_addr,
          machine->hc_num_in)) == USBH_OK)
      {
        /* Change state to issue next IN token */
        machine->state = HID_GET_DATA;

      }

    }
    break;

  default:
    break;
  }
  return status;
}

const USBH_Class_cb_TypeDef MIOS32_HID_USBH_Callbacks = {
  USBH_InterfaceInit,
  USBH_InterfaceDeInit,
  USBH_ClassRequest,
  USBH_Handle
};

#endif /* !defined(MIOS32_DONT_USE_USB_HOST) || !defined(MIOS32_DONT_USE_USB_HS_HOST) */

//! \}

#endif /* MIOS32_USE_USB_HID */

// $Id: app_lcd.c 2179 2015-06-10 18:36:14Z hawkeye $
/*
 * Application specific OLED driver for up to 1 * SSD1322 (more toDo)
 * Referenced from MIOS32_LCD routines
 *
 * ==========================================================================
 *
 *  Copyright (C) 2011 Thorsten Klose (tk@midibox.org)
 *  Licensed for personal non-commercial use only.
 *  All other rights reserved.
 *
 * ==========================================================================
 */

/////////////////////////////////////////////////////////////////////////////
// Include files
/////////////////////////////////////////////////////////////////////////////

#include <mios32.h>
#include <glcd_font.h>
//#include <glcd_font_4bit.h>
#include "app_lcd.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

/////////////////////////////////////////////////////////////////////////////
// Local defines
/////////////////////////////////////////////////////////////////////////////

// 0: J15 pins are configured in Push Pull Mode (3.3V)
// 1: J15 pins are configured in Open Drain mode (perfect for 3.3V->5V levelshifting)
#ifndef APP_LCD_OUTPUT_MODE
#define APP_LCD_OUTPUT_MODE  0
#endif

// for LPC17 only: should J10 be used for CS lines
// This option is nice if no J15 shiftregister is connected to a LPCXPRESSO.
// This shiftregister is available on the MBHP_CORE_LPC17 module
#ifndef APP_LCD_USE_J10_FOR_CS
#define APP_LCD_USE_J10_FOR_CS 0
#endif

/////////////////////////////////////////////////////////////////////////////
// Local variables
/////////////////////////////////////////////////////////////////////////////

static u32 display_available = 0;

// default color for legacy 1Bit bitmap
u8 app_lcd_back_grayscale = 0;
u8 app_lcd_fore_grayscale = 0;


// font bitmap
static mios32_lcd_bitmap_t font_bmp;

/////////////////////////////////////////////////////////////////////////////
// Initializes application specific LCD driver
// IN: <mode>: optional configuration
// OUT: returns < 0 if initialisation failed
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Init(u32 mode)
{
  // currently only mode 0 supported
  if( mode != 0 )
    return -1; // unsupported mode

  if( MIOS32_BOARD_J15_PortInit(APP_LCD_OUTPUT_MODE) < 0 )
    return -2; // failed to initialize J15

#if APP_LCD_USE_J10_FOR_CS
  int pin;
  for(pin=0; pin<8; ++pin)
    MIOS32_BOARD_J10_PinInit(pin, APP_LCD_OUTPUT_MODE ? MIOS32_BOARD_PIN_MODE_OUTPUT_OD : MIOS32_BOARD_PIN_MODE_OUTPUT_PP);
#endif

  // set LCD type
  mios32_lcd_parameters.lcd_type = MIOS32_LCD_TYPE_GLCD_CUSTOM;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_X;
  mios32_lcd_parameters.width = APP_LCD_WIDTH;
  mios32_lcd_parameters.num_x = APP_LCD_NUM_Y;
  mios32_lcd_parameters.height = APP_LCD_HEIGHT;
  mios32_lcd_parameters.colour_depth = APP_LCD_COLOUR_DEPTH;
  
  // set default(startup) forecolor to full white
  APP_LCD_FColourSet((u32)0xf);

  // hardware reset, we use CS1 as reset
  u16 ctr;
    MIOS32_BOARD_J15_DataSet(~(1<<1));
  // wait for some
  for (ctr=0; ctr<1000; ++ctr)
    MIOS32_DELAY_Wait_uS(1000);
  // end of reset
  MIOS32_BOARD_J15_DataSet(1<<1);
  
  // Initialize LCD
    APP_LCD_Cmd(0x11); //Exit Sleep
  for (ctr=0; ctr<50; ++ctr)
    MIOS32_DELAY_Wait_uS(1000);

    APP_LCD_Cmd(0x26);  //Set Default Gamma
    APP_LCD_Data(0x04);
    APP_LCD_Cmd(0xB1);
    APP_LCD_Data(0x08);//10
    APP_LCD_Data(0x10);//08
    APP_LCD_Cmd(0xC0);  //Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
    APP_LCD_Data(0x0C);
    APP_LCD_Data(0x05);
    APP_LCD_Cmd(0xC1);  //Set BT[2:0] for AVDD & VCL & VGH & VGL
    APP_LCD_Data(0x02);
    APP_LCD_Cmd(0xC5);  //Set VMH[6:0] & VML[6:0] for VOMH & VCOML
    APP_LCD_Data(0x4E);
    APP_LCD_Data(0x30);
    APP_LCD_Cmd(0xC7);
    APP_LCD_Data(0xc0);     //offset=0//C0
    APP_LCD_Cmd(0x3A);  //Set Color Format
    APP_LCD_Data(0x05);
    APP_LCD_Cmd(0x2A);  //Set Column Address
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x7F);
    APP_LCD_Cmd(0x2B);  //Set Page Address
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x9F);
 //   APP_LCD_Cmd(0xB4);  //frame inversion
//    APP_LCD_Data(0x07);
    APP_LCD_Cmd(0x36);  //Set Scanning Direction
    APP_LCD_Data(0xC0);
 //   APP_LCD_Cmd(0xEC);  //Set pumping clock frequency
   // APP_LCD_Data(0x0B);
    APP_LCD_Cmd(0xF2);  //Enable Gamma bit
    APP_LCD_Data(0x01);

    APP_LCD_Cmd(0xE0);
    APP_LCD_Data(0x3F);//p1          // xx VP63[5:0]             //       //
    APP_LCD_Data(0x31);//p2          // xx VP62[5:0]             //       //
    APP_LCD_Data(0x2D);//p3         // xx VP61[5:0]             //       //
    APP_LCD_Data(0x2F);//p4          // xx VP59[5:0]             //       //
    APP_LCD_Data(0x28);//p5          // xx VP57[5:0]             //       //
    APP_LCD_Data(0x0D);//p6         // xxx VP50[4:0]  //       //
    APP_LCD_Data(0x59);//p7          // x VP43[6:0]              //       //
    APP_LCD_Data(0xA8);//p8          // VP36[3:0] VP27[3:0]        //       //
    APP_LCD_Data(0x44);//p9          // x VP20[6:0]              //       //
    APP_LCD_Data(0x18);//p10       // xxx VP13[4:0]  //       //
    APP_LCD_Data(0x1F);//p11       // xx VP6[5:0]               //       //
    APP_LCD_Data(0x10);//p12       // xx VP4[5:0]               //       //
    APP_LCD_Data(0x07);//p13       // xx VP2[5:0]               //       //
    APP_LCD_Data(0x02);//p14       // xx VP1[5:0]               //       //
    APP_LCD_Data(0x00);//p15       // xx VP0[5:0]               //       //
    APP_LCD_Cmd(0xE1);
    APP_LCD_Data(0x00);//p1          // xx VN0[5:0]               //       //
    APP_LCD_Data(0x0E);//p2         // xx VN1[5:0]               //       //
    APP_LCD_Data(0x12);//p3          // xx VN2[5:0]               //       //
    APP_LCD_Data(0x10);//p4          // xx VN4[5:0]              //       //
    APP_LCD_Data(0x17);//p5          // xx VN6[5:0]               //       //
    APP_LCD_Data(0x12);//p6          // xxx VN13[4:0] //       //
    APP_LCD_Data(0x26);//p7          // x VN20[6:0]              //       //
    APP_LCD_Data(0x57);//p8          // VN36[3:0] VN27[3:0]       //       //
    APP_LCD_Data(0x3B);//p9         // x VN43[6:0]              //       //
    APP_LCD_Data(0x07);//p10       // xxx VN50[4:0] //       //
    APP_LCD_Data(0x20);//p11       // xx VN57[5:0]            //       //
    APP_LCD_Data(0x2F);//p12       // xx VN59[5:0]            //       //
    APP_LCD_Data(0x38);//p13       // xx VN61[5:0]            //       //
    APP_LCD_Data(0x3D);//p14       // xx VN62[5:0]            //       //
    APP_LCD_Data(0x3f);//p15         // xx VN63[5:0]            //       /

    
/*    APP_LCD_Cmd(0xE0);
    APP_LCD_Data(0x36);//p1
    APP_LCD_Data(0x29);//p2
    APP_LCD_Data(0x12);//p3
    APP_LCD_Data(0x22);//p4
    APP_LCD_Data(0x1C);//p5
    APP_LCD_Data(0x15);//p6
    APP_LCD_Data(0x42);//p7
    APP_LCD_Data(0xB7);//p8
    APP_LCD_Data(0x2F);//p9
    APP_LCD_Data(0x13);//p10
    APP_LCD_Data(0x12);//p11
    APP_LCD_Data(0x0A);//p12
    APP_LCD_Data(0x11);//p13
    APP_LCD_Data(0x0B);//p14
    APP_LCD_Data(0x06);//p15
    APP_LCD_Cmd(0xE1);
    APP_LCD_Data(0x09);//p1
    APP_LCD_Data(0x16);//p2
    APP_LCD_Data(0x2D);//p3
    APP_LCD_Data(0x0D);//p4
    APP_LCD_Data(0x13);//p5
    APP_LCD_Data(0x15);//p6
    APP_LCD_Data(0x40);//p7
    APP_LCD_Data(0x48);//p8
    APP_LCD_Data(0x53);//p9
    APP_LCD_Data(0x0C);//p10
    APP_LCD_Data(0x1D);//p11
    APP_LCD_Data(0x25);//p12
    APP_LCD_Data(0x2E);//p13
    APP_LCD_Data(0x34);//p14
    APP_LCD_Data(0x39);//p15       */

    APP_LCD_Cmd(0x29); // Display On
    APP_LCD_Cmd(0x2C);
  
  return (display_available & (1 << mios32_lcd_device)) ? 0 : -1; // return -1 if display not available
}


/////////////////////////////////////////////////////////////////////////////
// Sends data byte to LCD
// IN: data byte in <data>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Data(u8 data)
{
#if 0  // TODO
  // select LCD depending on current cursor position
  // THIS PART COULD BE CHANGED TO ARRANGE THE 8 DISPLAYS ON ANOTHER WAY
  u8 cs = mios32_lcd_y / APP_LCD_HEIGHT;

  if( cs >= 8 )
    return -1; // invalid CS line
#endif

  u8 cs=0;

  // chip select and DC
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(~(1 << cs));
#else
  MIOS32_BOARD_J15_DataSet((~(1 << cs))|(1<<1));
#endif
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC

  // send data
  MIOS32_BOARD_J15_SerDataShift(data);

  // increment graphical cursor
  //++mios32_lcd_x;

#if 0
  // if end of display segment reached: set X position of all segments to 0
  if( (mios32_lcd_x % APP_LCD_WIDTH) == 0 ) {
    APP_LCD_Cmd(0x75); // set X=0
    APP_LCD_Data(0x00);
    APP_LCD_Data(0x00);
  }
#endif

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Sends command byte to LCD
// IN: command byte in <cmd>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Cmd(u8 cmd)
{
  // select all LCDs
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(0x00);
#else
  MIOS32_BOARD_J15_DataSet(0x02);
#endif
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC

  MIOS32_BOARD_J15_SerDataShift(cmd);

  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Clear Screen
// IN: -
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Clear(void)
{
  u8 i, j;
  APP_LCD_GCursorSet(0, 0)
  for (j=0; j<160; j++)
    for (i=0; i<120; i++)
    {
       APP_LCD_Data(0);
       APP_LCD_Data(0);
    }

  return 0;
}


/////////////////////////////////////////////////////////////////////////////
// Sets cursor to given position
// IN: <column> and <line>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CursorSet(u16 column, u16 line)
{
  // mios32_lcd_x/y set by MIOS32_LCD_CursorSet() function
  return APP_LCD_GCursorSet(column, line*8);
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  s32 error = 0;
  u8 YSL,YEL;
  
  mios32_lcd_x = x;
  mios32_lcd_y = y;
  
    YSL=y & 0xff;

    YEL=159;

    error |= APP_LCD_Cmd(0x2A);
    error |= APP_LCD_Data(0x00);
    error |= APP_LCD_Data(x0 & 0x7f);
    error |= APP_LCD_Data(0x00);
    error |= APP_LCD_Data(0x7f);
    error |= APP_LCD_Cmd(0x2B);
    error |= APP_LCD_Data(0x00);
    error |= APP_LCD_Data(YSL);
    error |= APP_LCD_Data(0x00);
    error |= APP_LCD_Data(YEL);
    error |= APP_LCD_Cmd(0x2C);//LCD_WriteCMD(GRAMWR);

  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes the graphical font<BR>
//! Only relevant for SSD1322
//! \param[in] *font pointer to font, colour_depth Is1BIT or Is4BIT
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FontInit(u8 *font, app_lcd_color_depth_t colour_depth)
{
  font_bmp.memory = (u8 *)&font[MIOS32_LCD_FONT_BITMAP_IX] + (size_t)font[MIOS32_LCD_FONT_X0_IX];
  font_bmp.width = font[MIOS32_LCD_FONT_WIDTH_IX];
  font_bmp.height = font[MIOS32_LCD_FONT_HEIGHT_IX];
  font_bmp.line_offset = font[MIOS32_LCD_FONT_OFFSET_IX];
  font_bmp.colour_depth = colour_depth;
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Initializes a single special character
// IN: character number (0-7) in <num>, pattern in <table[8]>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_SpecialCharInit(u8 num, u8 table[8])
{
  return -1; // not supported
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a single character in bitmap(1 or 4Bit depends on bitmap)
//! \param[in] destination bitmap, x/y position, fusion mod, character to be print
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintChar(mios32_lcd_bitmap_t bitmap, float luma, s16 x, s16 y, app_lcd_fusion_t fusion, char c)
{
  if( !font_bmp.width )
    return -2;  // font not initialized yet!
  
  // legacy 1bit to 1bit
  if((bitmap.colour_depth == font_bmp.colour_depth) && (font_bmp.colour_depth == Is1BIT)) {
    mios32_lcd_bitmap_t char_bmp = font_bmp;
    char_bmp.memory += (char_bmp.height>>3) * char_bmp.line_offset * (size_t)c;
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);
    
    // 4bit depth
  }else if((bitmap.colour_depth == font_bmp.colour_depth) && (font_bmp.colour_depth == Is4BIT)) {
    mios32_lcd_bitmap_t char_bmp = font_bmp;
    char_bmp.line_offset = char_bmp.width*16;   // font table in ASCII format(16 char by line)
    char_bmp.memory += (char_bmp.width*char_bmp.height*((size_t)c & 0xf0)/2 + ((((size_t)c %16)*char_bmp.width)/2));
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);

    // legacy 1bit to 4bit depth
  }else if((bitmap.colour_depth == Is4BIT) && (font_bmp.colour_depth == Is1BIT)) {
    mios32_lcd_bitmap_t char_bmp = font_bmp;
    char_bmp.memory += (char_bmp.height>>3) * char_bmp.line_offset * (size_t)c;
    char_bmp.line_offset = char_bmp.width*16;   // font table in ASCII format(16 char by line)
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);
    
    // 4bit to legacy 1bit depth
  }else if((bitmap.colour_depth == Is1BIT) && (font_bmp.colour_depth == Is4BIT)) {
    // write it if you need it ;)
    return -1;    // not supported
  }else return -1;   // not supported
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a \\0 (zero) terminated string
//! \param[in] destination bitmap, x/y position, fusion mod,
//! str pointer to string.
//! \param[in]
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintString(mios32_lcd_bitmap_t bitmap, float luma, s16 x, s16 y, app_lcd_fusion_t fusion, const char *str)
{
  s32 status = 0;
  u16 offset =0;
  while( *str != '\0' ){
    status |= APP_LCD_PrintChar(bitmap, luma, x+(font_bmp.width*offset), y, fusion, *str++);
    offset++;
  }
  return status;
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a \\0 (zero) terminated formatted string (like printf)
//! \param[in] destination bitmap, x/y position, fusion mod,
//! *format zero-terminated format string - 64 characters supported maximum!
//! \param ... additional arguments
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintFormattedString(mios32_lcd_bitmap_t bitmap, float luma, s16 x, s16 y, app_lcd_fusion_t fusion, const char *format, ...)
{
  char buffer[64]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;
  
  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return APP_LCD_PrintString(bitmap, luma, x, y, fusion, buffer);
}

/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  app_lcd_back_grayscale = rgb & 0x0f;
  return -1; // n.a.
}


/////////////////////////////////////////////////////////////////////////////
// Sets the foreground colour
// Only relevant for colour legacy 1Bit bitmap and font
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_FColourSet(u32 rgb)
{
  app_lcd_fore_grayscale = rgb & 0x0f;
  return 0; // no error
}



/////////////////////////////////////////////////////////////////////////////
//! Only supported for graphical SSD1322 OLEDs:
//! initializes a bitmap type.
//!
//! Example:
//! \code
//!   // global array (!)
//!   u8 bitmap_array[APP_OLED_BITMAP_SIZE];
//!
//!   // Initialisation:
//!   mios32_lcd_bitmap_t bitmap = BM_LCD_BitmapClear(bitmap_array,
//!   						    APP_LCD_NUM_X*APP_OLED_WIDTH,
//!   						    APP_LCD_NUM_Y*APP_OLED_HEIGHT,
//!   						    APP_LCD_NUM_X*APP_OLED_WIDTH.
//!                   APP_LCD_COLOUR_DEPTH);
//! \endcode
//!
//! \param[in] memory pointer to the bitmap array
//! \param[in] width width of the bitmap (usually APP_LCD_NUM_X*APP_OLED_WIDTH)
//! \param[in] height height of the bitmap (usually APP_LCD_NUM_Y*APP_LCD_HEIGHT)
//! \param[in] line_offset byte offset between each line (usually same value as width)
//! \param[in] colour_depth how many bits are allocated by each pixel (usually APP_LCD_COLOUR_DEPTH)
//! \return a configured bitmap as mios32_lcd_bitmap_t
/////////////////////////////////////////////////////////////////////////////
mios32_lcd_bitmap_t APP_LCD_BitmapInit(u8 *memory, u16 width, u16 height, u16 line_offset, app_lcd_color_depth_t colour_depth)
{
  mios32_lcd_bitmap_t bitmap;
  
  bitmap.memory = memory;
  bitmap.width = width;
  bitmap.height = height;
  bitmap.line_offset = line_offset;
  bitmap.colour_depth = colour_depth;
  
  return bitmap;
}


/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and fusion mode for native 4Bit bitmap
// color is given by APP_LCD_FColourSet,
// app_lcd_fore_grayscale>0 is a white pixel for legacy 1Bit bimap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  if( x >= bitmap.width || y >= bitmap.height )
    return -1; // pixel is outside bitmap
  
  // prepare colour on both nibbles
  colour &= 0x0f;
  colour |= ((colour <<4) & 0xf0);    // clean colour
  
    /* native 4bit depth */
  if(bitmap.colour_depth == Is4BIT) {
    u8 *pixel = bitmap.memory + ((bitmap.line_offset*y + x)/2);
    if(x & 1){    // xi is even
          *pixel &= 0xf0;   // blank nibble
          *pixel |= (colour & 0x0f);
    }else{      // xi is odd
          *pixel &= 0x0f;   // blank nibble
          *pixel |= (colour & 0xf0);
    }
    
    /* legacy 1bit pixel print */
  }else if(bitmap.colour_depth == Is1BIT) {
    u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
    u8 mask = 1 << (y % 8);
    
    *pixel &= ~mask;
    if( colour )
      *pixel |= mask;
  }else return -1;  // not supported
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sets a byte in the bitmap, whathever its position in y,
// byte doesn't need to match the oled segment
// used for legacy 1bit bitmap
// IN: bm_cs_lcd_screen_bmp, x/y position and colour value (value range depends on APP_LCD_COLOUR_DEPTH)
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapByteSet(mios32_lcd_bitmap_t bitmap, s16 x, s16 y, u8 value)
{
  if( x >= bitmap.width || y >= bitmap.height || x < 0 || ((y + 8) < 0))
    return -1; // pixel is outside bm_cs_lcd_screen_bmp
  
  u8 mask;
  u8 val;
  if((y % 8) !=0){
    if(y >=0){
      u8 *byte1 = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
      mask = 0xff << (y % 8);
      val = value << (y % 8);
      *byte1 &= ~mask;
      if( value ) *byte1 |= val;
    }
    if(((y+8) < bitmap.height) && ((y+8) >=0)){
      u8 *byte2 = (u8 *)&bitmap.memory[bitmap.line_offset*((y+8) / 8) + x];
      mask = 0xff >> (8-((y+8) % 8));
      val = value >> (8-((y+8) % 8));
      *byte2 &= ~mask;
      if( value ) *byte2 |= val;
    }
  }else if(y >=0){
    u8 *byte = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
    *byte = value;
  }
  
  return 1; // ok
}

/////////////////////////////////////////////////////////////////////////////
// local, used by APP_LCD_Bitmap4BitLuma and APP_LCD_BitmapFusion
/////////////////////////////////////////////////////////////////////////////
u8 APP_LCD_HelpPixelLuma(u8 pix_mem, u8 pix_parity, float luma)
{
  
 //DEBUG_MSG("[BM_RoutingPg_MIDI_Process] io %d-%s: 0x%08x ! \n", io.index, io.port_name, midi_package.ALL);
  if(pix_parity!=0){
    u8 result = (u8)((pix_mem & 0x0f) + ((pix_mem & 0x0f)*(luma)));
    if(result>0x0f)result=0x0f;
    pix_mem &= 0xf0;   // blank nibble
    pix_mem |= (result & 0x0f);
  }else{
    u8 result = (u8)(((pix_mem>>4) & 0x0f) + (((pix_mem>>4) & 0x0f)*(luma)));
    if(result>0x0f)result=0x0f;
    pix_mem &= 0x0f;   // blank nibble
    pix_mem |= ((result <<4) & 0xf0);
  }
  return pix_mem;
}


/////////////////////////////////////////////////////////////////////////////
//! Change the Luminance of a native 4Bit bitmap within given boundaries
//! IN: bitmap, x/y position, width and heigth, luma
//! Notes:
//! luma is a float between -1.0 and +16.0(from black to all pixels saturated)
//! luma neutral is 0.0
//!
//! OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Bitmap4BitLuma(mios32_lcd_bitmap_t bitmap, s16 x, s16 y, u16 width, u16 height, float luma)
{
  if( (x >= bitmap.width) || (y >= bitmap.height) || ((x+width) < 0) || ((y+height) < 0))
    return -2;  // bitmap is outside screen
  
  /* native 4bit depth only */
  if(bitmap.colour_depth == Is4BIT) {
    u16 xi, yi;
    // loop y (with crop)
    for(yi=((y<0)? 0 : y); yi<(((height+y)>bitmap.height)? bitmap.height : (height+y)); yi++){
      // set src and dest pointers (with crop)
      //u8* src_mem_ptr = src_bmp.memory + ((yi*src_bmp.line_offset + ((x<0) ? (0-x) : 0))/2);
      u8* bmp_mem_ptr = bitmap.memory + ((yi*bitmap.line_offset + ((x<0) ? 0 : x))/2);
      // loop y (with crop)
      for(xi=((x<0)? 0 : x); xi<(((width+x)>bitmap.width)? bitmap.width : (width+x)); xi++){
        if(xi & 1){    // msb
          *bmp_mem_ptr = APP_LCD_HelpPixelLuma(*bmp_mem_ptr, 1, luma);
          bmp_mem_ptr++;      // next dest pointer
          
        }else{      // lsb
          *bmp_mem_ptr = APP_LCD_HelpPixelLuma(*bmp_mem_ptr, 0, luma);
        }
      }
    }
  }else return -1;  // not supported
  return 1; // ok
}

/////////////////////////////////////////////////////////////////////////////
//! Only supported for graphical SSD1322 OLEDs:
//! fusion, with different modes, of two bitmaps at specific position.
//! Luminance of the source can be modified at the same time
//! Note: if you need to mod luma the dest please use APP_LCD_Bitmap4BitLuma
//! see notes in APP_LCD_Bitmap4BitLuma
//!
//! Example for a legacy 1Bit(bitmap) to a native 4Bit bitmap(screen_bmp):
//! \code
//!   // bitmap is source
//!   APP_LCD_FColourSet(55);
//!   APP_LCD_BitmapFusion(bitmap, 0.0, screen_bmp, 0, 0, XOR);
//!   APP_LCD_BitmapPrint(screen_bmp);
//! \endcode
//!
//! \param[in] source, destination bitmaps, x/y position, fusion mode
//! fusion modes(new):
//!   REPLACE, replace bit or nibble (pixels)
//!   NOBLACK, pixel or nibble replace except if 0(black)
//!   OR, or bit or nibble (pixels)
//!   AND, and bit or nibble (pixels)
//!   XOR, xor bit or nibble (pixels)
//! \return < 0 on errors, resulting bimap is in destination bitmap
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapFusion(mios32_lcd_bitmap_t src_bmp, float src_luma, mios32_lcd_bitmap_t dst_bmp, s16 x, s16 y, app_lcd_fusion_t fusion)
{
  if( (x >= dst_bmp.width) || (y >= dst_bmp.height) || ((x+src_bmp.width) < 0) || ((y+src_bmp.height) < 0))
    return -2;  // bitmap is outside screen
  
  /* native 4bit depth */
  if((src_bmp.colour_depth == dst_bmp.colour_depth) && (dst_bmp.colour_depth == Is4BIT)) {
    u16 xi, yi;
    // loop y (with crop)
    for(yi=((y<0)? (0-y) : 0); yi<(((src_bmp.height+y)>dst_bmp.height)? (dst_bmp.height+y) : src_bmp.height); yi++){
      // set src and dest pointers (with crop)
      u8* src_mem_ptr = src_bmp.memory + ((yi*src_bmp.line_offset + ((x<0) ? (0-x) : 0))/2);
      u8* dst_mem_ptr = dst_bmp.memory + (((yi+y)*dst_bmp.line_offset + ((x<0) ? 0 : x))/2);
      // loop y (with crop)
      u16 xi_max = (((src_bmp.width+x)>dst_bmp.width)? (dst_bmp.width-x) : src_bmp.width);
      for(xi=((x<0)? (0-x) : 0); xi<xi_max; xi++){
        // process luma

        if(x & 1){    // start point x is odd
          if(!(xi & 1)){    // xi is even
            u8 pixel = *src_mem_ptr;
            pixel = (u8)APP_LCD_HelpPixelLuma(pixel, 0, src_luma);
            switch (fusion) {
              case NOBLACK:
                if(!(pixel)){
                  break;
                }
              case REPLACE:
                *dst_mem_ptr &= 0xf0;   // blank nibble
              case OR:
                *dst_mem_ptr |= ((pixel >>4) & 0x0f);
                break;
              case AND:
                *dst_mem_ptr &= ((pixel >>4) | 0xf0);
                break;
              case XOR:
                *dst_mem_ptr ^= ((pixel >>4) & 0x0f);
                break;
              default:
                break;
            }
            dst_mem_ptr++;      // next dest pointer
            
          }else{      // xi is odd
            u8 pixel = *src_mem_ptr;
            pixel = (u8)APP_LCD_HelpPixelLuma(pixel, 1, src_luma);
            switch (fusion) {
              case NOBLACK:
                if(!(pixel)){
                  break;
                }
              case REPLACE:
                *dst_mem_ptr &= 0x0f;   // blank nibble
              case OR:
                *dst_mem_ptr |= ((pixel <<4) & 0xf0);
                break;
              case AND:
                *dst_mem_ptr &= ((pixel <<4) | 0x0f);
                break;
              case XOR:
                *dst_mem_ptr ^= ((pixel <<4) & 0xf0);
                break;
              default:
                break;
            }
            src_mem_ptr++;      // next source pointer
            
          }
        }else{
          if(!(xi & 1)){   // only when xi is even
            if(xi == (xi_max-1)){  // end of line
              u8 pixel = *src_mem_ptr;
              pixel = (u8)APP_LCD_HelpPixelLuma(pixel, 0, src_luma);
              switch (fusion) {
                case NOBLACK:
                  if(!(pixel)){
                    break;
                  }
                case REPLACE:
                  *dst_mem_ptr &= 0x0f;   // blank nibble
                case OR:
                  *dst_mem_ptr |= (pixel & 0xf0);
                  break;
                case AND:
                  *dst_mem_ptr &= (pixel | 0x0f);
                  break;
                case XOR:
                  *dst_mem_ptr ^= (pixel & 0xf0);
                  break;
                default:
                  break;
              }
            }else{  // aligned, we copy the byte
              u8 pixel = *src_mem_ptr;
              pixel = (u8)APP_LCD_HelpPixelLuma(pixel, 0, src_luma);
              pixel = (u8)APP_LCD_HelpPixelLuma(pixel, 1, src_luma);
              switch (fusion) {
                case NOBLACK:
                  if(!(pixel)){
                    break;
                  }
                case REPLACE:
                  *dst_mem_ptr = pixel;
                  break;
                case OR:
                  *dst_mem_ptr |= pixel;
                  break;
                case XOR:
                  *dst_mem_ptr &= pixel;
                  break;
                case AND:
                  *dst_mem_ptr ^= pixel;
                  break;
                default:
                  break;
              }
              src_mem_ptr++;    // next source pointer
              dst_mem_ptr++;    // next dest pointer
            }
          }
        }
      }
    }
    
    /* legacy 1bit to 4bit depth */
  }else if((src_bmp.colour_depth == Is1BIT) && (dst_bmp.colour_depth == Is4BIT)) {
    u16 xi, yi;
    // prepare colour on both nibbles
    u8 gray = app_lcd_fore_grayscale & 0x0f;
    gray |= ((gray <<4) & 0xf0);
    // loop y (with crop)
    for(yi=((y<0)? (0-y) : 0); yi<(((src_bmp.height+y)>dst_bmp.height)? (dst_bmp.height-y) : src_bmp.height); yi++){
      // set src and dest pointers (with crop)
      u8* src_mem_ptr = src_bmp.memory + ((yi/8) * src_bmp.line_offset + ((x<0) ? (0-x) : 0));
      u8* dst_mem_ptr = dst_bmp.memory + (((yi+y)*dst_bmp.line_offset + ((x<0) ? 0 : x))/2);
      u8 bit = yi % 8;
      // loop y (with crop)
      for(xi=((x<0)? (0-x) : 0); xi<(((src_bmp.width+x)>dst_bmp.width)? (dst_bmp.width-x) : src_bmp.width); xi++){
        u8 pixel = *src_mem_ptr++;
        pixel = (pixel & (1<<bit)? gray : 0);
        //Process luma
        pixel = APP_LCD_HelpPixelLuma(pixel, xi & 1, src_luma);
        //APP_LCD_HelpPixelLuma(&pixel, xi & 1, src_luma);
        if((xi & 1) == (x & 1)){      // x parity == xi parity
          switch (fusion) {
            case NOBLACK:
              if(!pixel){
                break;
              }
            case REPLACE:
              *dst_mem_ptr &= 0x0f;   // blank nibble
            case OR:
              *dst_mem_ptr |= (pixel & 0xf0);
              break;
            case AND:
              *dst_mem_ptr &= (pixel | 0x0f);
              break;
            case XOR:
              *dst_mem_ptr ^= (pixel & 0xf0);
              break;
            default:
              break;
          }
        }else{                      // x parity != xi parity
          switch (fusion) {
            case NOBLACK:
              if(!pixel){
                break;
              }
            case REPLACE:
              *dst_mem_ptr &= 0xf0;   // blank nibble
            case OR:
              *dst_mem_ptr |= (pixel & 0x0f);
              break;
            case AND:
              *dst_mem_ptr &= (pixel | 0xf0);
              break;
            case XOR:
              *dst_mem_ptr ^= (pixel & 0x0f);
              break;
            default:
              break;
          }
          dst_mem_ptr++;
        }
      }
    }
    
    /* legacy 1bit to 1bit, no depth here we copy the pixels, notes: the position in the legacy segment doesn't care ;) */
  }else if((src_bmp.colour_depth == dst_bmp.colour_depth) && (dst_bmp.colour_depth == Is1BIT) && (fusion == REPLACE)) {
    int i, j;
    u8 *byte = src_bmp.memory;
    for(j=0; j< (src_bmp.height/8); j++)
      // forward to legacy 1bit process
      for(i=0; i< src_bmp.width; i++)
        APP_LCD_BitmapByteSet(dst_bmp, x+i, y+(j*8), *(byte+i+(j*src_bmp.line_offset)));
    
    
    /* 4bit to legacy 1bit depth */
  }else if((src_bmp.colour_depth == Is4BIT) && (dst_bmp.colour_depth == Is1BIT)) {
    // write it if you need it ;)
    return -1;  // not supported
  }else return -1;  // not supported
  
  return 0; // no error
}


/////////////////////////////////////////////////////////////////////////////
// Transfers a Bitmap to the LCD
// Notes: Because we've got 2 pixels by byte, try to transfer an even width bitmap
// to an even x position on screen or you will overwrite some pixels
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  int x, y;
  u16 initial_x = mios32_lcd_x;
  u16 initial_y = mios32_lcd_y;
  u16 x_max = (((bitmap.width+initial_x)>APP_LCD_WIDTH)? APP_LCD_WIDTH : (bitmap.width+initial_x));
  for(y=initial_y; y<(((bitmap.height+initial_y)>APP_LCD_HEIGHT)? APP_LCD_HEIGHT : (bitmap.height+initial_y)); y++){
    APP_LCD_GCursorSet(initial_x, y);
    APP_LCD_Cmd(0x5c); // Write RAM
    
    /* 4bit bitmap print */
    if(bitmap.colour_depth == Is4BIT) {
      // calculate pointer to bitmap line
      u8 *memory_ptr = bitmap.memory + (((y-initial_y)*bitmap.line_offset)/2);
      u8 byte = 0;
      // loop x (with crop)
      for(x= initial_x; x<x_max; x++){
        // transfer bitmap
        if(initial_x & 1){    // initial_x is odd
          if(!(x & 1)){    // x is even
            byte &= 0xf0;   // blank nibble
            byte |= ((*memory_ptr >>4) & 0x0f);
            APP_LCD_Data(byte);   // send byte
          }else{      // xi is odd
            byte &= 0x0f;   // blank nibble
            byte |= ((*memory_ptr <<4) & 0xf0);
            memory_ptr++;      // next source pointer
          }
        }else{
          if(!(x & 1)){   // only when xi is even
            if(x == (x_max-1)){  // end of line
              byte &= 0x0f;   // blank nibble
              byte |= (*memory_ptr & 0xf0);
              APP_LCD_Data(byte);   // send byte
            }else{  // aligned, we copy the byte
              APP_LCD_Data(*memory_ptr++);   // send byte
            }
          }
        }
      }
    /* legacy 1bit bitmap print */
    }else if(bitmap.colour_depth == Is1BIT) {
      // calculate pointer to bitmap line
      u8 *memory_ptr = bitmap.memory + (((y-initial_y)/8) * bitmap.line_offset);
      // transfer bitmap
      u8 mem = *memory_ptr++;
      u8 byte = 0;
      u8 bit = (y-initial_y) % 8;
      // loop x (with crop)
      for(x=initial_x; x<x_max; x++){
        if((initial_x & 1) == (x & 1)){      // x parity == xi parity
          byte &= 0xf0;   // blank nibble
          mem = *memory_ptr++;
          if(mem & (1<<bit))byte |= (app_lcd_fore_grayscale & 0x0f);
          APP_LCD_Data(byte);
        }else{      // xi is odd
          byte &= 0x0f;   // blank nibble
          mem = *memory_ptr++;
          if(mem & (1<<bit))byte = (app_lcd_fore_grayscale & 0x0f)<<4;
        }
      }
      
    }else return -1;  // not supported
    // reset y to initial position
    //mios32_lcd_y = initial_y;
  }
  // reset x to next position
  //mios32_lcd_x = x_max;
  APP_LCD_GCursorSet(x_max, initial_y);
  return 0; // no error
}

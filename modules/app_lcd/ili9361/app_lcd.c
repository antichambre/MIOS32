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
#include <string.h>
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
u16 app_lcd_back_color = 0;
u16 app_lcd_fore_color = 0;


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
  
  APP_LCD_Cmd(0x26);    //Set Default Gamma
  APP_LCD_Data(0x04);
  APP_LCD_Cmd(0xB1);
  APP_LCD_Data(0x08);   //10
  APP_LCD_Data(0x10);   //08
  APP_LCD_Cmd(0xC0);    //Set VRH1[4:0] & VC[2:0] for VCI1 & GVDD
  APP_LCD_Data(0x0C);
  APP_LCD_Data(0x05);
  APP_LCD_Cmd(0xC1);    //Set BT[2:0] for AVDD & VCL & VGH & VGL
  APP_LCD_Data(0x02);
  APP_LCD_Cmd(0xC5);    //Set VMH[6:0] & VML[6:0] for VOMH & VCOML
  APP_LCD_Data(0x4E);
  APP_LCD_Data(0x30);
  APP_LCD_Cmd(0xC7);
  APP_LCD_Data(0xc0);   //offset=0//C0
  APP_LCD_Cmd(0x3A);    //Set Color Format
  APP_LCD_Data(0x05);
  APP_LCD_Cmd(0x2A);    //Set Column Address
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x7F);
  APP_LCD_Cmd(0x2B);    //Set Page Address
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x00);
  APP_LCD_Data(0x9F);
  //APP_LCD_Cmd(0xB4);  //frame inversion
  //APP_LCD_Data(0x07);
  APP_LCD_Cmd(0x36);    //Set Scanning Direction
  APP_LCD_Data(0xC0);
  APP_LCD_Cmd(0xF2);    //Enable Gamma bit
  APP_LCD_Data(0x00);
  
  APP_LCD_Cmd(0xE0);
  APP_LCD_Data(0x3F);   //p1     xx VP63[5:0]
  APP_LCD_Data(0x31);   //p2     xx VP62[5:0]
  APP_LCD_Data(0x2D);   //p3     xx VP61[5:0]
  APP_LCD_Data(0x2F);   //p4     xx VP59[5:0]
  APP_LCD_Data(0x28);   //p5     xx VP57[5:0]
  APP_LCD_Data(0x0D);   //p6     xxx VP50[4:0]
  APP_LCD_Data(0x59);   //p7     x VP43[6:0]
  APP_LCD_Data(0xA8);   //p8     VP36[3:0] VP27[3:0]
  APP_LCD_Data(0x44);   //p9     x VP20[6:0]
  APP_LCD_Data(0x18);   //p10    xxx VP13[4:0]
  APP_LCD_Data(0x1F);   //p11    xx VP6[5:0]
  APP_LCD_Data(0x10);   //p12    xx VP4[5:0]
  APP_LCD_Data(0x07);   //p13    xx VP2[5:0]
  APP_LCD_Data(0x02);   //p14    xx VP1[5:0]
  APP_LCD_Data(0x00);   //p15    xx VP0[5:0]
  APP_LCD_Cmd(0xE1);
  APP_LCD_Data(0x00);   //p1     xx VN0[5:0]
  APP_LCD_Data(0x0E);   //p2     xx VN1[5:0]
  APP_LCD_Data(0x12);   //p3     xx VN2[5:0]
  APP_LCD_Data(0x10);   //p4     xx VN4[5:0]
  APP_LCD_Data(0x17);   //p5     xx VN6[5:0]
  APP_LCD_Data(0x12);   //p6     xxx VN13[4:0]
  APP_LCD_Data(0x26);   //p7     x VN20[6:0]
  APP_LCD_Data(0x57);   //p8     VN36[3:0] VN27[3:0]
  APP_LCD_Data(0x3B);   //p9     x VN43[6:0]
  APP_LCD_Data(0x07);   //p10    xxx VN50[4:0]
  APP_LCD_Data(0x20);   //p11    xx VN57[5:0]
  APP_LCD_Data(0x2F);   //p12    xx VN59[5:0]
  APP_LCD_Data(0x38);   //p13    xx VN61[5:0]
  APP_LCD_Data(0x3D);   //p14    xx VN62[5:0]
  APP_LCD_Data(0x3f);   //p15    xx VN63[5:0]
  
  APP_LCD_Cmd(0x29);    // Display On
  APP_LCD_Cmd(0x2C);
  
  // set the startup colors
  APP_LCD_BColourSet(0x00000000);  // black
  APP_LCD_FColourSet(0x00ffffff);  // White
  display_available = 1;
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

  // chip select and DC
#if APP_LCD_USE_J10_FOR_CS
  MIOS32_BOARD_J10_Set(~(1 << cs));
#else
  MIOS32_BOARD_J15_DataSet(0x00);
#endif
  MIOS32_BOARD_J15_RS_Set(1); // RS pin used to control DC
  
  // send data
  MIOS32_BOARD_J15_SerDataShift(data);
  
  MIOS32_BOARD_J15_DataSet(0x02);
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
  MIOS32_BOARD_J15_DataSet(0x00);
#endif
  MIOS32_BOARD_J15_RS_Set(0); // RS pin used to control DC
  
  MIOS32_BOARD_J15_SerDataShift(cmd);
  
  MIOS32_BOARD_J15_DataSet(0x02);
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
  APP_LCD_GCursorSet(0, 0);
  for (j=0; j<160; j++){
    APP_LCD_GCursorSet(0, j);
    for (i=0; i<128; i++)
    {
      APP_LCD_Data(app_lcd_back_color >> 8);
      APP_LCD_Data(app_lcd_back_color & 0xff);
    }
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
  return APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
}


/////////////////////////////////////////////////////////////////////////////
// Sets graphical cursor to given position
// IN: <x> and <y>
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_GCursorSet(u16 x, u16 y)
{
  s32 error = 0;
  
  
  mios32_lcd_x = x;
  mios32_lcd_y = y;
  
  error |= APP_LCD_Cmd(0x2A);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(x);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(127);
  error |= APP_LCD_Cmd(0x2B);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(y);
  error |= APP_LCD_Data(0x00);
  error |= APP_LCD_Data(159);
  error |= APP_LCD_Cmd(0x2C);   //LCD_WriteCMD(GRAMWR);
  
  return error;
}

/////////////////////////////////////////////////////////////////////////////
//! Initializes the graphical font<BR>
//! Only relevant for SSD1322
//! \param[in] *font pointer to font, colour_depth Is1BIT or IsILI
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
// return a character kerning length
// IN: the character
// OUT: kerning length
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_CharKernGet(char c)
{
  s32 len=0;
  if(font_bmp.line_offset == 0){
    // kerning(char offset)
    if(font_bmp.colour_depth == Is1BIT) {
      u8 height = (font_bmp.height/8) + ((font_bmp.height%8) ? 1 : 0);
      u8 *byte = font_bmp.memory + ((u8)(c/16)*height*font_bmp.width*16) + (font_bmp.width*(c%16));
      len++;
      int i, j;
      for(i=1; i< font_bmp.width; i++){
        u32 char_slice = 0;
        for(j=0; j< height; j++)char_slice |= ( (*(byte+i+(j*font_bmp.width*16))) << (j*8) );
        //DEBUG_MSG("%c slice:%d -> %x", c, i, char_slice);
        if(char_slice==0){
          len +=i;
          break;
        }
      }
        //DEBUG_MSG("%c %d", c, len);
    }
  }else{
    // no kerning(legacy)
    len=font_bmp.width;
  }

  return len; // not supported
}

/////////////////////////////////////////////////////////////////////////////
// return a character kerning length
// IN: the character
// OUT: kerning length
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_StringKernGet(const char *str)
{
  s32 len=0;
  while( *str != '\0' )len +=(s32)APP_LCD_CharKernGet(*str++);
  return len; // not supported
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
    u8 height = (char_bmp.height/8) + ((char_bmp.height%8) ? 1 : 0);
    char_bmp.line_offset = char_bmp.width*16;   // font table in ASCII format(16 char by line)
    char_bmp.memory += ((u8)(c/16)*char_bmp.line_offset*height) + (font_bmp.width*(c%16));
    char_bmp.width=(u16)APP_LCD_CharKernGet(c);
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);
    
    // toDo ili special depth 5:6:5
  }else if((bitmap.colour_depth == font_bmp.colour_depth) && (font_bmp.colour_depth == Is16BIT)) {
    mios32_lcd_bitmap_t char_bmp = font_bmp;
    char_bmp.line_offset = char_bmp.width*16;   // font table in ASCII format(16 char by line)
    char_bmp.memory += (char_bmp.width*char_bmp.height*((size_t)c & 0xf0)*2 + ((((size_t)c %16)*char_bmp.width)*2));
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);
    
    // legacy 1bit to '16bit' depth
  }else if((bitmap.colour_depth == Is16BIT) && (font_bmp.colour_depth == Is1BIT)) {
    mios32_lcd_bitmap_t char_bmp = font_bmp;
    u8 height = (char_bmp.height/8) + ((char_bmp.height%8) ? 1 : 0);
    char_bmp.line_offset = char_bmp.width*16;   // font table in ASCII format(16 char by line)
    char_bmp.memory += ((u8)(c/16)*char_bmp.line_offset*height) + (font_bmp.width*(c%16));
    char_bmp.width=(u16)APP_LCD_CharKernGet(c);
    APP_LCD_BitmapFusion(char_bmp, luma, bitmap, x, y, fusion);
    
    // '16bit' to legacy 1bit depth
  }else if((bitmap.colour_depth == Is1BIT) && (font_bmp.colour_depth == Is16BIT)) {
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
//! \return < 0 on errors, or string length in pixels
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintString(mios32_lcd_bitmap_t bitmap, float luma, s16 x, s16 y, app_lcd_fusion_t fusion, u8 alignment, const char *str)
{
  s32 status = 0;
  u16 offset = 0;
  
  // calc start point depending on alignment
  const char *s = str;
  u16 len=0;
  // kerning(char offset)
  while( *s != '\0' ){
    len += (u16)APP_LCD_CharKernGet(*s);
    s++;
  }
  if(alignment==APP_LCD_STRING_ALIGN_CENTER)x -= len/2;
  if(alignment==APP_LCD_STRING_ALIGN_RIGHT)x -= len;
  // start spelling
  offset = 0;
  while( *str != '\0' ){
    status |= APP_LCD_PrintChar(bitmap, luma, x+offset, y, fusion, *str);
    offset +=(u16)APP_LCD_CharKernGet(*str);
    str++;
  }
  return (status<0)?status:(s32)offset;
}

/////////////////////////////////////////////////////////////////////////////
//! Prints a \\0 (zero) terminated formatted string (like printf)
//! \param[in] destination bitmap, x/y position, fusion mod,
//! *format zero-terminated format string - 64 characters supported maximum!
//! \param ... additional arguments
//! \return < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PrintFormattedString(mios32_lcd_bitmap_t bitmap, float luma, s16 x, s16 y, app_lcd_fusion_t fusion, u8 alignment, const char *format, ...)
{
  char buffer[64]; // TODO: tmp!!! Provide a streamed COM method later!
  va_list args;
  
  va_start(args, format);
  vsprintf((char *)buffer, format, args);
  return APP_LCD_PrintString(bitmap, luma, x, y, fusion, alignment, buffer);
}

/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
u16 APP_LCD_ColourConvert(u32 rgb)
{
  rgb &= 0x00ffffff;
  u8 r = (rgb >> 19) & 0x1f;
  u8 g = (rgb >> 10) & 0x3f;
  u8 b = (rgb >> 3) & 0x1f;
  return ((r<<11) | (g<<5) | b);
}

/////////////////////////////////////////////////////////////////////////////
// Sets the background colour
// Only relevant for colour GLCDs
// IN: r/g/b value
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BColourSet(u32 rgb)
{
  app_lcd_back_color = APP_LCD_ColourConvert(rgb);
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

  app_lcd_fore_color = APP_LCD_ColourConvert(rgb);
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
// app_lcd_fore_color>0 is a white pixel for legacy 1Bit bimap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_PixelSet(u16 x, u16 y, u32 colour)
{
  if( x >= APP_LCD_WIDTH || y >= APP_LCD_HEIGHT )
    return -1; // pixel is outside bitmap
  
  colour &= 0x00ffffff;
  u8 r = (colour >> 19) & 0x1f;
  u8 g = (colour >> 10) & 0x3f;
  u8 b = (colour >> 3) & 0x1f;
  u16 color = (r<<11) | (g<<5) | b;
  APP_LCD_GCursorSet(x, y);
  APP_LCD_Data(color >> 8);
  APP_LCD_Data(color & 0xff);
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Sets a pixel in the bitmap
// IN: bitmap, x/y position and fusion mode for native 4Bit bitmap
// color is given by APP_LCD_FColourSet,
// app_lcd_fore_color>0 is a white pixel for legacy 1Bit bimap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPixelSet(mios32_lcd_bitmap_t bitmap, u16 x, u16 y, u32 colour)
{
  if( x >= bitmap.width || y >= bitmap.height )
    return -1; // pixel is outside bitmap
  
  
  /* native 16bit depth. r(15:11), g(10:5), b(4:0)   */
  if(bitmap.colour_depth == APP_LCD_COLOUR_DEPTH){
    // prepare colour for 5:6:5
    colour &= 0x00ffffff;
    u8 r = (colour >> 19) & 0x1f;
    u8 g = (colour >> 10) & 0x3f;
    u8 b = (colour >> 3) & 0x1f;
    u16 color = (r<<11) | (g<<5) | b;
    u8 *pixel = bitmap.memory + ((bitmap.line_offset*y + x)*2);
    *pixel++ = color>>8;
    *pixel = (color & 0xff);
    
    /* legacy 1bit pixel print */
  }else if(bitmap.colour_depth == 1) {  // 1bit format
    u8 *pixel = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
    u8 mask = 1 << (y % 8);
    *pixel &= ~mask;
    if( colour ) *pixel |= mask;
    
  }else return -1;  // not supported
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Draw a rectangle in the bm_cs_lcd_screen_bmp from position and size
// IN: x1/y1 first point, x2/y2 second point, border(e.g. 0x55 is dot line) and fill 0=none 1=empty 2=fill
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_Rectangle(u16 x, u16 y, u16 width, u16 height, u8 border, u32 bd_color, u8 fill, u32 back_color)
{
  if( x >= APP_LCD_WIDTH || y >= APP_LCD_HEIGHT )
    return -1; // pixel is outside bitmap
  
  s16 i, j;
    // fill rect first
    if(fill)for(i=0; i< (width); i++)for(j=0; j< (height); j++)APP_LCD_PixelSet((u16)(x+i), (u16)(y+j), back_color);

    // border
    u16 border_pix=0;
    for(i=0; i< (width); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_PixelSet((u16)(x+i), (u16)y, bd_color);
      else
        APP_LCD_PixelSet((u16)(x+i), (u16)y, back_color);
      border_pix++;
    }
    for(i=1; i< (height); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_PixelSet((u16)(x+width-1), (u16)(y+i), bd_color);
      else
        APP_LCD_PixelSet((u16)(x+width-1), (u16)(y+i), back_color);
      border_pix++;
    }
    for(i=1; i< (width); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_PixelSet((u16)(x+width-i-1), (u16)(y + height-1), bd_color);
      else
        APP_LCD_PixelSet((u16)(x+width-i-1), (u16)(y + height-1), back_color);
      border_pix++;
    }
    for(i=1; i< (height); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_PixelSet((u16)x, (u16)(y+height-i-1), bd_color);
      else
        APP_LCD_PixelSet((u16)x, (u16)(y+height-i-1), back_color);
      border_pix++;
    }

  return 1; // ok
}


/////////////////////////////////////////////////////////////////////////////
// Draw a rectangle in the bm_cs_lcd_screen_bmp from position and size
// IN: x1/y1 first point, x2/y2 second point, border(e.g. 0x55 is dot line) and fill 0=none 1=empty 2=fill
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapRectangle(mios32_lcd_bitmap_t bitmap, s16 x, s16 y, u16 width, u16 height, u8 border, u32 bd_color, u8 fill, u32 back_color)
{
  if( (x >= bitmap.width) || (y >= bitmap.height) || ((x + width) < 0) || ((y + height) < 0) )return -1; // pixel is outside bm_cs_lcd_screen_bmp
  s16 i, j;

//  /* native 16bit depth. r(15:11), g(10:5), b(4:0)   */
//  if(bitmap.colour_depth == APP_LCD_COLOUR_DEPTH){
//    // toDo
//
//    /* legacy 1bit pixel print */
//  }else if(bitmap.colour_depth == 1) {  // 1bit format
    // fill rect first
    if(fill)for(i=0; i< (width); i++)for(j=0; j< (height); j++)APP_LCD_BitmapPixelSet(bitmap, (u16)(x+i), (u16)(y+j), back_color);

    // border
    if(border){
    u16 border_pix=0;
    for(i=0; i< (width); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+i), (u16)y, bd_color);
      else
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+i), (u16)y, back_color);
      border_pix++;
    }
    for(i=1; i< (height); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+width-1), (u16)(y+i), bd_color);
      else
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+width-1), (u16)(y+i), back_color);
      border_pix++;
    }
    for(i=1; i< (width); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+width-i-1), (u16)(y + height-1), bd_color);
      else
        APP_LCD_BitmapPixelSet(bitmap, (u16)(x+width-i-1), (u16)(y + height-1), back_color);
      border_pix++;
    }
    for(i=1; i< (height); i++){
      if((border >> (border_pix%8))&0x01)
        APP_LCD_BitmapPixelSet(bitmap, (u16)x, (u16)(y+height-i-1), bd_color);
      else
        APP_LCD_BitmapPixelSet(bitmap, (u16)x, (u16)(y+height-i-1), back_color);
      border_pix++;
    }
    }
  //}else return -1;  // not supported

  return 1; // ok
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
    if(y > 0){
      u8 *byte1 = (u8 *)&bitmap.memory[bitmap.line_offset*(y / 8) + x];
      mask = 0xff << (y % 8);
      val = value << (y % 8);
      *byte1 &= ~mask;
      if( value ) *byte1 |= val;
    }
    if((y+8) >=0){
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
u16 APP_LCD_HelpPixelLuma(u16 pix_mem, float luma)
{
  if(luma == 1.0)return pix_mem;
  u8 r = (u8)(((pix_mem >> 11) & 0x1f)*(luma));
  u8 g = (u8)(((pix_mem >> 5) & 0x3f)*(luma));
  u8 b = (u8)((pix_mem & 0x1f)*(luma));
  return ((r<<11) | (g<<5) | b);
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
s32 APP_LCD_Bitmap16BitLuma(mios32_lcd_bitmap_t bitmap, s16 x, s16 y, u16 width, u16 height, float luma)
{
  if( (x >= bitmap.width) || (y >= bitmap.height) || ((x+width) < 0) || ((y+height) < 0))
    return -2;  // bitmap is outside screen
  
  /* native 4bit depth only */
  if(bitmap.colour_depth == Is16BIT) {
    u16 xi, yi;
    // loop y (with crop)
    for(yi=((y<0)? 0 : y); yi<(((height+y)>bitmap.height)? bitmap.height : (height+y)); yi++){
      // loop x (with crop)
      for(xi=((x<0)? 0 : x); xi<(((width+x)>bitmap.width)? bitmap.width : (width+x)); xi++){
        // set pointer
        u8* bmp_mem_ptr = bitmap.memory + (yi*bitmap.line_offset + xi)*2;
        // get the pixels
        u16 bmp_pix = *bmp_mem_ptr <<8;
        bmp_pix |= *(bmp_mem_ptr+1);
        // set luma
        bmp_pix = APP_LCD_HelpPixelLuma(bmp_pix, luma);
        *bmp_mem_ptr++ = bmp_pix >>8;
        *bmp_mem_ptr++ = bmp_pix &0xff;
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
u16 APP_LCD_PixelFusion(u16 fore_pix, float fore_luma, u16 back_pix, float back_luma, app_lcd_fusion_t fusion)
{
  u16 pix;
        //Process luma
        fore_pix = APP_LCD_HelpPixelLuma(fore_pix, fore_luma);
        back_pix = APP_LCD_HelpPixelLuma(back_pix, back_luma);
        pix = back_pix;
        switch (fusion) {
          case NOBLACK:
            if(!fore_pix){
              break;
            }
          case REPLACE:
            pix = fore_pix;
            break;
          case OR:
            pix |= fore_pix;
            break;
          case AND:
            pix &= fore_pix;
            break;
          case XOR:
            pix ^= fore_pix;
            break;
          default:
            break;
        }

  return pix; // no error
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
s32 APP_LCD_BitmapFusion(mios32_lcd_bitmap_t top_bmp, float top_luma, mios32_lcd_bitmap_t bmp, s16 top_pos_x, s16 top_pos_y, app_lcd_fusion_t fusion)
{
  if( (top_pos_x >= bmp.width) || (top_pos_y >= bmp.height) || ((top_pos_x+top_bmp.width) < 0) || ((top_pos_y+top_bmp.height) < 0))
    return -2;  // bitmap is outside screen
  
  /* native 16bits ili depth */
  if((top_bmp.colour_depth == bmp.colour_depth) && (bmp.colour_depth == Is16BIT)) {
    u16 x, y;
    // loop pos y (with crop)
    for(y=((top_pos_y<0)? (0-top_pos_y) : 0); y<(((top_bmp.height+top_pos_y)>bmp.height)? (bmp.height+top_pos_y) : top_bmp.height); y++){
      // set src and dest pointers (with crop)
      u8* top_mem_ptr = top_bmp.memory + ((y*top_bmp.line_offset + ((top_pos_x<0) ? (0-top_pos_x) : 0))*(bmp.colour_depth/8));
      u8* bmp_mem_ptr = bmp.memory + (((y+top_pos_y)*bmp.line_offset + ((top_pos_x<0) ? 0 : top_pos_x))*(bmp.colour_depth/8));
      // loop pos x (with crop)
      u16 xi_max = (((top_bmp.width+top_pos_x)>bmp.width)? (bmp.width-top_pos_x) : top_bmp.width);
      for(x=((top_pos_x<0)? (0-top_pos_x) : 0); x<xi_max; x++){
        // get the pixels
        u16 top_pix = *top_mem_ptr <<8;
        top_pix |= *(top_mem_ptr+1);
        u16 bmp_pix = *bmp_mem_ptr<<8;
        bmp_pix |= *(bmp_mem_ptr+1);
        //Process luma
        top_pix = APP_LCD_HelpPixelLuma(top_pix, top_luma);
        //bmp_pix = APP_LCD_HelpPixelLuma(bmp_pix, bmp_luma);
        switch (fusion) {
          case NOBLACK:
            if(!top_pix){
              break;
            }
          case REPLACE:
            bmp_pix = top_pix;
            break;
          case OR:
            bmp_pix |= top_pix;
            break;
          case AND:
            bmp_pix &= top_pix;
            break;
          case XOR:
            bmp_pix ^= top_pix;
            break;
          default:
            break;
        }
        top_mem_ptr +=2;    // next top pointer
        *bmp_mem_ptr = bmp_pix >>8;
        bmp_mem_ptr++;    // next dest pointer
        *bmp_mem_ptr = bmp_pix &0xff;
        bmp_mem_ptr++;    // next dest pointer
      }
    }
    
    /* legacy 1bit to 16bits ili depth */
  }else if((top_bmp.colour_depth == Is1BIT) && (bmp.colour_depth == Is16BIT)) {
    u16 x, y;
    // prepare colour on both nibbles
    u8 gray = app_lcd_fore_color & 0x0f;
    gray |= ((gray <<4) & 0xf0);
    // loop top_pos_y (with crop)
    for(y=((top_pos_y<0)? (0-top_pos_y) : 0); y<(((top_bmp.height+top_pos_y)>bmp.height)? (bmp.height-top_pos_y) : top_bmp.height); y++){
      // set src and dest pointers (with crop)
      u8* top_mem_ptr = top_bmp.memory + ((y/8) * top_bmp.line_offset + ((top_pos_x<0) ? (0-top_pos_x) : 0));
      u8* bmp_mem_ptr = bmp.memory + (((y+top_pos_y)*bmp.line_offset + ((top_pos_x<0) ? 0 : top_pos_x))*(bmp.colour_depth/8));
      u8 bit = y % 8;
      // loop top_pos_y (with crop)
      for(x=((top_pos_x<0)? (0-top_pos_x) : 0); x<(((top_bmp.width+top_pos_x)>bmp.width)? (bmp.width-top_pos_x) : top_bmp.width); x++){
        // get the pixels
        u16 top_pix = (u16)(*top_mem_ptr++);
        top_pix = (top_pix & (1<<bit)? app_lcd_fore_color : app_lcd_back_color);
        u16 bmp_pix = *bmp_mem_ptr<<8;
        bmp_pix |= *(bmp_mem_ptr+1);
        //Process luma
        top_pix = APP_LCD_HelpPixelLuma(top_pix, top_luma);
        //bmp_pix = APP_LCD_HelpPixelLuma(bmp_pix, bmp_luma);
        switch (fusion) {
          case NOBLACK:
            if(!top_pix){
              break;
            }
          case REPLACE:
            bmp_pix = top_pix;
            break;
          case OR:
            bmp_pix |= top_pix;
            break;
          case AND:
            bmp_pix &= top_pix;
            break;
          case XOR:
            bmp_pix ^= top_pix;
            break;
          default:
            break;
        }
        *bmp_mem_ptr = bmp_pix >>8;
        bmp_mem_ptr++;    // next dest pointer
        *bmp_mem_ptr = bmp_pix &0xff;
        bmp_mem_ptr++;    // next dest pointer
      }
    }
    
    /* legacy 1bit to 1bit, no depth here we copy the pixels, notes: the position doesn't care  */
  }else if((top_bmp.colour_depth == bmp.colour_depth) && (bmp.colour_depth == Is1BIT) ) {
    int i, j;
    u8 height = top_bmp.height/8 + ((top_bmp.height%8) ? 1 : 0);
    u8 *byte = top_bmp.memory;
    for(i=0; i< top_bmp.width; i++){
      // forward to legacy 1bit process
      for(j=0; j< height; j++){
        if(!byte)APP_LCD_BitmapByteSet(bmp, top_pos_x+i, top_pos_y+(j*8), *(byte+i+(j*top_bmp.line_offset)));
      }
    }
    
    /* 4bit to legacy 1bit depth */
  }else if((top_bmp.colour_depth == Is16BIT) && (bmp.colour_depth == Is1BIT)) {
    // write it if you need it ;)
    return -1;  // not supported
  }else return -1;  // not supported
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Transfers Bitmap to the TFT
// Notes: using back/fore colors respectively from pixel off/on for 1bit,
// trasferred to APP_LCD_NativeBitmapPrint for native 16bit
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapHBoundaryPrint(mios32_lcd_bitmap_t bitmap, u16 b_x, u16 b_width)
{
  
  //if( !MIOS32_LCD_TypeIsGLCD() )
  //return -1; // no GLCD
  
  // abort if max. width reached
  //if( mios32_lcd_x >= mios32_lcd_parameters.width )
  //return -2;
  
  /* native 16bit depth. r(15:11), g(10:5), b(4:0)   */
  if(bitmap.colour_depth == APP_LCD_COLOUR_DEPTH){
    //    u16 *memory_ptr = bitmap.memory + ((bitmap.line_offset*top_pos_y + top_pos_x)*2);
    //    // transfer bitmap
    //    int top_pos_x, y;
    //    for(y=0; y<8; ++y){
    //      for(x=0; x<bitmap.width; ++x){
    //        APP_LCD_Data(*memory_ptr >> 8);
    //        APP_LCD_Data(*memory_ptr++ & 0xff);
    //      }
    //    }
    /* legacy 1bit pixel print */
  }else if(bitmap.colour_depth == 1) {  // 1bit format
    //fill fromr regular 1bit using back and fore colors
    // all GLCDs support the same bitmap scrambling
    int line;
    int y_lines = (bitmap.height >> 3);

    u16 initial_y = mios32_lcd_y;
    for(line=0; line<y_lines; ++line) {
      
      // calculate pointer to bitmap line
      u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset + b_x;
      
      // set graphical cursor after second line has reached
      //    if( line > 0 ) {
      //      mios32_lcd_x = initial_x;
      //      mios32_lcd_y += 1;
      //      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
      //    }
      
      // transfer bitmap
      int x, y;
      for(y=0; y<8; ++y){
        for(x=b_x; ((b_width+b_x)>bitmap.width)? (x<bitmap.width) : (x< (b_width+b_x)); ++x){
          //for(x=b_x; x< (b_width+b_x); ++x){
          if(*memory_ptr & (1<<y)){
            APP_LCD_Data(app_lcd_fore_color >> 8);
            APP_LCD_Data(app_lcd_fore_color & 0xff);
          }else{
            APP_LCD_Data(app_lcd_back_color >> 8);
            APP_LCD_Data(app_lcd_back_color & 0xff);
          }
          //DEBUG_MSG("%d %d %d", x, y, memory_ptr);
          memory_ptr++;
        }
        memory_ptr = bitmap.memory + line * bitmap.line_offset + b_x;
        mios32_lcd_y += 1;
        APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
      }
    }
    // fix graphical cursor if more than one line has been print
    mios32_lcd_x += bitmap.width;
    if( y_lines >= 1 ) {
      mios32_lcd_y = initial_y;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }
  }else return -1;  // not supported
  
  
  
  
  return 0; // no error
}

/////////////////////////////////////////////////////////////////////////////
// Transfers Bitmap to the TFT
// Notes: using back/fore colors respectively from pixel off/on for 1bit,
// trasferred to APP_LCD_NativeBitmapPrint for native 16bit
// IN: bitmap
// OUT: returns < 0 on errors
/////////////////////////////////////////////////////////////////////////////
s32 APP_LCD_BitmapPrint(mios32_lcd_bitmap_t bitmap)
{
  
  //if( !MIOS32_LCD_TypeIsGLCD() )
  //return -1; // no GLCD
  
  // abort if max. width reached
  //if( mios32_lcd_x >= mios32_lcd_parameters.width )
  //return -2;
  
  /* native 16bit depth. r(15:11), g(10:5), b(4:0)   */
  if(bitmap.colour_depth == APP_LCD_COLOUR_DEPTH){
    
    u8 *memory_ptr = bitmap.memory;
    u16 initial_x = mios32_lcd_x;
    u16 initial_y = mios32_lcd_y;
    // transfer bitmap
    int x, y;
    APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    
    for(y=0; y<(((initial_y + bitmap.height)<=APP_LCD_HEIGHT)? bitmap.height : (APP_LCD_HEIGHT-initial_y)); ++y){
      for(x=0; x<(((initial_x + bitmap.width)<=APP_LCD_WIDTH)? bitmap.width : (APP_LCD_WIDTH-initial_x)); ++x){
        APP_LCD_Data(*memory_ptr++);
        APP_LCD_Data(*memory_ptr++);
        //DEBUG_MSG("%d %d %d", x, y, memory_ptr);
      }
      if((mios32_lcd_x + bitmap.width)>APP_LCD_WIDTH)memory_ptr +=(mios32_lcd_x + bitmap.width -APP_LCD_WIDTH)*2;
      mios32_lcd_y += 1;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }
    
    /* legacy 1bit pixel print */
  }else if(bitmap.colour_depth == 1) {  // 1bit format
    //fill fromr regular 1bit using back and fore colors
    // all GLCDs support the same bitmap scrambling
    int line;
    int y_lines = (bitmap.height >> 3);

    u16 initial_y = mios32_lcd_y;
    for(line=0; line<y_lines; ++line) {
      
      // calculate pointer to bitmap line
      u8 *memory_ptr = bitmap.memory + line * bitmap.line_offset;
      
      // transfer bitmap
      int x, y;
      for(y=0; y<8; ++y){
        for(x=0; x<bitmap.width; ++x){
          if(*memory_ptr & (1<<y)){
            APP_LCD_Data(app_lcd_fore_color >> 8);
            APP_LCD_Data(app_lcd_fore_color & 0xff);
          }else{
            APP_LCD_Data(app_lcd_back_color >> 8);
            APP_LCD_Data(app_lcd_back_color & 0xff);
          }
          //DEBUG_MSG("%d %d %d", x, y, memory_ptr);
          memory_ptr++;
        }
        memory_ptr = bitmap.memory + line * bitmap.line_offset;
        mios32_lcd_y += 1;
        APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
      }
    }
    // fix graphical cursor if more than one line has been print
    mios32_lcd_x += bitmap.width;
    if( y_lines >= 1 ) {
      mios32_lcd_y = initial_y;
      APP_LCD_GCursorSet(mios32_lcd_x, mios32_lcd_y);
    }
  }else return -1;  // not supported
  
  return 0; // no error
}

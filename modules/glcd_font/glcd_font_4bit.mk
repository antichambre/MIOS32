# $Id: glcd_font.mk 1849 2013-11-09 23:02:03Z tk $
# defines the rule for creating the glcd_font_*.o objects,

# enhance include path
C_INCLUDE +=	-I $(MIOS32_PATH)/modules/glcd_font

# add modules to thumb sources
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/glcd_font/glcd_font_4bit_pix.c \


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/glcd_font

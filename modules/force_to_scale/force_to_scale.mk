# $Id: file.mk 1261 2011-07-17 18:21:45Z tk $
# defines additional rules for integrating the module

# enhance include path
C_INCLUDE += -I $(MIOS32_PATH)/modules/force_to_scale


# add modules to thumb sources (TODO: provide makefile option to add code to ARM sources)
THUMB_SOURCE += \
	$(MIOS32_PATH)/modules/force_to_scale/force_to_scale.c


# directories and files that should be part of the distribution (release) package
DIST += $(MIOS32_PATH)/modules/force_to_scale


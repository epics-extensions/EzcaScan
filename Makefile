#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# $Id$
#
TOP = ../..
#If epics/extensions/configure directory exists, build with it.
#Otherwise use epics/extensions/config.
ifeq (0, $(words $(notdir $(wildcard $(TOP)/configure))))
include $(TOP)/config/CONFIG_EXTENSIONS
include $(TOP)/config/RULES_ARCHS
else
include $(TOP)/configure/CONFIG
INC += EzcaScan.h

USR_CFLAGS = -DACCESS_SECURITY

# Use the following line if building ezca to be called from
# Visual Basic or other languages
#SHARED_LIBRARIES=YES
# Use the following line to build EzcaScan and ezcaIDL as
# standlone DLLs, i.e. without the need for ezca.dll in the path.
SHARED_LIBRARIES=NO

LIBRARY_HOST := EzcaScan
EzcaScan_SRCS = EzcaFunc.c EzcaHash.c EzcaArray.c EzcaUtil.c \
                EzcaMonitor.c  EzcaScan.c EzcaQueue.c

EzcaFunc_CPPFLAGS += -DBASE_3_14

EzcaScan_LIBS += ezca ca Com
EzcaScan_SYS_LIBS_WIN32 = ws2_32 advapi32 user32

#PROD_LIBS += EzcaScan ezca ca Com

#PROD_HOST = caget caput
#caget_SRCS = caget.c getopt.c
#caput_SRCS = caput.c getopt.c

include $(TOP)/configure/RULES
endif


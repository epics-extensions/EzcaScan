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

LIBRARY := EzcaScan
EzcaScan_SRCS = EzcaFunc.c EzcaHash.c EzcaArray.c EzcaUtil.c \
                EzcaMonitor.c  EzcaScan.c EzcaQueue.c

EzcaScan_LIBS += ezca ca Com
PROD_LIBS += EzcaScan ezca ca Com

PROD = caget caput
caget_SRCS = caget.c getopt.c
caput_SRCS = caput.c getopt.c

include $(TOP)/configure/RULES
endif


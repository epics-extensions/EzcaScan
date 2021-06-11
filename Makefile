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
include $(TOP)/configure/CONFIG
INC += EzcaScan.h

USR_CFLAGS = -DACCESS_SECURITY

LIBRARY_HOST := EzcaScan
EzcaScan_SRCS = EzcaFunc.c EzcaHash.c EzcaArray.c EzcaUtil.c \
                EzcaMonitor.c  EzcaScan.c EzcaQueue.c

EzcaScan_LIBS += ezca ca Com
EzcaScan_SYS_LIBS_WIN32 = ws2_32 advapi32 user32

include $(TOP)/configure/RULES


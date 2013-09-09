/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
 *
 */

#ifdef _WIN32
#include "windows.h"
#endif

#include <dbDefs.h>
#include <cadef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <shareLib.h>
#include <epicsMutex.h>

#ifdef EZCA
	#include <ezca.h>
#else 
	#define BOOL char
	#define FALSE 0
	#define TRUE 1
#endif

#ifndef FLDNAME_SZ
#define FLDNAME_SZ 4  /*Field Name Size*/
#endif

#define NAME_LENGTH	PVNAME_SZ+FLDNAME_SZ+2
#define CA_SUCCESS 0
#define CA_FAIL   -1
#define CA_WAIT   -2            /* error = -2 waiting for new value */
                                /* error = -1 not connected */
#define CA_ALLOC  -3            /* error = -3 calloc failed */


#define cs_never_conn   0       /* channel never conn*/
#define cs_prev_conn    1       /* channel previously  conn*/
#define cs_conn         2       /* channel conn*/
#define cs_closed       3       /* channel cleared because never conn*/

struct caGlobals {
        int CA_ERR;
        int devprflag;
        int PEND_EVENT_ON;
        float PEND_IO_TIME;
        float PEND_IOLIST_TIME;
        float PEND_EVENT_TIME;
        int version;
        };

typedef struct event_queue {
	double *pvalues;
	short maxvalues;
	short numvalues;
	short overflow;
        char *scan_names;
        char **pscan;
    
	} EVENT_QUEUE;

typedef struct chandata{
	struct chandata *next;
        chid chid;
        chtype type;
        evid evid;
	int state;
	int error;  
	int event;
        int status;
        int severity;
        double value; 
	char *pvalue;
	char string[MAX_STRING_SIZE]; 
	TS_STAMP stamp;
	EVENT_QUEUE *p_event_queue;
    epicsMutexId mutexId;
        } chandata;

epicsShareExtern struct caGlobals CA;

epicsShareFunc double epicsShareAPI Ezca_iocClockTime(TS_STAMP *stamp);
epicsShareFunc int epicsShareAPI Ezca_getTypeCount(int noName,char **pvName,int *type,int *count);
epicsShareFunc int epicsShareAPI Ezca_getArray(int noName,char **pvName,int type,int nodata,void *value);
epicsShareFunc int epicsShareAPI Ezca_getArrayEvent(int noName,char **pvName,int type,int nodata,void *value);
epicsShareFunc int epicsShareAPI Ezca_putArray(int noName,char **pvName,int type,int nodata,void *value);
epicsShareFunc int epicsShareAPI Ezca_putArrayEvent(int noName,char **pvName,int type,int nodata,void *value);


epicsShareFunc void epicsShareAPI Ezca_sleep_time(double t);
epicsShareFunc int epicsShareAPI Ezca_find_dev(char *name,chandata **pchandata);
epicsShareFunc int epicsShareAPI Ezca_search_list(int noName,char **pvName,chandata *list);
epicsShareFunc int epicsShareAPI Ezca_pvlist_search(int noName,char **pvName,chandata **list, chandata ***chanlist);
epicsShareFunc int epicsShareAPI Ezca_get_error_array(int noName,char **pvName,int *value);

epicsShareFunc chandata * epicsShareAPI Ezca_check_hash_table(char *name);

epicsShareFunc int epicsShareAPI Ezca_monitorArrayAdd(int noName,char **pvName);
epicsShareFunc int epicsShareAPI Ezca_monitorArrayClear(int noName,char **pvName);
epicsShareFunc int epicsShareAPI Ezca_monitorArrayCheck(int noName,char **pvName,int *vals);
epicsShareFunc int epicsShareAPI Ezca_monitorArrayGet(int noName,char **pvName,double *vals);

epicsShareFunc EVENT_QUEUE * epicsShareAPI Ezca_queueAlloc(int size);
epicsShareFunc int epicsShareAPI Ezca_queueGet(char *pvName,double *vals,int *overflow,int mode,int *num);

epicsShareFunc int epicsShareAPI Ezca_queueZero(int noName,char **pvName);
epicsShareFunc int epicsShareAPI Ezca_queueAdd(int mode,int maxvalues,int noName,char **pvName);
epicsShareFunc int epicsShareAPI Ezca_queueClear(int noName,char **pvName);

epicsShareFunc EVENT_QUEUE * epicsShareAPI Ezca_scanAlloc(int size,int nonames);
epicsShareFunc int epicsShareAPI Ezca_scanAdd(int npts,int nonames,char *pv,char **pvnames);
epicsShareFunc int epicsShareAPI Ezca_scanAddMonitor(chandata *pchandata,int npts,int nonames,char **pvnames);
epicsShareFunc int epicsShareAPI Ezca_scanClearMonitor(int noName,char **pvName);
epicsShareFunc int epicsShareAPI Ezca_scanGetMonitor(char *pvName,double *vals);
epicsShareFunc int epicsShareAPI Ezca_scanZeroMonitor(int noName,char **pvName);

epicsShareFunc int epicsShareAPI Ezca_setPendTime(int flag, float *rtime);
epicsShareFunc int epicsShareAPI Ezca_init(int flag);
epicsShareFunc int epicsShareAPI Ezca_debug(int flag);
epicsShareFunc int epicsShareAPI Ezca_version(char *str);
epicsShareFunc int epicsShareAPI Ezca_timeStamp(char *name, char *str);


void Ezca_getTimeCallback(struct event_handler_args args);
void Ezca_putTimeCallback(struct event_handler_args args);
void Ezca_stsDoubleCallback(struct event_handler_args args);
void Ezca_stsStringCallback(struct event_handler_args args);
void Ezca_queueValueChangeCallback(struct event_handler_args args);
void Ezca_queueValueChangeCallback3(struct event_handler_args args);
void Ezca_scanValueChangeCallback(struct event_handler_args args);
void Ezca_monitorValueChangeEvent(struct event_handler_args args);
void Ezca_monitorStringChangeEvent(struct event_handler_args args);
void Ezca_connectionAddEvent(chandata *pchandata);
void Ezca_connect_change_event(struct connection_handler_args args);


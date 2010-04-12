/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* EzcaMonitor.c
 *
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
 *
 * Currently monitored as double 
 * If enum or string type monitored as string
 *
 */

#ifdef _WIN32
#include "windows.h"
#endif

#include <dbDefs.h>
#include <cadef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef EZCA
    #include <ezca.h>
#endif

#define epicsExportSharedSymbols
#include "EzcaScan.h"

extern chandata **chanlist;
#if !defined(_WIN32) && !defined(LINUX) && !defined(SOLARIS)
extern double atof();
#endif
/******************************************************
  add a monitor list
******************************************************/
int epicsShareAPI Ezca_monitorArrayAdd(noName,pvName)
int noName;
char **pvName;
{
int i,status,command_error=0;
chandata *list,*pchan;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];
                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
		else  if (pchan->evid == NULL) {


	if (ca_field_type(pchan->chid) == DBR_STRING) 
       	 	status = ca_add_masked_array_event(DBR_TIME_STRING,0,
                	pchan->chid,
			Ezca_monitorStringChangeEvent,
       		        pchan,
       		        (float)0,(float)0,(float)0,
       		        &(pchan->evid),
       		        DBE_VALUE | DBE_ALARM);
	
	else

       		status = ca_add_masked_array_event(DBR_TIME_DOUBLE,0,
                	pchan->chid,
			Ezca_monitorValueChangeEvent,
	                pchan,
       	        	(float)0,(float)0,(float)0,
       	        	&(pchan->evid),
                	DBE_VALUE | DBE_ALARM);
			pchan->error = status;

			/* automatically add a change connection event */

			Ezca_connectionAddEvent(pchan);  

			if (CA.devprflag > 0)
			fprintf(stderr,"Ezca_monitorArrayAdd: name=%s, evid=%p\n",
				ca_name(pchan->chid),pchan->evid);
			}
                }

	ca_pend_event(CA.PEND_EVENT_TIME);
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else {
	fprintf(stderr,"Ezca_monitorArrayAdd: failed to alloc\n");
	return(CA_FAIL);
	}
}

/******************************************************
  clear a monitor list
******************************************************/
int epicsShareAPI Ezca_monitorArrayClear(noName,pvName)
int noName;
char **pvName;
{
int i,status,command_error=0;
chandata *list,*pchan;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];
                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
		else 
		if (pchan->evid) {
			status = ca_clear_event(pchan->evid);
			pchan->error = status; 
			pchan->evid = NULL;
			if (CA.devprflag > 0)
			fprintf(stderr,"Ezca_monitorArrayClear: name=%s\n",
				ca_name(pchan->chid));
			}
                }

	ca_pend_event(CA.PEND_EVENT_TIME);
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else {
	fprintf(stderr,"Ezca_monitorArrayClear: failed to alloc\n");
	return(CA_FAIL);
	}
}



/******************************************************
  value change event callback 
******************************************************/
void Ezca_monitorValueChangeEvent(args)
struct event_handler_args args;
{
chandata *pchandata;
double time;

	pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif
	
	if (CA.devprflag > 1)
	fprintf(stderr,"Old: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
		ca_name(pchandata->chid),
		pchandata->value, pchandata->status,
		pchandata->severity,pchandata->event);
	pchandata->value = ((struct dbr_time_double *)args.dbr)->value;
        pchandata->status= ((struct dbr_time_double *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_double *)args.dbr)->severity;
	pchandata->event = 1;
	pchandata->stamp = ((struct dbr_time_double *)args.dbr)->stamp;
	if (CA.devprflag > 1) {
	time = Ezca_iocClockTime(&pchandata->stamp);
	fprintf(stderr,"New: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
		ca_name(pchandata->chid),
		pchandata->value, pchandata->status,
		pchandata->severity,pchandata->event);
		
	}

#ifdef ACCESS_SECURITY
	}
	else 
	if (CA.devprflag > 0) 
	fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                ca_name(pchandata->chid));
#endif
}

/******************************************************
  string change event callback 
******************************************************/
void Ezca_monitorStringChangeEvent(args)
struct event_handler_args args;
{
chandata *pchandata;
double time;

	pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif

	if (CA.devprflag > 1)
	fprintf(stderr,"OldString: name=%s, value=%s, stat=%d, sevr=%d, event=%d\n",
		ca_name(pchandata->chid),
		pchandata->string, pchandata->status,
		pchandata->severity,pchandata->event);
	strcpy(pchandata->string, ((struct dbr_time_string *)args.dbr)->value);
        pchandata->status= ((struct dbr_time_string *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_string *)args.dbr)->severity;
	pchandata->event = 1;
	pchandata->stamp = ((struct dbr_time_string *)args.dbr)->stamp;
	if (CA.devprflag > 1) {
	time = Ezca_iocClockTime(&pchandata->stamp);
	fprintf(stderr,"NewString: name=%s, value=%s, stat=%d, sevr=%d, event=%d\n",
		ca_name(pchandata->chid),
		pchandata->string, pchandata->status,
		pchandata->severity,pchandata->event);
	}

if (pchandata->type != DBR_STRING) 
	ca_get_callback(DBR_TIME_DOUBLE,pchandata->chid,
                  Ezca_stsDoubleCallback,pchandata);
#ifdef ACCESS_SECURITY
	}
	else 
	if (CA.devprflag > 0) 
	fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                ca_name(pchandata->chid));
#endif
}

/******************************************************
  process event id for a monitored list and reset to 0
******************************************************/
int epicsShareAPI Ezca_monitorArrayCheck(noName,pvName,vals)
int noName;
char **pvName;
int *vals;
{
int i,command_error=0;
chandata *list,*pchan;

	ca_pend_event(0.00001);

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];

                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
		else  if (pchan->evid ) {
			if (CA.devprflag > 0) 
				fprintf(stderr,"Ezca_monitorArrayCheck: name=%s, value=%f, status=%d, event=%d\n",
				ca_name(pchan->chid),pchan->value,
				pchan->status,pchan->event);

			 *(vals+i) = pchan->event; 
		      if (pchan->event ) 
				pchan->event = 0;  /* reset after read*/
			  
			}
		else {
		if (CA.devprflag >= 0)
			command_error = CA_FAIL;	
			fprintf(stderr,"Error: %s is not monitored yet.\n",
			ca_name(pchan->chid)); 
			}
        }

	free(chanlist);
        chanlist = NULL;
        return(command_error);
} else {
        fprintf(stderr,"Ezca_monitorArrayCheck: failed to alloc\n");
        return(CA_FAIL);
        }
}


/******************************************************
  get event values for a monitored list
******************************************************/
int epicsShareAPI Ezca_monitorArrayGet(noName,pvName,vals)
int noName;
char **pvName;
double *vals;
{
int i,command_error=0;
chandata *list,*pchan;

	ca_pend_event(0.00001);

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];

                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
		else  if (pchan->evid ) {

	if (ca_field_type(pchan->chid) == DBR_STRING)
	{
			if (CA.devprflag > 0) 
				fprintf(stderr,"Ezca_monitorArrayGet: name=%s, value=%s, status=%d, event=%d\n",
				ca_name(pchan->chid),pchan->string,
				pchan->status,pchan->event);
				pchan->value = atof(pchan->string);
			 *(vals+i) = pchan->value; 
			if (pchan->event)
				 pchan->event = 0;  /* reset after read*/

	} else {

			if (CA.devprflag > 0) 
				fprintf(stderr,"Ezca_monitorArrayGet: name=%s, value=%f, status=%d, event=%d\n",
				ca_name(pchan->chid),pchan->value,
				pchan->status,pchan->event);

			 *(vals+i) = pchan->value; 
			if (pchan->event)
				 pchan->event = 0;  /* reset after read*/
	}
			}
		else  {
		if (CA.devprflag >= 0)
			fprintf(stderr,"Error: %s is not monitored yet.\n",
			ca_name(pchan->chid)); 
			}
       }
        free(chanlist);
        chanlist = NULL;
        return(command_error);
} else {
        fprintf(stderr,"Ezca_monitorArrayGet: failed to alloc\n");
        return(CA_FAIL);
        }

}



/******************************************************
  get value callback instead of ca_get function
******************************************************/
void Ezca_stsDoubleCallback(args)
struct event_handler_args args;
{
chandata *pchandata;

	pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif

        if (CA.devprflag > 1)
        fprintf(stderr,"Old: name=%s, value=%f, stat=%d, sevr=%d, error=%d\n",
                ca_name(pchandata->chid),
                pchandata->value, pchandata->status,
                pchandata->severity,pchandata->error);

	/* get new value */

        pchandata->value = ((struct dbr_sts_double *)args.dbr)->value;
        pchandata->status= ((struct dbr_sts_double *)args.dbr)->status;
        pchandata->severity = ((struct dbr_sts_double *)args.dbr)->severity;
        pchandata->error= 0;

        if (CA.devprflag > 1)
        fprintf(stderr,"New: name=%s, value=%f, stat=%d, sevr=%d, error=%d\n",
                ca_name(pchandata->chid),
                pchandata->value, pchandata->status,
                pchandata->severity,pchandata->error);

#ifdef ACCESS_SECURITY
	}
	else 
	if (CA.devprflag > 0) 
	fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                ca_name(pchandata->chid));
#endif
}



/******************************************************
  get value callback instead of ca_get function
******************************************************/
void Ezca_stsStringCallback(args)
struct event_handler_args args;
{
chandata *pchandata;

	pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif

        if (CA.devprflag > 1)
        fprintf(stderr,"Old: name=%s, value=%s, stat=%d, sevr=%d, error=%d\n",
                ca_name(pchandata->chid),
                pchandata->string, pchandata->status,
                pchandata->severity,pchandata->error);

	/* get new value */

        strcpy(pchandata->string, ((struct dbr_sts_string *)args.dbr)->value);
        pchandata->status= ((struct dbr_sts_string *)args.dbr)->status;
        pchandata->severity = ((struct dbr_sts_string *)args.dbr)->severity;
	/*pchandata->value = atof(pchandata->string);*/
        pchandata->error= 0;

        if (CA.devprflag > 1)
        fprintf(stderr,"New: name=%s, value=%s, stat=%d, sevr=%d, error=%d\n",
                ca_name(pchandata->chid),
                pchandata->string, pchandata->status,
                pchandata->severity,pchandata->error);
#ifdef ACCESS_SECURITY
	}
	else 
	if (CA.devprflag > 0) 
	fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                ca_name(pchandata->chid));
#endif
	
}

/****************************************************
 *  change connection event callback
 ****************************************************/
void Ezca_connect_change_event(args)
struct connection_handler_args args;
{
chandata *pchandata;

        pchandata = (chandata *)ca_puser(args.chid);
        if (pchandata->state == cs_conn) {
                fprintf(stderr,"****Connection lost happened on %s ****\n",
                                ca_name(pchandata->chid));
                pchandata->state = cs_prev_conn;
                }
        else {
                fprintf(stderr,"****Reconnection established on %s ****\n",
                                ca_name(pchandata->chid));
                pchandata->state = cs_conn;
                }
}
/****************************************************
 *  add connection event for a channel
****************************************************/
void Ezca_connectionAddEvent(pchandata)
chandata *pchandata;
{
        ca_set_puser(pchandata->chid,pchandata);
        ca_change_connection_event(pchandata->chid,Ezca_connect_change_event);
}


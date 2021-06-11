/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* EzcaScan.c
 *  Monitored as double values
 *
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
 *
 */

#ifdef _WIN32
#include "windows.h"
#endif

#include <dbDefs.h>
#include <cadef.h>
#include <epicsThread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef EZCA
    #include <ezca.h>
#endif

#define epicsExportSharedSymbols
#include "EzcaScan.h"

//extern chandata *pchandata;
//chandata *pchandata;

/* trigger scan: event allocation  */
EVENT_QUEUE * epicsShareAPI Ezca_scanAlloc(int size,int nonames)
{
    int i;
    EVENT_QUEUE *pevent;
    pevent = (EVENT_QUEUE *)calloc(1,sizeof(EVENT_QUEUE));
    pevent->pvalues = (double *)calloc(nonames*size+1,sizeof(double));
    if (pevent->pvalues == NULL) {
        fprintf(stderr,"Error: Ezca_scanAlloc failed to calloc !\n");
        exit(CA_ALLOC);
    }

    pevent->maxvalues = size;
    pevent->numvalues = 0;          /* current index */
    pevent->overflow = nonames;       /* this used for nonames*/
    /* alloc space for detectors  PV names*/
    pevent->scan_names = (char *)calloc(nonames,MAX_STRING_SIZE);
    pevent->pscan = (char **)calloc(nonames,sizeof(char *));
    for (i=0;i<nonames;i++) {
        pevent->pscan[i] = (char *)(pevent->scan_names + i * MAX_STRING_SIZE);
    }
    return(pevent);
}


/******************************************************
trigger scan queuing value change event callback 
******************************************************/
void Ezca_scanValueChangeCallback(struct event_handler_args args)
{
    chandata *pchan;
    chandata *pchandata;
    int num,nonames,i;

    pchan = (chandata *)args.usr;
    epicsMutexLock(pchan->mutexId);

#ifdef ACCESS_SECURITY
    if (args.status == ECA_NORMAL) {
#endif
    
    if (CA.devprflag > 1)
    {
        fprintf(stderr,"Old: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
            ca_name(pchan->chid),
            pchan->value, pchan->status,
            pchan->severity,pchan->event);
    }
    pchan->value = ((struct dbr_time_double *)args.dbr)->value;
    pchan->status= ((struct dbr_time_double *)args.dbr)->status;
    pchan->severity = ((struct dbr_time_double *)args.dbr)->severity;
    pchan->event = 3;

    pchan->stamp = ((struct dbr_time_double *)args.dbr)->stamp;
    if (CA.devprflag > 1) {
        Ezca_iocClockTime(&pchan->stamp);
        fprintf(stderr,"New: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
            ca_name(pchan->chid),
            pchan->value, pchan->status,
            pchan->severity,pchan->event);
    }

#ifdef ACCESS_SECURITY
    }
    else 
    {
        fprintf(stderr,"Diagnostic: Read access denied : %s\n",
            ca_name(pchan->chid));
    }
#endif

    if (pchan->p_event_queue != NULL ) {
        if (pchan->p_event_queue->numvalues < pchan->p_event_queue->maxvalues) {

            /* get the trigger scan pvname's  values */

            nonames = pchan->p_event_queue->overflow;
            num = pchan->p_event_queue->numvalues * nonames;
            for (i=0;i<nonames;i++) {
                Ezca_find_dev(pchan->p_event_queue->pscan[i],&pchandata);
                if (pchandata->type  != TYPENOTCONN) {
                    *(pchan->p_event_queue->pvalues+num+i) = pchandata->value;
                    if (pchandata->event) pchandata->event = 0;
                }
            }
        
            /* increase the num */

            pchan->p_event_queue->numvalues += 1;
        }
    }
    epicsMutexUnlock(pchan->mutexId);

}


int epicsShareAPI Ezca_scanAdd(int npts,int nonames,char *pv,char **pvnames)
{
    chandata *pchan;
    chandata *pchandata;
    int command_error=0;

    pchandata = NULL;
    command_error = Ezca_find_dev(pv,&pchandata);
    if (pchandata != NULL){
        pchan = pchandata;
        if (pchan->type == TYPENOTCONN) return(command_error);

        if (pchan->evid == NULL) {
            command_error =    Ezca_scanAddMonitor(pchan,npts,nonames,pvnames);
            return(command_error);
        }
        else return(1);   /* already monitored return 1 */
    }
    return command_error;
}

/******************************************************
 trigger scan : add the  monitor list only double value, return CA.CA_ERR
******************************************************/
int epicsShareAPI Ezca_scanAddMonitor(chandata *pchandata,int npts,int nonames,char **pvnames)
{
    int i=0,status=0,command_error=0;


    pchandata->type = ca_field_type(pchandata->chid);
    if (pchandata->state != cs_conn) 
    {
        command_error = CA_FAIL;
    }
    else  if (pchandata->evid == NULL) {
        pchandata->p_event_queue = 
            (EVENT_QUEUE *)Ezca_scanAlloc(npts,nonames);

        /*  add the scan record monitoring for detectors */
        for (i=0;i<nonames;i++ ) 
        {
            sprintf(pchandata->p_event_queue->pscan[i],"%s",pvnames[i]); 
        }

        status = Ezca_monitorArrayAdd(nonames,pchandata->p_event_queue->pscan);
        if ( status == CA_FAIL) 
        {
            fprintf(stderr,"Error: trigger scan monitor 2 failed\n");
        }

        /*  add the scan record monitoring */
        status = ca_add_masked_array_event(DBR_TIME_DOUBLE,0,
                pchandata->chid,
                Ezca_scanValueChangeCallback,
                pchandata,
                (float)0,(float)0,(float)0,
                &(pchandata->evid),
                DBE_VALUE | DBE_ALARM);
    }
    if (status != ECA_NORMAL) command_error = CA_FAIL;
    /* automatically add a change connection event */

    /* ca_connect_add_event(pchandata); */


    ca_pend_event(CA.PEND_EVENT_TIME); 
    if (CA.devprflag > 0)
    {
        fprintf(stderr,"Ezca_scanAddMonitor: %d\n",command_error);
    }

    return(command_error);
}


/******************************************************
 trigger scan:  clear the trigger monitor list , return CA.CA_ERR 
******************************************************/
int epicsShareAPI Ezca_scanClearMonitor(int noName,char **pvName)
{
    int status,command_error=0;
    chandata *list,*snode,*pchan;
    chandata **chanlist;
    int nonames;

    chanlist = (chandata **)calloc(noName,sizeof(chandata *));
    command_error = Ezca_pvlist_search(noName,pvName,&list, &chanlist);

    snode = list;
    while (snode)  {
        pchan = snode;
        pchan->type = ca_field_type(pchan->chid);
        if (pchan->state != cs_conn) 
        {
            command_error = CA_FAIL;
        }
        else if (pchan->evid) 
        {
            epicsMutexLock(pchan->mutexId);
            if ( pchan->p_event_queue != NULL) {
                nonames = pchan->p_event_queue->overflow;
                status = Ezca_monitorArrayClear(nonames, pchan->p_event_queue->pscan);
                if (CA.devprflag > 0 && status != ECA_NORMAL) 
                {
                    fprintf(stderr,"Error: trigger scan clear event 2 failed.\n");
                }
                status = ca_clear_event(pchan->evid);
                if (status != ECA_NORMAL) command_error = CA_FAIL;
                pchan->evid = NULL;
                pchan->event = 0;
                free(pchan->p_event_queue->pvalues);
                free(pchan->p_event_queue->scan_names);
                free(pchan->p_event_queue->pscan);
                free(pchan->p_event_queue);
                pchan->p_event_queue = NULL;
            }
            epicsMutexUnlock(pchan->mutexId);
        
        }
        snode = snode->next;
    }

    ca_pend_event(CA.PEND_EVENT_TIME);
    if (CA.devprflag > 0) 
    {
        fprintf(stderr,"Ezca_scanClearMonitor: %d\n",command_error);
    }
    return(command_error);
}


/******************************************************
trigger scan :  get event values for monitored pvs, 
it  returns CA_FAIL if error  else return the index of
scan process
******************************************************/
int epicsShareAPI Ezca_scanGetMonitor(char *pvName,double *vals)
{
    int i,num,ii=0;
    evid temp_evid;
    chandata *pchan;
    chandata *pchandata;
    int nonames;

    num = CA_FAIL;
    ca_pend_event(0.00001);

    Ezca_find_dev(pvName,&pchandata); 
    /* take care the racing problem  for scan record */

    while ( strcmp(pvName,ca_name(pchandata->chid)) != 0 ) 
    {
        Ezca_find_dev(pvName,&pchandata); 
    }
    pchan = pchandata;
    epicsMutexLock(pchan->mutexId);
    pchan->type = ca_field_type(pchan->chid);
    temp_evid = pchan->evid;
    if (pchan->state != cs_conn) 
    {
        epicsMutexUnlock(pchan->mutexId);
        return CA_FAIL;
    }
    else  if (temp_evid ) 
    {
        if (pchan->p_event_queue != NULL) 
        {
            if (pchan->p_event_queue->numvalues > 0) 
            { 
                nonames = pchan->p_event_queue->overflow;
                ii = nonames*pchan->p_event_queue->numvalues;
                for (i=0;i<ii;i++) {     
                    *(vals+i) = *(pchan->p_event_queue->pvalues+i); 
                 }   
            }

            num =  pchan->p_event_queue->numvalues;
                 
        } 
        else 
        {
            if (CA.devprflag > 0)
            {   
                fprintf(stderr, "Ezca_scanGetMonitor found p_event_queue unitialized.   Channel Name %s.  Returning CA_FAIL.\n", 
                        ca_name(pchan->chid));
            }
            epicsMutexUnlock(pchan->mutexId);
            return CA_FAIL;
        }
    } 
    else 
    {
        if (CA.devprflag >= 0)
        {
            fprintf(stderr,"Error (Ezca_scanGetMonitor): %s is not monitored yet.\n",
                ca_name(pchan->chid)); 
        }
    }

    if (CA.devprflag > 0) 
    {
        fprintf(stderr,"Ezca_scanGetMonitor: %d\n",num);
    }
    epicsMutexUnlock(pchan->mutexId);
    return (num);
}

/******************************************************
trigger scan :  zero a monitor queue list 
******************************************************/
int epicsShareAPI Ezca_scanZeroMonitor(int noName,char **pvName)
{
    int i,command_error=0,size;
    chandata *list,*snode,*pchan;
    chandata **chanlist;
    int nonames;

    chanlist = (chandata **)calloc(noName,sizeof(chandata *));
    command_error = Ezca_pvlist_search(noName,pvName,&list,&chanlist);

    snode = list;
    while (snode)  {
        pchan = snode;
        epicsMutexLock(pchan->mutexId);
        pchan->type = ca_field_type(pchan->chid);
        if (pchan->state != cs_conn) 
        {
            command_error = CA_FAIL;
        }
        else
        { 
            if (pchan->evid) 
            {
                if (pchan->p_event_queue != NULL) 
                {
                    size = pchan->p_event_queue->maxvalues;
                    nonames = pchan->p_event_queue->overflow;
                    for (i=0;i<nonames*size;i++){
                         pchan->p_event_queue->pvalues[i] = 0.;
                    }
                    pchan->p_event_queue->numvalues = 0;
                    pchan->event = 0;
                }
                else 
                {
                    if (CA.devprflag > 0)
                    {
                        fprintf(stderr, "Ezca_scanZeroMonitor found p_event_queue unitialized.  Channel Name %s. Returning CA_FAIL.\n",
                            ca_name(pchan->chid));
                    }
                    epicsMutexUnlock(pchan->mutexId);
                    return CA_FAIL;
                }
                epicsMutexUnlock(pchan->mutexId);
                
            }
            snode = snode->next;
        }
    }

    ca_pend_event(CA.PEND_EVENT_TIME);
 
    if (CA.devprflag > 0)
    {
        fprintf(stderr,"Ezca_scanZeroMonitor: %d\n",command_error);
    }
    return(command_error);
}


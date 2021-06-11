/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/

/* EzcaQueue.c
 *  special monitor queue for scalar type record
 *  
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
 *
 * Monitor queue can operate in 3 modes
 *        1 - clear buffer for each get           
 *        2 - new event added until the buffer is full,
 *                  a user has to use zero to clear the buffer
 *        3 - most current nmax values are kept in buffer
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

//chandata *pchandata;
/**
*/
EVENT_QUEUE * epicsShareAPI Ezca_queueAlloc(int size)
{
    EVENT_QUEUE *pevent;
    pevent = (EVENT_QUEUE *)calloc(1,sizeof(EVENT_QUEUE));
    pevent->pvalues = (double *)calloc(size,sizeof(double));
    if (pevent->pvalues == NULL) {
        fprintf(stderr,"Error: Ezca_queueAlloc failed to calloc !\n");
        exit(CA_ALLOC);
    }
    pevent->maxvalues = size;
    pevent->numvalues = 0;
    pevent->overflow = 0;
    return(pevent);
}


/******************************************************
  queuing value change event callback 
******************************************************/
void Ezca_queueValueChangeCallback(args)
struct event_handler_args args;
{
    chandata *pchandata;

    pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
    if (args.status == ECA_NORMAL) {
#endif
    
        if (CA.devprflag > 1) 
        {
            fprintf(stderr,"Old: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
                ca_name(pchandata->chid),
                pchandata->value, pchandata->status,
                pchandata->severity,pchandata->event);
        }
        pchandata->value = ((struct dbr_time_double *)args.dbr)->value;
        pchandata->status= ((struct dbr_time_double *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_double *)args.dbr)->severity;
        pchandata->event = 2;

        pchandata->stamp = ((struct dbr_time_double *)args.dbr)->stamp;
        if (CA.devprflag > 1) {
            Ezca_iocClockTime(&pchandata->stamp);
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
    if (pchandata->p_event_queue->numvalues < pchandata->p_event_queue->maxvalues) {
        *(pchandata->p_event_queue->pvalues+pchandata->p_event_queue->numvalues) = 
            pchandata->value; 
        pchandata->p_event_queue->numvalues += 1;
        if (pchandata->p_event_queue->numvalues >= 
            pchandata->p_event_queue->maxvalues) 
        {
           pchandata->p_event_queue->overflow = 1;
        }
    }
}

/******************************************************
 circular buffer queuing value change event callback 
******************************************************/
void Ezca_queueValueChangeCallback3(args)
struct event_handler_args args;
{
    chandata *pchandata;
    int ne;

    pchandata = (chandata *)args.usr;

#ifdef ACCESS_SECURITY
    if (args.status == ECA_NORMAL) {
#endif
    
        if (CA.devprflag > 1)
        {
            fprintf(stderr,"Old: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
                ca_name(pchandata->chid),
                pchandata->value, pchandata->status,
                pchandata->severity,pchandata->event);
        }
        pchandata->value = ((struct dbr_time_double *)args.dbr)->value;
        pchandata->status= ((struct dbr_time_double *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_double *)args.dbr)->severity;
        pchandata->event = 2;

        pchandata->stamp = ((struct dbr_time_double *)args.dbr)->stamp;
        if (CA.devprflag > 1) {
            Ezca_iocClockTime(&pchandata->stamp);
            fprintf(stderr,"New: name=%s, value=%f, stat=%d, sevr=%d, event=%d\n",
                ca_name(pchandata->chid),
                pchandata->value, pchandata->status,
                pchandata->severity,pchandata->event);
        }
#ifdef ACCESS_SECURITY
    }
    else 
        if (CA.devprflag > 0) fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                    ca_name(pchandata->chid));
#endif
    if (pchandata->p_event_queue->numvalues <= pchandata->p_event_queue->maxvalues) 
    {
        *(pchandata->p_event_queue->pvalues+pchandata->p_event_queue->numvalues) = pchandata->value; 
        pchandata->p_event_queue->numvalues += 1;
        if (pchandata->p_event_queue->numvalues >= 
            pchandata->p_event_queue->maxvalues) 
        {
            pchandata->p_event_queue->overflow = 1;
        }
    }
    else 
    {
        pchandata->p_event_queue->numvalues += 1;
        pchandata->p_event_queue->overflow = 1;
        ne = pchandata->p_event_queue->numvalues%pchandata->p_event_queue->maxvalues;
        *(pchandata->p_event_queue->pvalues+ne) = pchandata->value;
    }
}

/******************************************************
  get event queue values for a monitored pv, return CA.CA_ERR
******************************************************/
int epicsShareAPI Ezca_queueGet(pvName,vals,overflow,mode,num)
int *overflow,mode,*num;
char *pvName;
double *vals;
{
    int i,command_error=0,ne,ii;
    chandata *pchandata;
    chandata *pchan;

    *num = CA_FAIL;
    ca_pend_event(0.00001);

    Ezca_find_dev(pvName,&pchandata);
    pchan=pchandata;

    pchan->type = ca_field_type(pchan->chid);
    if (pchan->state != cs_conn) 
    {
        command_error = CA_FAIL;
    }
    else  if (pchan->evid ) 
    {

        if (mode == 1) {
            for (i=0;i<pchan->p_event_queue->numvalues;i++) 
            {
                vals[i] = pchan->p_event_queue->pvalues[i];
            }
            if (pchan->event) {
               *num =  pchan->p_event_queue->numvalues;
                *overflow = pchan->p_event_queue->overflow;
                pchan->p_event_queue->numvalues = 0;
                pchan->p_event_queue->overflow = 0;
                pchan->event = 0;  /* reset after read*/
            }
        }

        if (mode == 2) 
        {
            for (i=0;i<pchan->p_event_queue->numvalues;i++) {     
                vals[i] = pchan->p_event_queue->pvalues[i]; 
            }
            /* reset after read*/
            *num =  pchan->p_event_queue->numvalues;
            *overflow = pchan->p_event_queue->overflow;
        }

        if (mode == 3) 
        {
            if (pchan->p_event_queue->numvalues < pchan->p_event_queue->maxvalues) 
            {
                for (i=0;i<pchan->p_event_queue->numvalues;i++) {     
                    vals[i] = pchan->p_event_queue->pvalues[i]; 
                    *num =  pchan->p_event_queue->numvalues;
                    *overflow = pchan->p_event_queue->overflow;
                }
            } 
            else 
            {
                ne = pchan->p_event_queue->numvalues%pchan->p_event_queue->maxvalues;
                for (i=0;i<pchan->p_event_queue->maxvalues;i++) {
                    ii = ne + 1 + i;
                    if (ii >= pchan->p_event_queue->maxvalues) 
                       ii = ii - pchan->p_event_queue->maxvalues;
                    vals[i] = pchan->p_event_queue->pvalues[ii];
                    pchan->p_event_queue->overflow = 1;
                }
                *num =  pchan->p_event_queue->maxvalues;
                           *overflow = pchan->p_event_queue->overflow; 
            }
            pchan->event = 0;
        }

        if (CA.devprflag > 0) { 
            fprintf(stderr,"Ezca_queueGet: name=%s, value=%f, status=%d, event=%d\n",
                ca_name(pchan->chid),pchan->value,
                pchan->status,pchan->event);
            fprintf(stderr,"p_event_queue->values: ");
            
            for (i=0;i<*num;i++) {     
                fprintf(stderr,"%f ",vals[i]);
            }
            fprintf(stderr,"\n");
        }

    } else {
        if (CA.devprflag >= 0)
             fprintf(stderr,"Ezca_queueGet Error: %s is not monitored yet.\n",
                ca_name(pchan->chid)); 
    }

    return (command_error);
}

/******************************************************
zero a monitor queue list , return CA.CA_ERR 
******************************************************/
int epicsShareAPI Ezca_queueZero(int noName,char **pvName)
{
    int i,command_error=0,size;
    chandata **chanlist;
    chandata *list,*snode,*pchan;

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
        else 
        {    
            if (pchan->evid) {
                size =  pchan->p_event_queue->maxvalues;
                for (i=0;i<size;i++) {
                    pchan->p_event_queue->pvalues[i] = 0.;
                }
                pchan->p_event_queue->numvalues = 0;
                pchan->p_event_queue->overflow = 0;
                pchan->event = 0;  
            }
        }
        snode = snode->next;
    }

    ca_pend_event(CA.PEND_EVENT_TIME);

    return(command_error);
}



/******************************************************
  add a circular buffer monitor queue list only double value, return CA.CA_ERR
******************************************************/
int epicsShareAPI Ezca_queueAdd(mode,maxvalues,noName,pvName)
int noName,mode,maxvalues;
char **pvName;
{
int i=0,status=0,command_error=0;
chandata *list,*snode,*pchan;
chandata **chanlist;
        chanlist = (chandata **)calloc(noName,sizeof(chandata *));
        command_error = Ezca_pvlist_search(noName,pvName,&list, &chanlist);

        snode = list;
        while (snode)  {
                pchan = snode;
                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
        else  if (pchan->evid == NULL) {

    if (mode < 3)  {
                status = ca_add_masked_array_event(DBR_TIME_DOUBLE,0,
                        pchan->chid,
                        Ezca_queueValueChangeCallback,
                        pchan,
                        (float)0,(float)0,(float)0,
                        &(pchan->evid),
                        DBE_VALUE | DBE_ALARM);
            }
    if (mode == 3)  {
               status = ca_add_masked_array_event(DBR_TIME_DOUBLE,0,
                    pchan->chid,
            Ezca_queueValueChangeCallback3,
                    pchan,
                       (float)0,(float)0,(float)0,
                       &(pchan->evid),
                    DBE_VALUE | DBE_ALARM);
            }

            pchan->error = status;

            pchan->p_event_queue = 
                   (EVENT_QUEUE *)Ezca_queueAlloc(maxvalues);

            /* automatically add a change connection event */

            Ezca_connectionAddEvent(pchan);  

            if (CA.devprflag > 0)
            fprintf(stderr,"name=%s, evid=%p\n",
                ca_name(pchan->chid),pchan->evid);
            }
        snode = snode->next;
        i++;
                }

    ca_pend_event(CA.PEND_EVENT_TIME);
    
    return(command_error);
}


/******************************************************
  clear a monitor queue list , return CA.CA_ERR 
******************************************************/
int epicsShareAPI Ezca_queueClear(noName,pvName)
int noName;
char **pvName;
{
int status,command_error=0;
chandata *list,*snode,*pchan;
chandata **chanlist;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));
        command_error = Ezca_pvlist_search(noName,pvName,&list, &chanlist);

        snode = list;
        while (snode)  {
                pchan = snode;
                pchan->type = ca_field_type(pchan->chid);
                if (pchan->state != cs_conn) 
                        command_error = CA_FAIL;
        else 
            if (pchan->evid) {
            status = ca_clear_event(pchan->evid);
            pchan->error = status;
            pchan->evid = NULL;
            free(pchan->p_event_queue->pvalues);
            free(pchan->p_event_queue);
            pchan->p_event_queue = NULL;
                }
        snode = snode->next;
                }


    ca_pend_event(CA.PEND_EVENT_TIME);

    return(command_error);
}


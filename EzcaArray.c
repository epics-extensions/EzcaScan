/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* EzcaArray.c
 *
 *  Array Get and Array Put  
 *
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
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

/****************************************************
 *  get native DB field type and count for the given array 
 *  	return 0 if success, -1  if error encountered 
 ****************************************************/
int epicsShareAPI Ezca_getTypeCount(noName,pvName,type,count)
int noName;
char **pvName;
int  *type;
int  *count;
{
int i,noelem,command_error=0;
chandata *list,*pchan;


        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
	command_error = Ezca_pvlist_search(noName,pvName,&list);

	for (i=0;i<noName;i++) {
                pchan = chanlist[i];
		pchan->type = ca_field_type(pchan->chid);
		noelem = ca_element_count(pchan->chid);
                if (pchan->state != cs_conn) {
			command_error = CA_FAIL;
			*(type+i) = CA_FAIL;
			*(count+i) = CA_FAIL;
			}
                else {
			*(type+i) = pchan->type;
			*(count+i) = noelem;
			}
	}
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else {
        fprintf(stderr,"Ezca_getTypeCount: failed to alloc chanlist\n");
        return(CA_FAIL);
        }

}

/*************************************************
 *  get waveform array back for a device array using ca_get_array 
the value array should hold the space big enough for storing
return data
 *************************************************/
/* there is problem with return as string when there are
 * more than one element for a given array, the callback 
 * Ezca_getArrayEvent will work correctly
 */
int epicsShareAPI Ezca_getArray(noName,pvName,type,nodata,value)
int noName,type,nodata;
char **pvName;
void *value;
{
int i,status,rtype=0,command_error=0;
unsigned long offset=0,count;
chandata *list, *pchan;
char   *tempbuff;
void   **temp;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {

        command_error = Ezca_pvlist_search(noName,pvName,&list);

switch (type)  {
	case DBR_DOUBLE:
		offset = nodata * dbr_value_size[DBR_DOUBLE];
		rtype = DBR_DOUBLE;
		break;
	case DBR_FLOAT:
		offset = nodata * dbr_value_size[DBR_FLOAT];
		rtype = DBR_FLOAT;
		break;
	case DBR_INT:
		offset = nodata * dbr_value_size[DBR_INT];
		rtype = DBR_INT;
		break;
	case DBR_ENUM:
		offset = nodata * dbr_value_size[DBR_ENUM];
		rtype = DBR_ENUM;
		break;
	case DBR_LONG:
		offset = nodata * dbr_value_size[DBR_LONG];
		rtype = DBR_LONG;
		break;
	case DBR_CHAR:
		offset = nodata * dbr_value_size[DBR_CHAR];
		rtype = DBR_CHAR;
		break;
	case DBR_STRING:
		offset = nodata * MAX_STRING_SIZE;
		rtype = DBR_STRING;
		break;
	}

	tempbuff = (char *)calloc(offset,noName); 
	temp = (void **)calloc(noName,sizeof(void *));
	if (tempbuff == NULL || temp == NULL) {
		fprintf(stderr,"Error: Ezca_getArray failed to calloc!\n");
		return(CA_ALLOC);
		}

	for (i=0;i<noName;i++) {
		pchan = chanlist[i];

		if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {
			ca_set_puser(pchan->chid,pchan);
			pchan->error = 0;

	temp[i] = (char *)(tempbuff+i*offset); 
	pchan->pvalue = (char *)(tempbuff+i*offset);

	count = ca_element_count(pchan->chid);
	if ((int)count >= nodata) 
		status = ca_array_get(rtype,nodata,pchan->chid,temp[i]);
	else
		status = ca_array_get(rtype,count,pchan->chid,temp[i]);

		if (status != ECA_NORMAL) {
		    pchan->error = status;
		    SEVCHK(status,"Ezca_getArray:  ca_array_get failed!");
		    }
		}
	}

        status = ca_pend_io(CA.PEND_IOLIST_TIME);
	if (status != ECA_NORMAL) command_error = CA_FAIL;

	memcpy((char *)value,(char *)tempbuff,noName*offset); 
	free(tempbuff);
	free(temp);

	free(chanlist);
	chanlist = NULL;
	return(command_error);
} else {
        fprintf(stderr,"Ezca_getArray: failed to alloc chanlist\n");
        return(CA_FAIL);
        }

}



/**************************************************
 *  get waveform array back for a device array using callback
the value array should hold the space big enough for storing
return data
 *************************************************/
int epicsShareAPI Ezca_getArrayEvent(noName,pvName,type,nodata,value)
int noName,type,nodata;
char **pvName;
void *value;
{
int i,ii=0,imax,status,rtype=0,command_error=0;
unsigned long offset=0,count;
chandata *list, *pchan;
char   *tempbuff;
struct dbrtemp { chandata *pchan;
        int rtype;
        int rcount;
        } temparg;


        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);
	
switch (type)  {
	case DBR_DOUBLE:
		offset = nodata * dbr_value_size[DBR_TIME_DOUBLE];
		rtype = DBR_TIME_DOUBLE;
		break;
	case DBR_FLOAT:
		offset = nodata * dbr_value_size[DBR_TIME_FLOAT];
		rtype = DBR_TIME_FLOAT;
		break;
	case DBR_INT:
		offset = nodata * dbr_value_size[DBR_TIME_INT];
		rtype = DBR_TIME_INT;
		break;
	case DBR_ENUM:
		offset = nodata * dbr_value_size[DBR_TIME_ENUM];
		rtype = DBR_TIME_ENUM;
		break;
	case DBR_LONG:
		offset = nodata * dbr_value_size[DBR_TIME_LONG];
		rtype = DBR_TIME_LONG;
		break;
	case DBR_CHAR:
		offset = nodata * dbr_value_size[DBR_TIME_CHAR];
		rtype = DBR_TIME_CHAR;
		break;
	case DBR_STRING:
		offset = nodata * MAX_STRING_SIZE;
		rtype = DBR_TIME_STRING;
		break;
	}

	tempbuff = (char *)calloc(offset,noName); 
	if (tempbuff == NULL)  {
		fprintf(stderr,"Error: Ezca_getArrayEvent failed to calloc !\n");
		return(CA_ALLOC);
		}

 
        for (i=0;i<noName;i++) {
                pchan = chanlist[i];
		if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {
			ca_set_puser(pchan->chid,pchan);
			pchan->error = CA_WAIT;

	pchan->pvalue = (char *)(tempbuff+i*offset);
	temparg.pchan = pchan;
	temparg.rtype = rtype;
	temparg.rcount = nodata;
	count = ca_element_count(pchan->chid);
	if (nodata > (int)count) temparg.rcount = count;

	if ((int)count >= nodata) 
		status = ca_array_get_callback(rtype,nodata,pchan->chid,
			Ezca_getTimeCallback, &temparg);
	else
		status = ca_array_get_callback(rtype,count,pchan->chid,
			Ezca_getTimeCallback, &temparg);

		if (status != ECA_NORMAL) {
		    pchan->error = status;
		    SEVCHK(status,"Ezca_getArrayEvent:  ca_array_get_callback failed!");
		    }
		}
       }

        imax = (int)(CA.PEND_IO_TIME/CA.PEND_EVENT_TIME);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];
                if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {

                        /* wait for ca_get_callback to finish */

                        while (pchan->error == CA_WAIT) {
                                ii++;
                                if (ii > imax) {
                	fprintf(stderr,"Ezca_getArrayEvent timeout on : %s\n",
				ca_name(pchan->chid));
				command_error = CA_FAIL;
					break;
					}
                                ca_pend_event(CA.PEND_EVENT_TIME);
			}

		}
	}

	memcpy((char *)value,(char *)tempbuff,noName*offset); 
	free(tempbuff);
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else {
	fprintf(stderr,"Error: Ezca_getArrayEvent failed to calloc chanlist\n");
	return(CA_FAIL);
	} 
}


/**************************************************
 *    put array to IOC 
 *************************************************/
int epicsShareAPI Ezca_putArray(noName,pvName,type,nodata,value)
int noName,type,nodata;
char **pvName;
void *value;
{
int i,status,command_error=0;
unsigned long offset=0,count;
chandata *list, *pchan;
void   **temp;
/* char *tempbuff; */

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);

switch (type)  {
	case DBR_DOUBLE:
		offset = nodata * dbr_value_size[DBR_DOUBLE];
		break;
	case DBR_FLOAT:
		offset = nodata * dbr_value_size[DBR_FLOAT];
		break;
	case DBR_INT:
		offset = nodata * dbr_value_size[DBR_INT];
		break;
	case DBR_ENUM:
		offset = nodata * dbr_value_size[DBR_ENUM];
		break;
	case DBR_LONG:
		offset = nodata * dbr_value_size[DBR_LONG];
		break;
	case DBR_CHAR:
		offset = nodata * dbr_value_size[DBR_CHAR];
		break;
	case DBR_STRING:
		offset = nodata * MAX_STRING_SIZE;
		break;
	}

	temp = (void **)calloc(noName,sizeof(void *));
	if (temp == NULL)  {
		fprintf(stderr,"Error: Ezca_putArray failed to calloc !\n");
		return(CA_ALLOC);
		}
	
        for (i=0;i<noName;i++) {
                pchan = chanlist[i];


#ifdef ACCESS_SECURITY
	if (ca_write_access(pchan->chid) == 0) {
	fprintf(stderr,"Write access denied on : %s\n",ca_name(pchan->chid));
	command_error = CA_FAIL;
		}
#endif
 
	if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {
	ca_set_puser(pchan->chid,pchan);
	pchan->error = 0;

	temp[i] = (char *)value+i*offset; 
        count = ca_element_count(pchan->chid);
        if ((int)count > nodata) count=nodata;

	if (type == DBR_STRING) strncpy(pchan->string,temp[i],MAX_STRING_SIZE);
	
 	status = ca_array_put(type,count,pchan->chid,temp[i]); 

	if (status != ECA_NORMAL) {
		if (CA.devprflag > 0) fprintf(stderr,"Ezca_putArray:  ca_array_put failed on %s!\n", ca_name(pchan->chid));
	    	pchan->error = status;
		    }
        }
       }

        ca_pend_event(CA.PEND_EVENT_TIME);
        status = ca_pend_io(CA.PEND_IOLIST_TIME);
	if (status != ECA_NORMAL) command_error = CA_FAIL;

	free(temp);
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else  {
	fprintf(stderr,"Error: Ezca_putArray failed to calloc chanlist\n");
	return(CA_FAIL);
	}
}



/**************************************************
 *    put array to IOC  using callback
 *************************************************/
int epicsShareAPI Ezca_putArrayEvent(noName,pvName,type,nodata,value)
int noName,type,nodata;
char **pvName;
void *value;
{
int i,status,command_error=0,imax,ii=0;
unsigned long offset=0,count;
chandata *list, *pchan;
void   **temp;
struct dbrtemp { chandata *pchan;
        int rtype;
        int rcount;
        } temparg;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {

        command_error = Ezca_pvlist_search(noName,pvName,&list);

switch (type)  {
	case DBR_DOUBLE:
		offset = nodata * dbr_value_size[DBR_DOUBLE];
		break;
	case DBR_FLOAT:
		offset = nodata * dbr_value_size[DBR_FLOAT];
		break;
	case DBR_INT:
		offset = nodata * dbr_value_size[DBR_INT];
		break;
	case DBR_ENUM:
		offset = nodata * dbr_value_size[DBR_ENUM];
		break;
	case DBR_LONG:
		offset = nodata * dbr_value_size[DBR_LONG];
		break;
	case DBR_CHAR:
		offset = nodata * dbr_value_size[DBR_CHAR];
		break;
	case DBR_STRING:
		offset = nodata * MAX_STRING_SIZE;
		break;
	}

	temp = (void **)calloc(noName,sizeof(void *));
	if (temp == NULL)  {
		fprintf(stderr,"Error: Ezca_putArrayEvent failed to calloc !\n");
		return(CA_ALLOC);
		}

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];

#ifdef ACCESS_SECURITY
	if (ca_write_access(pchan->chid) == 0) {
	fprintf(stderr,"Write access denied on : %s\n",ca_name(pchan->chid));
	command_error = CA_FAIL;
		}
#endif
 
	if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {
	ca_set_puser(pchan->chid,pchan);
	pchan->error = CA_FAIL;

	temp[i] = (char *)value+i*offset; 
        count = ca_element_count(pchan->chid);
        if ((int)count > nodata) count=nodata;


	pchan->pvalue = (char *)value+i*offset;
temparg.rtype = type;
temparg.rcount = count;
temparg.pchan = pchan;

        status = ca_array_put_callback(type,count,pchan->chid,temp[i],
			Ezca_putTimeCallback,&temparg);

	if (status != ECA_NORMAL) {
		if (CA.devprflag > 0) fprintf(stderr,"Ezca_putArrayEvent:  ca_array_put_callback failed on %s!\n", ca_name(pchan->chid));
		pchan->error = status;
		    }
        }
       }

        ca_pend_event(CA.PEND_EVENT_TIME);

        imax = (int)(CA.PEND_IOLIST_TIME/CA.PEND_EVENT_TIME);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];

                if (pchan->state == cs_conn && pchan->type != TYPENOTCONN) {

                        /* wait for put callback to finish */

                        while (pchan->error != CA_SUCCESS) {
                                ii++;
                                if (ii > imax) {
                        fprintf(stderr,"ca_array_put_callback timeout on : %s\n",
                                ca_name(pchan->chid));
				command_error = CA_FAIL;
                                        break;
                                        }
                                ca_pend_event(CA.PEND_EVENT_TIME);
                        }
                }
        }

	free(temp);
        free(chanlist);
        chanlist = NULL;
	return(command_error);
} else {
	fprintf(stderr,"Error: Ezca_putArrayEvent failed to calloc chanlist\n");
	return(CA_FAIL);
	}
}


/******************************************************
  time stamp in seconds get from IOC 
******************************************************/
double epicsShareAPI Ezca_iocClockTime(stamp)
TS_STAMP *stamp;
{
char nowText[33];
double time;

	if ((stamp->nsec + stamp->secPastEpoch) == 0 ) 
		tsLocalTime(stamp);
		
	time = 0.001 * (stamp->nsec / 1000000) + stamp->secPastEpoch;

	if (CA.devprflag > 0) {
		fprintf(stderr,"tsStampToText:%s",
			tsStampToText(stamp,TS_TEXT_MONDDYYYY,nowText));
		fprintf(stderr," IOC time=%.3f sec\n", time);
		}
	
	return time;
}


/******************************************************
  call dbr_time_callback instead of ca_get function
******************************************************/
void Ezca_getTimeCallback(args)
struct event_handler_args args;
{
/* char string[MAX_STRING_SIZE]; */
char *pchar;
double *pdouble;
float  *pfloat;
/* int    *pint; */
short  *ps;
long   *plong;
int count;
chandata *pchandata;
struct dbrtemp { chandata *pchan;
	int rtype;
	int rcount;
	} *temparg;

	temparg = (struct dbrtemp *)args.usr;
	pchandata = (chandata *)ca_puser(args.chid);
	count = ca_element_count(pchandata->chid);

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif

	/* get new value */

switch (temparg->rtype) {

case DBR_TIME_DOUBLE:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_double *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_DOUBLE]);
        pchandata->status = ((struct dbr_time_double *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_double *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_double *)args.dbr)->stamp;
	pdouble = (double *)pchandata->pvalue;
        pchandata->value = *pdouble;
	pchandata->error = 0;
	break;

case DBR_TIME_FLOAT:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_float *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_FLOAT]);
        pchandata->status = ((struct dbr_time_float *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_float *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_float *)args.dbr)->stamp;
	pfloat = (float *)pchandata->pvalue;
        pchandata->value = *pfloat;
	pchandata->error = 0;
	break;

case DBR_TIME_INT:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_short *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_INT]);
        pchandata->status = ((struct dbr_time_short *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_short *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_short *)args.dbr)->stamp;
	ps = (short *)pchandata->pvalue;
        pchandata->value = *ps;
	pchandata->error = 0;
	break;
case DBR_TIME_ENUM:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_short *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_ENUM]);
        pchandata->status = ((struct dbr_time_short *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_short *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_short *)args.dbr)->stamp;
	ps = (short *)pchandata->pvalue;
        pchandata->value = *ps;
	pchandata->error = 0;
	break;
case DBR_TIME_LONG:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_long *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_LONG]);
        pchandata->status = ((struct dbr_time_long *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_long *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_long *)args.dbr)->stamp;
	plong = (long *)pchandata->pvalue;
        pchandata->value = *plong;
	pchandata->error = 0;
	break;
case DBR_TIME_CHAR:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_char *)args.dbr)->value,
		temparg->rcount*dbr_value_size[DBR_TIME_CHAR]);
        pchandata->status = ((struct dbr_time_char *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_char *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_char *)args.dbr)->stamp;
	pchar = (char *)pchandata->pvalue;
        pchandata->value = *pchar;
	pchandata->error = 0;
	break;
case DBR_TIME_STRING:
	memcpy((char *)pchandata->pvalue,
		(char *)&((struct dbr_time_string *)args.dbr)->value,
		temparg->rcount*MAX_STRING_SIZE);
        pchandata->status = ((struct dbr_time_string *)args.dbr)->status;
        pchandata->severity = ((struct dbr_time_string *)args.dbr)->severity;
	pchandata->stamp = ((struct dbr_time_string *)args.dbr)->stamp;
	strncpy(pchandata->string,pchandata->pvalue,MAX_STRING_SIZE);
        pchandata->value = atof(pchandata->string);
	pchandata->error = 0;
	break;
default:
	fprintf(stderr,"NOT supported : %d\n",temparg->rtype);
	break;

}

#ifdef ACCESS_SECURITY
	}
	else 
	fprintf(stderr,"Diagnostic: Read access denied : %s\n",
                ca_name(pchandata->chid));
#endif


}


/******************************************************
  callback instead of ca_put function
******************************************************/
void Ezca_putTimeCallback(args)
struct event_handler_args args;
{
chandata *pchandata;
struct dbrtemp { chandata *pchan;
        int rtype;
        int rcount;
        } *temparg;

        temparg = (struct dbrtemp *)args.usr;

	pchandata = (chandata *)ca_puser(args.chid);

#ifdef ACCESS_SECURITY
	if (args.status == ECA_NORMAL) {
#endif

	if (CA.devprflag > 0) fprintf(stderr,"Ezca_putTimeCallback: type=%d, count=%d\n",
		temparg->rtype,temparg->rcount);
        pchandata->error = 0;

#ifdef ACCESS_SECURITY
	}
	else 
	fprintf(stderr,"Diagnostic: write access denied : %s\n",
                ca_name(pchandata->chid));
#endif

}



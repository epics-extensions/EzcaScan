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
 */

#ifdef _WIN32
#include "windows.h"
#endif

#include <dbDefs.h>
#include <cadef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <epicsTime.h>
#ifdef EZCA
    #include <ezca.h>
#endif

#define epicsExportSharedSymbols
#include "EzcaScan.h"

double atof();

/******************************************************
  time stamp in seconds get from IOC 
******************************************************/
double epicsShareAPI Ezca_iocClockTime(stamp)
TS_STAMP *stamp;
{
char nowText[33];
double time;

	if ((stamp->nsec + stamp->secPastEpoch) == 0 ) 
		epicsTimeGetCurrent(stampTS);
		
	time = 0.001 * (stamp->nsec / 1000000) + stamp->secPastEpoch;

	if (CA.devprflag > 0) {
		epicsTimeToStrftime(nowText,"%b %d, %Y %H:%M:%S.%09f",stamp)
		fprintf(stderr,"epicsTimeToStrftime:%s", nowText);
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
char string[MAX_STRING_SIZE];
char *pchar;
double *pdouble;
float  *pfloat;
int    *pint;
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



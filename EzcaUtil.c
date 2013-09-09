/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/

/* EzcaUtil.c
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
#include <string.h>
#include <stdlib.h>
#ifdef EZCA
    #include <ezca.h>
#endif

#include "epicsVersion.h"

#define epicsExportSharedSymbols
#include "EzcaScan.h"

int epicsShareAPI Ezca_setPendTime(int flag, float *rtime)
{
    float dd;

    dd = *rtime;
    if (flag == 1) {
	    if (dd > 0.) CA.PEND_IO_TIME = dd;
	    if ( CA.devprflag > 0) {
		    fprintf(stderr,"Ezca_setPendTime: \n");
        	fprintf(stderr," CA.PEND_IO_TIME = %f\n",
				    CA.PEND_IO_TIME);
	    }
    }
    if (flag == 2) {
	    if (dd > 0.) CA.PEND_IOLIST_TIME = dd;
	    if ( CA.devprflag > 0) {
		    fprintf(stderr,"Ezca_setPendTime: \n");
        	fprintf(stderr," CA.PEND_IOLIST_TIME = %f\n",
				    CA.PEND_IOLIST_TIME);
	    }
    }
    if (flag == 3) {
	    if (dd > 0.) CA.PEND_EVENT_TIME = dd;
	    if ( CA.devprflag > 0) {
		    fprintf(stderr,"Ezca_setPendTime: \n");
        	fprintf(stderr," CA.PEND_EVENT_TIME = %f\n",
				    CA.PEND_EVENT_TIME);
	    }
    }
	return(CA_SUCCESS);
}

int epicsShareAPI Ezca_init(int flag)
{
    if (flag >= 0) {
            CA.PEND_IO_TIME = 1.;
            CA.PEND_IOLIST_TIME = 3.;
            CA.PEND_EVENT_ON = 1;
            CA.PEND_EVENT_TIME = 0.001f;
    } else {
            CA.PEND_IO_TIME = 5.;
            CA.PEND_IOLIST_TIME = 10.;
            CA.PEND_EVENT_ON = 0;
            CA.PEND_EVENT_TIME = 0.001f;
    }
    if ( CA.devprflag > 0) {fprintf(stderr,"Ezca_init: \n");
            fprintf(stderr," CA.PEND_IO_TIME = %f\n",CA.PEND_IO_TIME);
            fprintf(stderr," CA.PEND_IOLIST_TIME = %f\n",CA.PEND_IOLIST_TIME);
            fprintf(stderr," CA.PEND_EVENT_TIME = %f\n",CA.PEND_EVENT_TIME);
            fprintf(stderr," CA.PEND_EVENT_ON = %d\n",CA.PEND_EVENT_ON);
    }
    return (CA_SUCCESS);

}

int epicsShareAPI Ezca_debug(int flag)
{
    CA.devprflag = flag;

	if ( CA.devprflag > 0) 
	   fprintf(stderr,"Ezca_debug: level %d\n",CA.devprflag);
    return (CA_SUCCESS);
}

int epicsShareAPI Ezca_version(char *str)
{
	sprintf(str," (ezca) (EzcaScan%d.0) (%s)", CA.version,
                EPICS_VERSION_STRING);
    return (CA_SUCCESS);

}

int epicsShareAPI Ezca_timeStamp(char *name, char *str)
{
    TS_STAMP stamp;
    char nowText[33];
    int command_error=0;
    //extern chandata *pchandata;
    chandata *pchandata;

	command_error = Ezca_find_dev(name,&pchandata);
    if (pchandata->type == TYPENOTCONN) return(command_error);

	if (!pchandata->evid ) {
		/* temporarily add the the monitor to get timestamp */
		command_error = Ezca_monitorArrayAdd(1,&name);
		if (command_error == 0) {
            stamp = pchandata->stamp;
    		epicsTimeToStrftime(nowText,32,"%b %d, %Y %H:%M:%S.%09f",&stamp);
    		strcpy(str,nowText);
    		command_error = Ezca_monitorArrayClear(1,&name);
		}
	} else if (pchandata->evid) {
            stamp = pchandata->stamp;
    		epicsTimeToStrftime(nowText,32,"%b %d, %Y %H:%M:%S.%09f",&stamp);
    		strcpy(str,nowText);
    }
    return (command_error);
}

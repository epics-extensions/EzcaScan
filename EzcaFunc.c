/* EzcaFunc.c
 *
 *      Original Author: Ben-chin Cha
 *      Date:            11-21-95
 *
 * .01  06-13-96 bkc     Add the support of channel list to generate the true
 *                       list to take care the problem produced in the next
 *                       chandata in the link list approach
 * .02  09-02-98 MLR     Changed include from Ezca.h to EzcaScan.h.  Added
 *                       sleep for WIN32
 * .02  10-05-98 bkc     Cleanup the unreferenced local variable and add 
 *                       function prototype to get rid of make warning message 
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
#include <shareLib.h>

#define DB_TEXT_GLBLSOURCE
#include "EzcaScan.h"

extern int usleep();

chandata **chanlist;


/****************************************************
   sleep for micro seconds
*****************************************************/
void epicsShareAPI Ezca_sleep_time(t)
double t;    /* wait in seconds can be decimal*/
{
/* usleep isn't ANSI-C/POSIX
*/
unsigned u;
#if defined(HP_UX)
	sleep((unsigned int)t);
#elif defined(VMS)
	LIB$WAIT(t);
#elif defined(SGI)
	sleep((unsigned int)t);
#elif defined(SOLARIS)
	usleep((unsigned int)t);
#elif defined(_WIN32)
        u = 1000 * t;
	Sleep(u);
#else
	u = 1000000 * t;
	usleep(u);
#endif
}



/**************************************************
 * find the channel name from IOC 
 **************************************************/
int epicsShareAPI Ezca_find_dev(name,pchandata)
char *name;
chandata *pchandata;
{
int status,command_error;

/* populate hash table here return the address */

	if (name) pchandata = (chandata *)Ezca_check_hash_table(name);
	else pchandata = (chandata *)Ezca_check_hash_table(" ");

  if (pchandata->type != TYPENOTCONN) return (CA_SUCCESS);

  	status = ca_pend_io(CA.PEND_IO_TIME);

        pchandata->type = ca_field_type(pchandata->chid);
	pchandata->state = ca_state(pchandata->chid);


        if (pchandata->state != cs_conn || pchandata->type == TYPENOTCONN) {
	if (CA.devprflag > 0) 
		fprintf(stderr,"%s --- Invalid channel name\n",ca_name(pchandata->chid));
		command_error = CA_FAIL;
		} else  command_error = CA_SUCCESS;
 
	if (CA.devprflag > 0) fprintf(stderr,"chid=%x, type=%d, state=%d\n",
	pchandata->chid,pchandata->type,pchandata->state);

	return (command_error);
}


/**************************************************
 *  casearch for a device array
 *  time out return CA_FAIL else return 0
 *************************************************/
int epicsShareAPI Ezca_search_list(noName,pvName,list)
int noName;
char **pvName;
chandata *list;
{
int command_error=0;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
        command_error = Ezca_pvlist_search(noName,pvName,&list);
        free(chanlist);
        chanlist = NULL;
        return(command_error);
} else {
        fprintf(stderr,"Ezca_monitorArrayAdd: failed to alloc\n");
        return(CA_FAIL);
        }

}


/**************************************************
 *  casearch for a device array
 *  time out return CA_FAIL else return 0
 *************************************************/
int epicsShareAPI Ezca_pvlist_search(noName,pvName,list)
int noName;
char **pvName;
chandata **list;
{
int i,status,command_error=CA_SUCCESS;
chandata *pnow,*phead,*pchan;

	phead = (chandata *)list;
	pnow = phead;
	for (i=0;i<noName;i++) {
	if (pvName[i])
		pchan = (chandata *)Ezca_check_hash_table(pvName[i]);
	else pchan=(chandata *)Ezca_check_hash_table(" ");

		pnow->next = pchan;
		pchan->next = NULL;
		pnow = pchan;

	if (chanlist != NULL) chanlist[i] = (chandata *)pchan;
	}

        status = ca_pend_io(CA.PEND_IOLIST_TIME);
	if (status != ECA_NORMAL) command_error= CA_FAIL;

if (chanlist != NULL) {
	for (i=0;i<noName;i++) {
		pchan = chanlist[i];
		pchan->type = ca_field_type(pchan->chid);
		pchan->state = ca_state(pchan->chid);

		if (pchan->state != cs_conn) {
			if (CA.devprflag > 0)
			fprintf(stderr,"%-30s  ***  channel not found\n",pvName[i]);
			pchan->error = CA_WAIT;
			command_error = CA_FAIL;
			}  else   pchan->error = 0;
	}
} else {
	pnow = phead->next; i=0;
	while (pnow)  {
		pchan = pnow;
		pchan->type = ca_field_type(pchan->chid);
		pchan->state = ca_state(pchan->chid);

		if (pchan->state != cs_conn) {
			if (CA.devprflag > 0)
			fprintf(stderr,"%-30s  ***  channel not found\n",pvName[i]);
			pchan->error = CA_WAIT;
			command_error = CA_FAIL;
			}  else   pchan->error = 0;
		pnow = pnow->next; i++;
		}
}
		return(command_error);
}

/**************************************************
 *  get error code  for a device array
 *************************************************/
int epicsShareAPI Ezca_get_error_array(noName,pvName,value)
int noName;
char **pvName;
int  *value;
{
int i,command_error=0;
chandata *list,*pchan;

        chanlist = (chandata **)calloc(noName,sizeof(chandata *));

if (chanlist != NULL) {
	command_error = Ezca_pvlist_search(noName,pvName,&list);

        for (i=0;i<noName;i++) {
                pchan = chanlist[i];
                if (pchan->state != cs_conn) {
			command_error = CA_FAIL;
			*(value+i) = CA_FAIL;
			}
                else {
			if (pchan->error != 0) command_error = CA_FAIL;
			*(value+i) = pchan->error;  /* timeout if is -2 */
			}
	}

        free(chanlist);
        chanlist = NULL;
        return(command_error);
} else {
        fprintf(stderr,"Ezca_get_error_array: failed to alloc chanlist\n");
        return(CA_FAIL);
        }
}

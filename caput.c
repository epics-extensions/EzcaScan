/*
 * Modification Log:
 * -----------------
 * .01  02-11-99        bkc     Replace old caput by this new version
 * .02  mm-dd-yy        iii     Comment
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

#include <shareLib.h>

#define DB_TEXT_GLBLSOURCE
#include "EzcaScan.h"

extern chandata **chanlist,*pchandata;
extern double atof();
int getopt();
int print_caget(void);
int no_elements(char *);
int data_array(char *,char **);

extern char *optarg;
extern int optind;
int TERSE=0,ENUM_V=1,FORMAT=0,VECTOR=0;

char *pvName,f_fmt[15],e_fmt[15];
int noName,req_no=1,rtype,type[1],count[1];
void *value;
char *buff,*string_value;
unsigned long offset;

main(argc,argv)
int argc;
char **argv;
{
int c,found,ret,i,j=0,len,first=1;
char **pv,**dataArray;
short *sv;
char *cv;
int *iv;
float *fv;
double *dv;
char *tempValue,*tempName,**pvNames;
int noName,noData;

CA.PEND_IO_TIME = 1.;

if (argc < 3) {
	printf("\nUsage:  caput [-t] [-s] [-w sec] pv_name  pv_value\n\n");
        printf("        -t   Terse mode, only the successfully put value is returned\n\n");
	printf("        -s   pv_value entered as database defined enumeral string\n\n");
        printf("    -w sec   Wait time, specifies bigger time out, default is 1 second\n\n");
        printf("  	-m   if this option specified the waveform values entered \n");
	printf("             must be a single string of multiple values been comma\n");
	printf("             separated and no blank space is allowed \n\n");
        printf("   pv_name   requested database process variable name\n\n");
        printf("  pv_value   new value to put to IOC\n\n");

	exit(1);
	}

while ((c = getopt(argc,argv,"tsmw:")) != -1) 
        switch (c) {
        case 't':
                TERSE = 1;
		break;
        case 's':
                ENUM_V = 0;
		break;
        case 'm':
                VECTOR = 1;
		break;
        case 'w':
                CA.PEND_IO_TIME = atof(optarg);
                if (CA.PEND_IO_TIME < 1.) CA.PEND_IO_TIME = 1.;
                break;
        }

/*
if (argv[optind] == NULL || strlen(argv[optind]) > NAME_LENGTH) {
	printf(" %s is an invalid channel name\n", argv[optind]);
	return(1);     
	}
*/
	first = optind;

	ca_task_initialize();

	noName = 1;
	pvName = argv[first];

/* check for pv list */
	noName = no_elements(argv[first]);

	len = strlen(argv[first]);
	tempName = (char *)calloc(1,len+1);
	strcpy(tempName,argv[first]);

	if (noName > 1 ) {
		pvNames = (char **)calloc(noName,sizeof(char *));
		noName = data_array(tempName,pvNames);

	noData = no_elements(argv[first+1]);

	if (noData > 1 ) {
	dataArray = (char **)calloc(noData,sizeof(char *));
	if (VECTOR==1 || noData == noName) {
	len = strlen(argv[first+1]);
	tempValue = (char *)calloc(1,len+1);
	strcpy(tempValue,argv[first+1]);

		if (argc == first+2) 
		req_no = data_array(tempValue,dataArray);
		else { printf("Syntax error in data elements!! \n");
			return(-1); }
	} else {
		 printf("Error: Inconsistant data elements entered!! \n");
			return(-1); 
	}

	found = Ezca_getTypeCount(noName,pvNames,type,count);

	buff = (char *)calloc(noData,MAX_STRING_SIZE);
	value = (void *)buff;
	pv = (char **)calloc(noName,sizeof(char *));
	for (i=0;i<req_no;i++) pv[i] = buff+i*MAX_STRING_SIZE;

	ret = Ezca_getArray(noName,pvNames,DBR_STRING,1,buff);
	for (i=0;i<req_no;i++) printf("Old : %-30s %s\n",pvNames[i],pv[i]);

	for (i=0;i<req_no;i++) 
		strcpy(buff+i*MAX_STRING_SIZE, dataArray[i]);

	ret = Ezca_putArray(noName,pvNames,DBR_STRING,1,buff);

	ret = Ezca_getArray(noName,pvNames,DBR_STRING,1,buff);
	for (i=0;i<req_no;i++) printf("New : %-30s %s\n",pvNames[i],pv[i]);

	free(pv);
	goto end_task;

	} else {
		 printf("Error: Inconsistant data elements entered!! \n");
			return(-1); 
		}
	}

	found = Ezca_getTypeCount(noName,&pvName,type,count);

	if (found != 0) {
		printf(" %s --- Invalid channel name\n",pvName);
		return(-1);
	} 
	
	rtype = type[0];
	req_no = argc-first-1;
	if (req_no > count[0]) req_no = count[0];

	offset = count[0]*dbr_value_size[rtype];
	buff = (char *)calloc(count[0],MAX_STRING_SIZE);
	value = (void *)buff;

	ret = Ezca_getArray(noName,&pvName,rtype,count[0],value);


/* get old value */


/* put value to device default as string except DBR_ENUM */

	dataArray = (char **)calloc(count[0],sizeof(char *));
	if (VECTOR==1) {
		if (argc == first+2) 
		req_no = data_array(argv[first+1],dataArray);
		else { printf("Syntax error in data elements!! \n");
			return(-1); }
	} else {
		for (i=0;i<req_no;i++)
		dataArray[i] = argv[i+first+1];
	}

	if (!TERSE)
	if (req_no > 1) printf("Old : %s %d ",pvName,req_no);
		else printf("Old : %-30s ",pvName);
	print_caget();

        switch(rtype) {
	case DBR_INT: 
		sv = (short *)value;
		for (i=0;i<req_no;i++) 
			sv[i] = atoi(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,sv);
		break;
	case DBR_CHAR: 
		cv = (char *)value;
		for (i=0;i<req_no;i++) 
			cv[i] = atoi(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,cv);
		break;
	case DBR_ENUM: 
		sv = (short *)value;
		if (ENUM_V == 0) {
			strcpy(string_value,dataArray[0]);
			if (req_no > 1 ) {
			for (i=1;i<req_no;i++) {
				strcat(string_value," ");
				strcat(string_value,dataArray[i]);
				}
			ret = Ezca_putArray(noName,&pvName,DBR_STRING,req_no,string_value);
			}
		} else {
		for (i=0;i<req_no;i++) 
			sv[i] = atoi(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,sv);
		}
		break;
	case DBR_FLOAT: 
		fv = (float *)value;
		for (i=0;i<req_no;i++) 
			fv[i] = (float)atof(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,fv);
		break;
	case DBR_DOUBLE: 
		dv = (double *)value;
		for (i=0;i<req_no;i++) 
			dv[i] = atof(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,dv);
		break;
	case DBR_STRING: 
		for (i=0;i<req_no;i++) {
			strcpy(buff+i*MAX_STRING_SIZE,dataArray[i]);
		}
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,value);
		break;
	default:
	case DBR_LONG: 
		iv = (int *)value;
		for (i=0;i<req_no;i++) 
			iv[i] = atoi(dataArray[i]);
		ret = Ezca_putArray(noName,&pvName,rtype,req_no,iv);
		break;
	}


/*get new value */

	ret = Ezca_getArray(noName,&pvName,rtype,count[0],value);

	if (!TERSE)
	if (req_no > 1) printf("New : %s %d ",pvName,req_no);
		else printf("New : %-30s ",pvName);
	print_caget();

end_task:
	ca_task_exit();

	free(tempName);
	free(pvNames);
	free(dataArray);
	free(tempValue);
	free(buff);
	return(0);
}


int print_caget(void)
{
int i,ret;
char **pv;
short *sv;
char *cv;
int *iv;
float *fv;
double *dv;

		switch (rtype) {
		case DBR_INT:  /* DBR_SHORT */
			sv = (short *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,(float)sv[i]);
			else if (FORMAT==2) printf(e_fmt,(float)sv[i]);
			else 	
			 	printf("%d ",sv[i]);
			}
		     break;
		case DBR_CHAR:  /* DBR_UCHAR */
			cv = (char *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,(float)cv[i]);
			else if (FORMAT==2) printf(e_fmt,(float)cv[i]);
			else 	
			 	printf("%d ",cv[i]);
			}
		     break;
		case DBR_ENUM: 
			sv = (short *)value;
			if (count[0] > 1) {
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,(float)sv[i]);
			else if (FORMAT==2) printf(e_fmt,(float)sv[i]);
			else 	
				printf("%d ",sv[i]);
				}
			} else {
			offset = MAX_STRING_SIZE;
			string_value = (char *)calloc(offset,noName);
			ret = Ezca_getArray(noName,&pvName,DBR_STRING,1,string_value);

			if (ENUM_V == 1) printf("%d ",sv[0]);
			else if (strlen(string_value) > 0 ) 
				printf("%s ",string_value);
			else	printf("%d ",sv[0]);
			free(string_value);
			}
		     break;
		case DBR_FLOAT:
			fv = (float *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,fv[i]);
			else if (FORMAT==2) printf(e_fmt,fv[i]);
			else 	printf("%f ",fv[i]);
			}
		     break;
		case DBR_DOUBLE:
			dv = (double *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,dv[i]);
			else if (FORMAT==2) printf(e_fmt,dv[i]);
			else 	printf("%f ",dv[i]);
			}
		     break;
		case DBR_STRING:
		pv = (char **)calloc(req_no,sizeof(char *));
		for (i=0;i<req_no;i++) pv[i]=buff+i*MAX_STRING_SIZE;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,atof(pv[i]));
			else if (FORMAT==2) printf(e_fmt,atof(pv[i]));
			else 	
			 	printf("%s ",pv[i]);
			}
			free(pv);
		     break;
		
		default:
		case DBR_LONG: 
			iv = (int *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,(float)iv[i]);
			else if (FORMAT==2) printf(e_fmt,(float)iv[i]);
			else 	
			 	printf("%d ",iv[i]);
			}
		     break;
		}

	printf("\n");
	return(0);
}

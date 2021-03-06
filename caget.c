/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/*
	$Id$
 * Modification Log:
 * -----------------
 * .03  09-24-2002      bkc     Remove check pv NAME_LENGTH restriction
 * .04  04-28-2003      bkc     Fix the problem with -w time out second 
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

#include "EzcaScan.h"
#include "epicsGetopt.h"

extern chandata **chanlist,*pchandata;

#if !defined(_WIN32) && !defined(LINUX) && !defined(SOLARIS)
extern double atof();
#endif
int no_elements(char *);
int data_array(char *,char **);

extern char *optarg;
extern int optind;
int TERSE=0,ENUM=0,FORMAT=0,WAVEFORM=0;

int main(int argc,char **argv)
{
char **Names,*pvName,*string_value;
char *buff;
void *value;
char f_fmt[15],e_fmt[15];
int c,rtype,len;
int req_no=0,sp_no=0,noName=1,found=-1,type[1],count[1],i,ret;
int j,nNums,Nums;
unsigned long offset;

char **pv;
short *sv;
dbr_char_t *cv;
int *iv;
float *fv;
double *dv;
int first=1;

	CA.PEND_IOLIST_TIME = 1.0;
if (argc < 2) {
	printf("\nUsage:  caget [-n] [-t] [-f n] [-e n] [-w sec] pv_name\n\n");
	printf("This command read a value or array of values from one or more IOC channels.\n\n");
	printf("        -n   returns numerical value instead of string for DBR_ENUM field\n");
	printf("	-t   Terse mode, process variable name not returned\n");
	printf("      -f n   Use f format, n specifies number of digits after the decimal point\n");
	printf("      -e n   Use e format, n specifies number of digits after the decimal point\n");
	printf("    -w sec   Wait time, specifies bigger time out, default is 1.0 second\n");
	printf("     -# no   The number 'no' specifies the number of values to be returned for\n");
	printf("             an array record. If this option is not used then the whole array\n");
	printf("             is returned. The first returned number is the size of the array ,\n");
	printf("             then it is followed with the array of values obtained.\n");
	printf("   pv_name   a single or a list of PV names from IOC\n\n");
	printf("   Examples:   caget  pv_name\n");
	printf("               caget  pv_name1 pv_name2 pv_name3\n\n");
	return(1); /*exit(1); */
	}

while ((c = getopt(argc,argv,"ntf:e:w:#:")) != -1) 
	switch (c) {
	case 'n':
		ENUM = 1;
		break;
	case 't':
		TERSE = 1;
		break;
	case 'f':
		FORMAT = 1;
		sprintf(f_fmt,"%c-.%sf ",37,optarg);
		break;
	case 'e':
		FORMAT = 2;
		sprintf(e_fmt,"%c-.%se ",37,optarg);
		break;
	case 'w':
		CA.PEND_IOLIST_TIME = atof(optarg);
		if (CA.PEND_IOLIST_TIME < 1.0) CA.PEND_IOLIST_TIME = 1.0;
		break;
	case '#':
		WAVEFORM = 1;
		sp_no = atoi(optarg);
		break;
	}

	first = optind;

	ca_task_initialize();

	Nums = argc - first;
	Names = (char **)calloc(Nums,sizeof(char *));

	if (Nums > 1 ) {
		for (j=0;j<Nums;j++) Names[j] = argv[j+first];
	} else {
		nNums = no_elements(argv[first]);
		if (nNums > 1) {
			free(Names);
			Names = (char **)calloc(nNums,sizeof(char *));
			len = strlen(argv[first]);
			Nums = data_array(argv[first],Names);
		} else {
			Names[0] = argv[first];
		}
	}


	for (j=0;j<Nums;j++) {
	req_no = sp_no;
	noName = 1;  
	pvName = Names[j];

	if (pvName == NULL) {
	printf(" %s is an invalid channel name\n", pvName);
	} else {

	found = Ezca_getTypeCount(noName,&pvName,type,count);
	
        if (found != 0) {
		 printf(" %s --- Invalid channel name\n",pvName);
		 req_no = -1;
	} else {
	rtype = type[0];
	if (req_no <= 0 || req_no > count[0])  req_no = count[0];
	buff = (char *)calloc(count[0],MAX_STRING_SIZE);


	if (count[0] > 0)   {
		offset = count[0]*dbr_value_size[rtype];
		value = (void *)buff;
		ret = Ezca_getArray(noName,&pvName,rtype,count[0],value);

		if (!TERSE) {
		if (count[0] > 1) printf("%s",pvName);
			else  printf("%-30s",pvName);
		}
		if (count[0] > 1) printf(" %d ",req_no);

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
			cv = (dbr_char_t *)value;
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

			if (ENUM == 1) printf("%d ",sv[0]);
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
			else 	printf("%g ",fv[i]);
			}
		     break;
		case DBR_DOUBLE:
			dv = (double *)value;
			for (i=0;i<req_no;i++) {
			if (FORMAT==1) printf(f_fmt,dv[i]);
			else if (FORMAT==2) printf(e_fmt,dv[i]);
			else 	printf("%g ",dv[i]);
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
		}
	free(buff);
	}
	}
	}
	ca_task_exit();

	free(Names);
	return(0);  /*exit(0); */
}


int no_elements(stringVar)
char *stringVar;
{
int i,j=0,len;
char *s1;
        len = strlen(stringVar);
        s1 = stringVar;

        for (i=0;i<len;i++) {
        if (*(s1+i) == 44) j++;    /* ',' */
        }
        return(j+1);
}


int data_array(stringVar,strarray)
char *stringVar;
char **strarray;
{
int i,j=0,c,len;
char *s1;
	len = strlen(stringVar);
	s1 = stringVar;
	strarray[0] = s1;

	for (i=0;i<len;i++) {
	c = *(s1+i);
	if (*(s1+i) == 44) {  /* ',' = 44,*/ 
		j++;
		strarray[j] = s1+i+1;
		*(s1+i)='\0';
		}
	}

	return(j+1);
}


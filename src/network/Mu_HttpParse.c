/*******************************************************************
* 					====Microunit Techonogy Co,.LTD.====
* File Name:
* 		Mu_HttpParse.c
* Description:
* 		This file contain the method of parse the headre form server return  
*		form the packet, we shall get the cookies, cotentlen, statecode, and so on.
* Revision History:
* 		14-5-2008 ver1.0
*Author:
* 		ssw (fzqing@gmail.com), cks, wkx
* 					***PROTECTED BY COPYRIGHT***
********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <fcntl.h>

#include "../include/Mu_HttpParse.h"

/************************************************
Description:
		get the cookies from the line read from server,
		store the info in pointer cookies
input:
		hdr :buffer pointer
Output;
		MUOK:	no error
		MUNBUF:	no buffer to get
Lock:
		NONE
************************************************/
int Mu_GetCookies(const char *hdr){
	
	int p;
	char *HDP = NULL;
	
	HDP=strstr(hdr, "Set-Cookie:");
	if(NULL==HDP){
		return MUNCOK;
	}
	
	HDP += 11;

	//skip the space
	for(; *hdr == ' ' || *hdr == '\t' ||*hdr == '\r' ||*hdr == '\n'; hdr ++)
		HDP ++;
	
	//store the info
	Mu_Free(cookies);
	if(!(p = Mu_Strlen(HDP))||
		((cookies = malloc(p+1)) == NULL)){
		Mu_ErrorPrint();
		return MUNBUF;
	}

	snprintf(cookies, p+1, "%s", HDP);
	return MUOK;

}

/************************************************
Description:
		get the locations  from the line read from server,
		store the info in pointer cookies
input:
		hdr :buffer pointer
Output;
		MUOK:	no error
		MUNBUF:	no buffer to get
Lock:
		NONE
************************************************/
int Mu_GetRelocation(const char *hdr)
{

	int p;
	char *HDP = NULL;
	
	HDP=strstr(hdr, "Location:");
	if(NULL==HDP){
		return MUNCOK;
	}
	HDP += 9;

	//store the info
	Mu_Free(location);
	if(!(p = Mu_Strlen(HDP)) || (NULL == (location = (char *)malloc(p+1)))){
		Mu_ErrorPrint();
		return MUNBUF;
	}

	snprintf(location, p+1, "%s", HDP); 
	return MUOK;

}

/************************************************
Description:
		get the statcode from the line read from server,
		
input:
		hdr :buffer pointer
Output;
		statcode
Lock:
		NONE
************************************************/
int Mu_GetStatusCode(const char *hdr)
{

	int statuscode;

	if(strncasecmp(hdr, "HTTP/", 5)){
		return MUNHST;
	}
	
	hdr += 8;
	statuscode = atoi(hdr);
#ifdef DEBUG
printf("############the statecode is %d\n ", statuscode);
#endif
	return statuscode;
}

/************************************************
Description:
		get the contentlen from the line read from server,
input:
		hdr :buffer pointer
Output;
		contentlen
Lock:
		NONE
************************************************/
off_t Mu_GetContentLen(const char *hdr)
{

	off_t len;

	if(strncasecmp(hdr,"Content-length:", 15)){
		Mu_ErrorPrint();
		return MUNLEN;
	}
	
	hdr += 15;
	len = atol(hdr);

#ifdef DEBUG
printf("#############the content length is %ld\n", len);
#endif
	return len;
}


/*******************************************************************
Description:
		After get the line from server, we need to call the parse functin to
		get the values, we need.
		including statecode, loction, cookies ,etc.
Input:
		socket:	we will read from this socket
		Ptr:		we will store information in this structure
Output:
		MUOK: no error
		others
Lock:
		NONE
*******************************************************************/
int Mu_ParseHeader(int socket,Mu_HttpStatusPtr Ptr){

	int ret=MUOK;
	int statusecode_exist = 0;
	int contentlen_exist = 0;
	int relocation_exist = 0;
	int cookies_exist = 0;
	char *hdr = NULL;

	//*Mu_HttpStatusPtr hs;
	//hs is very strange,where to define it
	while(1){ 
		//when the return back value is ==0. indicate that 
		//we have read a line successfully from server
		if((ret = Mu_FetchHeader(socket, &hdr)) < 0){
			Mu_ErrorPrint();
			Mu_Free(hdr);
			return ret;
		}

		//if have read the empty line, identicate that
		//we have reach the end of Header
		if(!*hdr){
			Mu_Free(hdr);
			break;
		}

		//get the statecode
		if((!statusecode_exist) && (ret = Mu_GetStatusCode(hdr)) > 0) {
			if(Ptr)
				Ptr->statcode= ret;
			statusecode_exist= 1;
			Mu_Free(hdr);
			continue;
		}

		//get the cotentlen
		if((!contentlen_exist) && (ret = Mu_GetContentLen(hdr)) > 0){
			if(Ptr){				
				Ptr->bytesexcept= ret;
				Ptr->bytesleft = ret;
			}
			contentlen_exist= 1;
			Mu_Free(hdr);
			continue;
		}

		//get the reloctation
		if(!relocation_exist){
			if((ret = Mu_GetRelocation(hdr)) < 0) {
				if(ret == MUNBUF){
					Mu_ErrorPrint();
					return ret;
				}
			}else{
				Mu_Free(hdr);
				relocation_exist= 1;
				continue;
			}
		}

		//get cookies
		if(!cookies_exist){
			if((ret = Mu_GetCookies(hdr)) < 0) {
				if(ret == MUNBUF){
					Mu_ErrorPrint();
					return ret;
				}
			}else{
				cookies_exist= 1;
				Mu_Free(hdr);
				continue;
			}
		}

		//others, we will skip it
		Mu_Free(hdr);
	}
	return MUOK;
}


/*******************************************************************
Description:
		fetch http header from server, we get the contents line by line
		int the function, we need to judge the condition, which is the end of
		line, or the SH
Input:
		socket:	we read from this socket to get line
		**hdr:	buffer, we return back
Output:
		MUOK: no error, we have read a line
		MUNBUF: no buffer
		MUEEOF: have reach the end of the packet
		MUERED: read the pacekt error
Lock:
		NONE
********************************************************************/
int Mu_FetchHeader(int socket, char **hdr)
{
	int i,bufsize;
	int ret = MUOK;
	char next;
	bufsize = MU_MAX_HEADER;
	
	//int count = 0;
	if(NULL == (*hdr = (char *)malloc(bufsize))){
		Mu_ErrorPrint();
		return MUNBUF;
	}
	
	memset(*hdr,0,bufsize);
	for(i = 0; 1; i ++){

		//if the space is not enough, we need to realloc the buffer
		if(i>bufsize-1){
			if(NULL == (*hdr = (char *)realloc(*hdr, bufsize <<=1))){
				Mu_ErrorPrint();
				return MUNBUF;
			}
		}
		
		ret = MuIO.reader(socket, *hdr+i,1);
		//ret = read(socket, *hdr+i, 1);
		if(ret == 1){
			//count ++;
#ifdef DEBUG
printf("%c", *(*hdr+i));
#endif
				if((*hdr)[i] == '\n'){
					
					//when the line is \r\n��that
					//identicate have reache the end of HEADER
					if(!(i==0||(i==1&&(*hdr)[0]=='\r'))){
						
						// we need to check if it
						//continues on to the other line.
						ret = MuIO.peeker(socket, &next, 1);
						//ret = read(socket, next, 1);
						//lseek(socket, count, SEEK_SET);
						if(ret == 0){
							return MUEEOF; //end of the file
						}else if(ret == -1){
							return MUERED;//error happened
						}
						//skip the SP and HT
						if(next == '\t' || next == ' '){
							continue;
						}
					}
					
					(*hdr)[i] = '\0';
					if(i > 0 && (*hdr)[i-1] == '\r'){
						(*hdr)[i-1] = '\0';
					}
					
					break;
				}
		}
		else if(ret == 0){
			return MUEEOF;
		}else{
			return MUERED;
		}
	}
	
	return MUOK;
}



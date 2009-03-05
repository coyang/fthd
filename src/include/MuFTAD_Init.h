#ifndef __MUFTAD_INIT_H__
#define __MUFTAD_INIT_H__
#include "Mu_Struct_main.h"
#include "Mu_Parsexml.h"	//for CHECK_2XX
#include "Mu_Buildxml.h"
#include "Mu_Creatxml.h"
#include "Mu_Util.h"

FddfStatusHeadPtr MuFddfStatHead;
DirStatusHeadPtr MuDirStatHead;
ServiceInfoPtr MuServiceInfo;
FddfListHeadPtr MuFddfListHead;
FddfFilePtr MuFddfFile;
SegmentHeadPtr MuSegmentHead;
EyewearADPPtr MuEyewearADPrcv; ////preserver the signal of current
EyewearADPPtr MuEyewearADPsnd; //preserver the signal
XmlDeviceInfoPtr MuDeviceInfo;
ResumeInfo MuResume[10];
FileInfoPtr MuFileInfo;
DBIInfoPtr MuDBIInfo;
DBUInfoPtr MuDBUInfo;
DBURatePtr MuDBURate;
DBUDirtPtr MuDBUDirt;

//for store information or signal from server
int rcv_fd;
int snd_fd;
int stat_fd;
int list_fd;

#define MuFTAD_Free(buf)\
	do{\
		if(buf){\
			free(buf);\
			buf = NULL;\
		}\
	}while(0)
			

int MuFTAD_DateInit(void);
int MuFTAD_DeviceInit(const char *serverinfo, const char *deviceinfo);

void MuFTAD_FreeSource(void);

#endif

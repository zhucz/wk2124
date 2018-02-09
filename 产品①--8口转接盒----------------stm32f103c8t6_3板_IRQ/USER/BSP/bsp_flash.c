#include "bsp_flash.h"


struct myflash DeviceInfo = {0x0,{'\0'},{'\0'}};

void myFLASHCTRL0_run_erase_page_mode(int FLASH_START_ADDR)
{

}

void myFLASHCTRL0_run_write_flash_mode(int FLASH_START_ADDR,unsigned char *src,int srcLength)
{

}


void ReadFlashData(unsigned short int *FlashAddress,unsigned char *RamAddress,int length)
{
	unsigned short int *ptrSrc;
	unsigned char *ptrDes;
	int i,tempLength;
	
	ptrSrc = FlashAddress;
	ptrDes = RamAddress;
	tempLength = length;
	for(i = 0; i < tempLength; i++)
	{
		*ptrDes = *ptrSrc;
		*ptrSrc++;
		*ptrDes++;
	}
}



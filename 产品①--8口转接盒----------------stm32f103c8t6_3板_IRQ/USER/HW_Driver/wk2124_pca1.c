#include "wk2124_pca1.h"
#include "wk2124s.h"
#include "bsp_spi.h"
#include "bsp_gpio.h"
#include "os_struct.h"
#include "bsp_timer2.h"
#include "soft_timer.h"
#include "special_buff.h"


extern uint8_t uCom_send_dataBase[8][540];
extern struct soft_timer pca1_timer;
OS_UART pca1 = {{'\0'},0,0,0};

/*******************************************************************************
* Function Name  : 
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void HW_WK2124_pca1_Init(void)
{
	/*切换到PAGE0页中的子串口寄存器组 */
	EXHW_WK2412S1_Write_Reg(SPAGE(2),0x00);

	/*子串口 2 控制寄存器 */
	EXHW_WK2412S1_Write_Reg(SPAGE0_SCR(2),0x03);	//子串口2 发送使能  接收使能

	/*子串口 2 配置寄存器*/
	EXHW_WK2412S1_Write_Reg(SPAGE0_LCR(2),0x00);	//子串口2 正常输出,普通模式,8位数据位,0校验,1位停止位

	/*子串口 2 FIFO控制寄存器*/
	EXHW_WK2412S1_Write_Reg(SPAGE0_FCR(2),0x0F);	//子串口2 发送触发点,接收触发点 
																								//使能发送,接收FIFO 复位发送接收FIFO
	/*子串口 2 中断使能寄存器*/
	EXHW_WK2412S1_Write_Reg(SPAGE0_SIER(2),0x83); //子串口2 使能接收FIFO数据错误中断
																								//禁止发送FIFO空中断
																								//禁止发送FIFO触点中断
																								//使能接收FIFO接收超时中断
																								//使能接收FIFO接收触点中断

	/*切换到PAGE1页中的子串口寄存器组 */
	EXHW_WK2412S1_Write_Reg(SPAGE(2),0x01);

	/*子串口1 波特率配置寄存器高字节 [Reg = 11.0592/(115200*16) = 6] */
	EXHW_WK2412S1_Write_Reg(SPAGE1_BAUD1(2),0x00);

	/*子串口1 波特率配置寄存器低字节 */
	EXHW_WK2412S1_Write_Reg(SPAGE1_BAUD0(2),0x05);

	/*子串口1 波特率配置寄存器小数部分*/
	EXHW_WK2412S1_Write_Reg(SPAGE1_PRES(2),0x00);

	/*切换到PAGE0页中的子串口寄存器组 */
	EXHW_WK2412S1_Write_Reg(SPAGE(2),0x00);
}

/*******************************************************************************
* Function Name  : void WK2124_pca1_IRQHandler(void)
* Description    : 子串口1中断处理函数
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void WK2124_pca1_IRQHandler(void)
{
	  volatile uint8_t pca1_irq_stat = 0; 
    volatile uint16_t pca1_recv_cnt = 0;
	
		/*判断是 串口 3 的那种类型的中断*/
		pca1_irq_stat = EXHW_WK2412S1_Read_Reg(SPAGE0_SIFR(2));
	
		if(pca1_irq_stat & (3 << 0)){//子串口 1 接收FIFO触点中断标志 ， 子串口 1 接收FIFO超时中断标志
			 pca1_recv_cnt = Wk2124S1_3_GetBuf(pca1.recvBuff + pca1.count);
			 pca1.count += pca1_recv_cnt;
			
      if(pca1.count > 498){
         pca1.count = 498;
			}
	
			reload_timer(&pca1_timer,2);
			start_timer(&pca1_timer);   
			
		}
//		if(pca1_irq_stat & (1 << 1)){//子串口 1 接收FIFO超时中断标志
//			Wk2124S1_2_GetBuf(pca1.recvBuff);
//		}
		if(pca1_irq_stat & (1 << 7)){//子串口 1 接收FIFO数据错误中断标志
		
		}
		
		if(pca1_irq_stat & (1 << 2)){//子串口 1 发送FIFO触点中断标志
		
		}
		if(pca1_irq_stat & (1 << 3)){//子串口 1 发送FIFO空中断标志
		
		}
}
/*******************************************************************************
* Function Name  : 
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void WK2124_pca1_Send_String(uint8_t *src, uint16_t len)
{
	uint16_t send_cnt = 0,send_i = 0;
	
	SP3485_03_DE_SEND();
	Delay_ms(30); // 30  --- 3ms 
	
	send_cnt = len/256;
	
	for(send_i = 0; send_i <= send_cnt ;send_i++){
		send_cnt = Wk2124S1_3_SendBuf(src + (256*send_i),(len - (send_cnt * send_i)));
		
		while(EXHW_WK2412S1_Read_Reg(SPAGE0_TFCNT(2))  > 0);
		while((EXHW_WK2412S1_Read_Reg(SPAGE0_FSR(2)) & 0x01) == 1);
	}
	SP3485_03_DE_RECV();
	Delay_ms(30); // 30  --- 3ms 
}


uint16_t Wk2124S1_3_SendBuf(uint8_t *sendbuf,uint16_t len)
{
	uint16_t ret = 0,tfcnt = 0,sendlen = 0;
	uint8_t  fsr = 0;
	
	fsr = EXHW_WK2412S1_Read_Reg(SPAGE0_FSR(2));
	if(~fsr & 0x02 )//子串口发送FIFO未满
	{
		tfcnt = EXHW_WK2412S1_Read_Reg(SPAGE0_TFCNT(2));//读子串口发送fifo中数据个数
		sendlen = 256 - tfcnt;//FIFO能写入的最多字节数

		if(sendlen < len){
			ret = sendlen; 
			EXHW_WK2412S1_Write_FIFO(SPAGE0_FDAT(2),sendbuf,sendlen);
		}else{
			EXHW_WK2412S1_Write_FIFO(SPAGE0_FDAT(2),sendbuf,len);
			ret = len;
		}
	}

	return ret;
}


uint16_t Wk2124S1_3_GetBuf(uint8_t *getbuf)
{
	uint16_t ret=0,rfcnt = 0;
	uint8_t fsr = 0;
	
	fsr = EXHW_WK2412S1_Read_Reg(SPAGE0_FSR(2));
	if(fsr & 0x08 )//子串口接收FIFO未空
	{
		rfcnt = EXHW_WK2412S1_Read_Reg(SPAGE0_RFCNT(2));//读子串口发送fifo中数据个数
		if(rfcnt == 0)//当RFCNT寄存器为0的时候，有两种情况，可能是256或者是0，这个时候通过FSR来判断，如果FSR显示接收FIFO不为空，就为256个字节
		{
			rfcnt = 256;
		}
		EXHW_WK2412S1_Read_FIFO(SPAGE0_FDAT(2),getbuf,rfcnt);
		ret = rfcnt;
	}
	 return ret;	
}

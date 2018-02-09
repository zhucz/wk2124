/**
  ******************************************************************************
  * @file    :order.c used for Potevio interface board
  * @author  :linux.lucian@gmail.com
  * @version : 升级1.0
  * @date    :2014-08-23
  * @brief   :source file for interface board(sim3u146) update
  * 
  ******************************************************************************
  */
#include "s3c44b0x.h"
#include "cf8051.h"
#include "myService.h" 
//#include <SI32_PBHD_A_Type.h>
//#include <SI32_UART_A_Type.h>
//#include "myUART1.h"
#include "bsp_flash.h"
#include "bsp_timer2.h"
#include "bsp_usart2.h"

uint8_t TrayInfo[17] = {0x7e,0x00,0x0F,0x03,0x0f,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x01,0x00,0x5a};
uint8_t version[57]  = {0x7e,0x00,0x0b,0x05,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
//单元板升级数组
uint8_t cf8051update[40] = {0x7e,0x00,0x21,0x06,0x01,0x3,0x06}; //用来向主控返回单元板升级的 启动、数据包写入、重启状态值给主控
uint8_t update_trays[8]= {'\0'};
uint8_t update_trays_packet_status[8]= {'\0'};
uint8_t update_trays_startup_status[8]= {'\0'};
uint8_t update_trays_restart_status[8]= {'\0'};
extern uint8_t flashinfo[30];
extern volatile uint16_t lost_packet[8];
extern void resource_collect(void);//快速分发数组
extern void repost_data_to_cf8051_immediately(void);//快速分发数组
extern void repost_update_packet(void);

volatile uint8_t HUB_output = 0;
volatile uint8_t HUB_output_alarm = 0;
volatile uint8_t kaiji_cmd = 0;

volatile uint8_t read_version = 0;

extern volatile uint8_t version_flag;
extern volatile uint8_t zd_resource_flag;
extern volatile uint8_t zd_tray_num;

extern volatile uint8_t v_zd_whitch;


volatile uint8_t updating_flags = 0;

extern volatile uint8_t save_order_s;

                            //    1    6    2    0    1    6    1    2    1    2  type
uint8_t soft_hard_1[13] = {0x01,0x09,0x02,0x00,0x01,0x07,0x00,0x05,0x01,0x01,0x06,0x00,0x00};
	 
//==================================================================================================
//             接口板               工单的业务操作，以及逻辑处理(以下几个函数)
//==================================================================================================

/**
  * @brief  读取框下面挂接了几个托盘(单元板)
  * @retval 无 或者 不符合要求的数据
{0x7e,0x00,0x0d,0x03,0x0f,0x06,0x01,0x02,0x03,0x04,0x05,0x06,0x01,0x00,0x5a}
  */
static void read_tray_info(void)
{
   uint8_t i = 0;

   TrayInfo[4]  = board.id;
   TrayInfo[5]  = 0x0;

   for(i = 0;i < 8;i++){
			if(TrayInfo[6 + i] != 0x00){
				TrayInfo[5]++;
			}
   }
   TrayInfo[15] = crc8(TrayInfo,0x0f); 

   add_a_new_node(TrayInfo,DATA_NORMAL,(TrayInfo[1] << 8|TrayInfo[2] + 2),DATA_RECV);		
      
}


/**
  * @brief   读取端口信息:读取某个端口|某个盘|某个框的port信息
  * @retval  无 或者 不符合要求的数据
  */
static void read_410_ports_info(void)
{
   uint8_t pTrayNum = 0;

	 if(uart1.recvBuff[5] == 0){
		return;
	 }
   pTrayNum = uart1.recvBuff[5] - 1; //确定要读取哪一个盘

	 if(uart1.recvBuff[18] == 0x02){//表示通过读取端口信息，来指定采集
			v_zd_whitch = 3;
			zd_tray_num = uart1.recvBuff[5];

			flashinfo[pTrayNum] = 0xff;//盘号清除
			DeviceInfo.Tary_Num[pTrayNum] = 0xff;//盘号清除
			flashinfo[14 + pTrayNum] = 0xFF;//说明是12端口的单元板 盘类型清除
		 
			ports.traylost[pTrayNum] = 0;
			ports.portstat[pTrayNum] = 0;
   }
	 
   //读取的是某一个端口
   memset(&uCom_send_dataBase[pTrayNum][0],0, uart1.recvBuff[1]<<8|uart1.recvBuff[2]+2);                  
   memcpy(&uCom_send_dataBase[pTrayNum][0],&uart1.recvBuff[0],uart1.recvBuff[1]<<8|uart1.recvBuff[2]+2);
   repost_data_to_cf8051_immediately();	 
	 

	 start_timer(&yijian_read_version_timer);	//利用软硬件版本号的定时器，来做指定盘采集
	 //SI32_UART_A_disable_rx(SI32_UART_1);
   usart2_rx_irq_enable(DISABLE); 
}

/**
  * @brief  读取版本信息：获取 当前框的软硬件版本信息|下面单元板|某一个单元板的软硬件版本信息
  * @retval 无 或者 不符合要求的数据
  *
  * type:集线器类型
  * 0x06 ------ 6口集线器
  * 0x08 ------ 8口集线器
  */
static void read_version_info(void)
{
   uint8_t pTrayNum = 0;	
	 uint8_t i = 0;
                         //    1    6    2    0    1    6    1    2    1    2  type
   uint8_t soft_hard[13] = {V_1,V_2,Y_1,Y_2,Y_3,Y_4,M_1,M_2,D_1,D_2,T_1,0x00,0x00};
   
   if(uart1.recvBuff[4] != 0x0 && uart1.recvBuff[5] == 0x00){
      //读框的软硬件
      version[0] = 0x7e;
      version[1] = 0x00;
      version[2] = 0x37;
      version[3] = 0x05;
      version[4] = soft_hard[10];
      version[5] = board.id;
      version[6] = 0x0;	
		 
      soft_hard[11] = flashinfo[10];//实际的框号盘号
      memcpy(&version[7], &soft_hard[0],13);
		  memset(&version[20] , 0 , 11);
      memcpy(&version[31],&soft_hard[0],13);
      memset(&version[44] , 0 , 11);
		 
      version[55] = crc8(version,55);
      version[56] = 0x5A;
      add_a_new_node(version,DATA_NORMAL,(version[1] << 8|version[2] + 2),DATA_RECV);		
	
   }else if((uart1.recvBuff[5] != 0x00) && (uart1.recvBuff[5] != 0xFF)){
      //读盘的软硬件
      pTrayNum = uart1.recvBuff[5] - 1;
      memset(&uCom_send_dataBase[pTrayNum],0,COM_TX_SIZE);                  
      memcpy(&uCom_send_dataBase[pTrayNum][0],uart1.recvBuff,8);					
      repost_data_to_cf8051_immediately();		
   }else if(uart1.recvBuff[5] == 0xFF){//一键读取版本号
		  kaiji_cmd = 1;	//2017-06-21号开放资源采集命令 如果没有开机的情况下；
			for(i = 0; i < 8; i++){
				memset(&uCom_send_dataBase[i][0],0,COM_TX_SIZE);                  
				memcpy(&uCom_send_dataBase[i][0],uart1.recvBuff,8);		
				uCom_send_dataBase[i][5] = (i+1);//盘
				uCom_send_dataBase[i][6]= crc8(&uCom_send_dataBase[i][0],0x06);
		 }		
		 memset(reSource.rxbuf,0,sizeof(reSource.rxbuf));
		 read_version = 1;
		 start_timer(&yijian_read_version_timer);	//启动读取软硬件版本号超时
		 v_zd_whitch = 2;//表示一键读取版本号
		// SI32_UART_A_disable_rx(SI32_UART_1);
		 usart2_rx_irq_enable(DISABLE); 
	 }   
}

/**
  * @brief  软件升级
  * @retval 无 或者 不符合要求的数据
  */
static void software_update_cf8051(void)
{
   unsigned short datalen = 0;                         
   unsigned char Trayidex = 0,entires = 0,i = 0,j = 0;
   /*以下变量仅限用于单元板升级*/
   unsigned short start_idex = 0,end_idex = 0;

   datalen = uart1.recvBuff[1] << 8 | uart1.recvBuff[2];

   switch(uart1.recvBuff[4]){
      case 0x01:
         memset(&cf8051update[7],0x0e,24);
         cf8051update[31] = 0x00;//数据包序号置0
         cf8051update[32] = 0x00;//数据包序号置0
         memset(update_trays,0,sizeof(update_trays));//将标记托盘升级的数组全部清除为0：代表默认全部不升级
         memset(update_trays_startup_status,0xe,sizeof(update_trays_startup_status));//0xe:代表默认全部不启动
         start_idex = 7;
         end_idex   = 9 + (uart1.recvBuff[6] - 1)*3;
         cf8051update[7] = board.id;
         for(i = start_idex; i < end_idex; i+=3){
            if(uart1.recvBuff[i] == board.id){
               Trayidex = uart1.recvBuff[i+1];
               update_trays[Trayidex-1] = 1;//置1表示这个托盘是要升级的
               cf8051update[7+(Trayidex-1)*3] = board.id;
               cf8051update[8+(Trayidex-1)*3] = Trayidex;
               cf8051update[9+(Trayidex-1)*3] = 0x0f;
               memset(&uCom_send_dataBase[Trayidex - 1][0],0,540); 
               uCom_send_dataBase[Trayidex - 1][0] = 0x7e;
               uCom_send_dataBase[Trayidex - 1][1] = 0x00;
               uCom_send_dataBase[Trayidex - 1][2] = 0x0c;
               uCom_send_dataBase[Trayidex - 1][3] = 0x06;
               uCom_send_dataBase[Trayidex - 1][4] = uart1.recvBuff[4];
               uCom_send_dataBase[Trayidex - 1][5] = uart1.recvBuff[5];
               uCom_send_dataBase[Trayidex - 1][6] = 1;
               uCom_send_dataBase[Trayidex - 1][7] = board.id;
               uCom_send_dataBase[Trayidex - 1][8] = Trayidex;
               uCom_send_dataBase[Trayidex - 1][9] = 0x0;
               uCom_send_dataBase[Trayidex - 1][10]= 0x0;
               uCom_send_dataBase[Trayidex - 1][11]= uart1.recvBuff[datalen-1];
               uCom_send_dataBase[Trayidex - 1][12]= crc8(&uCom_send_dataBase[Trayidex - 1][0],0x0c);
               uCom_send_dataBase[Trayidex - 1][13]= 0x5a;
            }
         }
         repost_update_packet();
         reload_timer(&update_startup_timer,9000);	
         start_timer(&update_startup_timer);
      break;
      case 0x03:
         memset(&cf8051update[7],0x0e,24);
         cf8051update[31] = 0xff;//设置主控发来的数据包序号 0xff:代表重启单元板
         cf8051update[32] = 0xff;//设置主控发来的数据包序号 0xff:代表重启单元板
         memset(update_trays_restart_status,0xe,sizeof(update_trays_restart_status)); //标志那个盘需要发重启命令
         start_idex = 7;
         end_idex   = 9 + (uart1.recvBuff[6] - 1)*3;
         cf8051update[7] = board.id;		
         for(i = start_idex; i < end_idex; i+=3,j++){
            j = uart1.recvBuff[i+1];//取出当前这个托盘号，判断是不是上次启动的单元盘
            if(update_trays_startup_status[j-1] == 0x0){
               //只有刚才是启动的，并且启动成功的单元板才能发数据给它们
               if(uart1.recvBuff[i] == board.id){
                  Trayidex = uart1.recvBuff[i+1];
                  cf8051update[7+(Trayidex-1)*3] = board.id;
                  cf8051update[8+(Trayidex-1)*3] = Trayidex;
                  cf8051update[9+(Trayidex-1)*3] = 0x0f;
                  memset(&uCom_send_dataBase[Trayidex - 1][0],0,540); 
                  uCom_send_dataBase[Trayidex - 1][0] = 0x7e;
                  uCom_send_dataBase[Trayidex - 1][1] = 0x00;
                  uCom_send_dataBase[Trayidex - 1][2] = 0x0c;
                  uCom_send_dataBase[Trayidex - 1][3] = 0x06;
                  uCom_send_dataBase[Trayidex - 1][4] = uart1.recvBuff[4];
                  uCom_send_dataBase[Trayidex - 1][5] = uart1.recvBuff[5];
                  uCom_send_dataBase[Trayidex - 1][6] = 1;
                  uCom_send_dataBase[Trayidex - 1][7] = board.id;
                  uCom_send_dataBase[Trayidex - 1][8] = Trayidex;
                  uCom_send_dataBase[Trayidex - 1][9] = 0x0;
                  uCom_send_dataBase[Trayidex - 1][10]= 0x0;
                  uCom_send_dataBase[Trayidex - 1][11]= uart1.recvBuff[datalen-1];
                  uCom_send_dataBase[Trayidex - 1][12]= crc8(&uCom_send_dataBase[Trayidex - 1][0],0x0c);
                  uCom_send_dataBase[Trayidex - 1][13]= 0x5a;
               }
            }
         }
      repost_update_packet();
      reload_timer(&update_restart_timer,10000);
      start_timer(&update_restart_timer);
      break;
      case 0x02:
         entires = uart1.recvBuff[6];
         start_idex = 7;
         end_idex = 9 + (entires-1)*3;	
         cf8051update[31] = uart1.recvBuff[datalen-514];//先获取主控发来的数据包序号
         cf8051update[32] = uart1.recvBuff[datalen-513];//先获取主控发来的数据包序号
         memset(update_trays_packet_status,0xe,sizeof(update_trays_packet_status));
         for(i = start_idex;i < end_idex;i+= 3,j++){
            j = uart1.recvBuff[i+1];//取出当前这个托盘号，判断是不是上次启动的单元盘
            if(update_trays_startup_status[j-1] == 0x0){
               //只有刚才是启动的，并且启动成功的单元板才能发数据给它们
               if(uart1.recvBuff[i] == board.id){
                  Trayidex = uart1.recvBuff[i+1];	
                  cf8051update[7+(Trayidex-1)*3] = board.id;
                  cf8051update[8+(Trayidex-1)*3] = Trayidex;
                  cf8051update[9+(Trayidex-1)*3] = 0x01;//表示该盘正在升级中
                  memset(&uCom_send_dataBase[Trayidex - 1][0],0,540); 
                  uCom_send_dataBase[Trayidex - 1][0] = 0x7e;
                  uCom_send_dataBase[Trayidex - 1][1] = 0x02;
                  uCom_send_dataBase[Trayidex - 1][2] = 0x0c;
                  uCom_send_dataBase[Trayidex - 1][3] = 0x06;
                  uCom_send_dataBase[Trayidex - 1][4] = uart1.recvBuff[4];
                  uCom_send_dataBase[Trayidex - 1][5] = uart1.recvBuff[5];
                  uCom_send_dataBase[Trayidex - 1][6] = uart1.recvBuff[6];
                  uCom_send_dataBase[Trayidex - 1][7] = board.id;
                  uCom_send_dataBase[Trayidex - 1][8] = Trayidex;
                  uCom_send_dataBase[Trayidex - 1][9] = 0x0;
                  uCom_send_dataBase[Trayidex - 1][10]= uart1.recvBuff[datalen-514];
                  uCom_send_dataBase[Trayidex - 1][11]= uart1.recvBuff[datalen-513];
                  memcpy(&uCom_send_dataBase[Trayidex - 1][12],&uart1.recvBuff[datalen-512],512);
                  uCom_send_dataBase[Trayidex - 1][524] = crc8(&uCom_send_dataBase[Trayidex - 1][0],524);
                  uCom_send_dataBase[Trayidex - 1][525] = 0x5a;
               } 
            }else if(update_trays_startup_status[j] == 0x1){
               //不启动的||启动失败的 我们不需要给它们发数据包
                  Trayidex = uart1.recvBuff[i+1];
                  cf8051update[7+(Trayidex-1)*3] = board.id;
                  cf8051update[8+(Trayidex-1)*3] = Trayidex;
                  cf8051update[9+(Trayidex-1)*3] = 0x01;	
            }
         }
         repost_update_packet();
         reload_timer(&update_write_packets_timer,10000);
         start_timer(&update_write_packets_timer);	
      break;

   }

}

/**
  * @brief  写入EID到端口：向某个端口写入一个EID信息
  * @retval 无 或者 不符合要求的数据
  */
static uint8_t write_EID_to_410(void)
{
   uint8_t pTrayNum = 0;

   pTrayNum = uart1.recvBuff[5];
   memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE);
   memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,137);
   repost_data_to_cf8051_immediately();	
   return 0;
}

/**
  * @brief  LED灯测试
  * @retval 无 或者 不符合要求的数据
  */
static uint8_t led_guide_test(void)
{
   uint8_t pTrayNum = 0,i = 0;
   uint8_t temp_lost[25] = {'\0'};
	
	 if(uart1.recvBuff[7] == 0x05){//说明是一个读取丢包率的命令

			temp_lost[0] = 0x7e;
			temp_lost[1] = 0x00;
			temp_lost[2] = 0x15;
			temp_lost[3] = 0xFA;
			temp_lost[4] = uart1.recvBuff[4];//框号

			for(i = 0;i < 8;i++){
				temp_lost[5 + (2*i) + 0] = ((lost_packet[i] >> 8) & 0xff);
				temp_lost[5 + (2*i) + 1] = ((lost_packet[i] >> 0) & 0xff);
			}
			temp_lost[21] = crc8(temp_lost,21);
			temp_lost[22] = 0x5A;
			add_a_new_node(temp_lost,DATA_NORMAL,(temp_lost[1] << 8|temp_lost[2] + 2),DATA_RECV);		
			return 0;
	 }
		
   if(uart1.recvBuff[5] == 0xff){
		 kaiji_cmd = 1;	//2017-06-21号开放资源采集命令 如果没有开机的情况下；
      //整框灯测试
      for(;pTrayNum < 8;pTrayNum++){
         memset(&uCom_send_dataBase[pTrayNum],0,COM_TX_SIZE);
         memcpy(&uCom_send_dataBase[pTrayNum][0],uart1.recvBuff,10);
      }		
   }else if(uart1.recvBuff[5] > 0x0 && uart1.recvBuff[5] < 0x09){
      //盘的测试(可能是某个端口||可能是某个整盘)
      pTrayNum = uart1.recvBuff[5];
      memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE);
      memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,10);
   }
   repost_data_to_cf8051_immediately();
   return 0;
}


/**
  * @brief  连接追踪或跳纤追踪 
  * @retval 无 或者 不符合要求的数据
  */
static void unControl_wirteEID_info(void)
{
   uint8_t pTrayNum = 0;

   if((uart1.recvBuff[4] == 0x05) || (uart1.recvBuff[4] == 0x06)){
      if(uart1.recvBuff[5] == board.id){
            pTrayNum = uart1.recvBuff[6];
            if(pTrayNum != 0x0){//严谨性考虑，只有框号符合且盘号不为0的情况下，我们才向接口板发起数据
               memset(&uCom_send_dataBase[pTrayNum - 1][0],0,COM_TX_SIZE); 
               memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
            }else{
               return ;
            }
      }else if(uart1.recvBuff[9] == board.id){
            pTrayNum = uart1.recvBuff[10];
            if(pTrayNum != 0x0){//严谨性考虑，只有框号符合且盘号不为0的情况下，我们才向接口板发起数据
               memset(&uCom_send_dataBase[pTrayNum - 1][0],0,COM_TX_SIZE); 
               memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
            }else{
               return ;
            }
      }
               
   }else{
      pTrayNum = uart1.recvBuff[6];
      if(pTrayNum != 0x0){//严谨性考虑，只有框号符合且盘号不为0的情况下，我们才向接口板发起数据
         memset(&uCom_send_dataBase[pTrayNum - 1][0],0,COM_TX_SIZE); 
         memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
      }else{
         return ;
      }
   }

   repost_data_to_cf8051_immediately();
}

/**
  * @brief  批量工单新建中的指引
  * @retval 无 或者 不符合要求的数据
  */
static void Patch_guide_action(void)
{
   uint8_t i = 0;
   uint8_t pTrayNum = 0;

   if(0x88 == uart1.recvBuff[3]){
      for(i = 0;i<2;i++){
         pTrayNum = uart1.recvBuff[5+(3*i)];
         memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE); 
         memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
      }
   }else{
         pTrayNum = uart1.recvBuff[5];
         memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE); 
         memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
   }
   repost_data_to_cf8051_immediately();

}

/**
  * @brief   
  * @retval 无 或者 不符合要求的数据
  */
static uint8_t Load_Data(void)
{
   uint8_t retval = 0;
   uint8_t pTrayNum = 0;
	 uint8_t load2flash[30] = {'\0'};
	 uint8_t load[11] = {'\0'};
	
   if(uart1.recvBuff[4] == 0x03)//单元板需要加载的数据
   {
       pTrayNum = uart1.recvBuff[7];
         memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE); 
         memcpy(&uCom_send_dataBase[pTrayNum - 1][0],uart1.recvBuff,(uart1.recvBuff[1] << 8 |uart1.recvBuff[2] + 2));
   }
	 /* 2017-07-06 zhuchengzhi add */
	 else if(uart1.recvBuff[4] == 0x02){
			memcpy(&load2flash[0],&uart1.recvBuff[9],30);
			cpu_disable_irq(0);
			myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);
			myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR,load2flash,sizeof(load2flash));
			ReadFlashData(FLASH_TEST_PAGE_ADDR_PTR,flashinfo,sizeof(flashinfo));
			cpu_enable_irq();
		 
		  //读框的软硬件
      load[0] = 0x7e;
      load[1] = 0x00;
      load[2] = 0x09;
      load[3] = 0x13;
      load[4] = 0x02;
      load[5] = uart1.recvBuff[9]; //框
      load[6] = 0x0;							 //盘
		  load[7] = 0x0;               //端口
      load[8] = 0x0;							 //成功失败
      load[9] = crc8(load,9);
      load[10] = 0x5A;
      add_a_new_node(load,DATA_NORMAL,(load[1] << 8|load[2] + 2),DATA_RECV);		
	 }
	 
   return retval;
}



static uint8_t Back_Order_To_Source(void)
{
   uint8_t retval = 0;
   uint8_t pTrayNum = 0;
   uint8_t index_i = 0;

   for(index_i=0;index_i<uart1.recvBuff[6];index_i++)
   {
      if(uart1.recvBuff[8+(4 * index_i)] == board.id)
      {
         pTrayNum = uart1.recvBuff[9+(4 * index_i)];
         memset(&uCom_send_dataBase[pTrayNum - 1],0,COM_TX_SIZE);
         memcpy(&uCom_send_dataBase[pTrayNum - 1],uart1.recvBuff,(uart1.recvBuff[1] << 8 | uart1.recvBuff[2]));
      }			
   }
   return retval;
}

/**
  * @brief  施工工单操作相关
  * @retval 无 或者 不符合要求的数据
  */
static void orders_operations(void)
{
   uint8_t sub_cmd = 0, tray_num = 0;
   uint16_t i = 0;
   /* 以下这部分变量只给二次采集覆盖用 */	
   uint16_t datalen = 0,recoverlen = 0;
   uint16_t frameidex_start = 0,frameidex_end = 0;
   uint16_t  idex[8] = {'\0'};
   uint8_t  Trayidex = 0;
   uint8_t wirte2flash[10], tray_stat[15];   

   sub_cmd = uart1.recvBuff[4];
	 
	 if(sub_cmd != 0x0A){
		 save_order_s = sub_cmd; //zhuchengzhi 2017-06-12 
	 }
	 
   switch(sub_cmd)
   {
      case 0x01://资源采集	
      case 0x11://2次采集	
			   kaiji_cmd = 1;	//2017-06-21号开放资源采集命令 如果没有开机的情况下；
			
         for(;i < 8;i++){
            memset(&uCom_send_dataBase[i][0],0,COM_TX_SIZE);                  
            memcpy(&uCom_send_dataBase[i][0],uart1.recvBuff,8);						
         }			
         reSource.idex = 0;	
         reSource.flag = 0;

				 memset(&ports.traylost[0],0,8);
				 memset(&ports.portstat[0],0,8);
				 
         reSource.reSourceNow = 1;						
         memset(reSource.tray_units, TRAY_OFFLINE, 10);         
         memset(reSource.rxbuf,0,sizeof(reSource.rxbuf));
         reSource.type = uart1.recvBuff[4];
         start_timer(&resource_timer);	//启动采集的计时命令，采集超时1.5秒
         memset(&DeviceInfo.Tary_Num[0],0xff,8);
//         SI32_UART_A_disable_rx(SI32_UART_1); 
usart2_rx_irq_enable(DISABLE); 		 
         break;	
      case 0xb1:
      case 0xb2:			
      case 0x02://odf架内单端跳接            
      case 0x03://改跳
      case 0x04://odf架内单端拆除	
      case 0x05://双端新建
      case 0x06://双端拆除	
      case 0x07://批量新建	
      case 0x08://架间新建一对
      case 0x09://架间拆除一对
      case 0x10://架间批量多对
      case 0x18:
      case 0x19:
      case 0x30:		
      case 0x0a://确认
      case 0xA2://取消命令
      case 0x8c://清楚410单元板对端
      case 0x8a://直接换线	
      case 0x9a://换线的取消
         if(((sub_cmd == 0x07) || (sub_cmd == 0xA2)||(sub_cmd == 0x10)||(sub_cmd == 0x30)||(sub_cmd == 0xb2)) 
            &&  ((uart1.recvBuff[5] == 0x07) || (uart1.recvBuff[5] == 0x10)|| (uart1.recvBuff[5] == 0x30)|| (uart1.recvBuff[5] == 0x11)|| (uart1.recvBuff[5] == 0x12))
         )
         {
            //批量新建中的点灯和取消操作
            goto patch_07_or_A2;
         }else{
            if(uart1.recvBuff[6] == uart1.recvBuff[10]){//同框
               if(uart1.recvBuff[7] == uart1.recvBuff[11]){//同盘，发给一个托盘
                  tray_num = uart1.recvBuff[7] - 1;
                  if(tray_num < 8){
                     memset(&uCom_send_dataBase[tray_num],0,COM_TX_SIZE);                  
                     memcpy(&uCom_send_dataBase[tray_num][0],uart1.recvBuff,uart1.recvBuff[2] + 2);                   
                  }
               }else{//不同盘,发给2个托盘	
                  tray_num = uart1.recvBuff[7] - 1;
                  if(tray_num < 8){
                     memset(&uCom_send_dataBase[tray_num],0,COM_TX_SIZE);                  
                     memcpy(&uCom_send_dataBase[tray_num][0],uart1.recvBuff,uart1.recvBuff[2] + 2);                   
                  }
                  
                  tray_num = uart1.recvBuff[11] - 1;
                  if(tray_num < 8){
                     memset(&uCom_send_dataBase[tray_num],0,COM_TX_SIZE);                  
                     memcpy(&uCom_send_dataBase[tray_num][0],uart1.recvBuff,uart1.recvBuff[2] + 2);                   
                  } 	
               }
            }else{//不同框,只要发给一个托盘
                  tray_num = uart1.recvBuff[7] - 1;
                  if(tray_num < 8){
                     memset(&uCom_send_dataBase[tray_num],0,COM_TX_SIZE);                  
                     memcpy(&uCom_send_dataBase[tray_num][0],uart1.recvBuff,uart1.recvBuff[2] + 2);                   
                  } 
            }
         }
         break;
      case 0x0c://二次采集的覆盖命令
         datalen = uart1.recvBuff[1] << 8 | uart1.recvBuff[2]; //首先求出字节总长度(不包含crc和0x5a位)
         frameidex_start = 5;//定位起始端口在下标5号处
         frameidex_end = datalen - 1;//终止在下标总长度(不包含crc和0x5a位)		
      
         for(i = 0;i < 8;i++){
            idex[i] = 5;
         }
         
         for(i = frameidex_start; i < frameidex_end; i +=4){
            Trayidex = uart1.recvBuff[i + 1] - 1;
            memcpy(&uCom_send_dataBase[Trayidex][idex[Trayidex]],&uart1.recvBuff[i],4);
            idex[Trayidex] += 4;
         }
         
         for(i = 0; i < 8;i++){
            if(idex[i] != 5){
               recoverlen = idex[i];
               uCom_send_dataBase[i][0] = 0x7e;
               uCom_send_dataBase[i][1] = (((recoverlen) >> 8) & 0xff);
               uCom_send_dataBase[i][2] = ((recoverlen) & 0xff);
               uCom_send_dataBase[i][3] = 0x0d;
               uCom_send_dataBase[i][4] = 0x0c;
               uCom_send_dataBase[i][recoverlen] = crc8(&uCom_send_dataBase[i][0],recoverlen);
               uCom_send_dataBase[i][recoverlen+1] = 0x5a;
            }
         }
         
         //如果这个盘，原来不存在，但资源采集后，有了数据，说明是新增的盘，同时，主控让接口板覆盖掉它，需要记录到flash中去
         //但如果这个框原来在，现在不在了，你也得从中间剔除么？？？？
         for(i = 0; i < 8; i++){
            //上次采集时，此处是没有接盘的
            if(DeviceInfo.Tary_Num[i] == 0xff){
               if(reSource.tray_units[i] == TRAY_ONLINE){
                  //添加一个新增盘数据录入成功的"告警"信息给主控，因为之前有上报了非法插入盘的"告警"
                  alarm.data[alarm.idex++] = board.id;
                  alarm.data[alarm.idex++] = i + 1;
                  alarm.data[alarm.idex++] = 0x00;
                  alarm.data[alarm.idex++] = 0x0C;
                  alarm.data[0] = 0x7e; 
                  alarm.data[3] = 0x09; 
                  alarm.flag = 1;
                  if(alarm.idex > 292)alarm.idex = 4; 
                  ports.portstat[i] = BACK_TO_NORMAL;
//                  ports.tray[i] = 0;
                  
                  DeviceInfo.Tary_Num[i] = i + 1;                  
               }
            
            }else if(DeviceInfo.Tary_Num[i] == (i + 1)){
            //上次采集时，此处是接了盘的
               if(reSource.tray_units[i] == TRAY_OFFLINE){
                  //删除的盘
                  DeviceInfo.Tary_Num[i] = 0xff;
               }
            
            }
            
         }
         
         memset(wirte2flash,0xff,sizeof(wirte2flash));
         memcpy(wirte2flash,&DeviceInfo.Tary_Num[0],8);
         cpu_disable_irq(0);
         wirte2flash[6] = 0x1;//已经被资源采集过了的
         myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);
         myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR,wirte2flash,sizeof(wirte2flash));
         cpu_enable_irq();
         
         memset(tray_stat, 0x0, 15);
         tray_stat[0] = 0x7e;
         tray_stat[1] = 0x00;
         tray_stat[2] = 0x0b;
         tray_stat[3] = 0x33;
         memcpy(&tray_stat[4], wirte2flash, 7);
         tray_stat[11] = crc8(tray_stat, 11);
         tray_stat[12] = 0x5a;   
         add_a_new_node(tray_stat,DATA_NORMAL,13,DATA_RECV);
      break;
      case 0xa4:
         Back_Order_To_Source();
      break;
      
      default:break;
    }

   if(sub_cmd == 0x01 || sub_cmd == 0x11){
      return;
   }else{
      repost_data_to_cf8051_immediately();	 
      return; 
   }
    
patch_07_or_A2:
   //快速点亮/灭灯 操作，分拣出来到每一个盘上的数据后下发
   datalen = uart1.recvBuff[1] << 8 | uart1.recvBuff[2]; //首先求出字节总长度(不包含crc和0x5a位)
   frameidex_start = 6;//定位起始端口在下标6号处
   frameidex_end = datalen - 1;//终止在下标总长度(不包含crc和0x5a位)		

   for(i = 0;i < 6;i++){
      idex[i] = 6;
   }
   
   for(i = frameidex_start; i < frameidex_end; i +=4){
      Trayidex = uart1.recvBuff[i + 1] - 1;
      memcpy(&uCom_send_dataBase[Trayidex][idex[Trayidex]],&uart1.recvBuff[i],4);
      idex[Trayidex] += 4;
   }
   
   for(i = 0; i < 8;i++){
      if(idex[i] != 6){
         recoverlen = idex[i];
         uCom_send_dataBase[i][0] = 0x7e;
         uCom_send_dataBase[i][1] = (((recoverlen) >> 8) & 0xff);
         uCom_send_dataBase[i][2] = ((recoverlen) & 0xff);
         uCom_send_dataBase[i][3] = 0x0d;
         uCom_send_dataBase[i][4] = uart1.recvBuff[4];
         uCom_send_dataBase[i][5] = uart1.recvBuff[5]; 
         uCom_send_dataBase[i][recoverlen] = crc8(&uCom_send_dataBase[i][0],recoverlen);
         uCom_send_dataBase[i][recoverlen+1] = 0x5a;
      }
   }
   repost_data_to_cf8051_immediately();
   return ;
}

/**
  * @brief  升级接口板，此处将跳入BIOS中去
  * @retval 
  */
static void software_update_sim3u1xx(void)
{
//   unsigned char flashinfo[30];

//   memset(flashinfo,0xff,sizeof(flashinfo));
   ReadFlashData(FLASH_TEST_PAGE_ADDR_PTR,flashinfo,sizeof(flashinfo));
   
   flashinfo[11] = 0x78;
   flashinfo[12] = 0x78;
   flashinfo[13] = 0x78;//表示我从app跳到bios里，需要在bios程序中给主控一个启动成功的标志位
   
   myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);//0x7C00
   
   myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR,flashinfo,30);
   
   reset_mcu_enter_default_mode();
   
//   SI32_RSTSRC_A_generate_software_reset(SI32_RSTSRC_0);  //强制软件复位  // 跳转到用户程序
}


/**
  * @brief  只用来填充资源采集的数组
  * @retval 无 或者 不符合要求的数据
  * @description: 接口板返回给s3c2416x主控板的数据格式如下：一共是10字节+(8+32*12)*6+2 = 2364个字节
  -------------------------------------------------------------------------------------------------------------------------------------
  数据帧头(1字节)+数据高位(1字节)+数据低位（1字节）+主命令字(0x0d)+采集类型(1个字节)+框编号(1个字节)+框的软硬件版本号(4字节)+
  -------------------------------------------------------------------------------------------------------------------------------------
  { 
      托盘软硬件版本号(4个字节)+托盘所属框号(1个字节)+托盘所属盘号(1个字节)+托盘上计数端口1(1个字节)+托盘上计数端口12(1个字节)+
      32*12(一个托盘的EID信息总数) 
  } X 6次
  -------------------------------------------------------------------------------------------------------------------------------------
  数据体的crc(1个字节)+帧数据尾0x5a(1个字节)	
  -------------------------------------------------------------------------------------------------------------------------------------
  */
static void fulling_resource_trayNums_data(void)
{
	
   unsigned short didex=6, EID_infolen = 35;
   unsigned short datalen = 0x0;	
   unsigned char  port_id = 0;//,tray_id = 0; 
	 unsigned char i =1,j =1;
	
   /*每个数字的含义：
         32	: EID_infolen 每个EID标签信息的数据长度
         8	：资源采集时，每个托盘会返回除了EID标签信息外，还包含了托盘所属的框号，盘号，托盘上起始端口（0x1,0xC）托盘的软硬件版本号(4个字节)
         12	：每个托盘上12个端口 
         6	: 每个框6个托盘 
         14	：其他字节长度总和，包括数据帧头，帧尾，长度高低位，框编号，以及软硬件版本号等
   */
   //datalen = (EID_infolen*12)*6+6;

	 for(i=1;i<=8;i++){
	 	datalen += EID_infolen * back_trays_types(i);
	 }
	 datalen += 7;//6 + 1 前面6个字节和最后一个自己手动添加的框类型0x06--6口 0x08--8口集线器
	 
	 if(datalen > 4800){//防止数据溢出
		reSource.idex = 0;
		reSource.flag = 0;
		DeviceInfo.resource = 0;
		return;
	 }
	 
	 
   reSource.rxbuf[0] = 0x7e;
   reSource.rxbuf[1] = ( datalen >> 8 ) & 0xff;
   reSource.rxbuf[2] = ( datalen & 0xff);
   reSource.rxbuf[3] = 0x0d;
   reSource.rxbuf[4] = reSource.type;	
   reSource.rxbuf[5] = board.id;

   /* ↓这里只是简单的赋值，只针对没有采集上来的盘或者该托盘处空缺的情况下才有效*/
   //tray_id = 1;
   port_id = 1;
	 
	for(i=1;i<=8;i++){
		if(back_trays_types(i) == 0x0c){    //说明该盘是一个12端口的盘
			for(j=1;j<=12;j++){
				reSource.rxbuf[didex]   = board.id;
				reSource.rxbuf[didex+1] = i;//tray_id
				reSource.rxbuf[didex+2] = port_id;
				didex += 35;
				
				port_id++;
				if(port_id == 13){
					port_id = 1;     
				}
			}
		}else if(back_trays_types(i) == 0x11){//说明该盘是一个17端口的盘
			for(j=1;j<=17;j++){
				reSource.rxbuf[didex]   = board.id;
				reSource.rxbuf[didex+1] = i;//tray_id
				reSource.rxbuf[didex+2] = port_id;
				didex += 35;
				
				port_id++;
				if(port_id == 18){
					port_id = 1;     
				}
			}
		}else{
			//do nothing
		}
	}
	 reSource.rxbuf[datalen - 1] = 0x08;//自己手动添加的框类型0x06--6口 0x08--8口集线器
   reSource.rxbuf[datalen] = crc8(&reSource.rxbuf[0],datalen);  
   reSource.rxbuf[datalen+1] = 0x5a;
   reSource.reSourceNow = 0x0;		
}





static void fulling_version_trayNums_data(void)
{
	unsigned short datalen = 0x0;	

	                       //    1   6   2   0   1   6   1   2   1   2 type
	uint8_t soft_hard_1[13] = {V_1,V_2,Y_1,Y_2,Y_3,Y_4,M_1,M_2,D_1,D_2,T_1,0x00,0x00};
	
	soft_hard_1[11] = flashinfo[10];
	
	datalen = 439; //8块盘的版本号总长度
	reSource.rxbuf[0] = 0x7e;
	reSource.rxbuf[1] = ( datalen >> 8 ) & 0xff;
	reSource.rxbuf[2] = ( datalen & 0xff);
	reSource.rxbuf[3] = 0xAA;
	reSource.rxbuf[4] = board.id;//frame
	reSource.rxbuf[5] = 0xff;    //tray
	reSource.rxbuf[6] = 0x08;    //类型这个HUB最大能接入8块盘
	memcpy(&reSource.rxbuf[7], &soft_hard_1[0],13);
	memcpy(&reSource.rxbuf[31],&soft_hard_1[0],13);
	
	reSource.rxbuf[439] = crc8(reSource.rxbuf, 439);
	reSource.rxbuf[440] = 0x5a;
}

/*这里有点问题  后期需要修改*/
static void fulling_zd_tray_resource_trayNums_data(void)
{
	unsigned short datalen = 0x0;	
	
	datalen = 391; //8块盘的版本号总长度 一块盘的数据长度
	reSource.rxbuf[0] = 0x7e;
	reSource.rxbuf[1] = ( datalen >> 8 ) & 0xff;
	reSource.rxbuf[2] = ( datalen & 0xff);
	reSource.rxbuf[3] = 0xFC;
	reSource.rxbuf[4] = board.id;       //frame
	reSource.rxbuf[5] = zd_tray_num;    //tray

	reSource.rxbuf[391] = crc8(reSource.rxbuf, 391);
	reSource.rxbuf[392] = 0x5a;
}
//==================================================================================================
//             接口板                  验证、处理、答复主控具体的操作函数(以下3个函数)
//==================================================================================================


uint8_t  Oops_verify_data_volume_frameid(uint8_t *dat)
{
   volatile uint8_t key = *(dat + 3), ret = 0;//uint8_t key = dat[3], ret = 0;

   switch(key){
      
      case 0x03:
      case 0x07:
      case 0x88://工单指引,有效的EID
      case 0x89://工单指引-无效的EID
         if(*(dat + 4) == board.id) ret = 1;
      break; 

			case 0x10:
      case 0x20:
		  case 0x04:
			case 0x05:
         if((board.id == 0xff) || (board.id != *(dat + 4))){
						if(*(dat + 4) > 0x11){
							ret = 0;
						}else{
							board.id = *(dat + 4);//dat[4];
							ret = 1;
						}

         }else{  
            if(*(dat + 4) == board.id)//if(dat[4] == board.id)
               ret = 1;   
         }

      break;
         
      case 0x06:
//         if(dat[7] == board.id)
            ret = 1;
      break;
      
      case 0x13:
         if(*(dat + 6) == board.id)//if(dat[6] == board.id) 
            ret = 1;
      break;
      
      case 0x0d://工单操作  
         switch(*(dat + 4)){//switch(dat[4]){
            case 0x01://一次采集
            case 0x11://二次采集
                board.id = *(dat + 5);//dat[5];//zhuchengzhi 2017-01-20  这样主控不需要配置就可以直接资源采集了
                  ret = 1;
            break;

            case 0x0c://二次采集覆盖
               if(*(dat + 5) == board.id)//if(dat[5] == board.id) 
                  ret = 1;
            break;
            case 0xb1:
            case 0xb2:
            case 0x02://单端新建
            case 0x03://单端改跳
            case 0x04://单端拆除	
            case 0x06://双端拆除	
            case 0x07://批量新建	
            case 0x08://架间A端新建一对
            case 0x09://架间A端拆除一对
            case 0x10://架间A端批量多对
            case 0x18://架间Z端新建一对
            case 0x19://架间Z端拆除一对
            case 0x30://架间Z端批量多对
            case 0xA2://工单取消	
            case 0x9a://双端换线的取消
            case 0x8a://换线
            case 0x05://双端新建
            case 0x8c://工单清除
            case 0x0a://确认指令 --- 》 可能是新建，可能是拆除，可能是改跳
               if(*(dat + 6) == board.id )ret = 1;//if(dat[6] == board.id )ret = 1;
            break;
            case 0xa4://确认指令 --- 》 可能是新建，可能是拆除，可能是改跳
               if(*(dat + 8) == board.id )ret = 1;//if(dat[8] == board.id )ret = 1;
            break;
            default:break;
         }
      break;
         
      case 0x0f:
         if((*(dat + 4) == 0x05) || (*(dat + 4) == 0x06)){
            if((*(dat + 5) == board.id) || (*(dat + 9) == board.id)) ret = 1;
         }else{
            if(*(dat + 5) == board.id) ret = 1;
         }
//         if((dat[4] == 0x05) || (dat[4] == 0x06)){
//            if((dat[5] == board.id) || (dat[9] == board.id)) ret = 1;
//         }else{
//            if(dat[5] == board.id) ret = 1;
//         }
      break;
            
      case 0xa6:
      case 0xf6:
         //if(dat[6] == board.id)ret = 1;
			   ret = 1;
      break;

      }

   return ret;
}


/**
  * @brief  专门用来清除主控收到的数据
  * @retval 无 或者 不符合要求的数据
  */
static void mainboard_A6_or_F6_msgs_handle(void)
{
   uint8_t src_cmd = 0, chain_cmd = 0, sub_cmd = 0;
   unsigned char wirte2flash[30];

   src_cmd   = uart1.recvBuff[3];
   chain_cmd = uart1.recvBuff[4];
   sub_cmd   = uart1.recvBuff[5];

   switch(src_cmd){
   case 0xa6:
      switch(chain_cmd){
      case 0x33://测试用的
            remove_a_old_node();         
      break;
      case 0xBB://开机命令成功
//					kaiji_cmd = 1;
//					remove_a_old_node();         
      break;
			case 0xAA:
			  memset(reSource.rxbuf,0,sizeof(reSource.rxbuf));  
				version_flag = 0;
			  read_version = 0;
			break;
      case 0x0d:
         switch(sub_cmd){
         case 0x01://第一次信息录入,直接写入到flash,不用等待主控的写入flash命令
            memset(reSource.rxbuf,0,sizeof(reSource.rxbuf));  
            reSource.idex = 0;
            reSource.flag = 0;
            DeviceInfo.resource = 1;
            memset(wirte2flash,0xff,sizeof(wirte2flash));
            memcpy(wirte2flash,&DeviceInfo.Tary_Num[0],8);
				    memcpy(&wirte2flash[14],&flashinfo[14],8);
            wirte2flash[9] = DeviceInfo.resource;//flash[9] 代表该框是否被采集过
            wirte2flash[10] = board.id;//flash[10] 代表采集后的框号
            cpu_disable_irq(0);
            myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);
            myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR,wirte2flash,sizeof(wirte2flash));
				    ReadFlashData(FLASH_TEST_PAGE_ADDR_PTR,flashinfo,sizeof(flashinfo));
				 
						
            cpu_enable_irq();
         break;
         case 0x11://第二次之后的采集、资源巡检，都不直接写入到flash，而是等待主控要求写入时再写入.
            memset(reSource.rxbuf,0,sizeof(reSource.rxbuf));  
            reSource.idex = 0;
            reSource.flag = 0;
         break;
         case 0x02:
         case 0x03:
         case 0x04:
         case 0x05:
         case 0x06:
         case 0x07:
         case 0x08://架间A端新建一对
         case 0x09://架间A端拆除一对
         case 0x10://架间A端批量多对
         case 0x18://架间Z端新建一对
         case 0x19://架间Z端拆除一对
         case 0x30://架间Z端批量多对
         case 0xb1://光分路器单端新建
         case 0xb2:
         case 0x0a:
         case 0x8c:
         case 0x8a:
         case 0x88:
         case 0x89:
         case 0xEE:
				 case 0xA2:
               remove_a_old_node();
         break;
         default:break;
         }
      break;//end of 0x0d
      case 0x09://报警的处理有点微妙，它依据了上报时记录的发送数组下标，
                //跟当前实际的下标做了比较，有可能在上报数据后在等待主控回复
                //0xa6/0xf6的时候，接口板又获取了新的报警数据，并接在了后面，所以判别是有必要的。
         if(alarm.idex == alarm.pendingBytes){
            alarm.flag = 0;
            alarm.idex = 4;
            alarm.pendingBytes = alarm.idex;
            memset(alarm.data,0,sizeof(alarm.data));					
         }else if(alarm.idex > alarm.pendingBytes){
            memset(&alarm.data[4],0,alarm.pendingBytes - 4);
            memcpy(&alarm.data[4],&alarm.data[alarm.pendingBytes],alarm.idex - alarm.pendingBytes);
            alarm.idex = alarm.idex - alarm.pendingBytes + 4;
            alarm.pendingBytes = 4;
         }
      break;//end of 0x09
      case 0x03:
      case 0x04:
      case 0x05:
      case 0x0f:
      case 0x13:
      case 0x07:
			case 0xFA:
         remove_a_old_node();
      break;
			case 0xFC://指定盘采集
				  zd_resource_flag = 0;
					memset(reSource.rxbuf,0,sizeof(reSource.rxbuf)); 
			    memcpy(&wirte2flash[0],&flashinfo[0],30);
					cpu_disable_irq(0);
					myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);
					myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR,wirte2flash,sizeof(wirte2flash));
					ReadFlashData(FLASH_TEST_PAGE_ADDR_PTR,flashinfo,sizeof(flashinfo));
					cpu_enable_irq();
			break;
			
      case 0x10:
				 kaiji_cmd = 1;//开机命令成功
         remove_a_old_node();
      break;
      case 0x06:
         remove_a_old_node();
         if(flashinfo[13] == 0x78){
            memset(flashinfo, 0xff, sizeof(flashinfo));
            ReadFlashData(FLASH_TEST_PAGE_ADDR_PTR, flashinfo, sizeof(flashinfo));    
            flashinfo[11] = 0xff;
            flashinfo[12] = 0xff;
            flashinfo[13] = 0xff;    
            myFLASHCTRL0_run_erase_page_mode(FLASH_TEST_PAGE_ADDR);//0x7C00        
            myFLASHCTRL0_run_write_flash_mode(FLASH_TEST_PAGE_ADDR, flashinfo, sizeof(flashinfo));         
         }
      break;   
      default:break;
      }
   break;//end of 0xa6 
   case 0xf6:
      
   break;//end of 0xf6

   default:break;
   }
}


/**
  * @brief  对于主控发过来的数据,进行派发处理
  * @retval 无 或者 不符合要求的数据

      case 0x04://读取端口信息
         read_ports_info();
				 read_i++;
      break; 
  */
static uint8_t part3_repost_s3c44bx0_data_to_cf8051(void)
{
   uint8_t retval = 0; //依赖业务的类型，决定要不是回复0xA6与否
   uint8_t src_cmd = uart1.recvBuff[3];

   switch(src_cmd)
   {
      case 0x03://主控要读取盘信息
         read_tray_info();					
      break;
			case 0x04:
		   read_410_ports_info();
			break;
      case 0x05://读取软硬件版本信息
         read_version_info();	
      break;
      case 0x06://软件升级
         if(uart1.recvBuff[5] == 0x02){
            software_update_sim3u1xx();
         }else{
            software_update_cf8051();
         }
      break;    
      case 0x07://写入电子标签信息到设备
         write_EID_to_410();
      break;     
      case 0x0d://施工工单操作
         orders_operations();
      break;
      case 0x10://LED指示灯测试
         led_guide_test();
      break;
      case 0x0f://受控写入EID信息
         unControl_wirteEID_info();
      break;
      case 0x88://工单指引,有效的EID
      case 0x89://工单指引-无效的EID
         Patch_guide_action();
      break;
      case 0x13:
         Load_Data();
      break;		
      default:break;
   }

   return retval;
}

static void sim3u146_updata_ok_to_s3c2416(void)
{
	   unsigned char dfu_reply[18];
		//设备当前处于特殊状态,即，刚从BIOS跳到APP，应该要给予主控一个重启成功的状态信息
		dfu_reply[0] = 0x7e;
		dfu_reply[1] = 0x00;
		dfu_reply[2] = 0x0e;
		dfu_reply[3] = 0x06;
		dfu_reply[4] = 0x03;
		dfu_reply[5] = 0x02;//update type
		dfu_reply[6] = 0x01;//update nums
		dfu_reply[7] = board.id;
		dfu_reply[8] = 0x00;//tray id,
		dfu_reply[9] = 0x00;//excute result
		dfu_reply[10] = 0x00;
		dfu_reply[11] = 0x00;
		dfu_reply[12] = 0x00;
		dfu_reply[13] = 0x00;  
		dfu_reply[14] = crc8(dfu_reply, 14);
		dfu_reply[15] = 0x5a;
	  add_a_new_node(dfu_reply, DATA_NORMAL, 16, DATA_RECV); 
		return;
}

#if 1
static void sim3u146_kaiji_cmd_to_s3c2416(void)
{
	unsigned char dfu_reply[18];
	//设备的开机命令只有设备开机成功了才能做其他的事情
	dfu_reply[0] = 0x7e;
	dfu_reply[1] = 0x00;
	dfu_reply[2] = 0x0e;
	dfu_reply[3] = 0x10;    //开机命令
	dfu_reply[4] = 0x02;    //类型：01:单元板 02:HUB 03:主控
	dfu_reply[5] = board.id;//框号
	dfu_reply[6] = 0x00;    //盘号
	dfu_reply[7] = 0x00;    //重启类型 01：APP重启  02：软件升级重启 03：BIOS中软件升级的重启(重启后还在BIOS中)
	dfu_reply[8] = 0x00;    //类型：01:主控发送命令的重启  02:HUB自己重启  
	dfu_reply[9] = 0x00;    
	dfu_reply[10] = 0x00;
	dfu_reply[11] = 0x00;
	dfu_reply[12] = 0x00;
	dfu_reply[13] = 0x00;  //excute result
	dfu_reply[14] = crc8(dfu_reply, 14);
	dfu_reply[15] = 0x5a;
	add_a_new_node(dfu_reply, DATA_NORMAL, 16, DATA_RECV); 
	return;
}
#endif


/**
  * @brief  向主控反馈当前框中的运行状态
  * @retval 无 或者 不符合要求的数据
  */
static void part3_feedback_msgs_to_s3c44b0x(void)
{
   uint16_t datalen = 0;
	 static volatile unsigned char kaiji_first = 0;
	
	 if(kaiji_first == 0){
		  kaiji_first = 1;
		  kaiji_cmd = 0;
			sim3u146_kaiji_cmd_to_s3c2416();
	 }
	
	 if(flashinfo[13] == 0x78){
			sim3u146_updata_ok_to_s3c2416();
	 }
	
   if(reSource.flag){                                    //资源采集||资源巡检 最高优先级上传		
      fulling_resource_trayNums_data();
      myUART1_send_multi_bytes(&reSource.rxbuf[0], reSource.rxbuf[1]<<8|reSource.rxbuf[2]+2); 		
   }else if(alarm.flag){ //报警，第二优先级返回，收不到主控的确认0xa6，不清除 
      datalen = alarm.idex;
      alarm.data[1] = (datalen >> 8) & 0xff; 
      alarm.data[2] =  datalen & 0xff;
      alarm.data[datalen] = crc8(alarm.data,datalen);
      datalen += 1;
      alarm.data[datalen] = 0x5a;
      myUART1_send_multi_bytes(alarm.data,datalen+1);
    alarm.pendingBytes = alarm.idex;
   }else if(EMPTY != Is_this_EmptyList()){               //其他，任务工单数据，第三优先级返回，数据存在pNewHead为表头的链表中				
      OS_NEWTASKLIST* p = NULL;
      p = pNewHead->next;
      myUART1_send_multi_bytes(p->data,p->dataLen);
      p->NodeStat = DATA_SEND;
   }else if (version_flag == 1){//一键读取版本号
      fulling_version_trayNums_data();
      myUART1_send_multi_bytes(&reSource.rxbuf[0], reSource.rxbuf[1]<<8|reSource.rxbuf[2]+2); 	
	 }else if (zd_resource_flag == 1){//指定盘采集
		 fulling_zd_tray_resource_trayNums_data();
		 myUART1_send_multi_bytes(&reSource.rxbuf[0], reSource.rxbuf[1]<<8|reSource.rxbuf[2]+2); 	
	 }else{
		 if(flashinfo[10] != uart1.recvBuff[4]){//2017-04-20 用于判断HUB自己是否处于非法插入中
				if(HUB_output < TRAY_TIMES_3){
					HUB_output++;
					if(HUB_output == TRAY_TIMES_3){
						HUB_output = TRAY_TIMES_3;
						HUB_output_alarm = 1;
						memset(&ports.traylost[0], 0 ,sizeof(ports.traylost));
					}
				}
			}else{
				HUB_output = 0;
				HUB_output_alarm = 0;
			}

      frame_run_ok[4] = board.id;
		 	frame_run_ok[7] = flashinfo[10];
      frame_run_ok[9] = crc8(frame_run_ok,9);
      myUART1_send_multi_bytes(frame_run_ok,11); //最低级别，上面什么数据都没有的，则返回接口板运行正常指令	           
   }

}


/**
  * @brief  s3c44b0x的数据处理程序
            主要对主控发过来的数据流，先检测框号对不对，然后计算CRC，符合要求了才收下来处理
  * @retval 无 或者 不符合要求的数据
  */
// 2017-04-20 必须先判断CRC 朱承智修改
uint8_t mainboard_service_routine(void)
{
   uint8_t is_this_frame = 0;	
   uint8_t is_crc_right = 0,main_cmd = 0x0;
	
	is_crc_right = app_calccrc8(&uart1.recvBuff[0], uart1.count - 2);
	if(RIGHT == is_crc_right){
		/* 如果CRC对的话就取出框号*/
		is_this_frame = Oops_verify_data_volume_frameid(&uart1.recvBuff[0]);
		if(WRONG == is_this_frame){
			memset(&uart1.recvBuff[0],0, sizeof(uart1.recvBuff));
			uart1.count = 0x0;
			uart1.recvFlag = 0;
			return 0;
		}
		
		//step2.是这个框的数据，进入下面的数据判断与处理
		main_cmd = uart1.recvBuff[3];
		if(0x20 == main_cmd){//step2.2  数据体有效，判断出主控发送的是巡检命令，即 命令字；0x20
			part3_feedback_msgs_to_s3c44b0x();
		}else if(0xa6 == main_cmd || 0xf6 == main_cmd){//数据体有效，判断出主控发送的是数据确认命令，即 命令字；0xa6/0xf6 
			mainboard_A6_or_F6_msgs_handle();
		}else{//数据体有效，主控制器发出了业务逻辑命令，可能有需要接口板直接回复或者转发给单元板
			if(kaiji_cmd == 1){//只有开机成功了才能处理数据
				frame_is_right[4] = board.id;
				frame_is_right[5] = crc8(frame_is_right, 5); 
				myUART1_send_multi_bytes(frame_is_right,7);

				part3_repost_s3c44bx0_data_to_cf8051();
			}
		}
	}else{
	
	}
	
	uart1.count = 0x0;
	uart1.recvFlag = 0x0;   

	return 0;

}

//-----------------------------------eof------------------------------------------	


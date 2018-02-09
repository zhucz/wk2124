/*------------------------------------------------------------------------------
 | Copyright (c) 2014,南京普天通信股份有限公司.
 | All rights reserved.
 |------------------------------------------------------------------------------
 | 文件名称： Silion labs-sim3u146 接口板完整程序
 | 文件批号： 详见任务计划书
 | 简单摘要： 
 |------------------------------------------------------------------------------
 | 当前版本： 1.1
 | 作   者： Mingliang.Lu
 | 完成时间：2014-07-15 
 |------------------------------------------------------------------------------
 | 原版本 ： 1.0
 | 作  者：  MingLiang.lu
 | 完成时间：2013-05-20
 |------------------------------------------------------------------------------
 | 若有问题，可联系: linux.lucian@gmail.com
 |------------------------------------------------------------------------------
 */

//#include "myCPU.h"
#include "cf8051.h"
#include "s3c44b0x.h"
#include "bsp_timer2.h"
#include "bsp_flash.h"

OS_PORTLOST    ports = {{'\0'},{'\0'},{'\0'},{'\0'},{'\0'},0};
OS_BOARDINFO   board = {0x100,0x100,0x0};
OS_ALARMINFO   alarm = {0x0004,0x0000,0x0,{'\0'}};
OS_RESOURCE    reSource = {0x0,0x0,0x0,0x0,{0x7e,0x08,0x8d,0x0d,0x0}};

uint8_t poll_410_cmd[8] = {0x7e,0x00,0x06,0x20,0x01,0x00,0x00,0x5a};
uint8_t frame_stats[10] = {0x7e,0x00,0x08,0x09,0x01,0x00,0x00,0x00,0x00,0x5a};

volatile uint8_t T_type[8] = {0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C}; //zhuchengzhi2017-01-25
volatile uint16_t lost_packet[8] = {'\0'};

extern volatile uint8_t HUB_output_alarm;
extern volatile uint8_t HUB_output;

extern uint8_t flashinfo[30];

extern volatile uint8_t read_version;
extern volatile uint8_t updating_flags;
extern volatile uint8_t v_zd_whitch;


uint8_t check_0xA6_count[6] = {'\0'};

volatile uint8_t save_order_s = 0;


static OS_UART* detect_uarts(uint8_t uartnum);
static void cf8051_boards_info_handler(uint8_t *src,uint8_t com);
static uint8_t cf8051_recv_data_crc_check(uint8_t *src);
static void communication_logs(uint8_t communication_port,uint8_t connect_times, uint8_t cf8051_tray_number,uint8_t cf8051_board_number);
static void cf8051_send_data_pthread(uint8_t serivced_serial,uint8_t com_n,uint8_t communications);
static int  cf8051_recv_handle_pthread(OS_UART* uart, uint8_t serivced_serial,uint8_t com_n);

//------------------------------------------------------------------------------
//          System application Sim3U1xxx devices families.(cf8051 parts)
//------------------------------------------------------------------------------

/**
  * @brief  接口板对单元板的主体服务程序
  * @param   
  * @retval None.
  */
void cf8051_service_routine( void ) 
{
   static uint8_t serial = 1;
   uint8_t communication_times = 0, crc_stat = 0;
   uint8_t s_index = serial - 1, cf8051_tray_index = 0;	
   uint8_t time_stamp = 0;
//2017-08-22 zhuchnegzhi 
   uint8_t cf8051_board_index = 0;
//END
   
   OS_UART *p = detect_uarts(serial);

   //step1.发数据前，预先准备接收状态位,清除一些状态标志位
   p->sendFlag = 0;
   p->recvFlag = 0;
  
   //使能托盘的数据发送
   mySerial_enable_tx(serial);  
   cf8051_send_data_pthread(serial,s_index,communication_times);
   mySerial_enable_rx(serial);   
   
   //step3.数据发送到某个具体的托盘上后，等待托盘回复数据，当接收超时时，停止等待，等待上限每个盘120ms.
   if(reSource.reSourceNow == 0x01){
      Delay_ms(1450);
   }else{
      for(time_stamp = 0; time_stamp < 120; time_stamp++){
         if(uart1.recvFlag == 1 && uart1.recvBuff[3] == 0x20){
            mainboard_service_routine();
         }else{
//            imprecise_msdelay();//不完全精确延时1ms
							Delay_ms(11);
         }
      }
   }
  
   //step4.在经历一段时间等待后，查看是否有收到托盘回复的数据,如果有，进入处理，如果没有直接跳过
	 if((p->recvFlag)){
      p->recvFlag = 0;
      crc_stat = cf8051_recv_handle_pthread(p,serial,s_index);
	 //													轮询命令  处于APP中  	 告警命令
      if(crc_stat && 
				(((p->recvBuff[3] == 0x20) && (p->recvBuff[7] == 0x01))|| (p->recvBuff[3] == 0x82))){
						
         communication_times += 1;
         cf8051_tray_index = p->recvBuff[5];//盘号
				 cf8051_board_index = p->recvBuff[4];//框号
				 TrayInfo[6 + s_index] = s_index+1;
      }	
   }else{
			TrayInfo[6 + s_index] = 0x00;
		  lost_packet[s_index]++;  //2017-03-26添加了丢包率的检测
		  if(lost_packet[s_index] > 0xfffe){
				lost_packet[s_index] = 0x00;
			}
	 }
	 
   //step5.数据处理后期，关断这一485口上所有的操作，并做一些记录
//   mySerial_disable_tx(serial);
//   mySerial_disable_rx(serial);
   
   if((0x01 == DeviceInfo.resource) && (HUB_output_alarm == 0)){
      //如果该机框已经资源采集过了
      communication_logs(s_index,communication_times, cf8051_tray_index,cf8051_board_index);// 记录双方通信的次数
   }else{
      //do nothing 
   }

   //step6,完成与托盘的一次通信后，累加托盘编号，用于下一个托盘的访问
   serial++;
   if(serial > 8){
      serial = 1;
   }

}

/**
  * @brief  识别本框接入的盘类型有 12端口 和 17端口
  * @retval 
  */
uint8_t back_trays_types(uint8_t dat)
{
	uint8_t ret = 0;
	if(flashinfo[14 + (dat - 1)] == 0x11){
		ret = 0x11;//17个端口的
	}else{
		ret = 0x0c;//12个端口的
	}
	return ret;
}

/**
  * @brief  非常重要的一个模块，用来暂存当前单元板反馈的状态信息。
  * @retval 无 或者 不符合要求的数据
  */
static void cf8051_boards_info_handler(uint8_t *src,uint8_t com)
{
    uint8_t IsEffect = 0,i = 0,Trayidex = 0x0;  
    uint16_t srcLen = 0x0;
		static uint8_t tray_types = 0;
	  static uint8_t tray_index = 0;
		tray_types = 0;
		tray_index = 0;
	
   IsEffect = cf8051_recv_data_crc_check(src);  

   if(IsEffect){
      srcLen = src[1] << 8|src[2] + 2;
      switch(src[3]){

         case 0x07://写入电子标签
         case 0x0f://受控写入电子标签
         case 0x10://LED指示灯测试	
         case 0x13:
				 case 0x04:
               *(src + srcLen - 1) = 0x5a;
               add_a_new_node(src,DATA_NORMAL,srcLen,DATA_RECV);
         break;
				 case 0xfc://读取端口信息	
					 memcpy(&reSource.rxbuf[6], &src[6], srcLen - 8);
				 
				   if(v_zd_whitch == 3){//说明这是一个指定盘采集，如果是0说明指示读取端口信息
					   flashinfo[(*(src + 5)) - 1] = *(src + 5);           //单元板返回的盘号
					   DeviceInfo.Tary_Num[(*(src + 5)) - 1] = *(src + 5); //单元板返回的盘号
				 
					   if(srcLen == 393){ 
						   flashinfo[14 + ((*(src + 5)) - 1)] = 0x0C;//说明是12端口的单元板
					   }else{
							 flashinfo[14 + ((*(src + 5)) - 1)] = 0x11;//说明是17端口的单元板
						 }
				   }
				 break;
				 
				case 0x05://读取软硬件版本号	
					if(read_version == 1){
						memcpy(&reSource.rxbuf[55 + (48 * (src[6]-1))], &src[7], 48);
						if(src[6] == 8){
							read_version = 1;
						}
					}else{
						 *(src + srcLen - 1) = 0x5a;
						 add_a_new_node(src,DATA_NORMAL,srcLen,DATA_RECV);
					}

				 break;
				
         case 0x06://软件升级	
               if(update_trays[com-1] == 0x0)break;
               if(src[4] == 0x01){//升级启动 <-----------------------------
                  Trayidex = com;
                  cf8051update[9+(Trayidex-1)*3] = src[9];
                  update_trays_startup_status[Trayidex-1] = src[9];
                  for(i = 0;i < 8;i++){
                     if((update_trays[i] == 1) && ((update_trays_startup_status[i] == 0xe)))
                     { //如果升级标志在，但单元板的启动答复还没收齐，就不回复主控暂时
                        break;
                     }
                     
                     if(i == 7){
                        stop_timer(&update_startup_timer);	
                        cf8051update[4] = 0x01;								
                        cf8051update[33] = crc8(cf8051update,33);
                        cf8051update[34] = 0x5a;
                        add_a_new_node(cf8051update,DATA_NORMAL,35,DATA_RECV);				
                     }
                  }
               }else if(src[4] == 0x02){//升级中... <-----------------------------
                  Trayidex = src[8];
                  cf8051update[9+(Trayidex-1)*3] = src[9];
                  update_trays_packet_status[Trayidex-1] = src[9];
                  for(i = 0;i < 8;i++){
                     if((update_trays_startup_status[i] == 0x0) && ((update_trays_packet_status[i] == 0xe)))
                     { //如果升级标志在，但单元板的启动答复还没收齐，就不回复主控暂时
                        break;
                     }						
                     if(i == 7){	
                        stop_timer(&update_write_packets_timer);		
                        cf8051update[4] = 0x02;		
                        cf8051update[33] = crc8(cf8051update,33);
                        cf8051update[34] = 0x5a;
                        add_a_new_node(cf8051update,DATA_NORMAL,35,DATA_RECV);			
                     }
                  }	
               }else if(src[4] == 0x03){//升级完成，重新启动中... <-----------------------------
                  Trayidex = src[8];
                  cf8051update[9+(Trayidex-1)*3] = src[9];
                  update_trays_restart_status[Trayidex-1] = src[9];
                  for(i = 0;i < 8;i++){
                     if((update_trays_startup_status[i] == 0x0) && ((update_trays_restart_status[i] == 0xe)))
                     { //如果升级标志在，但单元板的启动答复还没收齐，就不回复主控暂时
                        break;
                     }						
                     if(i == 7){	
                        stop_timer(&update_restart_timer);		
                        cf8051update[4] = 0x03;		
                        cf8051update[33] = crc8(cf8051update,33);
                        cf8051update[34] = 0x5a;
                        add_a_new_node(cf8051update,DATA_NORMAL,35,DATA_RECV);		
                        memset(update_trays_startup_status,0,sizeof(update_trays_startup_status));//标记所有托盘是不升级的，主要在上报托盘失联起作用								
                     }
                  }
								  update_trays[Trayidex-1] = 0x00;//zhuchengzhi 2015-05-18
               }
//							 updating_flags = 0;//zhuchengzhi 2015-05-18 软件升级的时候有告警
							ports.traylost[Trayidex-1] = 0;//2017-06-15 升级失败后盘告警要能再次识别；
							ports.portstat[Trayidex-1] = 0;
         break;
         case 0x0d://工单操作
            switch(src[4]){
               case 0x02://单端新建
               case 0x03://改跳拆除							
               case 0x04://单端拆除						
               case 0x05://双端新建	
               case 0x06://双端拆除
               case 0x07://批量新建	
               case 0x08://架间A端新建一对
               case 0x09://架间A端拆除一对
               case 0x10://架间A端批量多对	
               case 0x18://架间Z端新建一对
               case 0x19://架间Z端拆除一对
               case 0x30://架间Z端批量多对
               case 0xb1://光分路器单端新建
               case 0xb2://光分路器单端新建
               case 0x8a://
               case 0xa2://取消
               case 0x0a://确认
               case 0x8c://施工中拔出	
               case 0xee:
                  *(src + srcLen - 1) = 0x5a;
                  add_a_new_node(src,DATA_NORMAL,(src[1] << 8 |src[2] + 2),DATA_RECV);	
                  break;
               case 0x01:
               case 0x11://资源采集
//								     tray_types = *(src + srcLen - 3);//获取盘类型 是12端口的还是17端口的  默认是12端口的
//								 	   //tray_types = back_trays_types(src[6]);
//										 tray_index = (*(src+6) - 1);
//							       flashinfo[14 + tray_index] = tray_types;//保持资源采集后各个托盘的类型
//                     reSource.idex = tray_index * (35 * tray_types) + 6;
//                     reSource.tray_units[tray_index] = TRAY_ONLINE;//记录下这个盘被采集过了
//                     DeviceInfo.Tary_Num[tray_index] = *(src + 6);//记下盘采集过
//                     memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35*tray_types));
							 
										//zhuchengzhi2017-01-25 TODO		
										if((srcLen <= 427)){//如果接入了没有加入盘类型的盘会导致死亡
											return ;
										}
										tray_types = *(src + srcLen - 3);//获取盘类型 是12端口的还是17端口的  默认是12端口的
										tray_index = (*(src+6) - 1);
										flashinfo[14 + tray_index] = tray_types;//保持资源采集后各个托盘的类型
										reSource.tray_units[tray_index] = TRAY_ONLINE;//记录下这个盘被采集过了
										DeviceInfo.Tary_Num[tray_index] = *(src + 6);//记下盘采集过
							 
										switch(*(src + 6)){
											case 0x01:
												memcpy(&reSource.rxbuf[6], &src[5], (35*tray_types));
												break;
											case 0x02:
												reSource.idex = (35 * T_type[0]) + 6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x03:
												reSource.idex = 35 * (T_type[0] + T_type[1]) + 6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x04:
												reSource.idex = 35 * (T_type[0] + T_type[1] + T_type[2]) +6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x05:
												reSource.idex = 35 * (T_type[0] + T_type[1] + T_type[2]+ T_type[3]) +6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x06:
												reSource.idex = 35 * (T_type[0] + T_type[1] + T_type[2]+ T_type[3] + T_type[4]) + 6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x07:
												reSource.idex = 35 * (T_type[0] + T_type[1] + T_type[2]+ T_type[3] + T_type[4]+ T_type[5]) + 6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											case 0x08:
												reSource.idex = 35 * (T_type[0] + T_type[1] + T_type[2]+ T_type[3] + T_type[4]+ T_type[5] + T_type[6]) + 6;
												memcpy(&reSource.rxbuf[reSource.idex], &src[5], (35 * tray_types));
												break;
											
											default:
												break;
										}

										break;
            }
            break;       
         case 0x82:
					  tray_types = back_trays_types(*(src + 5));//由盘类型决定分析告警数据的长度
				    if(tray_types == 0x11){
							tray_types = 23;
					  }else{
					    tray_types = 18;
					  }
            for(i = 6;i < tray_types;i++){//for(i = 6;i < 18;i++){
               if(*(src + i) != 0x0){//从该盘的一号端口到12号端口，进行挨个扫描识别，谁有问题就记录到里面去
                  alarm.data[alarm.idex++] = board.id;
                  alarm.data[alarm.idex++] = *(src + 5);
                  alarm.data[alarm.idex++] = i - 5;						
                  alarm.data[alarm.idex++] = *(src + i);
                  if(alarm.idex > 292)alarm.idex = 4; 
               }
            }
            alarm.data[0] = 0x7e; 				
            alarm.data[3] = 0x09; 	
            alarm.flag = 1;
            break;
         default:
            break;
      }
   }
}

/**
  * @brief  HUB对于工单类型的超时后的数据返回 main_cmd = 0x0D,sub_cmd = 0x9C			
  * @retval 无 或者 不符合要求的数据
  */
void cmd_0x0d_sub_cmd_xx_timeout_func(uint8_t *src, uint8_t c_tray)
{
	uint8_t sub_cmd = 0;
	uint8_t num_tray = (c_tray + 1);
	uint8_t ofs = 0;
	uint8_t len = 0,i = 0;
	uint8_t order_timer_out[60] = {'\0'};
	sub_cmd = *(src + 4);
	switch(sub_cmd){
		case 0x02://单端新建
		case 0x03://改跳
		case 0x04://单端拆除
			
		
		case 0x05://双端新建
		case 0x08://架间双端新建主机
		case 0x18://架间双端新建从机
			
		
//	case 0x41://双端发送对端时，失败
		case 0x06://双端拆除
		case 0x09://架间双端拆除主机
		case 0x19://架间双端拆除从机
		
		case 0xA2://工单取消
		case 0x0A://工单确认
			order_timer_out[6]  = sub_cmd;//第三个命令 
			if(sub_cmd == 0x0A){
				sub_cmd = save_order_s;
			}
			order_timer_out[4]  = sub_cmd;//次命令
			memcpy(&order_timer_out[7] , (src + 5) , 9);//[类型]、[A框 A盘 A口 A状态]、[Z框 Z盘 Z口 Z状态]
		  if(num_tray == src[7]){//A盘
				order_timer_out[11] = 0x0e;
			}
		  if(num_tray == src[11]){//z盘
				order_timer_out[15] = 0x0e;
			}
			ofs = 0x11;
			break;
		case 0xB1://光分路器单端新建
		  order_timer_out[4]  = sub_cmd;//次命令
			memcpy(&order_timer_out[7] , (src + 5) , 5);
			memset(&order_timer_out[12], 0 , 4);
			break;
		
		case 0x07: //批量的回退
		case 0x10: //架间批量 主机
		case 0x30: //架间批量 从机
			order_timer_out[6]  = sub_cmd;//第三个命令 
			if(sub_cmd == 0x0A){
				sub_cmd = save_order_s;
			}
			order_timer_out[4]  = sub_cmd;//次命令	
		
			len = (src[2] - 5);
			memcpy(&order_timer_out[7] , (src + 5) , len);
		  ofs = (len + 8);
		  for(i = 0;i < ((src[2] - 6)/4);i++){
				if(num_tray == src[7 + (4*i)]){
					order_timer_out[11 + (4*i)] = 0x0e;
				}
			}
			break;
		
		default:
			break;
	}
	
	order_timer_out[0]  = 0x7e;
	order_timer_out[1]  = 0x00;
	order_timer_out[2]  = ofs;//ofs = 0x11;
	order_timer_out[3]  = 0x0D;//主命令
//order_timer_out[4]  = 0x05;//次命令
	order_timer_out[5]  = 0xAc;//第三个命令
	order_timer_out[6]  = 0xAc;//第三个命令 
	
	order_timer_out[ofs-1] = 0x00;//预留状态
	order_timer_out[ofs] = crc8(order_timer_out, ofs);//CRC8
	order_timer_out[ofs+1] = 0x5A;//5A
	add_a_new_node(order_timer_out, DATA_NORMAL, (ofs+2), DATA_RECV); 
	
	save_order_s = 0;
}


/**
  * @brief  接口板对cf8051单元板的|| 接收 ||处理程序				
  * @retval 无 或者 不符合要求的数据
  */
static int cf8051_recv_handle_pthread(OS_UART* uart, uint8_t serivced_serial,uint8_t com_n)
{
   uint8_t crc_status = 0;
	 crc_status = cf8051_recv_data_crc_check(uart->recvBuff);
	 if(crc_status == CRC_OK){
			if(uart->recvBuff[3] == 0x20){
					if(uart->recvBuff[6] == 0x11){
							T_type[uart->recvBuff[5] - 1] = 0x11;
					}else{
							T_type[uart->recvBuff[5] - 1] = 0x0C;
					}
					return 1;
			 }
			
      //单元板返回的数据crc正确
      if(((uart->recvBuff[3] != 0x20) && (uart->recvBuff[3] != 0xf6) )&& (uart->recvBuff[3] != 0x00)){
         if(uart->recvBuff[3] == 0xa6){
            memset(&uCom_send_dataBase[com_n][0],0,COM_TX_SIZE);
            memset(uart->recvBuff,0,sizeof(uart->recvBuff));
         }else{
            if(0x01 == DeviceInfo.resource || reSource.reSourceNow == 0x1||uart->recvBuff[3] == 0x05|| \
							uart->recvBuff[3] == 0x06||uart->recvBuff[3] == 0xfc||uart->recvBuff[3] == 0x04)
            {//说明是资源采集过的
							 //mySerial_send_string(serivced_serial,frame_is_right);							//发确认收到单元板数据信息给cf8051
               if(ports.portstat[com_n] == PORT_RUNNING_NORMAL || reSource.reSourceNow == 0x1|| \
									uart->recvBuff[3] == 0x05||uart->recvBuff[3] == 0x06|| \
									uart->recvBuff[3] == 0xfc||uart->recvBuff[3] == 0x04)
               {
                  //如果这时是正常运行的盘，就响应，如果不是正常运行的有失联的，就不care
                  cf8051_boards_info_handler(uart->recvBuff,serivced_serial);
               }else{
                  return 1;
               }
            }else{//说明本框还没有被采集,不返回任何数据给主控
               memset(uart->recvBuff,0,sizeof(uart->recvBuff));
            }
            mySerial_send_string(serivced_serial,frame_is_right);							//发确认收到单元板数据信息给cf8051
         }
      }else if( uart->recvBuff[3] == 0xf6 ){
         if(uCom_send_dataBase[com_n][3] != 0x0){
            mySerial_send_string(serivced_serial,&uCom_send_dataBase[com_n][0]);
         }else{
            mySerial_send_string(serivced_serial,poll_410_cmd);
         }
      }else{
         memset(uart->recvBuff,0,sizeof(uart->recvBuff));
      }
      
      return 1; 
	 }else{
      //单元板返回的数据crc不正确，即刻清楚掉接收到的数据
      //mySerial_send_string(serivced_serial,frame_is_wrong);	//2017-05-08 zhuchengzhi
      //uart->sendFlag = 1;
      
			TrayInfo[6 + serivced_serial - 1] = 0x00;
			lost_packet[serivced_serial - 1]++;  //2017-03-26添加了丢包率的检测
			if(lost_packet[serivced_serial - 1] > 0xfffe){
				lost_packet[serivced_serial - 1] = 0x00;
			}
			
			memset(uart->recvBuff,0,sizeof(uart->recvBuff)); //2017-05-08 zhuchengzhi

      return 0;
	 }
}


/**
  * @brief  接口板对cf8051单元板的接收处理程序
  * @retval 无 或者 不符合要求的数据
  */
static void cf8051_send_data_pthread(uint8_t serivced_serial,uint8_t com_n,uint8_t communications)
{

   if(uCom_send_dataBase[com_n][3] != 0x0)
   {
      mySerial_send_string(serivced_serial,&uCom_send_dataBase[com_n][0]);

   }else{
      poll_410_cmd[4] = board.id;
      poll_410_cmd[5] = serivced_serial;
      poll_410_cmd[6] = crc8(poll_410_cmd,6);
      mySerial_send_string(serivced_serial,poll_410_cmd);
   }
}

/**
  * @brief  快速转发数据给单元板
  * @retval none
  */
void repost_update_packet(void)
{
   uint8_t unit_board = 1,loop_times = 0;	

   for(loop_times = 0; loop_times < 18;loop_times++){  //18/6个盘 = 3次/盘		
      if(uCom_send_dataBase[unit_board-1][3] != 0x0){
         OS_UART *p = detect_uarts(unit_board);
         p->sendFlag = 0;
         p->recvFlag = 0;
         
         mySerial_enable_tx(unit_board);  
         mySerial_enable_rx(unit_board);   
         
         if(!p->sendFlag){
            mySerial_send_string(unit_board,&uCom_send_dataBase[unit_board-1][0]);
            p->sendFlag = 1;
         }
         Delay_ms(1450);//等待上面的接收标志位超时判定产生

         if(p->recvFlag){
            p->recvFlag = 0;
            if(p->recvBuff[3] == 0xa6){
               memset(&uCom_send_dataBase[unit_board-1][0],0,COM_TX_SIZE);
            }
         }
         
         mySerial_disable_tx(unit_board);  
//         mySerial_disable_rx(unit_board);           

      }//end of if(uCom_send_dataBase[unit_board_idex][3] != 0x0)
      
      unit_board += 1;
      if(unit_board > 8)unit_board = 1;
   }

   for(unit_board = 0; unit_board < 8;unit_board++)
         memset(&uCom_send_dataBase[unit_board][0],0,COM_TX_SIZE);	

}

/**
  * @brief  快速转发数据给单元板
  * @retval none
  */
void repost_data_to_cf8051_immediately(void)
{
   uint8_t unit_board = 0, out_loop = 0;	

   for(out_loop = 0; out_loop < 5; out_loop++){ //2017-07-11 zhuchenzhi 修改的循环
      for(unit_board = 1; unit_board < 9; unit_board++){     
         if(uCom_send_dataBase[unit_board-1][3] != 0x0){
            OS_UART *p = detect_uarts(unit_board);
            p->sendFlag = 0;
            p->recvFlag = 0;
            
            mySerial_enable_tx(unit_board);  
            mySerial_enable_rx(unit_board);         
             
            if(!p->sendFlag){
               mySerial_send_string(unit_board, &uCom_send_dataBase[unit_board-1][0]);
               p->sendFlag = 1;
            }
            
            Delay_ms(1450);//等待上面的接收标志位超时判定产生
            
            if(p->recvFlag){
               p->recvFlag = 0;
               if(p->recvBuff[3] == 0xa6){
                  if(uCom_send_dataBase[unit_board-1][3] == 0x0d && uCom_send_dataBase[unit_board-1][4] == 0xa6){
                     Delay_ms(450);//等待上面的接收标志位超时判定产生                           
                  }
                  memset(&uCom_send_dataBase[unit_board-1][0],0x0, COM_TX_SIZE);
               }else{
                  p->sendFlag = 0;
               }
            }
            
            mySerial_disable_tx(unit_board);  
//            mySerial_disable_rx(unit_board);   
            
         }//end of if(uCom_send_dataBase[unit_board_idex][3] != 0x0)   
      }
   }
   
	for(unit_board = 1; unit_board < 9;unit_board++){
		if(uCom_send_dataBase[unit_board-1][3] == 0x0D){//对于工单命令需要超时返回
				cmd_0x0d_sub_cmd_xx_timeout_func(&uCom_send_dataBase[unit_board-1][0],unit_board-1);//准备上报给ODF某条工单下发失败
				memset(&uCom_send_dataBase[unit_board-1][0],0, COM_TX_SIZE);  //清除数据发送数组，准备发送轮询
				check_0xA6_count[unit_board-1] = 0;													 //清除发送失败计数

		}
	}	 
	 
   for(unit_board = 0; unit_board < 8;unit_board++)
      memset(&uCom_send_dataBase[unit_board][0],0,COM_TX_SIZE);
}

/**
  * @brief  usart0接收数据处理函数.
  * @retval None.
  */
static OS_UART* detect_uarts(uint8_t uartnum)
{
   OS_UART* p = NULL;

   switch(uartnum){
      case 8:
         p = &uart0;          
      break;
      case 7:
         p = &usart1;          
      break;
      case 6:
         p = &pca1;           
      break;
      case 5:
         p = &pca0;
      break;
      case 4:
         p = &epca2;         
      break;
      case 3:
         p = &epca1;
      break;       
      case 2:
         p = &epca0;       
      break;
      case 1:
         p = &usart0;
      break;
      default:
         break;		
   }
   
   return p;
}

/**
  * @brief  cf8051 port crcCheck~
  * @retval 无 或者 不符合要求的数据
  */
static uint8_t cf8051_recv_data_crc_check(uint8_t *src)
{
   uint16_t calced_len = 0;
   uint8_t  calced_crc = 0;
   uint8_t  calced_user = 0;
   
   if((src == NULL) || (*(src + 0) == 0x00))return 0;//程序健壮性考虑，指针检查是基本
   calced_len = ((*(src + 1) << 8)| (*(src + 2)));

   if(calced_len > 700){
         return 0;
   }else{
         calced_crc = crc8(src,calced_len);
         calced_user = *(src + calced_len);
         if((calced_crc == calced_user) && (*(src + 0) == 0x7E) && (*(src + calced_len + 1) == 0x5A)){
            return 1;
         }else{
            return 0;
         }
   }
}

/**
  * @brief  托盘的非法插入 命令字09 子命令04
  * @retval 无 或者 不符合要求的数据
  */
static void clear_cache_data(uint8_t serial_port)
{
   /* 如果有判断到失联的话，
      但没有满足这个失联上报条件就恢复了的*/
   if(ports.traylost[serial_port] > 0){
      ports.traylost[serial_port] = 0;
		  ports.portstat[serial_port] = 0;
    }
}

/**
  * @brief  托盘的非法插入 命令字09 子命令04
  * @retval 无 或者 不符合要求的数据
  */
static void alarm_module_tray_input_illeagle(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number)
{
   if(ports.illegalaccess[communication_port] > TIMES_20){
      return ;     
   }else if(ports.illegalaccess[communication_port] == TIMES_20){
      alarm.data[alarm.idex++] = board.id;
      alarm.data[alarm.idex++] = communication_port + 1;
      alarm.data[alarm.idex++] = 0x00;
      alarm.data[alarm.idex++] = 0x04;
      alarm.data[0] = 0x7e; 
      alarm.data[3] = 0x09; 
      alarm.flag = 1;
      if(alarm.idex > 292)alarm.idex = 4;  
      ports.portstat[communication_port] |= ILLEGAL_ACCESS;
   }else{
      ports.illegalaccess[communication_port] += 1;
   }
}

/**
  * @brief  托盘的非法插入的恢复
  * @retval 无 或者 不符合要求的数据
  */
static void alarm_module_tray_input_illeagle_return_to_normal(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number)
{
   if(ports.illegalaccess_recover[communication_port] < 3){
      ports.illegalaccess_recover[communication_port] += 1;
   }else if(ports.illegalaccess_recover[communication_port] == 3){
      alarm.data[alarm.idex++] = board.id;
      alarm.data[alarm.idex++] = communication_port + 1;
      alarm.data[alarm.idex++] = 0x00;
      alarm.data[alarm.idex++] = 0x0C;
      alarm.data[0] = 0x7e; 
      alarm.data[3] = 0x09; 
      alarm.flag = 1;
      if(alarm.idex > 292)alarm.idex = 4; 
      
      ports.portstat[communication_port] &= ~ILLEGAL_ACCESS; 
      
      ports.illegalaccess_recover[communication_port] = 0;
      ports.illegalaccess[communication_port] = 0;      
   }else{
      return ;
   }
}


/**
  * @brief  托盘的非法拔出
  * @retval 无 或者 不符合要求的数据
  */
static void alarm_module_tray_output_illeagle(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number)
{
   if(ports.traylost[communication_port] > TIMES_20){
      return;
   }else if(ports.traylost[communication_port] == TIMES_20){
      ports.traylost[communication_port] = TIMES_20 + 1;
      alarm.data[alarm.idex++] = board.id;
      alarm.data[alarm.idex++] = communication_port + 1;
      alarm.data[alarm.idex++] = 0x00;
      alarm.data[alarm.idex++] = 0x03;
      alarm.data[0] = 0x7e; 
      alarm.data[3] = 0x09; 
      alarm.flag = 1;
      if(alarm.idex > 292)alarm.idex = 4; 
      ports.portstat[communication_port] |= TRAY_LOST;
   }else{
      ports.traylost[communication_port] += 1;
   }   
}

/**
  * @brief  托盘的非法拔出的恢复
  * @retval 无 或者 不符合要求的数据
  */
static void alarm_module_tray_output_illeagle_return_to_normal(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number)
{
   if(ports.traylost_recover[communication_port] < TIMES_3){
      ports.traylost_recover[communication_port] += 1;   
   }else if(ports.traylost_recover[communication_port] == TIMES_3){
      alarm.data[alarm.idex++] = board.id;
      alarm.data[alarm.idex++] = communication_port + 1;
      alarm.data[alarm.idex++] = 0x00;
      alarm.data[alarm.idex++] = 0x0d;
      alarm.data[0] = 0x7e; 
      alarm.data[3] = 0x09; 
      alarm.flag = 1;
      if(alarm.idex > 292)alarm.idex = 4; 
      ports.portstat[communication_port] &= ~TRAY_LOST; 
      
      ports.traylost_recover[communication_port] = 0;
      ports.traylost[communication_port] = 0;      
   }else{
      return;
   }
}

/* 集线器与单元板没有数据应答,此处有2种可能:
   1.这个接线器端口上次采集时候,没有盘接入，当前处于未接入状态,
     如果该盘之前没有非法插入的现象,那就不管
   2.如果这个集线器端口也没有盘非法插入,那就不管了，正常运作中     
   */
static void alarm_module_tray_disconnect(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number)
{
   if(DeviceInfo.Tary_Num[communication_port] == 0xff){
      /* 该盘没有采集 有2种情况：
         1.一直都稳定运行，没有接入任何盘的动作
         2.之前有非法接入了其他盘，而此刻，正处于非法接入的恢复中...
      */
      if(ports.portstat[communication_port] & TRAY_ILLEAGLE_INPUT_NOWING){
         /* 非法接入计数加一 */
         alarm_module_tray_input_illeagle_return_to_normal(communication_port,
                                          connect_times,cf8051_tray_number);
      }
			else
			{
				if(ports.portstat[communication_port] & TRAY_LOST){ //非法拔出的恢复 2017-04-20 zhuchengzhi
						alarm_module_tray_output_illeagle_return_to_normal(
									 communication_port,
									 connect_times, 
									 cf8051_tray_number); 
				}
			}
   }else{
      /* 该盘没有采集 有2种情况：
         1.该托盘可能正在处于即将失联状态...
         2.之前有非法接入了其他盘，而此刻，正处于非法接入的恢复中...
      */ 
      if(ports.portstat[communication_port] & TRAY_ILLEAGLE_INPUT_NOWING){
         /* 非法接入计数加一 */
         alarm_module_tray_input_illeagle_return_to_normal(
            communication_port,
            connect_times,
            cf8051_tray_number);      
      }else if(ports.portstat[communication_port] & TRAY_LOST){
         /* 已经失联了,就不用管了*/
      }else{
         /* 这即将失联了... */
         alarm_module_tray_output_illeagle(
            communication_port,
            connect_times,
            cf8051_tray_number);       
      }       
   }

}

static void alarm_module_tray_connect(
         uint8_t communication_port,
         uint8_t connect_times, 
         uint8_t cf8051_tray_number,
				 uint8_t cf8051_board_number)
{
   //这里只有2种情况，一，正常通讯，二，非法接入外盘
   if(DeviceInfo.Tary_Num[communication_port] == 0xff){
   /* 采集时没盘，现在却有数据，说明了有非法接入了...*/
      if(ports.portstat[communication_port] 
         & TRAY_ILLEAGLE_INPUT_NOWING)
      {
      /* 该托盘以及处于非法接入中了,就不管它...*/

      }else{
      /* 托盘可能处于非法插入中了,告警类型0x04...
         调用托盘非法接入处理函数*/         
      alarm_module_tray_input_illeagle(
         communication_port,
         connect_times, 
         cf8051_tray_number);
      }
   }else{ 
   /* 这里有2种情况：
      1.通讯中,收到的单元板返回的框号==当然询问的框号
      2.通讯中,但收到单元板返回的框号!=当前询问的框号,这种情况要报警非法接入*/

      if(ports.portstat[communication_port] 
         & TRAY_ILLEAGLE_INPUT_NOWING )
      {
         /* 非法接入中，什么都不干... */
         if((DeviceInfo.Tary_Num[communication_port] == cf8051_tray_number)&&
					  (cf8051_board_number == board.id)) //2017-08-22 zhuchengzhi 添加由于框号不一样导致盘告警有问题
         {
            /* 托盘可能处于非法插入中了,告警类型0x04...
               调用托盘非法接入处理函数*/         
            alarm_module_tray_input_illeagle_return_to_normal(
                  communication_port,
                  connect_times, 
                  cf8051_tray_number);                                 
         }         
      }else if(ports.portstat[communication_port] 
         & TRAY_LOST)
      {
         if((DeviceInfo.Tary_Num[communication_port] != cf8051_tray_number) ||
					  (cf8051_board_number != board.id))//2017-08-22 zhuchengzhi 添加由于框号不一样导致盘告警有问题
         {
            /* 托盘可能处于非法插入中了,告警类型0x04...
               调用托盘非法接入处理函数*/         
            alarm_module_tray_input_illeagle(
                  communication_port,
                  connect_times, 
                  cf8051_tray_number);                                 
         }else{
            /* 非法拔出的恢复中...*/
            alarm_module_tray_output_illeagle_return_to_normal(
               communication_port,
               connect_times, 
               cf8051_tray_number);             
         }
      }else{
         if((DeviceInfo.Tary_Num[communication_port] != cf8051_tray_number) ||
					   (cf8051_board_number != board.id))//2017-08-22 zhuchengzhi 添加由于框号不一样导致盘告警有问题
         {
            /* 托盘可能处于被非法拔出，而且立刻又非法插入别的盘,告警类型0x04...
               调用托盘非法接入处理函数*/         
            alarm_module_tray_input_illeagle(
                  communication_port,
                  connect_times, 
                  cf8051_tray_number);                                 
         }else{
            /* 当没有检测到失联，也没有检测到非法插入的时候，观察缓冲区中
               如果有计数的，及时将清空*/
            clear_cache_data(communication_port);
         }       
      }            
   }

}


/**
  * @brief  用来记录通信的双方，一问一答是不是正常，若有问而没答的便要记录
  * @retval 无 或者 不符合要求的数据
  */

static void communication_logs(
            uint8_t communication_port,
            uint8_t connect_times, 
            uint8_t cf8051_tray_number,
						uint8_t cf8051_board_number)
{
   /* 如果当前在执行资源采集，那么直接返回，
      因为资源采集的时候是不上报任何告警信息 
      如果这个盘正在升级中...
      可能在重启或者正在进入bootloader，
      短暂失联不上报告警 */
   if((reSource.reSourceNow == 0x1) 
      || (cf8051update[9+communication_port*3] == 0xf)
      || (cf8051update[9+communication_port*3] == 0x1)
	    || (update_trays[communication_port] == 0x01)
      )
   {
		  clear_cache_data(communication_port);
      return;
   }

   if(connect_times == 0){
      /* 本次通信无应答 */   
      alarm_module_tray_disconnect(
          communication_port,
          connect_times, 
          cf8051_tray_number);     
   }else{
      /* 本次通信有应答 */ 
      alarm_module_tray_connect(
          communication_port,
          connect_times, 
          cf8051_tray_number,
					cf8051_board_number); 
   }

}

/*
@更新历史：
   2016年11月3日 17:31:05 增加了communication_logs的判断条件
                          只有在巡检命令时，采取判断是否出现了失联或者接入

   2016年11月4日 16:30:16 放弃昨日的修改，换成今天的这种告警类型处理
*/


// -------------------------------------------------eof----------------------------------------------------


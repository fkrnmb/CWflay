#pragma once
#include "stdint.h"

/*USB端口*/
struct USBProt
{
	//发送
	void(*Write)(uint8_t*,uint32_t);
	//接收
	void (*Read)(uint8_t*,uint32_t);
};
/*USB端口*/

/*串口*/
struct UartpROT
{
	//发送
	bool(*Write)(uint8_t*,uint16_t);
	//接收
	bool(*Read)(uint8_t*,uint16_t);
	//更改波特率
	void(*buand)(uint32_t);
};
/*串口*/

#define BSKLINK_MAX_PAYLOAD_LENGTH 100

//传感器校准命令
typedef struct
{
    uint8_t type;						//传感器类型
    uint8_t caliFlag;				//校准标志位
    uint8_t successFlag;    //成功标志位
    uint8_t step;						//校准步骤
} BSKLINK_PAYLOAD_SENSOR_CALI_CMD_t;

typedef struct
{
    uint8_t head1;                                 //帧头
    uint8_t head2;                                 //帧头
    uint8_t deviceid;                              //设备ID
    uint8_t sysid;								     						 //系统ID
    uint8_t msgid;                                 //消息ID
    uint8_t length;                                //数据负载长度
    uint8_t payload[BSKLINK_MAX_PAYLOAD_LENGTH];   //数据负载
    uint8_t checksum;                              //校验和

    uint8_t recvStatus;                            //解析状态
    uint8_t payloadRecvCnt;                        //负载接收计数
} mavlink_heartbeat_t;

//心跳包
typedef struct
{
    uint8_t  type;							//硬件类型
    uint8_t  version_high;			//飞控版本号高位
    uint8_t  version_mid;				//飞控版本号中位
    uint8_t  version_low;				//飞控版本号低位
    int32_t  time;          		//系统时间 单位：毫秒
    uint16_t freq;							//最大发送频率
} BSKLINK_PAYLOAD_HEARTBEAT_t;

enum
{
    BSKLINK_MSG_ID_FLIGHT_DATA      = 0x01,     //基本飞行数据
    BSKLINK_MSG_ID_FLIGHT_STATUS    = 0x02,     //飞控状态信息
    BSKLINK_MSG_ID_SENSOR           = 0x03,     //传感器数据
    BSKLINK_MSG_ID_SENSOR_CALI_DATA = 0x04,			//传感器校准数据
    BSKLINK_MSG_ID_SENSOR_CALI_CMD  = 0x05,			//传感器校准命令
    BSKLINK_MSG_ID_RC_DATA          = 0x08,     //遥控通道数据
    BSKLINK_MSG_ID_MOTOR            = 0x09,     //电机输出
    BSKLINK_MSG_ID_BATTERY          = 0x0A,     //电池信息
    BSKLINK_MSG_ID_PID_ATT          = 0x10,     //姿态PID参数
    BSKLINK_MSG_ID_PID_POS          = 0x11,     //位置PID参数
    BSKLINK_MSG_ID_PID_ACK          = 0x12,     //PID读写响应
    BSKLINK_MSG_ID_SETUP            = 0x15,     //飞控设置
    BSKLINK_MSG_ID_GPS              = 0x20,     //GPS数据
    BSKLINK_MSG_ID_SYS_ERROR 		= 0x25,					//系统错误信息
    BSKLINK_MSG_ID_SYS_WARNING 		= 0x26,				//系统警告信息
    BSKLINK_MSG_ID_ATT_ANALYSE      = 0x30,     //姿态估计与控制数据
    BSKLINK_MSG_ID_VEL_ANALYSE      = 0x31,     //速度估计与控制数据
    BSKLINK_MSG_ID_POS_ANALYSE      = 0x32,     //位置估计与控制数据
    BSKLINK_MSG_ID_USER_DEFINE      = 0x33,     //自定义数据
    BSKLINK_MSG_ID_FREQ_SETUP       = 0xF0,     //消息发送频率设置
    BSKLINK_MSG_ID_HEARTBEAT		= 0xFE					//心跳包
}; 

bool PortRegisterUAST(UartpROT p,uint8_t i);
bool PortRegisterUsb(USBProt p,uint8_t i);
bool CommunLinkUSBSend(uint8_t i,uint8_t*buf,uint32_t len);
bool cOMMLINKuasrT(uint8_t i,uint8_t*buf,uint16_t len);
bool usbrex(uint8_t i,uint8_t*buf,uint32_t len);
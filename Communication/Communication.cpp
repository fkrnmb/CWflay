#include "Communication.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "mavlink.h"
#include <stdlib.h>
#include "Uart1.hpp"


#define MAX_SEND_FREQ   100    
//mavlink的心跳包ID为0，无法参与发送列表排序，故为其重新定义一个ID（仅用于参与排序）
#define MAVLINK_MSG_ID_HEARTBEAT2   180
enum
{
    BOARD_BLUESKY_V3 = 1,
    BOARD_FUTURESKY
};

/*USB端口*/
static USBProt port[3] = {0};
/*USB端口*/
/*串口端口*/
static UartpROT uport[10] = {0};
/*串口端口*/

bool PortRegisterUAST(UartpROT p,uint8_t i)
{
	if(i ==0 || i>= 10)
		return false;
	if(uport[i].Write !=0 ||uport[i].Read != 0)
		return false;
	
	uport[i] = p;
	return true;
}

//注册
bool PortRegisterUsb(USBProt p,uint8_t i)
{
	if(i ==0 || i>= 3)
		return false;
	if(port[i].Write !=0 ||port[i].Read != 0)
		return false;
	
	port[i] = p;
	return true;
}

bool cOMMLINKuasrT(uint8_t i,uint8_t*buf,uint16_t len)
{
	if(i < 0||i>=3)
		return false;
	uport[i].Write(buf,len);
	return true;
}

//测试调用端口
bool CommunLinkUSBSend(uint8_t i,uint8_t*buf,uint32_t len)
{
	if(i < 0||i>=3)
		return false;
	port[i].Write(buf,len);
	
	return true;
}
bool usbrex(uint8_t i,uint8_t*buf,uint32_t len)
{
		if(i < 0||i>=3)
		return false;
	port[i].Read(buf,len);
		return true;
}

//通讯类型
enum MESSAGE_TYPE
{
    UNKNOWN = 0,
    BSKLINK = 1,
    MAVLINK = 2
};


uint8_t bsklinkSendFlag[0xFF];	            //发送标志位
uint8_t bsklinkSendFreq[0xFF];	            //发送频率
uint8_t bsklinkSortResult[0xFF];            //发送频率排序
uint8_t bsklinkSendList[MAX_SEND_FREQ];     //发送列表

uint8_t mavlinkSendFlag[0xFF];	            //发送标志位
uint8_t mavlinkSendFreq[0xFF];	            //发送频率
uint8_t mavlinkSortResult[0xFF];            //发送频率排序
uint8_t mavlinkSendList[MAX_SEND_FREQ];     //发送列表

//定义通信协议类型，根据接收到的数据帧进行自动检测
enum MESSAGE_TYPE messageType = UNKNOWN;
BSKLINK_PAYLOAD_SENSOR_CALI_CMD_t sensorCali; 


static int32_t GetRandom(void)
{
    srand(0);
    return rand();
}



/**********************************************************************************************************
*函 数 名: SendFreqSort
*功能说明: 根据消息发送频率来给消息ID排序
*形    参: 排序结果数组指针 发送频率数组指针
*返 回 值: 无
**********************************************************************************************************/
static void SendFreqSort(uint8_t* sortResult, uint8_t* sendFreq)
{
    uint8_t i = 0, j = 0;
    uint8_t temp;

    //先初始化消息ID排序序列
    for(i = 0; i<0xFF; i++)
        sortResult[i] = i;

    //开始按照发送频率来给消息ID排序
    for(i=0; i<0xFF; i++)
    {
        for(j=i+1; j<0xFF; j++)
        {
            if(sendFreq[sortResult[j]] > sendFreq[sortResult[i]])
            {
                temp = sortResult[i];
                sortResult[i] = sortResult[j];
                sortResult[j] = temp;
            }
        }
    }

    //除了第一个，其它改为倒序，目的是为了让所有数据帧发送尽可能均匀
    uint8_t validNum = 0;
    for(i=0; i<0xFF; i++)
    {
        if(sendFreq[sortResult[i]] != 0)
            validNum++;
    }

    validNum -= 1;

    for(i=1; i<=validNum/2; i++)
    {
        temp = sortResult[i];
        sortResult[i] = sortResult[validNum + 1 - i];
        sortResult[validNum + 1 - i] = temp;
    }
}

/**********************************************************************************************************
*函 数 名: SendListCreate
*功能说明: 根据各消息帧的发送频率自动生成发送列表
*形    参: 发送频率数组指针 排序结果数组指针 发送列表数组指针
*返 回 值: 无
**********************************************************************************************************/
static void SendListCreate(uint8_t* sendFreq, uint8_t* sortResult, uint8_t* sendList)
{
    uint8_t sendNum = 0;
    uint8_t i, j;
    static float interval;
    uint8_t random;

    //判断总发送量是否超出最大发送频率，若超过则退出该函数
    for(i=0; i<0xFF; i++)
    {
        if(sendFreq[sortResult[i]] == 0)
            break;

        sendNum += sendFreq[sortResult[i]];
    }
    if(sendNum > MAX_SEND_FREQ)
        return;

    //清空发送列表
    for(i=0; i<MAX_SEND_FREQ; i++)
        sendList[i] = 0;

    //开始生成发送列表
    for(i=0; i<0xFF; i++)
    {
        if(sendFreq[sortResult[i]] == 0)
            return;

        //发送间隔
        interval = (float)MAX_SEND_FREQ / sendFreq[sortResult[i]];
        //生成随机数，作为该帧数据在列表中的排序起始点，这样可以尽量使各帧数据分布均匀
        random   = GetRandom() % MAX_SEND_FREQ;

        for(j=0; j<sendFreq[sortResult[i]]; j++)
        {
            for(uint8_t k=0; k<MAX_SEND_FREQ-j*interval; k++)
            {
                if(sendList[(int16_t)(j*interval+k+random) % MAX_SEND_FREQ] == 0)
                {
                    sendList[(int16_t)(j*interval+k+random) % MAX_SEND_FREQ] = sortResult[i];
                    break;
                }
            }
        }
    }
}

/**********************************************************************************************************
*函 数 名: BsklinkMsgFormat
*功能说明: 帧格式化输出
*形    参: 消息结构体 缓存数组指针
*返 回 值: 无
**********************************************************************************************************/
void BsklinkMsgFormat(BSKLINK_MSG_t msg, uint8_t* msgTemp)
{
    //首字节为帧长度
    msgTemp[0] = msg.length + 7;

    msgTemp[1] = msg.head1;
    msgTemp[2] = msg.head2;
    msgTemp[3] = msg.deviceid;
    msgTemp[4] = msg.sysid;
    msgTemp[5] = msg.msgid;
    msgTemp[6] = msg.length;

    memcpy(msgTemp+7, msg.payload, msg.length);
    msgTemp[7+msg.length] = msg.checksum;
}
void Mavlink_SendMessage(uint8_t *data, uint16_t length)
{
	 Write1(data,length);
}
void BsklinkSendHeartBeat(uint8_t* sendFlag)
{
    mavlink_heartbeat_t msg;
    BSKLINK_PAYLOAD_HEARTBEAT_t payload;
    uint8_t msgToSend[BSKLINK_MAX_PAYLOAD_LENGTH+10];

    if(*sendFlag == 0)
        return;
    else
        *sendFlag = DISABLE;

    //数据负载填充
    payload.type = BOARD_BLUESKY_V3;
    payload.version_high = SOFTWARE_VERSION_HIGH;
    payload.version_mid  = SOFTWARE_VERSION_MID;
    payload.version_low  = SOFTWARE_VERSION_LOW;
    payload.time		 = GetSysTimeMs();
    payload.freq 		 = MAX_SEND_FREQ;

    /*********************************************消息帧赋值******************************************/
    msg.head1 	 = BSKLINK_MSG_HEAD_1;                //帧头
    msg.head2 	 = BSKLINK_MSG_HEAD_2;
    msg.deviceid = BSKLINK_DEVICE_ID;                 //设备ID
    msg.sysid 	 = BSKLINK_SYS_ID;							     //系统ID

    msg.msgid 	 = BSKLINK_MSG_ID_HEARTBEAT;                     //消息ID
    msg.length   = sizeof(BSKLINK_PAYLOAD_HEARTBEAT_t);          //数据负载长度
    memcpy(msg.payload, &payload, msg.length);                   //拷贝数据负载

    BsklinkMsgCalculateSum(&msg);                                //计算校验和
    /*************************************************************************************************/

    //消息帧格式化
    BsklinkMsgFormat(msg, msgToSend);
    //发送消息帧
    Mavlink_SendMessage(msgToSend+1, msgToSend[0]);
}
static void  Communication_Task_server(void* pvParameters)
{
	uint32_t i = 0;
	while(1)
	{
		if(messageType == BSKLINK)
		{
 			BsklinkSendHeartBeat(&bsklinkSendFlag[BSKLINK_MSG_ID_HEARTBEAT]);  
		}
	}
}

//创建通讯线程
void Init_Communication()
{
	//mavlinik出始化
	  /*初始化各帧的发送频率，各帧频率和不能超过MAX_SEND_FREQ*/
    //bsklink发送频率
    bsklinkSendFreq[BSKLINK_MSG_ID_FLIGHT_DATA]        = 10;
    bsklinkSendFreq[BSKLINK_MSG_ID_FLIGHT_STATUS]      = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_SENSOR]             = 3;
    bsklinkSendFreq[BSKLINK_MSG_ID_SENSOR_CALI_DATA]   = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_RC_DATA]            = 3;
    bsklinkSendFreq[BSKLINK_MSG_ID_MOTOR]              = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_GPS]                = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_BATTERY]            = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_ATT_ANALYSE]        = 30;
    bsklinkSendFreq[BSKLINK_MSG_ID_VEL_ANALYSE]        = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_POS_ANALYSE]        = 1;
    bsklinkSendFreq[BSKLINK_MSG_ID_USER_DEFINE]        = 0;
    bsklinkSendFreq[BSKLINK_MSG_ID_SYS_ERROR]          = 1;     //固定1Hz
    bsklinkSendFreq[BSKLINK_MSG_ID_SYS_WARNING]        = 1;     //固定1Hz
    bsklinkSendFreq[BSKLINK_MSG_ID_HEARTBEAT]          = 1;     //心跳包发送频率为固定1Hz
    //mavlink发送频率
    mavlinkSendFreq[MAVLINK_MSG_ID_SYS_STATUS]         = 1;
    mavlinkSendFreq[MAVLINK_MSG_ID_GPS_RAW_INT]        = 1;
    mavlinkSendFreq[MAVLINK_MSG_ID_ATTITUDE]           = 15;
    mavlinkSendFreq[MAVLINK_MSG_ID_LOCAL_POSITION_NED] = 10;
    mavlinkSendFreq[MAVLINK_MSG_ID_SCALED_IMU]         = 10;
    mavlinkSendFreq[MAVLINK_MSG_ID_RC_CHANNELS]        = 5;
    mavlinkSendFreq[MAVLINK_MSG_ID_HOME_POSITION]      = 1;
    mavlinkSendFreq[MAVLINK_MSG_ID_VFR_HUD]            = 10;
    mavlinkSendFreq[MAVLINK_MSG_ID_HEARTBEAT2]         = 1;     //心跳包发送频率为固定1Hz




	xTaskCreate(Communication_Task_server ,"Communication_Task_server " ,1024,NULL,3,NULL);
}


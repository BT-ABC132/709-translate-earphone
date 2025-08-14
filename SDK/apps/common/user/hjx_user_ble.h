#ifndef HJX_USER_BLE_H
#define HJX_USER_BLE_H



//
// characteristics <--> handles
//
//#define ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE 0x0003
//#define ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE 0x0006
//#define ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE 0x0008
//#define ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE 0x0009
#define ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE 0x000c
#define ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE 0x000e
#define ATT_CHARACTERISTIC_FF02_01_CLIENT_CONFIGURATION_HANDLE 0x000f









#define HJX_APP_CMD_CHECK_VERSION   0x01
#define HJX_APP_CMD_OPEN_MIC_OPUS   0x02
#define HJX_APP_CMD_CLOSE_MIC_OPUS   0x03
#define HJX_APP_CMD_SEND_OPUS_DATA   0x04///����OPUS���ݸ�APP
#define HJX_APP_CMD_PLAY_OPUS_DATA   0x05
#define HJX_APP_CMD_GET_BLE_MAC   0x06
#define HJX_APP_CMD_GET_TWS_STATS   0x07
#define HJX_APP_CMD_GET_BAT_STATS   0x08
#define HJX_APP_CMD_SEND_BAT_STATS   0x09
#define HJX_APP_CMD_APP_SEND_OPUS_FLOW   0x0A
#define HJX_APP_CMD_TEST   0xFF



#define HJX_TWS_TWO_BLE_EN///TWS同时开BLE
#define HJX_SPP_DEAL_CTRL//和APP通讯，有SPP



///#define HJX_OPUS_ENC_DEC_TEST///OPUS�Ա��Խ����

#define HJX_APP_HEAD_1    0xA5
#define HJX_APP_HEAD_2    0x5A

#define HJX_VERSION_NUMBER_1   'V'
#define HJX_VERSION_NUMBER_2   '0'
#define HJX_VERSION_NUMBER_3   '1'
#define HJX_VERSION_NUMBER_4   '.'
#define HJX_VERSION_NUMBER_5   '2'
#define HJX_VERSION_NUMBER_6   '1'



///OPUS场景
#define HJX_OPUS_STUDIO_NULL   0///
#define HJX_OPUS_STUDIO_AUDIO_AND_VIDEO   1///音视频场景
#define HJX_OPUS_STUDIO_CALL   2///通话场景
#define HJX_OPUS_STUDIO_TALKS  3///对话
#define HJX_OPUS_STUDIO_MAX    4///
/*-------------------------------------------*/
typedef struct _HJX_FLAG_
{
    u8 opus_studio_num;
    u8 tws_channel;
    u8 need_tws_connect;
    u8 opus_studio_tws_status;
}HJX_FALG;
extern HJX_FALG hjx_flag_info;



extern u8 hjx_ble_mac_buf[6];
extern u8 hjx_ble_mac_buf2[6];
extern u16 hjx_opus_packet_cnt;
extern u16 hjx_opus_data_len;
extern u8 hjx_opus_data_buf[500];

extern u8 app_send_data_buf[2000];

extern u8 hjx_tws_dis_flag;
extern u8 hjx_ble_connect_flag;

extern void hjx_open_opus_fun(u8 studio_num,u8 play_en);///ch_num:0单声道 1:立体声
extern void hjx_close_opus_all(void);
extern void hjx_play_opus_fun(u8 play_en);///1:打开 0：关闭
#endif


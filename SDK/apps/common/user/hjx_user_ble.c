#include "app_msg.h"
#include "app_main.h"
#include "hjx_user_ble.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "system/malloc.h"
#include "btstack_rcsp_user.h"
#include "ble_user.h"
#include "app_ble_spp_api.h"
#include "st_opus_enc/opus_stenc_api.h"
#include "node_uuid.h"
#include "bt_tws.h"

HJX_FALG hjx_flag_info;

extern void hjx_set_ble_work_state(ble_state_e state);
extern void hjx_check_connetion_updata_deal(hci_con_handle_t connection_handle);

u8 hjx_ble_mac_buf[6];
u8 hjx_ble_mac_buf2[6];
u16 hjx_opus_packet_cnt=0;
u16 hjx_opus_data_len=0;
u8 hjx_opus_data_buf[500]={0};
u8 app_send_data_buf[2000]={0};
u8 app_play_opus_data_buf[1000]={0};

u8 receive_app_data_buf[5000]={0};///接收到的APP数据，有的数据要保留到下包一起处理
u16 app_data_ptr=0;///APP数据接收到的位置
u16 app_data_deal_ptr=0;///APP数据处理到的位置
u16 app_data_len=0;///APP数据剩余长度

u8 hjx_tws_dis_flag=0;//1:断开TWS
u8 hjx_ble_connect_flag=0;


void hjx_flag_info_reset_fun(void)
{
    printf("---hjx_flag_info_reset_fun\n");
    hjx_flag_info.opus_studio_num = HJX_OPUS_STUDIO_NULL;
    hjx_flag_info.tws_channel = 0;
    hjx_flag_info.need_tws_connect=0;
    hjx_flag_info.opus_studio_tws_status=0;
}


void hjx_open_opus_fun(u8 studio_num,u8 play_en)///ch_num:0单声道 1:立体声
{
    printf("---hjx_open_opus_fun:%d %d\n",studio_num,play_en);

    void dev_flow_player_close(void);
    dev_flow_player_close();
    void translation_ear_recoder_close_all(void);
    translation_ear_recoder_close_all();

    if(studio_num >= HJX_OPUS_STUDIO_MAX){
        printf("-----opus_studio_OUT\n");
        return;
    }
    hjx_flag_info.opus_studio_num = studio_num;
    if(hjx_flag_info.opus_studio_num == HJX_OPUS_STUDIO_NULL){
        printf("-----opus_studio_err\n");
        return;
    }

    hjx_opus_packet_cnt=0;
    hjx_opus_data_len=0;

    int translation_ear_recoder_open_all(u8 ch_mode);
    if((studio_num == HJX_OPUS_STUDIO_AUDIO_AND_VIDEO)
    ||(studio_num == HJX_OPUS_STUDIO_CALL)
    ||(studio_num == HJX_OPUS_STUDIO_TALKS)){
        translation_ear_recoder_open_all(MIC_DAC_TO_STERO_OPUS);
        //translation_ear_recoder_open_all(MIC_TO_MONO_OPUS);
    }
    //translation_ear_recoder_open_all(MIC_TO_MONO_OPUS);
    
    int dev_flow_player_open(u8 ch_num, u16 source_uuid);
    if(play_en){
        dev_flow_player_open(1/*1*/, NODE_UUID_SOURCE_DEV0);
    }

}
void hjx_open_studio_no_opus_fun(u8 studio_num)
{
    printf("---hjx_open_studio_no_opus_fun:%d \n",studio_num);

    void dev_flow_player_close(void);
    dev_flow_player_close();
    void translation_ear_recoder_close_all(void);
    translation_ear_recoder_close_all();

    if(studio_num >= HJX_OPUS_STUDIO_MAX){
        printf("-----opus_studio_OUT\n");
        return;
    }
    hjx_flag_info.opus_studio_num = studio_num;
    if(hjx_flag_info.opus_studio_num == HJX_OPUS_STUDIO_NULL){
        printf("-----opus_studio_err\n");
        return;
    }

    hjx_opus_packet_cnt=0;
    hjx_opus_data_len=0;
#if 0
///这里不要打开，在收到OPUS数据会打开
///这里打开OPUS播放会变快，原因还没有去查，可能跟播放TWS断开提示音有关
    int dev_flow_player_open(u8 ch_num, u16 source_uuid);
    if(studio_num == HJX_OPUS_STUDIO_CALL){
        dev_flow_player_open(1/*1*/, NODE_UUID_SOURCE_DEV0);
    }
#endif
    if (get_bt_tws_connect_status()) {
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            if(studio_num == HJX_OPUS_STUDIO_CALL){
                hjx_flag_info.need_tws_connect=1;
                app_send_message(APP_MSG_TWS_DISCONNECT, 0);
            }
        }
    }
}
void hjx_close_opus_all(void)
{
    printf("---hjx_close_opus_all\n");

    hjx_flag_info.opus_studio_num = HJX_OPUS_STUDIO_NULL;///先清再关流程，APP还在发OPUS数据，可能会再次打开OPUS播放

    void translation_ear_recoder_close_all(void);
    translation_ear_recoder_close_all();

    void dev_flow_player_close(void);
    dev_flow_player_close();

    

    hjx_opus_packet_cnt=0;
    hjx_opus_data_len=0;

    if(hjx_flag_info.need_tws_connect){
        app_send_message(APP_MSG_TWS_CONNECT, 0);
    }
    hjx_flag_info.need_tws_connect=0;

    hjx_flag_info.opus_studio_tws_status=0;
    printf("---hjx_close_opus_all_end\n");
    
}
void hjx_play_opus_fun(u8 play_en)///1:打开 0：关闭
{
    printf("---hjx_play_opus_fun:%d\n",play_en);
    
    int dev_flow_player_open(u8 ch_num, u16 source_uuid);
    void dev_flow_player_close(void);

    dev_flow_player_close();
    if(play_en){
        dev_flow_player_open(1/*1*/, NODE_UUID_SOURCE_DEV0);
    }else{
        dev_flow_player_close();
    }

}


int hjx_custom_ble_data_send(u8 *data, u32 len)
{
    int ret = 0;
    int i;
    printf("custom_demo_ble_send len = %d", len);
    put_buf(data, len);
    ret = app_ble_att_send_data(rcsp_server_ble_hdl, ATT_CHARACTERISTIC_FF02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
    if (ret) {
        printf("send fail\n");
    }
    return ret;
}

u16 get_real_data_ptr(u16 ptr)
{
    u16 real_ptr=0;
    if(ptr>=sizeof(receive_app_data_buf)){
        real_ptr=ptr-sizeof(receive_app_data_buf);
    } else {
        real_ptr = ptr;
    }
    return real_ptr;
}
u8 dat_total_fun(u16 ptr, u32 len)
{
    u32 cnt=0;
    u8 total=0;
    total=0;
    for (cnt=0;cnt<len;cnt++)
    {
        total=total+receive_app_data_buf[get_real_data_ptr(ptr+cnt)];
    }
    printf("---total=0x%x \n",total);
    return total;
}
u8 send_dat_total_fun(u8 *buf, u32 len)
{
    u32 cnt=0;
    u8 total=0;
    total=0;
    for (cnt=0;cnt<len;cnt++)
    {
        total=total+buf[cnt];
    }
    //printf("---total=0x%x \n",total);
    return total;
}

void clear_app_data_fun(void)
{///清收到APP数据数组
    u16 cnt;

    for(cnt=0;cnt<sizeof(receive_app_data_buf);cnt++)
    {
        receive_app_data_buf[cnt]=0;
    }
    app_data_len = 0;
    app_data_ptr = 0;
    app_data_deal_ptr=0;
}

void app_data_receive_fun(u8 *buffer, u16 buffer_size)
{
    if(buffer_size)
    {
        if((app_data_ptr+buffer_size) >= sizeof(receive_app_data_buf))
        {
            memcpy(&receive_app_data_buf[app_data_ptr],buffer,sizeof(receive_app_data_buf) - app_data_ptr);
            //printf("#################################### \n");
            //put_buf(&receive_app_data_buf[app_data_ptr],sizeof(receive_app_data_buf) - app_data_ptr);
            //printf("#################################### \n");
            memcpy(&receive_app_data_buf[0],&buffer[sizeof(receive_app_data_buf) - app_data_ptr],(app_data_ptr+buffer_size) - sizeof(receive_app_data_buf));
            //printf("#################################### \n");
            //put_buf(&receive_app_data_buf[0],(app_data_ptr+buffer_size) - sizeof(receive_app_data_buf));
            //printf("#################################### \n");
            app_data_ptr = (app_data_ptr+buffer_size) - sizeof(receive_app_data_buf);
        }
        else
        {
            memcpy(&receive_app_data_buf[app_data_ptr],buffer,buffer_size);
            //printf("#################################### \n");
            //put_buf(&receive_app_data_buf[app_data_ptr],buffer_size);
            //printf("#################################### \n");
            app_data_ptr += buffer_size;
        }
        app_data_len += buffer_size;
        printf("----app_data_len=%d \n",app_data_len);
        //if (app_data_len >= sizeof(receive_app_data_buf))
        if (app_data_len > sizeof(receive_app_data_buf))
        {///数据超了
            printf("---app_data_buf_out \n");
            app_data_len = 0;
            app_data_ptr = 0;
            app_data_deal_ptr=0;
        }
        printf("----app_data_ptr=%d \n",app_data_ptr);
    }
}
void hjx_app_data_deal()
{
    u32 run_cnt=0;///处理的次数，大于一定次数后直接退出，防止死机
    u16 cnt=0;
    u16 dat_len=0;
    u16 ble_con_hdl=0;
    u32 play_opus_len=0;

    run_cnt=0;

    while(app_data_len)
    {
        run_cnt++;
        if(run_cnt>=sizeof(receive_app_data_buf)/*200*/)
        {///防止死机处理
            printf("---err000 \n");
            return;
        }

        if (app_data_len >= 2) {
            if((receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr)] == HJX_APP_HEAD_1)
            &&(receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+1)] == HJX_APP_HEAD_2)){///数据头对，进行下一步解析

                if (app_data_len < 5) {
                    printf("---len_err\n");
                    return;
                }
                dat_len = (receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+4)]<<8) | receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+3)];
                printf("---dat_len=%d\n",dat_len);
                if (app_data_len < dat_len+6) {
                    printf("---len_err2\n");
                    return;
                }
                if (receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+dat_len+5)] == dat_total_fun(app_data_deal_ptr,dat_len+5)) {
                    switch (receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+2)]) {
                        case HJX_APP_CMD_CHECK_VERSION:
                            if (dat_len) {
                                app_send_data_buf[0]=HJX_APP_HEAD_1;
                                app_send_data_buf[1]=HJX_APP_HEAD_2;
                                app_send_data_buf[2]=HJX_APP_CMD_CHECK_VERSION;
                                app_send_data_buf[3]=6;
                                app_send_data_buf[4]=0;
                                app_send_data_buf[5]=HJX_VERSION_NUMBER_1;
                                app_send_data_buf[6]=HJX_VERSION_NUMBER_2;
                                app_send_data_buf[7]=HJX_VERSION_NUMBER_3;
                                app_send_data_buf[8]=HJX_VERSION_NUMBER_4;
                                app_send_data_buf[9]=HJX_VERSION_NUMBER_5;
                                app_send_data_buf[10]=HJX_VERSION_NUMBER_6;
                                app_send_data_buf[11]=send_dat_total_fun(app_send_data_buf,11);
                                hjx_custom_ble_data_send(app_send_data_buf,12);
                            }
                            break;
                        case HJX_APP_CMD_OPEN_MIC_OPUS:
                            if (dat_len>=1) {
                                if(receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5)] == 1){
                                    if (tws_api_get_role() == TWS_ROLE_MASTER) {
                                        hjx_flag_info.opus_studio_tws_status=1;
								        app_send_message(APP_MSG_HJX_OPEN_MIC_OPUS, HJX_OPUS_STUDIO_CALL);
                                    }else{
                                        hjx_flag_info.opus_studio_tws_status=2;
                                        app_send_message(APP_MSG_HJX_OPEN_STUDIO_NO_OPUS, HJX_OPUS_STUDIO_CALL);                                        
                                    }
                                }else if(receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5)] == 2){
                                    if (tws_api_get_role() == TWS_ROLE_MASTER) {
                                        hjx_flag_info.opus_studio_tws_status=1;
                                        app_send_message(APP_MSG_HJX_OPEN_MIC_OPUS, HJX_OPUS_STUDIO_AUDIO_AND_VIDEO);
                                    }else{
                                        hjx_flag_info.opus_studio_tws_status=2;
                                        app_send_message(APP_MSG_HJX_OPEN_STUDIO_NO_OPUS, HJX_OPUS_STUDIO_AUDIO_AND_VIDEO);
                                    }
                                }else if(receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5)] == 3){
                                    hjx_flag_info.opus_studio_tws_status=0;
                                    app_send_message(APP_MSG_HJX_OPEN_MIC_OPUS, HJX_OPUS_STUDIO_TALKS);
                                }
                            }
                            break;
                        case HJX_APP_CMD_CLOSE_MIC_OPUS:
                            if (dat_len) {
								app_send_message(APP_MSG_HJX_CLOSE_MIC_OPUS, 0);
                            }
                            break;
                        case HJX_APP_CMD_SEND_OPUS_DATA:
                        case HJX_APP_CMD_PLAY_OPUS_DATA:
                            if (dat_len) {
                                //printf("---player:%d %d\n",dev_flow_player_runing(),hjx_flag_info.opus_studio_num);
                                if(dev_flow_player_runing() == 0){
                                    if((hjx_flag_info.opus_studio_num == HJX_OPUS_STUDIO_NULL)
                                    ||(hjx_flag_info.opus_studio_num >= HJX_OPUS_STUDIO_MAX)){

                                    }else{
                                        app_send_message(APP_MSG_HJX_PLAY_OPUS, 0);///打开OPUS解码 
                                    }                                     
                                }
                                if(dat_len>sizeof(app_play_opus_data_buf)){
                                    play_opus_len=sizeof(app_play_opus_data_buf);
                                }else{
                                    play_opus_len=dat_len;
                                }
                                for(cnt=0;cnt<play_opus_len;cnt++){
                                    app_play_opus_data_buf[cnt]=receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5+cnt)];
                                }
                                source_dev0_input_write(app_play_opus_data_buf, play_opus_len);

                                for(u32 t_cnt=0;t_cnt<play_opus_len/40;t_cnt++){
                                    if(app_play_opus_data_buf[t_cnt*40]&BIT(5)){
                                        printf("---opus_type_err:0x%x\n",app_play_opus_data_buf[t_cnt*40]);
                                    }
                                }
                                
                                {///test
                                    static u16 data_cur_pack=0;
                                    static u16 data_old_pack=0;
                                    static u32 data_total_cnt=0;
                                    data_cur_pack=receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5+dat_len+2)]<<8 | receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5+dat_len+1)];
                                    if(data_cur_pack == 1){
                                        data_total_cnt=dat_len;
                                    }else{
                                        data_total_cnt=data_total_cnt+dat_len;
                                    }
                                    printf("---opus_recieve_total_cnt:%d\n",data_total_cnt);
                                    //if(data_cur_pack != (data_old_pack+1)){
                                    if(data_cur_pack != (data_old_pack+2)){
                                        printf("-----pack:%d %d\n",data_cur_pack,data_old_pack);
                                    }
                                    data_old_pack=data_cur_pack;
                                }
                            }
                            break;
                        case HJX_APP_CMD_GET_BLE_MAC:
                            if (dat_len) {
                                if(hjx_flag_info.tws_channel == 'L'){
                                    app_send_data_buf[0]=HJX_APP_HEAD_1;
                                    app_send_data_buf[1]=HJX_APP_HEAD_2;
                                    app_send_data_buf[2]=HJX_APP_CMD_GET_BLE_MAC;
                                    app_send_data_buf[3]=13;
                                    app_send_data_buf[4]=0;
                                    app_send_data_buf[5]=receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5)];
                                    app_send_data_buf[6]=hjx_ble_mac_buf[5];
                                    app_send_data_buf[7]=hjx_ble_mac_buf[4];
                                    app_send_data_buf[8]=hjx_ble_mac_buf[3];
                                    app_send_data_buf[9]=hjx_ble_mac_buf[2];
                                    app_send_data_buf[10]=hjx_ble_mac_buf[1];
                                    app_send_data_buf[11]=hjx_ble_mac_buf[0];
                                    app_send_data_buf[12]=hjx_ble_mac_buf2[5];
                                    app_send_data_buf[13]=hjx_ble_mac_buf2[4];
                                    app_send_data_buf[14]=hjx_ble_mac_buf2[3];
                                    app_send_data_buf[15]=hjx_ble_mac_buf2[2];
                                    app_send_data_buf[16]=hjx_ble_mac_buf2[1];
                                    app_send_data_buf[17]=hjx_ble_mac_buf2[0];
                                    app_send_data_buf[18]=send_dat_total_fun(app_send_data_buf,18);
                                #ifdef HJX_SPP_DEAL_CTRL
                                    online_spp_send_data(app_send_data_buf,19);
                                #endif
                                }else{
                                    app_send_data_buf[0]=HJX_APP_HEAD_1;
                                    app_send_data_buf[1]=HJX_APP_HEAD_2;
                                    app_send_data_buf[2]=HJX_APP_CMD_GET_BLE_MAC;
                                    app_send_data_buf[3]=13;
                                    app_send_data_buf[4]=0;
                                    app_send_data_buf[5]=receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+5)];
                                    app_send_data_buf[6]=hjx_ble_mac_buf2[5];
                                    app_send_data_buf[7]=hjx_ble_mac_buf2[4];
                                    app_send_data_buf[8]=hjx_ble_mac_buf2[3];
                                    app_send_data_buf[9]=hjx_ble_mac_buf2[2];
                                    app_send_data_buf[10]=hjx_ble_mac_buf2[1];
                                    app_send_data_buf[11]=hjx_ble_mac_buf2[0];
                                    app_send_data_buf[12]=hjx_ble_mac_buf[5];
                                    app_send_data_buf[13]=hjx_ble_mac_buf[4];
                                    app_send_data_buf[14]=hjx_ble_mac_buf[3];
                                    app_send_data_buf[15]=hjx_ble_mac_buf[2];
                                    app_send_data_buf[16]=hjx_ble_mac_buf[1];
                                    app_send_data_buf[17]=hjx_ble_mac_buf[0];
                                    app_send_data_buf[18]=send_dat_total_fun(app_send_data_buf,18);
                                #ifdef HJX_SPP_DEAL_CTRL
                                    online_spp_send_data(app_send_data_buf,19);
                                #endif
                                }
                            }
                            break;
                        case HJX_APP_CMD_GET_TWS_STATS:
                            printf("----HJX_APP_CMD_GET_TWS_STATS\n");
                            app_send_data_buf[0]=HJX_APP_HEAD_1;
                            app_send_data_buf[1]=HJX_APP_HEAD_2;
                            app_send_data_buf[2]=HJX_APP_CMD_GET_TWS_STATS;
                            app_send_data_buf[3]=2;
                            app_send_data_buf[4]=0;
                            if (get_bt_tws_connect_status()) {
                                app_send_data_buf[5]=1;
                            }else{
                                app_send_data_buf[5]=0;
                            }                            
                            app_send_data_buf[6]=tws_api_get_role();
                            app_send_data_buf[7]=send_dat_total_fun(app_send_data_buf,7);
                            hjx_custom_ble_data_send(app_send_data_buf,8);
                            break;
                        case HJX_APP_CMD_GET_BAT_STATS:
                            printf("----HJX_APP_CMD_GET_BAT_STATS\n");
                            break;
                        case HJX_APP_CMD_TEST:
                            printf("----HJX_APP_CMD_TEST\n");
                            //bt_tws_role_switch();///主从切换测试
                            break;
                        default:
                            break;
                    }
                } else {
                    printf("---crc_err:0x%x 0x%x\n",receive_app_data_buf[get_real_data_ptr(app_data_deal_ptr+dat_len+5)],dat_total_fun(app_data_deal_ptr,dat_len+5));
                }

                app_data_len=app_data_len-(dat_len+6);
                app_data_deal_ptr=app_data_deal_ptr+(dat_len+6);
                if(app_data_deal_ptr>=sizeof(receive_app_data_buf))
                {
                    app_data_deal_ptr=app_data_deal_ptr-sizeof(receive_app_data_buf);
                }

            } else {///数据错误，丢掉一个byte，继续处理
                app_data_len=app_data_len-1;
                app_data_deal_ptr=app_data_deal_ptr+1;
                if(app_data_deal_ptr>=sizeof(receive_app_data_buf))
                {
                    app_data_deal_ptr=app_data_deal_ptr-sizeof(receive_app_data_buf);
                }
                printf("---cut \n");
            }

        } else {///不够数据头判断，留到下一包处理
            printf("---next \n");
            return;
        }
    }

}
#if 0
/**
* @brief 用于外部接收ble/spp自定义数据使用
*
* @param ble_con_hdl ble发送句柄
* @param remote_addr spp发送地址
* @param buf 接收数据
* @param len 接收数据的长度
* @param att_handle ble_con_hdl有值时，ble的特征值，一般是用户自定义的特征
*/
// 需要外部实现以下函数来处理自定义数据
void bt_rcsp_custom_recieve_callback(u16 ble_con_hdl, void *remote_addr, u8 *buf, u16 len, uint16_t att_handle)
{
   // custom data handle
   u16 handle = att_handle;

   printf("---bt_rcsp_custom_recieve_callback:0x%x 0x%x %d \n",ble_con_hdl,att_handle,len);

   switch (handle) {

    case ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE:
        printf("---ATT_CHARACTERISTIC_FF01_01_VALUE_HANDLE\n");
        put_buf(buf,len);
        app_data_receive_fun(buf,len);
        hjx_app_data_deal();
    #if 0///原数据发回给APP
        hjx_custom_ble_data_send(buf,len);
    #endif
    break;

    case ATT_CHARACTERISTIC_FF02_01_CLIENT_CONFIGURATION_HANDLE:
        printf("\n------write ccc:%04x,%02x\n", handle, buf[0]);
        hjx_set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        hjx_check_connetion_updata_deal(ble_con_hdl);
        att_set_ccc_config(handle, buf[0]);
    break;

    default:
    break;
   }
}
#endif






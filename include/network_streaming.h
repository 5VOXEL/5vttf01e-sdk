/*******************************************************
*
*  Copyright (c) 2019 5Voxel
*
*******************************************************/
#ifndef __NETWORK_STREAMING_HEADER__
#define __NETWORK_STREAMING_HEADER__

#define DEPTH_CAMERA_SERVER_IP    "192.168.0.100" /* Default IP */
#define DEPTH_CAMERA_SERVER_PORT  1111
#define DEPTH_FRAME_WIDTH   80
#define DEPTH_FRAME_HEIGHT  60
#define DEPTH_FRAME_PIXELS  4800 /* 80 * 60 pixels */
#define DEPTH_FRAME_SIZE    9600 /* 80 * 60 * 2Bytes */
#define IR_FRAME_SIZE       DEPTH_FRAME_SIZE
#define DEPTH_IR_FRAME_PIXELS   9600 /* 80 * 60 * 2 pixels */
#define DEPTH_IR_FRAME_SIZE 19200 /* 80 * 60 * 2 * 2Bytes */
#define CMD_RESP_LEN        128
#define RCVBUFSIZE          256   /* Size of receive buffer */
#define CMDBUFSIZE          256   /* Size of receive buffer */
#define FRAME_HEADER_START_SIZE 4 /* 0xF7 0xF7 0x00 0x00 */
#define FRAME_HEADER_START_B0   0xF7
#define FRAME_HEADER_START_B1   0xF7
#define FRAME_HEADER_START_B2   0x00  /* Reserved for future use */
#define FRAME_HEADER_START_B3   0x00  /* Reserved for future use */
#define FRAME_HEADER_END_SIZE   4 /* 0xF8 0xF8 0x00 0x00 */
#define FRAME_HEADER_END_B0     0xF8
#define FRAME_HEADER_END_B1     0xF8
#define FRAME_HEADER_END_B2     0x00  /* Reserved for future use */
#define FRAME_HEADER_END_B3     0x00  /* Reserved for future use */

#define CMD_GET_PROD_SN         "get product_sn"
#define CMD_GET_FW_VER          "get fw_ver"
#define CMD_GET_FW_BUILD_DATE   "get fw_build_date"
#define CMD_GET_MAC_ADDR        "get mac_addr"
#define CMD_CAMERA_START	    "start"
#define CMD_CAMERA_STOP	        "stop"
#define CMD_CAMERA_GET_DEPTH_FRAME      "get depthframe"
#define CMD_CAMERA_GET_IR_FRAME         "get irframe"
#define CMD_CAMERA_GET_DEPTH_IR_FRAME   "get depthirframe"
#define CMD_CAMERA_GET_H_DEPTH_IR_FRAME "get h_depthirframe"  /* With header */
#define CMD_SET_IP_ADDR         "set ip_addr"
#define CMD_SET_SUBNET_MASK     "set subnet_mask"
#define CMD_SET_DEFAULT_GATEWAY "set default_gateway"
#define CMD_SET_HOSTNAME        "set hostname"
#define CMD_SET_DNS             "set dns"
#define CMD_SET_DHCP            "set dhcp"
#define CMD_GET_HOSTNAME        "get hostname"
#define CMD_CONFIG_SAVE_REBOOT  "set save_config_n_reboot"
#define CMD_RESP_SET_OK         "Set variable OK"

enum depthcam_get_cmd_type {
    GET_PROD_SN = 0,
    GET_HOSTNAME,
    GET_FW_VER,
    GET_FW_BUILD_DATE,
    GET_MAC_ADDR,
    GET_DEPTH_FRAME,
    GET_IR_FRAME,
    GET_DEPTH_IR_FRAME,
    GET_H_DEPTH_IR_FRAME
};

enum depthcam_set_cmd_type {
    SET_IP_ADDR = 0,
    SET_SUBNET_MASK,
    SET_DEFAULT_GATEWAY,
    SET_HOSTNAME,
    SET_DNS,
    SET_DHCP,
};

#endif /* __NETWORK_STREAMING_HEADER__ */

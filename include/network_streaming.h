/*******************************************************
*
*  Copyright (c) 2019-2026 5Voxel
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
#define CMD_RESP_LEN        128
#define RCVBUFSIZE          256   /* Size of receive buffer */
#define CMDBUFSIZE          256   /* Size of receive buffer */

/*
 * Note: IR frames ("get irframe", "get depthirframe", "get h_depthirframe")
 * were removed in firmware 2.x for privacy protection. The device streams
 * depth frames only.
 */
#define CMD_GET_PROD_SN         "get product_sn"
#define CMD_GET_FW_VER          "get fw_ver"
#define CMD_GET_FW_BUILD_DATE   "get fw_build_date"
#define CMD_GET_MAC_ADDR        "get mac_addr"
#define CMD_CAMERA_START	    "start"
#define CMD_CAMERA_STOP	        "stop"
#define CMD_CAMERA_GET_DEPTH_FRAME      "get depthframe"
#define CMD_SET_IP_ADDR         "set ip_addr"
#define CMD_SET_SUBNET_MASK     "set subnet_mask"
#define CMD_SET_DEFAULT_GATEWAY "set default_gateway"
#define CMD_SET_HOSTNAME        "set hostname"
#define CMD_SET_DNS             "set dns"
#define CMD_SET_DHCP            "set dhcp"
#define CMD_GET_HOSTNAME        "get hostname"
#define CMD_CONFIG_SAVE_REBOOT  "set save_config_n_reboot"
#define CMD_RESP_SET_OK         "Set variable OK"
#define CMD_RESP_UNKNOWN        "Unknown Cmd"   /* e.g. "Unknown Cmd (State = 3)" */

enum depthcam_get_cmd_type {
    GET_PROD_SN = 0,
    GET_HOSTNAME,
    GET_FW_VER,
    GET_FW_BUILD_DATE,
    GET_MAC_ADDR,
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

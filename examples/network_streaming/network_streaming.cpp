/*******************************************************
*
*  Copyright (c) 2019-2026 5Voxel
*
*******************************************************/
#include <stdio.h>
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <WinSock.h>
#include <stdlib.h>
#include "network_streaming.h"
#include "opencv2/core/version.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

#pragma comment( lib, "ws2_32")

using namespace std;

using namespace cv;

static char frameBuff[DEPTH_FRAME_SIZE];

bool depth_camera_connect(SOCKET *sock, char *);
bool depth_camera_disconnect(SOCKET sock);
bool depth_camera_start(SOCKET sock);
bool depth_camera_stop(SOCKET sock);
bool depth_camera_get_frame(SOCKET sock, char *frame_buf, unsigned int buf_size);
bool depth_camera_get_info(SOCKET sock, unsigned int cmd);
bool depth_camera_set_info(SOCKET sock, unsigned int cmd, char *data);
bool depth_camera_reconfig_static_ip(SOCKET sock);

static bool depth_camera_config_save_n_reboot(SOCKET sock);
static bool depth_camera_send_cmd(SOCKET sock, char *camera_cmd, int len, char *cmd_resp);
static int iWaitKey = 0;

void DieWithError(char *errorMessage)  /* Error handling function */
{
    printf("%s\n", errorMessage);
    exit(0);
}

/*
 * Function name: main()
 * Description:
 *   Main funciton for network streaming depth info from camera
 * Input:
 *   argv[1] - (optional) device ip address, prompted for if omitted
 * Output:
 *   0 on exit
 */
int main(int argc, char *argv[])
{
    SOCKET sock;                             /* Socket descriptor */
    unsigned short depthFrame[DEPTH_FRAME_PIXELS];
    char *depth_fb = (char *)depthFrame;
    Mat depth = Mat(DEPTH_FRAME_HEIGHT, DEPTH_FRAME_WIDTH, CV_16UC1, Scalar(0));
    Mat depth8U = Mat(DEPTH_FRAME_HEIGHT, DEPTH_FRAME_WIDTH, CV_8UC1, Scalar(0));
    Mat colorMap = Mat(DEPTH_FRAME_HEIGHT, DEPTH_FRAME_WIDTH, CV_8UC1, Scalar(0));
    char target_device_ip[20];

    if (argc > 1) {
        strncpy_s(target_device_ip, argv[1], _TRUNCATE);
    }
    else {
        printf("Please input the IP address for the target device\n");
        printf("\nip address : ");
        fgets(target_device_ip, sizeof(target_device_ip), stdin);
        target_device_ip[strcspn(target_device_ip, "\r\n")] = 0;
    }
    printf("Connecting to %s:%d\n", target_device_ip, DEPTH_CAMERA_SERVER_PORT);

reconnect:

    if (depth_camera_connect(&sock, target_device_ip) == false) {
        fprintf(stderr, "failed to connect depth camera, retrying in 1s\n");
        Sleep(1000);
        goto reconnect;
    }

    /*
     * Optional: for getting/setting specific hostname to device
     *   Note:
     *     1. for static ip (not dhcp), hostname will be used just for storing user-specific info
     *        to device and retrieving by user
     *     2. call depth_camera_config_save_n_reboot() to store hostname to device flash
     *        otherwise, the name will be gone after power cycle device
     * 
     * Example:
     *   Get hostname:
     *        depth_camera_get_info(sock, GET_HOSTNAME); 
     *   Set hostname:
     *        depth_camera_set_info(sock, SET_HOSTNAME, "This is my device");
     *        depth_camera_config_save_n_reboot(sock);
     */

     /*
     * Optional: for setting user-specific ip/subnet/gateway to device
     *   Note:
     *     Device will reboot after storing user-specific ip address
     *     to device flash
     *
     * Example:
     *   Set static ip:
     *        depth_camera_reconfig_static_ip(sock);
     */

    depth_camera_get_info(sock, GET_HOSTNAME);

    depth_camera_get_info(sock, GET_PROD_SN);

    depth_camera_get_info(sock, GET_MAC_ADDR);

    depth_camera_get_info(sock, GET_FW_VER);

    depth_camera_get_info(sock, GET_FW_BUILD_DATE);


    if (depth_camera_start(sock) == false) {
        fprintf(stderr, "failed to start depth camera\n");
        depth_camera_disconnect(sock);
        goto reconnect;
    }

    printf("\nStart receiving frames\n");

    /*
    * Receive depth frames from the device and display them on
    * screen. Also print depth of center pixel to console
    */
    while (iWaitKey != 27) {
        memset(depthFrame, 0x0, sizeof(depthFrame));

        if (depth_camera_get_frame(sock, depth_fb, sizeof(depthFrame)) == false) {
            depth_camera_disconnect(sock);
            goto reconnect;
        }

        memcpy(depth.ptr<unsigned short>(0), depth_fb, DEPTH_FRAME_SIZE);
        printf("Depth at center pixel (x = 40, y = 30) is %d\n", depth.at<unsigned short>(DEPTH_FRAME_HEIGHT >> 1, DEPTH_FRAME_WIDTH >> 1));
        depth.convertTo(depth8U, CV_8UC1, 255.0 / 4096);
        applyColorMap(depth8U, colorMap, COLORMAP_JET);
        imshow("Depth", colorMap);

        iWaitKey = waitKey(1) & 0xFF;

        switch (iWaitKey) {
        case 'i':
            depth_camera_get_info(sock, GET_HOSTNAME);
            depth_camera_get_info(sock, GET_PROD_SN);
            depth_camera_get_info(sock, GET_MAC_ADDR);
            depth_camera_get_info(sock, GET_FW_VER);
            depth_camera_get_info(sock, GET_FW_BUILD_DATE);
            break;
        }
    }

    /*
     * Esc pressed: leave even if stop fails (reconnecting here would loop,
     * as iWaitKey is still 27)
     */
    if (depth_camera_stop(sock) == false) {
        fprintf(stderr, "failed to stop depth camera\n");
    }

    depth_camera_disconnect(sock);

    return 0;
}

/*
 * Function name:depth_camera_connect()
 * Description:
 * Input:
 *   sock - pointer of socket number to be filled by this function
 *   target_device_ip - string of device ip address to connect
 * Output:
 *   true - camera connect successfully
 *   false - failed to connect to depth camera
 */
bool depth_camera_connect(SOCKET *sock, char *target_device_ip)
{
    WSADATA wsaData;                        /* Structure for WinSock setup communication */
    struct sockaddr_in depthCameraServAddr; /* Depth camera server address */
    unsigned short depthCameraServPort;     /* Depth camera server port */
    char *servIP;                           /* Server IP address (dotted quad) */
    int timeout = 2000;                     /* timeout in 2s */

    servIP = target_device_ip;        /* First arg: server IP address (dotted quad) */

    depthCameraServPort = DEPTH_CAMERA_SERVER_PORT; /* Use given port, if any */

                                                    /*
                                                    * Load Winsock 2.0 DLL
                                                    */
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
    {
        fprintf(stderr, "WSAStartup() failed\n");
        return (false);
    }

    /*
    * Create a reliable, stream socket using TCP
    */
    if ((*sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        fprintf(stderr, "socket() failed\n");
        WSACleanup();
        return (false);
    }

    int ret = setsockopt(*sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

    if (ret == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt() failed\n");
        depth_camera_disconnect(*sock);
        return (false);
    }

    /*
     * Construct the server address structure
     */
    memset(&depthCameraServAddr, 0, sizeof(depthCameraServAddr));     /* Zero out structure */
    depthCameraServAddr.sin_family = AF_INET;                         /* Internet address family */
    depthCameraServAddr.sin_addr.s_addr = inet_addr(servIP);          /* Server IP address */
    depthCameraServAddr.sin_port = htons(depthCameraServPort);        /* Server port */

    /*
     * Establish the connection to the depth camera server
     */
    if (connect(*sock, (struct sockaddr *) &depthCameraServAddr, sizeof(depthCameraServAddr)) == SOCKET_ERROR) {
        fprintf(stderr, "connect() failed\n");
        depth_camera_disconnect(*sock);
        return (false);
    }

    return (true);
}

/*
 * Function name:depth_camera_disconnect()
 * Description:
 *   function to disconnect depth camera
 * Input:
 *   sock - socket number
 * Output:
 *   true - camera disconnect successfully
 *   false - failed to disconnect depth camera
 */
bool depth_camera_disconnect(SOCKET sock)
{
    closesocket(sock);
    WSACleanup();  /* Cleanup Winsock */

    return (true);
}

/*
 * Function name:depth_camera_start()
 * Description:
 *   function to start depth camera
 * Input:
 *   sock - socket number
 * Output:
 *   true - camera start successfully
 *   false - failed to start depth camera
 */
bool depth_camera_start(SOCKET sock)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */
    int cmdStringLen;                       /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;          /* Bytes read in single recv() and total bytes read */

    memset(cmdString, 0x0, sizeof(cmdString));
    sprintf_s(cmdString, CMD_CAMERA_START);
    cmdStringLen = (int)strlen(cmdString);

    /*
    * Send start cmd to depth camera for starting depth streaming
    */
    if (send(sock, cmdString, cmdStringLen, 0) != cmdStringLen) {
        fprintf(stderr, "send() sent a different number of bytes than expected\n");
        return (false);
    }

    /*
    * Expect to receive "Streaming started" string from the server
    */
    totalBytesRcvd = 0;
    printf("Received: ");
    while (totalBytesRcvd < CMD_RESP_LEN)
    {
        /*
        * Receive up to the buffer size (minus 1 to leave space for
        * a null terminator) bytes from the sender
        */
        if ((bytesRcvd = recv(sock, cmdRcvBuf, RCVBUFSIZE - 1, 0)) <= 0) {
            fprintf(stderr, "recv() failed or connection closed prematurely");
            return (false);
        }

        totalBytesRcvd += bytesRcvd;   /* Keep tally of total bytes */
        cmdRcvBuf[bytesRcvd] = '\0';   /* Add \0 so printf knows where to stop */
        printf("%s", cmdRcvBuf);       /* Print the receive info */
    }

    return (true);
}

/*
* Function name:depth_camera_stop()
* Description:
*   function to stop depth camera
* Input:
*   sock - socket number
* Output:
*   true - camera stop successfully
*   false - failed to stop depth camera
*/
bool depth_camera_stop(SOCKET sock)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */
    int cmdStringLen;                       /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;          /* Bytes read in single recv() and total bytes read */

    printf("\nStopping streaming\n");
    memset(cmdString, 0x0, sizeof(cmdString));
    sprintf_s(cmdString, "stop");
    cmdStringLen = (int)strlen(cmdString);

    /*
    * Send stop cmd to depth camera for stopping depth streaming
    */
    if (send(sock, cmdString, cmdStringLen, 0) != cmdStringLen) {
        fprintf(stderr, "send() sent a different number of bytes than expected\n");
        return (false);
    }

    /*
    * Expect to receive "Stop Streaming" string from the server
    */
    totalBytesRcvd = 0;
    printf("Received: ");
    while (totalBytesRcvd < CMD_RESP_LEN)
    {
        if ((bytesRcvd = recv(sock, cmdRcvBuf, RCVBUFSIZE - 1, 0)) <= 0) {
            fprintf(stderr, "recv() failed or connection closed prematurely\n");
            return (false);
        }

        totalBytesRcvd += bytesRcvd;   /* Keep tally of total bytes */
        cmdRcvBuf[bytesRcvd] = '\0';   /* Add \0 so printf knows where to stop */
        printf("%s", cmdRcvBuf);       /* Print the receive info */
    }

    printf("\n");    /* Print a final linefeed */

    return (true);
}

/*
 * Function name:depth_camera_get_frame()
 * Description:
 *   function to get one depth frame from device by sending
 *   CMD_CAMERA_GET_DEPTH_FRAME. The device answers with DEPTH_FRAME_SIZE
 *   (9600) bytes: 80 x 60 pixels, 16-bit little-endian, row by row.
 *
 *     Note:
 *       1. Call depth_camera_start() first. Without it the device answers with
 *          CMD_RESP_LEN zero bytes instead of a frame, and this function times out.
 *       2. Commands the firmware doesn't support are answered with a CMD_RESP_LEN
 *          text response starting with CMD_RESP_UNKNOWN ("Unknown Cmd (State = x)").
 *          The function detects it, prints it and returns false.
 *
 * Input:
 *   sock - socket number
 *   user_buf - pointer to user memory for storing frame data
 *   buf_size - indicate the size of user_buf
 * Output:
 *   true - get frame successfully
 *   false - failed to get depth frame from device
 */
bool depth_camera_get_frame(SOCKET sock, char *user_buf, unsigned int buf_size)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    int cmdStringLen;                       /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;          /* Bytes read in single recv() and total bytes read */
    int unknownLen = (int)strlen(CMD_RESP_UNKNOWN);
    int copy_len;

    if (!user_buf || buf_size == 0) {
        return (false);
    }

    memset(cmdString, 0x0, sizeof(cmdString));
    sprintf_s(cmdString, "%s", CMD_CAMERA_GET_DEPTH_FRAME);
    cmdStringLen = (int)strlen(cmdString);

    /*
    * Send get depth frame cmd to depth camera
    */
    if (send(sock, cmdString, cmdStringLen, 0) != cmdStringLen) {
        fprintf(stderr, "send() sent a different number of bytes than expected\n");
        return (false);
    }

    totalBytesRcvd = 0;
    memset(frameBuff, 0x0, sizeof(frameBuff));

    while (totalBytesRcvd < DEPTH_FRAME_SIZE)
    {
        if ((bytesRcvd = recv(sock, frameBuff + totalBytesRcvd,
            DEPTH_FRAME_SIZE - totalBytesRcvd, 0)) <= 0) {
            printf("recv() failed or connection closed prematurely\n");
            return (false);
        }

        totalBytesRcvd += bytesRcvd;   /* Keep tally of total bytes */

        /*
        * Text response instead of frame data, see note above
        */
        if (totalBytesRcvd >= unknownLen && totalBytesRcvd <= CMD_RESP_LEN &&
            !strncmp(frameBuff, CMD_RESP_UNKNOWN, unknownLen)) {
            while (totalBytesRcvd < CMD_RESP_LEN) {
                if ((bytesRcvd = recv(sock, frameBuff + totalBytesRcvd,
                    CMD_RESP_LEN - totalBytesRcvd, 0)) <= 0) {
                    break;
                }
                totalBytesRcvd += bytesRcvd;
            }
            frameBuff[CMD_RESP_LEN - 1] = '\0';
            printf("Device rejected \"%s\": %s\n", CMD_CAMERA_GET_DEPTH_FRAME, frameBuff);
            return (false);
        }
    }

    copy_len = buf_size > DEPTH_FRAME_SIZE ? DEPTH_FRAME_SIZE : (int)buf_size;
    memcpy(user_buf, frameBuff, copy_len);

    return (true);
}

/*
 * Function name:depth_camera_get_info()
 * Description:
 *   Get info from by sending specific command
 * Input:
 *   sock - socket number
 *   cmd - command to device for getting info
 * Output:
 *   true - get info successfully
 *   false - failed to get info from device
 */
bool depth_camera_get_info(SOCKET sock, unsigned int cmd)
{
    bool bRet = false;
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */

    memset(cmdString, 0x0, CMDBUFSIZE);
    memset(cmdRcvBuf, 0x0, RCVBUFSIZE);

    switch (cmd) {
    case GET_PROD_SN:
        sprintf_s(cmdString, CMD_GET_PROD_SN);
        bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
        if (bRet) {
            printf("Product SN: %s\n", cmdRcvBuf);
        }
        break;
    case GET_HOSTNAME:
        sprintf_s(cmdString, CMD_GET_HOSTNAME);
        bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
        if (bRet) {
            printf("Host name: %s\n", cmdRcvBuf);
        }
        break;
    case GET_MAC_ADDR:
        sprintf_s(cmdString, CMD_GET_MAC_ADDR);
        bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
        if (bRet) {
            printf("MAC addr: %s\n", cmdRcvBuf);
        }
        break; 
    case GET_FW_VER:
        sprintf_s(cmdString, CMD_GET_FW_VER);
        bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
        if (bRet) {
            printf("FW version: %s\n", cmdRcvBuf);
        }
        break;
    case GET_FW_BUILD_DATE:
        sprintf_s(cmdString, CMD_GET_FW_BUILD_DATE);
        bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
        if (bRet) {
            printf("Build date: %s\n", cmdRcvBuf);
        }
        break;
    default:
        printf("Unknown get command\n");
        return (bRet);
    }

    return (bRet);
}

/*
 * Function name:depth_camera_set_info()
 * Description:
 *   Set info by sending specific command to device
 * Input:
 *   sock - socket number
 *   cmd - command to device for info config
 *   data - pointer to the parameter of the command
 * Output:
 *   true - set info successfully
 *   false - failed to set info to device
 */
bool depth_camera_set_info(SOCKET sock, unsigned int cmd, char *data)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */
    bool bRet = false;

    memset(cmdString, 0x0, CMDBUFSIZE);
    memset(cmdRcvBuf, 0x0, RCVBUFSIZE);

    /*
     * input parameter check
     */
    if (sock == INVALID_SOCKET || data == nullptr) {
        return (false);
    }

    switch (cmd) {
    case SET_IP_ADDR:
        /*
         * data format: X.Y.Z.W
         * example: 192.168.0.100
         */
        sprintf_s(cmdString, "%s=%s", CMD_SET_IP_ADDR, data);
        break;
    case SET_SUBNET_MASK:
        /*
        * data format: X.Y.Z.W
        * example: 255.255.255.0
        */
        sprintf_s(cmdString, "%s=%s", CMD_SET_SUBNET_MASK, data);
        break;
    case SET_DEFAULT_GATEWAY:
        /*
        * data format: X.Y.Z.W
        * example: 255.255.255.0
        */
        sprintf_s(cmdString, "%s=%s", CMD_SET_DEFAULT_GATEWAY, data);
        break;
    case SET_HOSTNAME:
        /*
        * data format: Any string (NO '=' or ';' in the string)
        * example: MyHost
        */
        sprintf_s(cmdString, "%s=%s", CMD_SET_HOSTNAME, data);
        break;
    case SET_DNS:
        /*
        * data format: X.Y.Z.W (optional, device defaults to 8.8.8.8)
        * example: 8.8.8.8
        */
        sprintf_s(cmdString, "%s=%s", CMD_SET_DNS, data);
        break;
    case SET_DHCP:
        /*
        * data format: "1" for DHCP, "0" for static ip
        * example: 0
        */
        sprintf_s(cmdString, "%s=%s", CMD_SET_DHCP, data);
        break;
    default:
        printf("Unknown set command\n");
        return (bRet);
    }

    bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);
    if (bRet) {
        bRet = (strncmp(cmdRcvBuf, CMD_RESP_SET_OK, strlen(CMD_RESP_SET_OK)) == 0);
    }

    printf("Cmd (%s), Resp: %s\n", cmdString, cmdRcvBuf);

    return (bRet);
}

/*
 * Function name:depth_camera_reconfig_static_ip()
 * Description:
 *   Example function interacting with user to input ip address, subnet mask,
 *   and default gateway. It requests device to store & reboot after user
 *   confirm the setting
 * Input:
 *   sock - socket number
 * Output:
 *   true - ip reconfig successfully
 *   false - exit without change or failed to reconfig ip
 */
bool depth_camera_reconfig_static_ip (SOCKET sock)
{
    char ip_addr[16], subnet_mask[16], default_gateway[16];
    unsigned char user_key;
    int input_done = 0;
    bool bRet = false;

    printf("The change in static IP address requires write config to flash & reboot.\n");
    printf("Press [x/X] to exit without change or any other key to continue: ");
    user_key = getchar();

    switch (user_key) {
    case 'x':
    case 'X':
        return (bRet);
    }

    while (!input_done) {
        printf("\nInput ip address : ");
        fgets(ip_addr, sizeof(ip_addr), stdin);
        ip_addr[strcspn(ip_addr, "\r\n")] = 0;

        printf("\nInput subnet mask : ");
        fgets(subnet_mask, sizeof(subnet_mask), stdin);
        subnet_mask[strcspn(subnet_mask, "\r\n")] = 0;

        printf("\nInput default gateway : ");
        fgets(default_gateway, sizeof(default_gateway), stdin);
        default_gateway[strcspn(default_gateway, "\r\n")] = 0;

        printf("\nYour input: \n");
        printf("\n================================================");
        printf("\n  ip address: %s", ip_addr);
        printf("\n  subnet mask : %s", subnet_mask);
        printf("\n  default gateway : %s\n", default_gateway);
        printf("\nType [y/Y] to continue, [x/X] to exit without change, or other key to reconfig : ");

        user_key = getchar();

        switch (user_key) {
        case 'x':
        case 'X':
            return (bRet);
        case 'y':
        case 'Y':
            input_done = 1;
            break;
        }
    }

    printf("Sending IP configuration to device...\n");

    bRet = depth_camera_set_info(sock, SET_IP_ADDR, ip_addr);
    if (!bRet) {
        printf("Failed to reconfig IP\n");
        return (bRet);
    }

    bRet = depth_camera_set_info(sock, SET_SUBNET_MASK, subnet_mask);
    if (!bRet) {
        printf("Failed to reconfig subnet mask\n");
        return (bRet);
    }

    bRet = depth_camera_set_info(sock, SET_DEFAULT_GATEWAY, default_gateway);
    if (!bRet) {
        printf("Failed to reconfig default gateway\n");
        return (bRet);
    }

    bRet = depth_camera_set_info(sock, SET_DHCP, (char *)"0");
    if (!bRet) {
        printf("Failed to switch device to static ip mode\n");
        return (bRet);
    }

    bRet = depth_camera_config_save_n_reboot(sock);
    if (!bRet) {
        printf("Failed to save & reboot device\n");
        return (bRet);
    }

    bRet = true;
    return (bRet);
}

/*
 * Function name:depth_camera_config_save_n_reboot()
 * Description:
 *   Request device to store user config to device flash &
 *   reboot
 * Input:
 *   sock - socket number
 * Output:
 *   true - command completed successfully
 *   false - command for store & reboot device fails
 */
static bool depth_camera_config_save_n_reboot (SOCKET sock)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */
    bool bRet;

    memset(cmdString, 0x0, CMDBUFSIZE);
    memset(cmdRcvBuf, 0x0, RCVBUFSIZE);

    sprintf_s(cmdString, CMD_CONFIG_SAVE_REBOOT);
    bRet = depth_camera_send_cmd(sock, cmdString, (int)strlen(cmdString), cmdRcvBuf);

    return (bRet);
}

/*
 * Function name:depth_camera_send_cmd()
 * Description:
 *   Handle command to depth camera
 * Input:
 *   sock - socket number
 *   camera_cmd - command string sent to device
 *   len - length of command
 *   cmd_resp - pointer to the buffer for storing cmd response
 * Output:
 *   true - command completed successfully
 *   false - command to device fails
 */
static bool depth_camera_send_cmd(SOCKET sock, char *camera_cmd, int len, char *cmd_resp)
{
    char cmdString[CMDBUFSIZE];             /* String to send to server */
    char cmdRcvBuf[RCVBUFSIZE];             /* Buffer for receive string */
    int cmdStringLen;                       /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;          /* Bytes read in single recv() and total bytes read */
    bool bRet = false;

    if (camera_cmd == NULL) {
        return (bRet);
    }

    cmdStringLen = (len + 1);

    if (cmdStringLen > CMDBUFSIZE ||
        (int)strlen(camera_cmd) != len) {
        return (bRet);
    }

    memcpy(cmdString, camera_cmd, len);
    cmdString[len] = '\0';

    /*
    * Send start cmd to depth camera for starting depth streaming
    */
    if (send(sock, cmdString, cmdStringLen, 0) != cmdStringLen) {
        fprintf(stderr, "send() sent a different number of bytes than expected\n");
        return (bRet);
    }

    /*
    * Expect to receive "Streaming started" string from the server
    */
    totalBytesRcvd = 0;

    while (totalBytesRcvd < CMD_RESP_LEN)
    {
        /*
        * Receive up to the buffer size (minus 1 to leave space for
        * a null terminator) bytes from the sender
        */
        if ((bytesRcvd = recv(sock, cmdRcvBuf, RCVBUFSIZE - 1, 0)) <= 0) {
            fprintf(stderr, "recv() failed or connection closed prematurely");
            return (bRet);
        }

        totalBytesRcvd += bytesRcvd;   /* Keep tally of total bytes */
        cmdRcvBuf[bytesRcvd] = '\0';   /* Add \0 so printf knows where to stop */
    }

    /*
     * Copy response to user if point is not NULL
     */
    if (cmd_resp != NULL) {
        memcpy(cmd_resp, cmdRcvBuf, strlen(cmdRcvBuf) + 1);
    }

    bRet = true;
    return (bRet);
}

#ifndef __APP_PROTOCOL_H
#define __APP_PROTOCOL_H

/*
 * STM32 -> 上位机 文本帧打包（与 Qt 上位机 ProtocolParser 严格对应）
 * 帧格式：$MS901,R,P,Y,GX,GY,GZ,AX,AY,AZ,MX,MY,MZ,PRES,ALT,TEMP*CS\r\n
 * CS 为 '$' 与 '*' 之间内容的逐字节异或，2 位大写十六进制
 */

#include "atk_ms901m.h"

void app_protocol_send(const atk_ms901m_data_t *data);

#endif

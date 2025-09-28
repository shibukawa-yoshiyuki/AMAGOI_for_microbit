#ifndef ENVIRO_SENSOR_H
#define ENVIRO_SENSOR_H
//
// Filename :   EnviroSensor.hpp
// Abstruct :   Class definition for Enviro Sensor
// Author   :   application_division@atit.jp
// Update   :   2025/09/28  New Creation
#include "iic.h"

// Definition of constant
const INT   REG_ADDR_SOFTRESET = 0xE0; // ソフトリセット先頭アドレス
const INT   REG_SOFTRESET      = 0xB6; // ソフトリセットパラメータ
const INT   REG_ADDR_CTRLMEAS  = 0xF4; // コンフィグ設定(1)先頭アドレス
const INT   REG_ADDR_CONFIG    = 0xF5; // コンフィグ設定(2)先頭アドレス
const INT   REG_ADDR_CTRLHUM   = 0xF2; // コンフィグ設定(3)先頭アドレス
const INT   REG_ADDR_CTRLCORR1 = 0x88; // 補正データ(1)先頭アドレス
const INT   REG_ADDR_CTRLCORR2 = 0xA1; // 補正データ(2)先頭アドレス
const INT   REG_ADDR_CTRLCORR3 = 0xE1; // 補正データ(3)先頭アドレス
const INT   REG_ADDR_OBSERV    = 0xF7; // 観測データ先頭アドレス
const INT   BME280_I2C_ADDR    = 0x76; // I2Cアドレス(規定値)
const INT   OVER_SAMPLING      = 0x01; // オーバーサンプリング:×1(規定値)
const INT   MODE               = 0x03; // モード:ノーマル(規定値)
const INT   SPI3W              = 0x00; // 3線式SPI:未使用(固定値)
const INT   FILTER             = 0x00; // フィルタ:OFF(規定値)
const INT   T_STANDBY          = 0x05; // スタンバイ時間:1000(ms)

// Definition of method
void EnviroSensorInit();
void readCorrectionValue();
void getObservations( UW*, UW*, UW* );
W correctTemperature( W );
UW correctPressure( W );
UW correctHumidity( W );
void performObservations( double*, double*, double* );
#endif // #ifndef ENVIRO_SENSOR_H

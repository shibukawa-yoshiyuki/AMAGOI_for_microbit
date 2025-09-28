//
// Filename :   EnviroSensor.c
// Abstruct :   Method for EnviroSensor
// Author   :   application_division@atit.jp
// Update   :   2025/09/28  New Creation
#include "EnviroSensor.h"
#include <tk/tkernel.h>
#include <tm/tmonitor.h>

// ライブラリ関数宣言
IMPORT INT read_reg ( INT adr, INT reg );
IMPORT ER  write_reg( INT adr, INT reg, UB dat );

UH  dig_T1; // 補正パラメータ1(気温)
H   dig_T2; // 補正パラメータ2(気温)
H   dig_T3; // 補正パラメータ3(気温)
UH  dig_P1; // 補正パラメータ1(気圧)
H   dig_P2; // 補正パラメータ2(気圧)
H   dig_P3; // 補正パラメータ3(気圧)
H   dig_P4; // 補正パラメータ4(気圧)
H   dig_P5; // 補正パラメータ5(気圧)
H   dig_P6; // 補正パラメータ6(気圧)
H   dig_P7; // 補正パラメータ7(気圧)
H   dig_P8; // 補正パラメータ8(気圧)
H   dig_P9; // 補正パラメータ9(気圧)
UB  dig_H1; // 補正パラメータ1(湿度)
H   dig_H2; // 補正パラメータ2(湿度)
UB  dig_H3; // 補正パラメータ3(湿度)
H   dig_H4; // 補正パラメータ4(湿度)
H   dig_H5; // 補正パラメータ5(湿度)
B   dig_H6; // 補正パラメータ6(湿度)
W   t_fine; // 補正用気温

//
// Method   :   EnviroSensorInit
// Abstruct :   初期設定
// Argument :   n/a
// Return   :   n/a
EXPORT void EnviroSensorInit() {

    // I2C初期化
    iicsetup( TRUE );

    // 初期設定値の生成
    UB osrs_t   = OVER_SAMPLING;   // Temperature oversampling x 1
    UB osrs_p   = OVER_SAMPLING;   // Pressure oversampling x 1
    UB osrs_h   = OVER_SAMPLING;   // Humidity oversampling x 1
    UB mode     = MODE;            // Normal mode
    UB t_sb     = T_STANDBY;       // Timer Stand-by 1000ms
    UB filter   = FILTER;          // Filter off
    UB spi3w_en = SPI3W;           // 3-wire SPI Disable

    // 初期設定値をレジスタに書き込み
    UB reset    = REG_SOFTRESET;
    write_reg( BME280_I2C_ADDR, REG_ADDR_SOFTRESET, reset );
    tk_dly_tsk( 200 );
    UB ctrlMeas = (osrs_t << 5) | (osrs_p << 2) | mode;
    write_reg( BME280_I2C_ADDR, REG_ADDR_CTRLMEAS, ctrlMeas );
    tk_dly_tsk( 200 );
    UB config   = (t_sb << 5) | (filter << 2) | spi3w_en;
    write_reg( BME280_I2C_ADDR, REG_ADDR_CONFIG,   config );
    tk_dly_tsk( 200 );
    UB ctrlHum  = osrs_h;
    write_reg( BME280_I2C_ADDR, REG_ADDR_CTRLHUM,  ctrlHum );
    tk_dly_tsk( 200 );

    // 補正データをレジスタから読み込み
    readCorrectionValue();

    return;
}

//
// Method   :   readCorrectionValue
// Abstruct :   補正データをレジスタから読み出す
// Argument :   n/a
// Return   :   n/a
EXPORT void readCorrectionValue() {
    UB  data[32] = { 0 };   // 読み出しデータ用テンポラリ
    B   i        = 0;       // カウンタ

    // レジスタから補正値を取得(0x88番地から0xA0番地)
    for( B j = 0; j < 24; j++ ) {
    	data[i] = read_reg( BME280_I2C_ADDR, REG_ADDR_CTRLCORR1 + j );
        tk_dly_tsk(200);
        i++;
    }
    
    // レジスタから補正値を取得(0xA1番地)
   	data[i] = read_reg( BME280_I2C_ADDR, REG_ADDR_CTRLCORR2 );
   	tk_dly_tsk(200);
    i++;
    
    // レジスタから補正値を取得(0xE1番地から0xE7番地)
    for( B j = 0; j < 7; j++ ) {
    	data[i] = read_reg( BME280_I2C_ADDR, REG_ADDR_CTRLCORR3 + j );
    	tk_dly_tsk(200);
        i++;
    }

    // レジスタから取得したデータを補正値として記憶する
    // 気温算出用補正値(T1/T2/T3)
    dig_T1 = (data[1]  << 8) | data[0];
    dig_T2 = (data[3]  << 8) | data[2];
    dig_T3 = (data[5]  << 8) | data[4];

    // 気圧算出用補正値(P1/P2/P3/P4/P5/P6/P7/P8/P9)
    dig_P1 = (data[7]  << 8) | data[6];
    dig_P2 = (data[9]  << 8) | data[8];
    dig_P3 = (data[11] << 8) | data[10];
    dig_P4 = (data[13] << 8) | data[12];
    dig_P5 = (data[15] << 8) | data[14];
    dig_P6 = (data[17] << 8) | data[16];
    dig_P7 = (data[19] << 8) | data[18];
    dig_P8 = (data[21] << 8) | data[20];
    dig_P9 = (data[23] << 8) | data[22];

    // 湿度算出用補正値(H1/H2/H3/H4/H5/H6)
    dig_H1 = data[24];
    dig_H2 = (data[26] << 8) | data[25];
    dig_H3 = data[27];
    dig_H4 = (data[28] << 4) | (0x0F & data[29]);
    dig_H5 = (data[30] << 4) | ((data[29] >> 4) & 0x0F);
    dig_H6 = data[31];   

    return;
}

//
// Method   :   getObservations
// Abstruct :   観測値をレジスタから読み込む
// Argument :   UW* temp_raw : [O]補正前観測値(気温)
//          :   UW* pres_raw : [O]補正前観測値(気圧)
//          :   UW* hum_raw  : [O]補正前観測値(湿度)
// Return   :   n/a
void getObservations( UW* temp_raw, UW* pres_raw, UW* hum_raw ) {
    UW  data[8] = { 0 };    // 読み出しデータ用テンポラリ
    B   i       = 0;        // カウンタ

    // レジスタから観測値を取得(0xF7番地から8byte分)
    for( B j = 0; j < 8; j++ ) {
    	data[i] = read_reg( BME280_I2C_ADDR, REG_ADDR_OBSERV + j );
        i++;
    }

    // レジスタから読み出したデータを補正前観測値に変換
    *pres_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
    *temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
    *hum_raw  = (data[6] <<  8) | (data[7]);

    return;
}

//
// Method   :   correctTemperature
// Abstruct :   気温観測データを補正する
// Argument :   W adc_T   : [I]補正前データ(気温)
// Return   :   W
//              補正後の観測データ(気温)
//              ※整数値なので実値の100倍になっていることに注意
W correctTemperature( W adc_T ) {
    W var1 = 0L;  // 途中項1
    W var2 = 0L;  // 途中項2
    W T    = 0L;  // 補正後の観測データ

    // 補正後観測データの算出
    var1 = ((((adc_T >> 3) - ((W)dig_T1<<1))) * ((W)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((W)dig_T1)) * ((adc_T>>4) - ((W)dig_T1))) >> 12) * ((W)dig_T3)) >> 14;
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;

    return T;
}

//
// Method   :   correctPressure
// Abstruct :   気圧観測データを補正する
// Argument :   W adc_P   : [I]補正前データ(気圧)
// Return   :   UW
//              補正後の観測データ(気圧)
//              ※整数値なので実値の100倍になっていることに注意
UW correctPressure( W adc_P ) {
    W   var1 = 0L;  // 途中項1
    W   var2 = 0L;  // 途中項2
    UW  P    = 0UL; // 補正後の観測データ

    // 補正後観測データの算出
    var1 = (((W)t_fine)>>1) - (W)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11) * ((W)dig_P6);
    var2 = var2 + ((var1*((W)dig_P5))<<1);
    var2 = (var2>>2)+(((W)dig_P4)<<16);
    var1 = (((dig_P3 * (((var1>>2)*(var1>>2)) >> 13)) >>3) + ((((W)dig_P2) * var1)>>1))>>18;
    var1 = ((((32768+var1))*((W)dig_P1))>>15);
    if (var1 == 0)
    {
        return 0;
    }    
    P = (((UW)(((W)1048576)-adc_P)-(var2>>12)))*3125;
    if(P<0x80000000)
    {
       P = (P << 1) / ((UW) var1);   
    }
    else
    {
        P = (P / (UW)var1) * 2;    
    }
    var1 = (((W)dig_P9) * ((W)(((P>>3) * (P>>3))>>13)))>>12;
    var2 = (((W)(P>>2)) * ((W)dig_P8))>>13;
    P = (UW)((W)P + ((var1 + var2 + dig_P7) >> 4));

    return P;
}

//
// Method   :   correctHumidity
// Abstruct :   湿度観測データを補正する
// Argument :   W adc_P   : [I]補正前データ(湿度)
// Return   :   UW
//              補正後の観測データ(湿度)
//              ※整数値なので実値の1024倍になっていることに注意
UW correctHumidity( W adc_H ) {
    W   v_x1;   // 途中項
    
    v_x1 = (t_fine - ((W)76800));
    v_x1 = (((((adc_H << 14) -(((W)dig_H4) << 20) - (((W)dig_H5) * v_x1)) + 
              ((W)16384)) >> 15) * (((((((v_x1 * ((W)dig_H6)) >> 10) * 
              (((v_x1 * ((W)dig_H3)) >> 11) + ((W) 32768))) >> 10) + ((W)2097152)) * 
              ((W) dig_H2) + 8192) >> 14));
    v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((W)dig_H1)) >> 4));
    v_x1 = (v_x1 < 0 ? 0 : v_x1);
    v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);

    return (UW)(v_x1 >> 12);
}

//
// Method   :   performObservations
// Abstruct :   観測値を取得して補正値を返す
// Argument :   double* temp_act    : [O]補正後の気温
//          :   double* press_act   : [O]補正後の気圧
//          :   double* hum_act     : [O]補正後の湿度
// Return   :   n/a
EXPORT void performObservations( double* temp_act, double* press_act, double* hum_act ) {
    // レジスタからの読み出し値
    UW  temp_raw  = 0UL;
    UW  pres_raw  = 0UL;
    UW  hum_raw   = 0UL;

    // レジスタから観測値を取得
    getObservations( &temp_raw, &pres_raw, &hum_raw );
    
    // 取得した観測値を補正
    W   temp_cal  = correctTemperature( temp_raw );
    UW  press_cal = correctPressure( pres_raw );
    UW  hum_cal   = correctHumidity( hum_raw );

    // 補正値から気温/気圧/湿度を算出して返却する
    *temp_act     = (double)temp_cal  / 100.0;
    *press_act    = (double)press_cal / 100.0;
    *hum_act      = (double)hum_cal   / 1024.0;

    return;
}

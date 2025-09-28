#ifndef INFERENCE_ENGINE_H
#define INFERENCE_ENGINE_H

//
// Filename :   InferenceEngine.hpp
// Abstruct :   Class definition for inference engine
// Author   :   application_division@atit.jp
// Update   :   2025/09/28	New Creation
#include <tk/tkernel.h>

// Definition of constant
const unsigned long ONEHOUR_MSEC     = ( 60 * 60 * 1000 );  // 1時間をミリ秒に換算
const unsigned long OBS_INTERVAL     = ( 5 * 1000 );        // 計測処理実行間隔
const unsigned long EST_INTERVAL     = ( 5 * 60 * 1000 );   // 推定処理実行間隔
const unsigned int  EST_CALC_CNT     = ( 720 );             // 推定時フィルタ更新実行回数
const unsigned int  OBS_REC_CNT_MAX  = ( 13 );              // 観測値記録最大数
const unsigned int  EST_REC_CNT_MAX  = ( 12 );              // 推定値記録最大数
const unsigned int  EST_REC_CNT      = ( 60 );              // 推定時記録実行間隔
const unsigned int  EST_ARRAY_MAX    = ( 25 );              // 記録配列データ長

const int    WEATHER_IMPROVES        = 1;                   //天候回復
const int    WEATHER_NOT_CHANGE      = 2;                   //天候変化なし
const int    WEATHER_GETS_WORSE      = 3;                   //天候悪化

const double RAPID_PRESSURE_INCREASE_THRESHOLD = 0.001;     // 気圧急速上昇閾値
const double PRESSURE_RISE_THRESHOLD           = 0.0005;    // 気圧上昇閾値
const double RAPID_DROP_IN_PRESSURE_THRESHOLD  = -0.001;    // 気圧急速下降閾値
const double PRESSURE_DROP_THRESHOLD           = -0.0005;   // 気圧下降閾値	
const double HUMIDITY_RISE_THRESHOLD           = 0.001;     // 湿度上昇閾値
const double HUMIDITY_DROP_THRESHOLD           = -0.001;    // 湿度下降閾値

// Definition of method
void InferenceEngineInit( void );
unsigned char updateObservations( double, double );
void calcInferredValue( double, int, double, double, double* );
void calcPredictedValue( double*, double, double*, double* );
double updatePrediction( double*, int );
void arraySlide( double* );
double addNoise2Observ( double );
#endif // #ifndef INFERENCE_ENGINE_H

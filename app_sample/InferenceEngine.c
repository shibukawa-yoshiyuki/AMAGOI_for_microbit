//
// Filename :   InferenceEngine.cpp
// Abstruct :   Method for InferenceEngine class
// Author   :   application_division@atit.jp
// Update   :   2025/09/28	New Creation
#include "InferenceEngine.h"

// Definition of variable
double GP           = 0.0;      // カルマンゲイン(気圧)
double GH           = 0.0;      // カルマンゲイン(湿度)
double PP           = 1.0;      // 誤差共分散(気圧)
double PH           = 1.0;      // 誤差共分散(湿度)
double Q            = 0.005;    // システムノイズ
double R            = 0.0005;   // 観測ノイズ
double xhatP        = 0.0;	    // 推定値(気圧)
double xhatH        = 0.0;	    // 推定値(湿度)
double estValP[25]  = { 0.0 };  // 観測値および推定値記憶域(気圧)
double estValH[25]  = { 0.0 };	// 観測値および推定値記憶域(湿度)
int    observCnt    = 0;        // 観測回数カウンタ
int    estValCnt    = 0;        // 観測値データ数
double inclinationP = 0.0;	    // 傾き(気圧)
double inclinationH = 0.0;      // 傾き(湿度)

IMPORT void   myRandmizeInit( void );
IMPORT int    myRandmize( void );
IMPORT double mySine( double );
IMPORT double myCosine( double );

//
// Method   :   InferenceEngine
// Abstruct :   初期設定
// Argument :   n/a
// Return	:	n/a
void InferenceEngineInit( void ) 
{
	myRandmizeInit();
}

//
// Method   :   updateObservations
// Abstruct :   観測値を取り込んでフィルタステップを進める
// Argument :   double xP : [I]観測値(気圧)
// 			:   double xH : [I]観測値(湿度)
// Return   :   unsigned char
//              推定パターン
unsigned char updateObservations( double xP, double xH ) {
	unsigned char retVal = 0;
	
	// y初期値設定
	double yP = xP * xP * xP;
	double yH = xH * xH * xH;

	// xhat初期値設定(初回のみ)
	if( observCnt == 0 && estValCnt == 0 ) {
		xhatP = xP + 1.0;
		xhatH = xH + 1.0;
	}

	// フィルタ更新実行
	calcPredictedValue( &xhatP, yP, &GP, &PP );
	calcPredictedValue( &xhatH, yH, &GH, &PH );
	observCnt++;

	if( observCnt == 1 && estValCnt == 0 ) {
		// 観測値を記憶(初回)
		estValCnt++;
		estValP[estValCnt - 1] = xP;
		estValH[estValCnt - 1] = xH;
	} else if( observCnt > EST_REC_CNT ) {
		observCnt = 0;
		// 直近の観測値を記憶(5秒間隔×60=5分ごとの観測値を記憶)
		if( estValCnt == OBS_REC_CNT_MAX ) {
			// 記憶域がいっぱいの場合前にずらす
			arraySlide( estValP );
			arraySlide( estValH );
		}
		if( estValCnt < OBS_REC_CNT_MAX ) {
			// 記憶回数が OBS_REC_CNT_MAX に満たない場合インクリメント
			estValCnt++;
		}
		estValP[estValCnt - 1] = xP;
		estValH[estValCnt - 1] = xH;
		
		// 推定値を算出
		calcInferredValue( xhatP, estValCnt, GP, PP, estValP );
		calcInferredValue( xhatH, estValCnt, GH, PH, estValH );
		
		// 最小二乗法にて傾きを算出
		inclinationP = updatePrediction( estValP, estValCnt + EST_REC_CNT_MAX );
		inclinationH = updatePrediction( estValH, estValCnt + EST_REC_CNT_MAX );
		
		// 気圧および湿度の傾きより推定パターンを導出
		if( inclinationP > RAPID_PRESSURE_INCREASE_THRESHOLD ) {
			// 気圧が急速に上昇している場合は天候回復と推測
			retVal = WEATHER_IMPROVES;
		} else if( inclinationP < RAPID_DROP_IN_PRESSURE_THRESHOLD ) {
			// 気圧が急速に減少している場合は天候悪化と推測
			retVal = WEATHER_GETS_WORSE;
		} else if( inclinationP > PRESSURE_RISE_THRESHOLD && inclinationH < HUMIDITY_DROP_THRESHOLD ) {
			// 気圧が上昇し、かつ湿度が減少している場合は天候回復と推測
			retVal = WEATHER_IMPROVES;
		} else if( inclinationP < PRESSURE_DROP_THRESHOLD && inclinationH > HUMIDITY_RISE_THRESHOLD ) {
			// 気圧が減少し、かつ湿度が上昇している場合は天候悪化と推測
			retVal = WEATHER_GETS_WORSE;
		} else {
			// 気圧および湿度変化がほぼない場合、天候はそのまま維持されると推測
			retVal = WEATHER_NOT_CHANGE;
		}
	}

	return retVal;
}

//
// Method   :   calcInferredValue
// Abstruct :   規定時間経過後の状態推定値を算出する
// Argument :   double xhat    : [I]推定値初期値
//   		:	int dataHead   : [I]記録初期位置
//			:	double G	   : [I]カルマンゲイン初期値
//			:	double P	   : [I]誤差共分散初期値
//			:	double* estVal : [IO]観測値・推定値記憶域への参照
// Return   :   n/a
void calcInferredValue( double xhat, int dataHead, double G, double P, double* estVal ) {
	// 予測用のゲイン・誤差共分散・疑似観測値
	double Ghat = G;
	double Phat = P;
	double x    = xhat;

	for( int i = 1; i <= EST_CALC_CNT; i++ ) {
		// 疑似観測値 x に観測誤差を与える
		x           = addNoise2Observ( x );
		// yの算出
		double yhat = x * x * x;
		// 推定ステップを実行
		calcPredictedValue( &xhat, yhat, &Ghat, &Phat );
		if( i % 60 == 0 ) {
			// 5分相当経過ごとの推定値を記憶域に格納
			estVal[dataHead] = xhat;
			dataHead++;
		}
	}
	return;
}

//
// Method   :   calcPredictedValue
// Abstruct :   推定ステップを進める
// Argument :   double xhat : [IO]推定値
//			:	double y    : [I]
// 			:	double *G	: [IO]カルマンゲイン
//			: 	double *P	: [IO]誤差共分散
// Return   :   n/a
void calcPredictedValue( double *xhat, double y, double *G, double *P ) {
	// 事前推定値の算出
	double cosXHat = myCosine( *xhat / 10.0 );
	double sinXHat = mySine( *xhat / 10.0 );
    double xhatM = *xhat + 3.0 * cosXHat;
    double PM = ( 1.0 - 3.0 / 10.0 * sinXHat) * (*P) * ( 1.0 - 3.0 / 10.0 * sinXHat) + ( 1.0 ) * Q * ( 1.0 );

	// カルマンゲインの更新
	double Gh = xhatM * xhatM;
	double Gf = xhatM * xhatM * xhatM;
    *G = PM * ( 3.0 * Gh )  / (( 3.0 * Gh ) * PM * ( 3.0 * Gh ) + R);

	// 事後推定値の算出
	*xhat = xhatM + (*G) * ( y - Gf );
    *P = ( 1.0 ) - (*G) * ( 3.0 * Gh ) * PM;

	return;
}

//
// Method   :   updatePrediction
// Abstruct :   最小二乗法を用いて傾きを算出する
// Argument :   double y   : [I]観測及び推定値の記録配列
//			:	int    cnt : [I]データ個数
// Return   :   n/a
double updatePrediction( double* y, int cnt ) {
    double sum_xx = 0.0;
    double sum_xy = 0.0;
    double sum_x  = 0.0;
    double sum_y  = 0.0;
    
	// 傾きを算出
	// note : 横軸は時間(秒単位)
	for( int i = 0; i < cnt; i++) {
		sum_xx += ( ((double)EST_INTERVAL/1000.0) * (double)i ) * ( ((double)EST_INTERVAL/1000.0) * (double)i );
		sum_xy += ( ((double)EST_INTERVAL/1000.0) * (double)i ) * y[i];
		sum_x  += ( ((double)EST_INTERVAL/1000.0) * (double)i );
		sum_y  += y[i];
    }
    return ( cnt * sum_xy - sum_x * sum_y ) / ( cnt * sum_xx - sum_x * sum_x );
}

//
// Method   :   arraySlide
// Abstruct :   配列データをずらす
// Argument :   double* array : [I]処理対象配列
// Return   :   n/a
void arraySlide( double* array ) {
	array[0] = array[1];
	for( int i = 1; i <= ( OBS_REC_CNT_MAX - 1 ) - 1; i++ ) {
		array[i] = array[i + 1];
	}
	array[12] = 0.0;
	return;
}

//
// Method   :   addNoise2Observ
// Abstruct :   観測値にランダムな観測ノイズを与える
// Argument :   double x : [IO]ノイズを与える観測値
// Return   :   double
//				ノイズ加算後の観測値
double addNoise2Observ( double x ) {
	double rd = ((double)((int)myRandmize() % 100 - 50) / 50.0) * R;
	return x += rd;
}

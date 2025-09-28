//
// Filename :   app_main.c
// Abstruct :   Method for Mat
// Author   :   application_division@atit.jp
// Update   :   2025/09/28	New Creation
#include <tk/tkernel.h>
#include <tm/tmonitor.h>

//
// 観測タスク用定義
//
IMPORT void EnviroSensorInit();
IMPORT void performObservations( double*, double*, double* );
double obsT   = 0.0;	// 観測値(気温)
double obsP   = 0.0;	// 観測値(気圧)
double obsH   = 0.0;	// 観測値(湿度)
UB     envFlg = 0;		// 観測実行フラグ(推論エンジンキック用)
void tskEnviroSensor( INT stacd, void *exinf ) {
	tm_printf("Start EnviroSensor Task.\n");
	while( 1 ) {
		performObservations( &obsT, &obsP, &obsH );
		envFlg = 1;
		tk_dly_tsk( 5000 );	// 5秒ごとに観測実行する
	}
}
ID tidEnv;
const T_CTSK tskEnv = {0, (TA_HLNG | TA_RNG3), &tskEnviroSensor, 30, 1024, 0};

//
// 推論エンジンタスク用定義
//
IMPORT void InferenceEngineInit();
IMPORT UB updateObservations( double, double );
UB     infFlg = 0;		// 推論結果パターン
UB	   infFlgNew = 0;	// 推論結果パターン(表示パターン用)
void tskInferenceEngine( INT stacd, void *exinf ) {
	tm_printf("Start InferenceEngine Task.\n");
	while( 1 ) {
		if( envFlg == 1 ) {
			envFlg = 0;
			infFlg = updateObservations( obsP, obsH );
		}
		tk_dly_tsk( 500 );	// 0.5秒待ち
	}
}
ID tidInf;
const T_CTSK tskInf = {0, (TA_HLNG | TA_RNG3), &tskInferenceEngine, 20, 1024, 0};

//
// LEDマトリクス表示用定義
//

/*
 *  LED マトリクス表示関連コードについては、
 *  『micro:bitでµT-Kernel 3.0を動かそう』
 * ［第7回］ LEDのダイナミック点灯 のコードより流用している
 *  http://www.t-engine4u.com/info/mbit/7.html
 * 
 *	Copyright (C) 2022-2023 by T3 WG of TRON Forum
 */

 // 5行5列のLEDの点灯パターンの定義
LOCAL UB ledptn_i[4][5] = {{
	0b00000000,					// 点灯パターン: ●●●●●
	0b00000000,					// 点灯パターン: ●●●●●
	0b00001110,					// 点灯パターン: ●○○○●
	0b00000000,					// 点灯パターン: ●●●●●
	0b00000000					// 点灯パターン: ●●●●●
},
{
	0b00001111,					// 点灯パターン: ●○○○○
	0b00000011,					// 点灯パターン: ●●●○○
	0b00000101,					// 点灯パターン: ●●○●○
	0b00001001,					// 点灯パターン: ●○●●○
	0b00010000					// 点灯パターン: ○●●●●
},
{
	0b00000100,					// 点灯パターン: ●●○●●
	0b00000010,					// 点灯パターン: ●●●○●
	0b00011111,					// 点灯パターン: ○○○○○
	0b00000010,					// 点灯パターン: ●●●○●
	0b00000100					// 点灯パターン: ●●○●●
},
{
	0b00010000,					// 点灯パターン: ○●●●●
	0b00001001,					// 点灯パターン: ●○●●○
	0b00000101,					// 点灯パターン: ●●○●○
	0b00000011,					// 点灯パターン: ●●●○○
	0b00001111					// 点灯パターン: ●○○○○
}};

// LED制御のためのGPIOの初期設定
LOCAL void led_init(void)
{
	// COL1..COL5とROW1..ROW5に接続されたGPIOピンを出力に設定
	out_w(GPIO(P0, PIN_CNF(28)), 1);	// GPIO P0.28(COL1に接続)
	out_w(GPIO(P0, PIN_CNF(11)), 1);	// GPIO P0.11(COL2に接続)
	out_w(GPIO(P0, PIN_CNF(31)), 1);	// GPIO P0.31(COL3に接続)
	out_w(GPIO(P1, PIN_CNF(05)), 1);	// GPIO P1.05(COL4に接続)
	out_w(GPIO(P0, PIN_CNF(30)), 1);	// GPIO P0.30(COL5に接続)
	out_w(GPIO(P0, PIN_CNF(21)), 1);	// GPIO P0.21(ROW1に接続)
	out_w(GPIO(P0, PIN_CNF(22)), 1);	// GPIO P0.22(ROW2に接続)
	out_w(GPIO(P0, PIN_CNF(15)), 1);	// GPIO P0.15(ROW3に接続)
	out_w(GPIO(P0, PIN_CNF(24)), 1);	// GPIO P0.24(ROW4に接続)
	out_w(GPIO(P0, PIN_CNF(19)), 1);	// GPIO P0.19(ROW5に接続)
}

// GPIOの特定のピンpinの出力をval(0または1)に設定
//	pinで指定された以外のピンの状態は変化しない
LOCAL void out_gpio_pin(UW port, UW pin, UW val)
{
	INT port_addr;

	if(port == 1){							// P1の場合
		if(val)
			port_addr = GPIO(P1, OUTSET);	// P1でvalが1の場合
		else							
			port_addr = GPIO(P1, OUTCLR);	// P1でvalが0の場合

	} else {								// P0の場合
		if(val)
			port_addr = GPIO(P0, OUTSET);	// P0でvalが1の場合
		else
			port_addr = GPIO(P0, OUTCLR);	// P0でvalが0の場合
	}
	out_w(port_addr, (1 << pin));			// 指定のピンに1を出力
}

// ビットパターンbitptnの指定のビットに応じてGPIOの指定のピンを制御
//	bitptnの最下位からnビット目(n=1..5)が1の場合はピンpinの出力をvalに設定
//	そのビットが0の場合はvalの反対の値(1→0,0→1)をピンpinの出力に設定
LOCAL void bitptn_gpio_pin(UW bitptn, UW n, UW port, UW pin, UW val)
{
	if(bitptn & (1 << (n - 1))){		// bitptnの最下位からnビット目をチェック
		out_gpio_pin(port, pin, val);		// 1の場合はGPIOにvalを出力
	} else {
		out_gpio_pin(port, pin, (! val));	// 0の場合はvalの反対の値を出力
	}
}

// 5ビットのビットパターンbitptnに応じてLEDのCOL1..COL5を制御
LOCAL void out_led_col(UW bitptn)
{
	// 最下位から5番目のビットが1の場合にGPIO P0.28(COL1)を0に設定
	bitptn_gpio_pin(bitptn, 5, 0, 28, 0);

	// 最下位から4番目のビットが1の場合にGPIO P0.11(COL2)を0に設定
	bitptn_gpio_pin(bitptn, 4, 0, 11, 0);

	// 最下位から3番目のビットが1の場合にGPIO P0.31(COL3)を0に設定
	bitptn_gpio_pin(bitptn, 3, 0, 31, 0);

	// 最下位から2番目のビットが1の場合にGPIO P1.05(COL4)を0に設定
	bitptn_gpio_pin(bitptn, 2, 1, 5, 0);

	// 最下位のビットが1の場合にGPIO P0.30(COL5)を0に設定
	bitptn_gpio_pin(bitptn, 1, 0, 30, 0);
}

// ROW1..ROW5のいずれか(rowで番号指定)に接続されたGPIOピンにvalを設定
LOCAL void out_row_gpio(UW row, UW val)
{
	switch(row){
	  case 1:
		out_gpio_pin(0, 21, val);		// GPIO P0.21(ROW1に接続)にvalを設定
		return;
	  case 2:
		out_gpio_pin(0, 22, val);		// GPIO P0.22(ROW2に接続)にvalを設定
		return;
	  case 3:
		out_gpio_pin(0, 15, val);		// GPIO P0.15(ROW3に接続)にvalを設定
		return;
	  case 4:
		out_gpio_pin(0, 24, val);		// GPIO P0.24(ROW4に接続)にvalを設定
		return;
	  case 5:
		out_gpio_pin(0, 19, val);		// GPIO P0.19(ROW5に接続)にvalを設定
		return;
	}
}

// rowで指定した行のみを点灯させるためのGPIOピンの設定
//	ROW1..ROW5に接続されたGPIOピンのうち、
//	rowで指定した1本のみを1に設定し、他は0に設定する
LOCAL void set_row_gpio(UW row)
{
	UW	cnt, val;
	for(cnt = 1; cnt <= 5; cnt++){
		out_row_gpio(cnt, ((cnt == row) ? 1 : 0));	// 指定の1本のみ1を設定
	}
}

LOCAL W	led_disp_row = 5;		// 現在表示中のLEDの行の番号(1..5)

// LEDの表示行を切り替える物理タイマハンドラ
LOCAL void led_switch_row_hdr(void *exinf)
{
	if( infFlg != 0 && infFlg != infFlgNew )
		infFlgNew = infFlg;
	if(++led_disp_row > 5)		// 表示中のLEDの行の番号(1..5)を更新
		led_disp_row = 1;		// 行の番号が5を超えたら1に戻る
	out_led_col(0);				// ちらつき防止用に消灯
	set_row_gpio(led_disp_row);					// 表示行のGPIOのピンを設定
	out_led_col(ledptn_i[infFlgNew][led_disp_row - 1]);	// 表示行の点灯パターンを指定
}

// 物理タイマによるダイナミック点灯
const T_DPTMR dptmr = {0, TA_HLNG, &led_switch_row_hdr};	// ハンドラ定義情報
const UINT ptmrno = 1; 			// 物理タイマ番号として1を使用
const INT ptmr_clk_mhz = 16;	// 物理タイマのクロック(MHz単位)
const INT cycle_us = 1000;		// ハンドラの起動周期(μs単位)、1000μs＝1ms
const INT limit = cycle_us * ptmr_clk_mhz - 1;	// 物理タイマの上限値

//
// Method   :   usermain
// Abstruct :   エントリポイント関数 タスク起動/タイマー起動を行う
// Argument :   n/a
// Return   :   n/a
EXPORT void usermain( void ) {
	// 観測、推論エンジンおよびLEDマトリクス表示を初期化
	EnviroSensorInit();
	InferenceEngineInit();
	led_init();

	// LED タイマ動作用の定義
	DefinePhysicalTimerHandler(ptmrno, &dptmr);	// 物理タイマハンドラ定義
	StartPhysicalTimer(ptmrno, limit, TA_CYC_PTMR);	// 物理タイマの動作開始

	// タスク生成
	tidEnv = tk_cre_tsk( &tskEnv );
	tidInf = tk_cre_tsk( &tskInf );

	// タスク起動
	tk_sta_tsk( tidEnv, 0 );
	tk_sta_tsk( tidInf, 0 );

	tk_slp_tsk(TMO_FEVR); // 永久待ち
}

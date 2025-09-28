/*
 *	@(#)iic.h 2024-04-18
 *
 *	I2C 入出力インタフェース定義
 *	Copyright (C) 2024 by Personal Media Corporation
 *
 *	I2C を IIC と表記している。
 */

#ifndef __IIC_H__
#define __IIC_H__

#include <tk/typedef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
	IIC 入出力

	ER iicxfer(W ch, UH *cmddat, W words, W *xwords)

	IIC デバイスに対し、データの送受信を行う。

		ch:	使用 IIC チャネル
		cmddat:	コマンド/送受信データを格納する領域の先頭ポインタ
		words:	コマンド/送受信データのワード数
		xwords:	成功したコマンド/送受信データのワード数
			(NULL の場合は格納しない)

		戻り値:	E_OK もしくはエラー (E_IO, E_TMOUT, E_PAR)

	割り込みハンドラ、もしくは割り込みが禁止された状態から呼び出しては
	ならない。

	コマンド/送受信データは、以下のように記述する。

	例1)	1バイトのデータを送信する場合
		{
			IIC_START   | (address << 1) | 0x00,	// R/W# = 0
			IIC_SEND    | IIC_TOPDATA  | IIC_LASTDATA | txdata0,
			IIC_STOP,
		}

		送信するデータは、IIC_SEND を指定したワードの下位 8bit に
		格納しておく。

	例2)	複数バイトのデータを送信する場合
		{
			IIC_START   | (address << 1) | 0x00,	// R/W# = 0
			IIC_SEND    | IIC_TOPDATA  | txdata0,
			IIC_SEND    |                txdata1
				:
			IIC_SEND    | IIC_LASTDATA | txdataN,
			IIC_STOP,
		}

	例3)	送受信の切り替えを伴う場合
		{
			IIC_START   | (address << 1) | 0x00,	// R/W# = 0
			IIC_SEND    | IIC_TOPDATA  | IIC_LASTDATA | txdata0,
			IIC_RESTART | (address << 1) | 0x01,	// R/W# = 1
			IIC_RECV    | IIC_TOPDATA,				// rxdata0
			IIC_RECV,								// rxdata1
				:
			IIC_RECV    | IIC_LASTDATA,				// rxdataN
			IIC_STOP,
		}

		受信したデータは、IIC_RECV を指定したワードの下位 8bit に
		格納される (上位 8bit は不変)。

	コマンド/送受信データの最終ワードは、必ず IIC_STOP を記述すること。

	転送中にエラーが発生した場合は、ストップコンディションを自動的に
	発生させる。
*/

IMPORT ER iicxfer(W ch, UH *cmddat, W words, W *xwords);

#define	IIC_RESTART	(3 << 14)		// リスタートコンディションを送信
#define	IIC_START	(2 << 14)		// スタートコンディションを送信
#define	IIC_STOP	(1 << 14)		// ストップコンディションを送信
#define	IIC_SEND	(1 << 13)		// データを送信
#define	IIC_RECV	(1 << 12)		// データを受信
#define	IIC_TOPDATA	(1 << 11)		// 送受信データの先頭
#define	IIC_LASTDATA	(1 << 10)	// 送受信データの末端

IMPORT ER iicsetup( BOOL start );

#ifdef __cplusplus
}
#endif
#endif /* __IIC_H__ */

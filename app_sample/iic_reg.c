/*
 *	@(#)iic_reg.c 2024-05-01
 *
 *	I2Cデバイスのレジスタ操作用の関数 (micro:bitのI2C_INT用)
 *	Copyright (C) 2024 by Personal Media Corporation
 */

#include <tk/tkernel.h>
#include <tm/tmonitor.h>
#include <tstdlib.h>
#include "iic.h"

#define	VERBOSE		0	/* デバッグ用メッセージ 1:あり 0:なし */
// #define VERBOSE	1	/* デバッグ用メッセージ 1:あり 0:なし */

// upd- 2025/09/27 application_division@atit.jp 外部I2Cデバイスを用いるため、チャンネルをI2C_EXTにする
// #define	I2C_CH		0	/* I2C チャンネル (micro:bitのI2C_INT用) */
#define	I2C_CH		1	/* I2C チャンネル (micro:bitのI2C_EXT用) */
// -upd 2025/09/27 application_division@atit.jp

#define	WR(da)		(((da) << 1) | 0)
#define	RD(da)		(((da) << 1) | 1)

/*
 * レジスタ書き込み
 */
EXPORT ER write_reg( INT adr, INT reg, UB dat )
{
	UH	c[4];
	W	n;
	ER	err;

	c[0] = IIC_START | WR(adr);
	c[1] = IIC_SEND  | IIC_TOPDATA  | reg;
	c[2] = IIC_SEND  | IIC_LASTDATA | dat;
	c[3] = IIC_STOP;

	err = iicxfer(I2C_CH, c, sizeof(c) / sizeof(UH), &n);

#if VERBOSE
	tm_printf("write_reg 0x%02x 0x%02x <- 0x%02x : n=%d err=%d\n",
		  adr, reg, dat, n, err);
#endif

	return err;
}

/*
 * レジスタ読み出し
 */
EXPORT INT read_reg( INT adr, INT reg )
{
	UH	c[5];
	UB	dat;
	W	n;
	ER	err;

	c[0] = IIC_START   | WR(adr);
	c[1] = IIC_SEND    | IIC_TOPDATA | IIC_LASTDATA | reg;
	c[2] = IIC_RESTART | RD(adr);
	c[3] = IIC_RECV    | IIC_TOPDATA | IIC_LASTDATA;
	c[4] = IIC_STOP;

	err = iicxfer(I2C_CH, c, sizeof(c) / sizeof(UH), &n);

	dat = c[3] & 0xff;

#if VERBOSE
	tm_printf("read_reg 0x%02x 0x%02x -> 0x%02x : n=%d err=%d\n",
		  adr, reg, dat, n, err);
#endif

	return ( err < E_OK )? err: dat;
}

/*
 * レジスタ6個書き込み
 */
EXPORT ER write_reg_6( INT adr, INT reg, UB dat[6] )
{
	UH const cmd[] = {
		IIC_START | WR(0),				/* [0] adr */
		IIC_SEND  | IIC_TOPDATA | 0x80,	/* [1] reg */
		IIC_SEND,						/* [2] dat[0] */
		IIC_SEND,						/* [3] dat[1] */
		IIC_SEND,						/* [4] dat[2] */
		IIC_SEND,						/* [5] dat[3] */
		IIC_SEND,						/* [6] dat[4] */
		IIC_SEND  | IIC_LASTDATA,		/* [7] dat[5] */
		IIC_STOP
	};
	UH	c[sizeof(cmd) / sizeof(H)];
	W	n, i;
	ER	err;

	/* コマンドパケット生成 */
	knl_memcpy(c, cmd, sizeof(c));
	c[0] |= WR(adr);
	c[1] |= reg;

	/* データ設定 */
	for ( i = 0; i < 6; ++i ) {
		c[2 + i] |= dat[i];
	}

	/* レジスタ書き込み */
	err = iicxfer(I2C_CH, c, sizeof(c) / sizeof(UH), &n);

#if VERBOSE
	tm_printf("write_reg_6 0x%02x 0x%02x <- "
		  "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x : "
		  "n=%d err=%d\n",
		  adr, reg,
		  dat[0], dat[1], dat[2], dat[3], dat[4], dat[5],
		  n, err);
#endif

	return err;
}

/*
 * レジスタ6個読み出し
 */
EXPORT ER read_reg_6( INT adr, INT reg, UB dat[6] )
{
	UH const cmd[] = {
		IIC_START   | WR(0),			/* [0] adr */
		IIC_SEND    | IIC_TOPDATA | IIC_LASTDATA | 0x80, /* [1] reg */
		IIC_RESTART | RD(0),			/* [2] adr */
		IIC_RECV    | IIC_TOPDATA,		/* [3] dat[0] */
		IIC_RECV,						/* [4] dat[1] */
		IIC_RECV,						/* [5] dat[2] */
		IIC_RECV,						/* [6] dat[3] */
		IIC_RECV,						/* [7] dat[4] */
		IIC_RECV    | IIC_LASTDATA,		/* [8] dat[5] */
		IIC_STOP
	};
	UH	c[sizeof(cmd) / sizeof(H)];
	W	n, i;
	ER	err;

	/* コマンドパケット生成 */
	knl_memcpy(c, cmd, sizeof(c));
	c[0] |= WR(adr);
	c[1] |= reg;
	c[2] |= RD(adr);

	/* レジスタ読み出し */
	err = iicxfer(I2C_CH, c, sizeof(c) / sizeof(UH), &n);

	/* データ取り出し */
	for ( i = 0; i < 6; ++i ) {
		dat[i] = c[3 + i] & 0xff;
	}

#if VERBOSE
	tm_printf("read_reg_6 0x%02x 0x%02x -> "
		  "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x : "
		  "n=%d err=%d\n",
		  adr, reg,
		  dat[0], dat[1], dat[2], dat[3], dat[4], dat[5],
		  n, err);
#endif

	return err;
}

/*
 *	@(#)nrf5_iic.c 2024-04-18
 *
 *	nRF5 I2C ドライバー
 *	Copyright (C) 2024 by Personal Media Corporation
 *
 *	I2C を IIC と表記している。
 */

#include <tk/tkernel.h>
#include "iic.h"

LOCAL FastMLock	IICLock;	/* 排他制御用ロック */
LOCAL ID	IICFlgID;	/* 割込通知用イベントフラグ */

#define	IICMAX		2	/* 対応チャンネル数 */

/*
 * IIC 制御情報
 */
typedef struct iiccb {
	UW	iob;		/* IIC レジスターアドレス */
	UH	*cmddat;	/* コマンド現在位置 */
	UH	*end;		/* コマンド終了位置 */
} IICCB;
LOCAL IICCB	iiccb[IICMAX];

#define	IRQ(cb)		INTNO((cb)->iob)	/* I2C 割込番号 */
#define	IRQ_LEVEL	3			/* I2C 割込優先レベル */

/*
 * nRF5 TWI (I2C) 構成情報
 *	ピン割り当て
 *	P0.08	I2C_INT_SCL
 *	P0.16	I2C_INT_SDA
 *	P0.26	I2C_EXT_SCL
 *	P1.00	I2C_EXT_SDA
 */
LOCAL struct iic_conf {
	UW	iob;		/* I/O ベースアドレス */
	UW	psel_scl;	/* SCL ピン設定 */
	UW	psel_sda;	/* SDA ピン設定 */
	UW	freq;		/* クロック設定 */
} const iic_conf[IICMAX] = {
	{ 0x40003000, 0x08, 0x10, 0x06680000 },	/* TWI0 I2C_INT 400KHz */
	{ 0x40004000, 0x1a, 0x20, 0x06680000 }	/* TWI1 I2C_EXT 400KHz */
};

/*
 * nRF5 TWI (I2C) レジスタ
 *	レジスタ幅はすべて 32 ビット
 */
#define	IIC(cb, reg)	( (cb)->iob + (reg) )

#define	TASKS_STARTRX		0x000	/* Start TWI receive sequence */
#define	TASKS_STARTTX		0x008	/* Start TWI transmit sequence */
#define	TASKS_STOP		0x014	/* Stop TWI transaction */
#define	TASKS_SUSPEND		0x01C	/* Suspend TWI transaction */
#define	TASKS_RESUME		0x020	/* Resume TWI transaction */
#define	EVENTS_STOPPED		0x104	/* TWI stopped */
#define	EVENTS_RXDREADY		0x108	/* TWI RXD byte received */
#define	EVENTS_TXDSENT		0x11C	/* TWI TXD byte sent */
#define	EVENTS_ERROR		0x124	/* TWI error */
#define	EVENTS_BB		0x138	/* TWI byte boundary */
#define	EVENTS_SUSPENDED	0x148	/* TWI entered the suspended state */
#define	SHORTS			0x200	/* Shortcuts */
#define	INTENSET		0x304	/* Enable interrupt */
#define	INTENCLR		0x308	/* Disable interrupt */
#define	ERRORSRC		0x4C4	/* Error source */
#define	ENABLE			0x500	/* Enable TWI */
#define	PSEL_SCL		0x508	/* Pin select for SCL */
#define	PSEL_SDA		0x50C	/* Pin select for SDA */
#define	RXD			0x518	/* RXD register */
#define	TXD			0x51C	/* TXD register */
#define	FREQUENCY		0x524	/* TWI frequency */
#define	ADDRESS			0x588	/* Address used in the TWI transfer */

/*
 * 送受信開始
 *	*cb->cmddat が IIC_START または IIC_RESTART であること。
 *	戻値 0:正常 -1:異常
 */
LOCAL INT xfer_start( IICCB *cb )
{
	UW	d;

	if ( cb->end - cb->cmddat < 3 ) return -1;

	d = *cb->cmddat++ & 0xff;
	out_w(IIC(cb, ADDRESS), d >> 1);

	if ( (d & 1) == 0 ) {
		/* 送信開始 */
		out_w(IIC(cb, SHORTS), 0);
		out_w(IIC(cb, TASKS_STARTTX), 1);
		out_w(IIC(cb, TXD), *cb->cmddat & 0xff);
	} else {
		/* 受信開始 */
		if ( (*(cb->cmddat + 1) & 0xc000) == IIC_STOP ) {
			out_w(IIC(cb, SHORTS), 2); /* BB_STOP */
		} else {
			out_w(IIC(cb, SHORTS), 1); /* BB_SUSPEND */
		}
		out_w(IIC(cb, TASKS_STARTRX), 1);
	}

	return 0;
}

/*
 * コマンド実行
 *	戻値 0:継続 1:終了 -1:異常
 */
LOCAL INT xfer_cmddat( IICCB *cb )
{
	UW	cmd, d;

	cmd = *cb->cmddat;

	if ( (in_w(IIC(cb, EVENTS_ERROR)) & 1) != 0 ) {
		out_w(IIC(cb, EVENTS_ERROR), 0);
		goto err_stop;
	}

	if ( (in_w(IIC(cb, EVENTS_TXDSENT)) & 1) != 0 ) {
		out_w(IIC(cb, EVENTS_TXDSENT), 0);
		if ( (cmd & IIC_SEND) != 0 ) {
			/* データ送信完了 */
			if ( ++cb->cmddat >= cb->end ) goto err_stop;
			cmd = *cb->cmddat; /* 次へ */
		}

		if ( (cmd & IIC_SEND) != 0 ) {
			/* 次のデータを送信 */
			out_w(IIC(cb, TXD), cmd & 0xff);
		} else if ( (cmd & 0xc000) == IIC_RESTART ) {
			if ( xfer_start(cb) < 0 ) goto err_stop;
		} else if ( (cmd & 0xc000) == IIC_STOP ) {
			out_w(IIC(cb, TASKS_STOP), 1);
		}
	}

	if ( (in_w(IIC(cb, EVENTS_RXDREADY)) & 1) != 0 ) {
		out_w(IIC(cb, EVENTS_RXDREADY), 0);
		d = in_w(IIC(cb, RXD)) & 0xff;
		if ( (cmd & IIC_RECV) != 0 ) {
			/* 受信データを保存 */
			*cb->cmddat |= d;
			if ( ++cb->cmddat >= cb->end ) goto err_stop;
			cmd = *cb->cmddat; /* 次へ */
		}
	}

	if ( (in_w(IIC(cb, EVENTS_SUSPENDED)) & 1) != 0 ) {
		out_w(IIC(cb, EVENTS_SUSPENDED), 0);
		if ( (cmd & IIC_LASTDATA) != 0 ) {
			if ( cb->cmddat + 1 >= cb->end ) goto err_stop;
			if ( (*(cb->cmddat + 1) & 0xc000) == IIC_STOP ) {
				out_w(IIC(cb, SHORTS), 2); /* BB_STOP */
			}
		}
		if ( (cmd & 0xc000) == IIC_RESTART ) {
			if ( xfer_start(cb) < 0 ) goto err_stop;
		} else {
			/* 次のデータ受信開始 */
			out_w(IIC(cb, TASKS_RESUME), 1);
		}
	}

	if ( (in_w(IIC(cb, EVENTS_STOPPED)) & 1) != 0 ) {
		out_w(IIC(cb, EVENTS_STOPPED), 0);
		if ( (cmd & 0xc000) == IIC_STOP ) cb->cmddat++;
		return 1;
	}

	return 0;

  err_stop:
	out_w(IIC(cb, TASKS_STOP), 1);
	return -1;
}

/*
 * 割込ハンドラ
 */
LOCAL void iic_inthdr( UINT dintno )
{
	IICCB	*cb;
	UW	ch;

	for ( ch = 0; ch < IICMAX; ch++ ) {
		cb = &iiccb[ch];
		if ( DINTNO(IRQ(cb)) == dintno ) break;
	}
	if ( ch >= IICMAX ) return;

	if ( xfer_cmddat(cb) != 0 ) {
		out_w(IIC(cb, INTENCLR), 0xffffffff);

		/* 終了通知 */
		tk_set_flg(IICFlgID, 1 << ch);
	}
}

/*
 * IIC 送受信処理
 */
EXPORT ER iicxfer( W ch, UH *cmddata, W words, W *xwords )
{
	IICCB	*cb;
	UINT	ptn;
	UW	n;
	ER	err;

	if ( ch < 0 || ch >= IICMAX ) { err = E_PAR; goto err_ret; }
	if ( words < 3 ) { err = E_PAR; goto err_ret; }

	err = MLock(&IICLock, ch);
	if ( err < E_OK ) goto err_ret;

	cb = &iiccb[ch];

	/* イベント／ステータス・クリア */
	out_w(IIC(cb, EVENTS_STOPPED),		0);
	out_w(IIC(cb, EVENTS_RXDREADY),		0);
	out_w(IIC(cb, EVENTS_TXDSENT),		0);
	out_w(IIC(cb, EVENTS_ERROR),		0);
	out_w(IIC(cb, EVENTS_BB),		0);
	out_w(IIC(cb, EVENTS_SUSPENDED),	0);
	out_w(IIC(cb, SHORTS),			0);
	out_w(IIC(cb, ERRORSRC),		0xffffffff);
	out_w(IIC(cb, INTENCLR),		0xffffffff);

	/* 制御情報設定 */
	cb->cmddat = cmddata;
	cb->end	   = cmddata + words;

	/* IIC 動作開始 */
	n = xfer_start(cb);
	if ( n == 0 ) {
		out_w(IIC(cb, INTENSET),
		      (1 << 1) |	/* STOPPED */
		      (1 << 2) |	/* RXDREADY */
		      (1 << 7) |	/* TXDSENT */
		      (1 << 9) |	/* ERROR */
		      (1 << 18));	/* SUSPENDED */

		/* 終了待ち */
		err = tk_wai_flg(IICFlgID, 1 << ch, TWF_ANDW|TWF_BITCLR,
				 &ptn, 1000 + words * 10);

		out_w(IIC(cb, INTENCLR), 0xffffffff);
		if ( err < E_OK ) {
			/* 強制終了 */
			out_w(IIC(cb, TASKS_STOP), 1);
		} else {
			n = in_w(IIC(cb, ERRORSRC)) & 7;
			if ( (n & 4) != 0 &&
			     (*cb->cmddat & IIC_SEND) != 0 ) {
				/* スレーブ側から停止した */
				cb->cmddat++;
			}
			if ( n != 0 )	err = E_IO | n;
			else		err = E_OK;
		}
	} else {
		err = E_PAR;
	}

	/* 転送が完了したワード数の記録 */
	if ( xwords != NULL ) *xwords = cb->cmddat - cmddata;

	MUnlock(&IICLock, ch);

	return err;

  err_ret:
	if ( xwords != NULL ) *xwords = 0;
	return err;
}

/*
 * IIC ドライバ起動/終了
 */
EXPORT ER iicsetup( BOOL start )
{
#define	IICTag	"IIC_"

	ER	err;
	T_CFLG	cflg;
	T_DINT	dint;
	W	ch;
	IICCB	*cb;

	if ( !start ) {
		/* 終了処理 */
		err = E_OK;
		goto finish;
	}

	/* 排他制御用ロック生成 */
	err = CreateMLock(&IICLock, IICTag);
	if ( err < E_OK ) goto err_ret1;

	/* 割込通知用イベントフラグの作成 */
	SetOBJNAME(cflg.exinf, IICTag);
	cflg.flgatr  = TA_TFIFO | TA_WMUL;
	cflg.iflgptn = 0;
	err = tk_cre_flg(&cflg);
	if ( err < E_OK ) goto err_ret2;
	IICFlgID = err;

	for ( ch = 0; ch < IICMAX; ++ch ) {
		cb = &iiccb[ch];
		cb->iob = iic_conf[ch].iob;

		/* 割込ハンドラ登録 */
		dint.intatr = TA_HLNG;
		dint.inthdr = iic_inthdr;
		err = tk_def_int(DINTNO(IRQ(cb)), &dint);
		if ( err < E_OK ) goto err_ret3;

		/* TWI 初期設定 */
		out_w(IIC(cb, ENABLE),    5);
		out_w(IIC(cb, INTENCLR),  0xffffffff);
		out_w(IIC(cb, PSEL_SCL),  iic_conf[ch].psel_scl);
		out_w(IIC(cb, PSEL_SDA),  iic_conf[ch].psel_sda);
		out_w(IIC(cb, FREQUENCY), iic_conf[ch].freq);

		EnableInt(IRQ(cb), IRQ_LEVEL);
	}

	return E_OK;

  finish:
	/* 終了処理 */
	ch = IICMAX;
  err_ret3:
	while ( ch-- > 0 ) {
		cb = &iiccb[ch];
		MLock(&IICLock, ch);
		DisableInt(IRQ(cb));
		tk_def_int(DINTNO(IRQ(cb)), NULL);
	}
	tk_del_flg(IICFlgID);
  err_ret2:
	DeleteMLock(&IICLock);
  err_ret1:
	return err;
}

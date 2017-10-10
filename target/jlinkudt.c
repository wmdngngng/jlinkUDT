/*
 * jlinkudt.c
 *
 *  Created on: 2017年10月9日
 *      Author: houxd
 */
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

/*
 * configurations
 */
#define __DEBUG__													/* global enable */
#define CTRL_BLK_FLAG				"tHiSISflagSTRforUdt2017109"	/* for searching use */
#define CTRL_BLK_ALIGN				(0x1000)						/* 4k align */

/*************************************************************************************************/
/*
 * export
 */
int xgetc();
int xgetc_noblock();
int xfputc(int c, int file);
int xfprintf(int file,const char *fmt, ...);
int xputc(int c);
int xprintf(const char *fmt, ...);
int _xwrite(int file, const uint8_t *buf, unsigned len);
/*************************************************************************************************/
#ifdef __DEBUG__
#define UDT_ENABLE
#endif

#ifdef UDT_ENABLE
typedef struct _CtrlBlkInfo
{
	unsigned int flag;
	unsigned int data;
} __attribute__((packed)) CtrlBlkInfo;

typedef struct _CtrlBlk
{
	uint8_t flag[31];
	uint8_t act;
	CtrlBlkInfo tx;
	CtrlBlkInfo rx;
	uint8_t txbuf[256];
	uint8_t rxbuf[256];
} __attribute__((packed)) CtrlBlk;

static CtrlBlk ctrlBlk __attribute__((aligned(CTRL_BLK_ALIGN))) = { /* align 8k */
	.flag = CTRL_BLK_FLAG,
	.tx = {0,0},
	.rx = {0,0},
};

static int jlink_send_to_host(const uint8_t *s,int l)
{
	int f = 0x00;	//now nouse
	uint32_t res;
	uint32_t file = f & 0xFFFFu;
	uint32_t len = l & 0xFFu;
	memcpy(ctrlBlk.txbuf,s,len);
	ctrlBlk.tx.data = 0x5F000000|(file<<8)|len;
	ctrlBlk.tx.flag = 1;
	do{
		res = ctrlBlk.tx.flag;
	}while(res && ctrlBlk.act);
	return 0;
}

static int jlink_putc_to_host(int c)
{
	int f = 0x00; //now nouse
	uint32_t res;
	int file = f & 0xFFFFu;
	int cc = c & 0xFFu;
	ctrlBlk.tx.data = 0x54000000|(file<<8)|cc;
	ctrlBlk.tx.flag = 1;
	do{
		res = ctrlBlk.tx.flag;
	}while(res && ctrlBlk.act);
	return 0;
}
//static int jlink_recv_from_host(void)
//{
//	int res;
//	res = ctrlBlk.rx.flag;
//	if(res){
//		res = ctrlBlk.rx.data;
//		if((res&0xFF000000u) == 0x5E000000u ){
//			ctrlBlk.rx.data = 0x00000000;
//			return res & 0x0FFu;
//		}
//		ctrlBlk.rx.flag = 0x00000000;
//	}
//	return -1;
//}
static int jlink_getc_from_host(void)
{
	int res;
	res = ctrlBlk.rx.flag;
	if(res){
		res = ctrlBlk.rx.data;
		if((res&0xFF000000u) == 0x55000000u ){
			ctrlBlk.rx.data = 0x00000000;
			return res & 0x0FFu;
		}
		ctrlBlk.rx.flag = 0x00000000;
	}
	return -1;
}
int xgetc_noblock()
{
	return jlink_getc_from_host();
}
int xgetc()
{
	int c;
	do{
		c = xgetc_noblock();
	}while(c==-1);
}

#define NONE                 "\e[0m"
#define RED                  "\e[0;31m"
#define GREEN                "\e[0;32m"
#define BLUE                 "\e[0;34m"
static void parse_arg(int f,const char **cmd,int *cmdlen)
{
	if(f=='r'){
		*cmd = RED;
		*cmdlen = sizeof(RED);
	}else if(f=='g'){
		*cmd = GREEN;
		*cmdlen = sizeof(GREEN);
	}else if(f=='b'){
		*cmd = BLUE;
		*cmdlen = sizeof(BLUE);
	}else{
		*cmd = NONE;
		*cmdlen = sizeof(NONE);
	}

}
int xfputc(int c, int file)
{
	int res;
	const char *cmd;
	int cmdlen;
	if(file){
		parse_arg(file,&cmd,&cmdlen);
		jlink_send_to_host((uint8_t*)cmd,cmdlen);
	}
	res = jlink_putc_to_host(c);
	if(file){
		jlink_send_to_host((uint8_t*)NONE,sizeof(NONE));
	}
	return res;
}
int _xwrite(int file, const uint8_t *buf, unsigned len)
{
	if (len == 1) {
		jlink_putc_to_host(buf[0]);
	} else {
		const char *cmd;
		int cmdlen;
		if (file) {
			parse_arg(file, &cmd, &cmdlen);
			jlink_send_to_host((uint8_t*) cmd, cmdlen);
		}
		int i = 0;
		for (i = 0; i < len; i += 128) {
			int l = len - i;
			if (l >= 128)
				l = 128;
			jlink_send_to_host(buf, len);
		}
		if (file) {
			jlink_send_to_host((uint8_t*) NONE, sizeof(NONE));
		}
	}

	return len;
}
int xfprintf(int file,const char *fmt, ...)
{
	int res;
	va_list va;
	va_start(va,fmt);
	char buf[256];
	res = vsnprintf(buf,256,fmt,va);
	va_end(va);
	_xwrite(file,(uint8_t*)buf,res);
	if(res==256){
		const char inf[] = "\n*** xprintf may be truncated.\n";
		_xwrite(1,(uint8_t*)inf,sizeof(inf)-1);
	}
	return res;
}
int xputc(int c){
	xfputc(c,0);
}
int xprintf(const char *fmt, ...){
	int res;
	va_list va;
	va_start(va,fmt);
	char buf[256];
	res = vsnprintf(buf,256,fmt,va);
	va_end(va);
	_xwrite(0,(uint8_t*)buf,res);
	if(res==256){
		const char inf[] = "\n*** xprintf may be truncated.\n";
		_xwrite(1,(uint8_t*)inf,sizeof(inf)-1);
	}
	return res;
}
#else
int xgetc(){}
int xfputc(int c, int file){}
int xfprintf(int file,const char *fmt, ...){}
int xputc(int c){}
int xprintf(const char *fmt, ...){}
int _xwrite(int file, const uint8_t *buf, unsigned len){}
#endif



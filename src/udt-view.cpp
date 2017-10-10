//============================================================================
// Name        : udt-view.cpp
// Author      : houxd
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <dlfcn.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>

#define CTRL_BLK_FLAG				"tHiSISflagSTRforUdt2017109"	/*for searching use*/
#define CTRL_BLK_ALIGN				(0x1000)						/*4k align*/

#pragma pack(push)
#pragma pack(1)
struct CtrlBlkInfo {
	uint32_t flag;
	uint32_t data;
};
struct CtrlBlk {
	uint8_t flag[31];
	uint8_t actflag;
	CtrlBlkInfo tx;
	CtrlBlkInfo rx;
	uint8_t txbuf[256];
	uint8_t rxbuf[256];
};
#pragma pack(pop)

#define TRAD_OFFSET(m)		(cb_addr+offsetof(CtrlBlk,m))

uint32_t cb_addr = 0;
CtrlBlk ctrl_blk;
FILE* flog;
time_t log_time;
uint32_t log_tickcount;
volatile bool run_flag = 1;

bool (*JLINKARM_IsHalted)(void);
bool (*JLINKARM_Halt)(void);
void (*JLINKARM_Open)(void);
void (*JLINKARM_Close)(void);
bool (*JLINKARM_IsOpen)(void);
uint32_t (*JLINKARM_GetSN)(void);
uint32_t (*JLINKARM_GetId)(void);
uint32_t (*JLINKARM_GetDLLVersion)(void);
uint32_t (*JLINKARM_GetSpeed)(void);
uint32_t (*JLINKARM_ReadDCC)(uint32_t *buf, uint32_t size, int32_t timeout);
uint32_t (*JLINKARM_WriteDCC)(const uint32_t *buf, uint32_t size,int32_t timeout);
uint32_t (*JLINKARM_ReadDCCFast)(uint32_t *buf, uint32_t size, int32_t timeout);
uint32_t (*JLINKARM_WriteDCCFast)(const uint32_t *buf, uint32_t size,int32_t timeout);
uint32_t (*JLINKARM_WaitDCCRead)(uint32_t timeout);
void (*JLINKARM_SetSpeed)(uint32_t spd);
uint32_t (*JLINKARM_ExecCommand)(const char* cmd, uint32_t a, uint32_t b);

void (*JLINKARM_WriteU8)(uint32_t addr, uint8_t dat);
void (*JLINKARM_WriteU16)(uint32_t addr, uint16_t dat);
void (*JLINKARM_WriteU32)(uint32_t addr, uint32_t dat);
uint32_t (*JLINKARM_ReadMemU8)(uint32_t addr, uint32_t leng, uint8_t *buf,uint8_t *status);
uint32_t (*JLINKARM_ReadMemU16)(uint32_t addr, uint32_t leng, uint16_t *buf,uint8_t *status);
uint32_t (*JLINKARM_ReadMemU32)(uint32_t addr, uint32_t leng, uint32_t *buf,uint8_t *status);
void (*JLINKARM_ReadMemHW)(uint32_t addr, uint32_t leng, uint8_t *buf);
uint32_t (*JLINK_TIF_Select)(uint32_t tif);
uint32_t (*JLINK_Connect)(void);
uint32_t (*JLINK_IsConnected)(void);
void (*JLINKARM_ReadMem)(uint32_t addr, uint32_t leng, uint8_t *buf);
void (*JLINK_ReadMemU8)(uint32_t addr, uint32_t leng, uint8_t *buf,uint8_t *status);
uint32_t (*JLINK_GetMemZones)(uint32_t a, uint32_t b);
void (*JLINKARM_Go)(void);

int load_jlinkarm_dll(char *libpath) {
	void* lib = dlopen(libpath, RTLD_NOW);
	if (lib == NULL) {
		return -1;
	}

	JLINKARM_Open = (void (*)(void))(dlsym(lib, "JLINKARM_Open"));
	JLINKARM_Close = (void (*)(void))(dlsym(lib, "JLINKARM_Close"));
	JLINKARM_IsOpen = (bool (*)(void))dlsym(lib, "JLINKARM_IsOpen");
	JLINKARM_GetSN = (uint32_t (*)(void))dlsym(lib, "JLINKARM_GetSN");
	JLINKARM_GetId = (uint32_t (*)(void))dlsym(lib, "JLINKARM_GetId");
	JLINKARM_GetDLLVersion = (uint32_t (*)(void))dlsym(lib, "JLINKARM_GetDLLVersion");
	JLINKARM_GetSpeed = (uint32_t (*)(void))dlsym(lib, "JLINKARM_GetSpeed");
	JLINKARM_ReadDCC = (uint32_t (*)(uint32_t *, uint32_t,int32_t))dlsym(lib, "JLINKARM_ReadDCC");
	JLINKARM_WriteDCC = (uint32_t (*)(const uint32_t *, uint32_t, int32_t))dlsym(lib, "JLINKARM_WriteDCC");
	JLINKARM_ReadDCCFast = (uint32_t (*)(uint32_t *, uint32_t, int32_t))dlsym(lib, "JLINKARM_ReadDCCFast");
	JLINKARM_WriteDCCFast = (uint32_t (*)(const uint32_t *, uint32_t,int32_t))dlsym(lib, "JLINKARM_WriteDCCFast");
	JLINKARM_WaitDCCRead = (uint32_t (*)(uint32_t))dlsym(lib, "JLINKARM_WaitDCCRead");
	JLINKARM_SetSpeed = (void (*)(uint32_t))dlsym(lib, "JLINKARM_SetSpeed");
	JLINKARM_ExecCommand = (uint32_t (*)(const char *, uint32_t,uint32_t))dlsym(lib, "JLINKARM_ExecCommand");

	JLINKARM_WriteU8 = (void (*)(uint32_t, uint8_t))dlsym(lib, "JLINKARM_WriteU8");
	JLINKARM_WriteU16 = (void (*)(uint32_t, uint16_t))dlsym(lib, "JLINKARM_WriteU16");
	JLINKARM_WriteU32 = (void (*)(uint32_t, uint32_t))dlsym(lib, "JLINKARM_WriteU32");
	JLINKARM_ReadMemU8 = (uint32_t (*)(uint32_t, uint32_t, uint8_t *,uint8_t *))dlsym(lib, "JLINKARM_ReadMemU8");
	JLINKARM_ReadMemU16 = (uint32_t (*)(uint32_t, uint32_t, uint16_t *,uint8_t *))dlsym(lib, "JLINKARM_ReadMemU16");
	JLINKARM_ReadMemU32 = (uint32_t (*)(uint32_t, uint32_t, uint32_t *,uint8_t *))dlsym(lib, "JLINKARM_ReadMemU32");
	JLINKARM_ReadMem = (void (*)(uint32_t, uint32_t, uint8_t *))dlsym(lib, "JLINKARM_ReadMem");
	JLINKARM_ReadMemHW = (void (*)(uint32_t, uint32_t, uint8_t *))dlsym(lib, "JLINKARM_ReadMemHW");
	JLINK_ReadMemU8 = (void (*)(uint32_t, uint32_t, uint8_t *,uint8_t*))dlsym(lib, "JLINK_ReadMemU8");
	JLINK_GetMemZones = (uint32_t (*)(uint32_t, uint32_t))dlsym(lib,"JLINK_GetMemZones");
	JLINK_TIF_Select = (uint32_t (*)(uint32_t))dlsym(lib, "JLINK_TIF_Select");
	JLINK_Connect = (uint32_t (*)(void))dlsym(lib, "JLINK_Connect");
	JLINK_IsConnected = (uint32_t (*)(void))dlsym(lib, "JLINK_IsConnected");
	JLINKARM_Go = (void (*)(void))dlsym(lib, "JLINKARM_Go");
	JLINKARM_Halt = (bool (*)(void))dlsym(lib, "JLINKARM_Halt");
	JLINKARM_IsHalted = (bool (*)(void))dlsym(lib, "JLINKARM_IsHalted");
	return 0;
}

uint32_t JLINKARM_WriteMenU8(uint32_t addr, uint32_t leng, uint8_t *buf) {
	if (leng == 1) {
		JLINKARM_WriteU8(addr, buf[0]);
	} else if (leng == 2) {
		uint16_t dat = buf[0];
		dat <<= 8;
		dat |= buf[1];
		JLINKARM_WriteU16(addr, dat);
	} else if (leng == 3) {
		JLINKARM_WriteU8(addr, buf[0]);
		uint16_t dat = buf[1];
		dat <<= 8;
		dat |= buf[2];
		JLINKARM_WriteU16(addr, dat);
	} else {
		for (uint i = 0; i < leng; i += 4) {
			int l = leng - i;
			if (l >= 4) {
				uint32_t dat = buf[i + 0];
				dat <<= 8;
				dat |= buf[i + 1];
				dat <<= 8;
				dat |= buf[i + 2];
				dat <<= 8;
				dat |= buf[i + 3];
				JLINKARM_WriteU32(addr, dat);
			} else {
				for (int j = i; j < l; j++) {
					JLINKARM_WriteU8(addr, buf[j]);
				}
			}
		}
	}
	return leng;
}

int _getch(void) {
	return getchar();
}

int change_ch(int ch) {
  return ch;
}
void delay_ms(int ms) {
	usleep(ms * 1000);
}
unsigned long get_tick_count() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

void jlink_send_to_target(int c) {
	ctrl_blk.rx.data = 0x55000000u | c;
	uint32_t res;
	do {
		pthread_testcancel();
		JLINKARM_ReadMemU32(TRAD_OFFSET(rx.flag), 1, &res, 0);
		delay_ms(5);
	} while (res);
	JLINKARM_WriteU32(TRAD_OFFSET(rx.data), ctrl_blk.rx.data);
	//JLINKARM_WriteMenU8(TRAD_OFFSET(rx.data), sizeof(trad_cb.rx.data), (uint8_t*)&trad_cb.rx.data);
	JLINKARM_WriteU32(TRAD_OFFSET(rx.flag), 1);
}

void log_putchar(int c) {
	if (flog == NULL)
		return;
	fputc(c, flog);
	if (c == '\n') {
		uint32_t n = get_tick_count() - log_tickcount;
		time_t t = log_time + n / 1000;
		int ms = n % 1000;
		struct tm *tm = localtime(&t);
		fprintf(flog, "[%02u:%02u:%02u:%03u]\t", tm->tm_hour, tm->tm_sec,
				tm->tm_sec, ms);
	}
	fflush(flog);
}

void* thread_recv(void* pM) {
	while (run_flag) {
//		char buf[4096];
//		char *p = fgets(buf,4096,stdin);
		unsigned char c = _getch();
		jlink_send_to_target(c);
	}
	return NULL;
}

void parse_args(int argc, char*argv[], const char **interface, int *speed,
		const char **device, uint32_t *rambeg, uint32_t *ramend) {
	int oc;
	while ((oc = getopt(argc, argv, "i:d:s:b:e")) != -1) {
		switch (oc) {
		case 'i': {
			*interface = strdup(optarg);
			break;
		}
		case 'd': {
			*device = strdup(optarg);
			break;
		}
		case 's': {
			char *e = NULL;
			*speed = strtoul(optarg, &e, 10);
			if (e)
				goto usage;
			break;
		}
		case 'b': {
			char *e = NULL;
			*rambeg = strtoul(optarg, &e, 16);
			if (e)
				goto usage;
			break;
		}
		case 'e': {
			char *e = NULL;
			*ramend = strtoul(optarg, &e, 16);
			if (e)
				goto usage;
			break;
		}
		case '?':
		case ':': {
			goto usage;
			break;
		}
		}
	}
	return;

	usage: printf("udtview -i<Interface> -d<Device> -s<Speed> -a<Addr>\n");
	exit(1);
}
/*
 *
 *  .a =>  "" "" .a
 *  a =>  "" a ""
 * 	./a	=>		./ a  ""
 *  /.a => 		/ "" .a
 * 	/a.txt	=> 	/ a .txt
 * 	/home/a.txt => /home/ a .txt
 * 	/home/a => 	/home/ a ""
 * 	/home/ => 	/home/ "" ""
 * 	/home => 	/ home ""
 */
void path_split(const char *fpath, char (*parent)[PATH_MAX],
		char (*fname)[NAME_MAX], char (*fname_noext)[NAME_MAX],
		char (*ext)[NAME_MAX]) {
	const char *l = strrchr(fpath, '/');
	const char *d = strrchr(fpath, '.');
	const char *_fname;
	if (l == NULL) {
		l = fpath;
		_fname = fpath;
	} else {
		l++;
		_fname = l;
	}
	int x = l - fpath;
	if (parent) {
		if (x) {
			strncpy(*parent, fpath, x);
			(*parent)[x] = 0;
		} else {
			strcpy(*parent, "");
		}
	}
	if (fname)
		strcpy(*fname, _fname);
	if (d && d > l) {
		if (fname_noext) {
			int x = d - l;
			if (x) {
				strncpy(*fname_noext, _fname, x);
				(*fname_noext)[x] = 0;
			} else {
				strcpy(*fname_noext, "");
			}
		}
		if (ext)
			strcpy(*ext, d);
	} else {
		if (fname_noext)
			strcpy(*fname_noext, _fname);
		if (ext)
			strcpy(*ext, "");
	}
}
int path_get_parent(const char *fpath, char *buf, int bufmax) {
	char parent[PATH_MAX];
	path_split(fpath, &parent, 0, 0, 0);
	buf[0] = 0;
	if (strlen(parent) > (size_t) (bufmax - 1))
		return -1;
	strcpy(buf, parent);
	return 0;
}
uint32_t tif_from_str(const char *s) {
#define JLINKARM_TIF_JTAG	0
#define JLINKARM_TIF_SWD	1
#define JLINKARM_TIF_DBM3	2
#define JLINKARM_TIF_FINE	3
#define JLINKARM_TIF_2wire_JTAG_PIC32	4
	if (strcmp(s, "JTAG") == 0)
		return JLINKARM_TIF_JTAG;
	else if (strcmp(s, "SWD") == 0)
		return JLINKARM_TIF_SWD;
	else if (strcmp(s, "DBM3") == 0)
		return JLINKARM_TIF_DBM3;
	else if (strcmp(s, "FINE") == 0)
		return JLINKARM_TIF_FINE;
	else if (strcmp(s, "2wire_JTAG_PIC32") == 0)
		return JLINKARM_TIF_2wire_JTAG_PIC32;
	else {
		printf("WARN: Not surpport interface, default ust SWD\n");
		return JLINKARM_TIF_SWD;
	}
}
void reset_jlink(const char *interface, int speed, const char *device) {
	if (!JLINKARM_IsOpen()) {
		JLINKARM_Open();
		char buff[128];
		sprintf(buff, "device = %s", device);
		JLINKARM_ExecCommand(buff, 0, 0);
		JLINKARM_SetSpeed(speed);
	}
	JLINK_TIF_Select(tif_from_str(interface));
	JLINK_Connect();
	if (JLINKARM_IsHalted())
		JLINKARM_Go();
}
sighandler_t def_sighandler;
void sighandler(int sig)
{
	if(sig==SIGINT){
		run_flag = 0;
		JLINKARM_Close();
		puts("\e[0m");
		exit(0);
	}else{
		def_sighandler(sig);
	}
}
int main(int argc, char *argv[]) {
	int res;
	def_sighandler = signal(SIGINT, sighandler);
	//SetConsoleCtrlHandler(ConsoleHandler, true);
	printf("Jlink Debug Terminal View V0.2 by houxd ,build %s %s\n", __DATE__,
			__TIME__);

	/*
	 * parse args
	 */
	const char *interface = "SWD";
	const char *device = "S32K144";
	int speed = 200;
	uint32_t rambeg = 0x1FFF8000;
	uint32_t ramend = 0x20007000;
	cb_addr = 0xFFFFFFFF;
	parse_args(argc, argv, &interface, &speed, &device, &rambeg,&ramend);
	printf("Interface: %s\n", interface);
	printf("Device: %s\n", device);
	printf("Speed: %d\n", speed);

	/*
	 * load lib
	 */
	char sdir[512];
	path_get_parent(argv[0], sdir, 512);
	char libpath[512];
	snprintf(libpath, 512, "%slibjlinkarm.so", sdir);
	res = load_jlinkarm_dll(libpath);
	if (res) {
		printf("Cannot Load LibJLinkARM. \n");
		exit(-1);
	}
	printf("Load LibJLinkARM Success \n");

	/*
	 * open or create log file
	 */
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);
	char log_file[512];
	snprintf(log_file, 512, "%slog-%04u%02u%02u.txt", sdir, tm->tm_year + 1900,
			tm->tm_mon + 1, tm->tm_mday);
	flog = fopen(log_file, "a+");
	if (flog == NULL) {
		printf("WARNING: log file can not creat.\n");
	}
	fprintf(flog,"------------------------------------------------------------\n");
	log_tickcount = get_tick_count();
	log_time = time(NULL);
	log_putchar('\n');	//record a time label
	/*
	 * open jlink device
	 */
	printf("Open JLink ... ");
	JLINKARM_Open();
	if(JLINKARM_IsOpen()==false){
		printf("Error\n");
		exit(-1);
	} else {
		printf("OK\n");

		/*
		 * connect to device
		 */
		char cmdbuf[128];
		snprintf(cmdbuf, 128, "device = %s", device);
		JLINKARM_ExecCommand(cmdbuf, 0, 0);
		JLINK_TIF_Select(tif_from_str(interface));	//JLINKARM_TIF_SWD);
		JLINK_Connect();
		JLINKARM_SetSpeed(speed);
		printf("SN = %08u\n", JLINKARM_GetSN());
		printf("ID = %08X\n", JLINKARM_GetId());
		printf("VER = %u\n", JLINKARM_GetDLLVersion());
		printf("Speed = %u\n", JLINKARM_GetSpeed());

		/*
		 * search ctrl block
		 */
		printf("Find Ctrl block in [0x%08x,0x%08x] align 0x%x ...\n", rambeg,ramend,CTRL_BLK_ALIGN);
		while(run_flag){
			uint32_t pos = rambeg/CTRL_BLK_ALIGN*CTRL_BLK_ALIGN;
			do{
				uint8_t buf[32];
				JLINKARM_ReadMemU8(pos,4,buf,0);
				if(memcmp(buf,CTRL_BLK_FLAG,4)==0){
					JLINKARM_ReadMemU8(pos+4,32-4,&buf[4],0);
					if(memcmp(buf,CTRL_BLK_FLAG,sizeof(CTRL_BLK_FLAG))==0){
						cb_addr = pos;
						break;
					}
				}
				pos+=CTRL_BLK_ALIGN;
			}while(pos<ramend);
			if(cb_addr!=0xFFFFFFFF)
				break;
		}
		if(run_flag==0){
			JLINKARM_Close();
			exit(0);
		}
		printf("CtrlBLkAddr: 0x%08X\n", cb_addr);
		printf("=============================================================\n");
		JLINKARM_WriteU32(TRAD_OFFSET(actflag), 0xFF);

		/*
		 * create write thread
		 */
		pthread_t readth;
		int err = pthread_create(&readth, NULL, thread_recv, NULL);
		if (err != 0) {
			printf("create thread error: %s\n", strerror(err));
			return 1;
		}

		/*
		 * run
		 */
		while (run_flag) {
			/*
			 * read frame
			 */
			res = JLINKARM_ReadMemU8(TRAD_OFFSET(tx), sizeof(ctrl_blk.tx),
					(uint8_t*) &ctrl_blk.tx, 0);
			if (res == -1) {
				/*
				 * read interrupt reconnect it
				 */
				reset_jlink(interface, speed, device);
				delay_ms(100);
				continue;
			}
			/*
			 * parse frame
			 */
			if (res == sizeof(ctrl_blk.tx) && ctrl_blk.tx.flag) {
				uint32_t data = ctrl_blk.tx.data;
				if ((data >> 24) == 0x54) {
					int ch = (data >> 8) & 0xFFu;
					int chbak = change_ch(ch);
					int c = data & 0xFFu;
					//if (ch >= 0 && ch <= 0x7F) {
						putchar(c);
						log_putchar(c);
					//}
					change_ch(chbak);
				} else if ((data >> 24) == 0x5F) {
					int ch = (data >> 8) & 0xFFu;
					int chbak = change_ch(ch);
					int len = data & 0xFFu;
					//if (ch > 0 && ch <= 128) {
						uint8_t dat[128];
						res = JLINKARM_ReadMemU8(TRAD_OFFSET(txbuf), len, dat,
								0);
						if (res == -1) {
							reset_jlink(interface, speed, device);
							delay_ms(100);
						}
						for (int i = 0; i < len; i++) {
							putchar(dat[i]);
							log_putchar(dat[i]);
						}
					//}
					change_ch(chbak);
				}
				ctrl_blk.tx.flag = 0;
				//JLINKARM_WriteMenU8(TRAD_OFFSET(tx.flag), sizeof(trad_cb.tx.flag), (uint8_t*)&trad_cb.tx.flag);
				JLINKARM_WriteU32(TRAD_OFFSET(tx.flag), 0);

			}
			delay_ms(1);
		}
		pthread_cancel(readth);
		pthread_join(readth,NULL);
		JLINKARM_Close();
		puts("\e[0m");
	}

	return 0;
}

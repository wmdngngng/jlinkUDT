/*
 * jlinkudt.h
 *
 *  Created on: 2017年10月10日
 *      Author: houxd
 */

#ifndef JLINKUDT_H_
#define JLINKUDT_H_

extern int xgetc();
extern int xgetc_noblock();
extern int xfputc(int c, int file);
extern int xfprintf(int file,const char *fmt, ...);
extern int xputc(int c);
extern int xprintf(const char *fmt, ...);
extern int _xwrite(int file, const unsigned char *buf, unsigned len);

#endif /* JLINKUDT_H_ */

#include "font.c"
#include "main_text.h"

#define		LINESIZE	512				//in short
extern char *pgGetVramAddr(unsigned long x,unsigned long y);


unsigned short rgb2col(unsigned char r,unsigned char g,unsigned char b)
{
	return ((((b>>3) & 0x1F)<<10)+(((g>>3) & 0x1F)<<5)+(((r>>3) & 0x1F)<<0)+0x8000);
}




// ---------------------------------------------------------------------------------

unsigned short num2elisa(unsigned short c) {
	if (c >= 4418) {
		return c + (0xda1 - 4418);
	} else if (c >= 1410) {
		return c + (0x20c - 1410);
	} else if (c >= 690) {
		return 0x6b;
	} else if (c >= 658) {
		return c + (0x1ec - 658);
	} else if (c >= 612) {
		return c + (0x1cb - 612);
	} else if (c >= 564) {
		return c + (0x1aa - 564);
	} else if (c >= 502) {
		return c + (0x192 - 502);
	} else if (c >= 470) {
		return c + (0x17a - 470);
	} else if (c >= 376) {
		return c + (0x124 - 376);
	} else if (c >= 282) {
		return c + (0xd1 - 282);
	} else if (c >= 252) {
		return c + (0xb7 - 252);
	} else if (c >= 220) {
		return c + (0x9d - 220);
	} else if (c >= 203) {
		return c + (0x93 - 203);
	} else if (c >= 187) {
		return 0x92;
	} else if (c >= 175) {
		return c + (0x8a - 203);
	} else if (c >= 153) {
		return c + (0x7b - 153);
	} else if (c >= 135) {
		return c + (0x74 - 135);
	} else if (c >= 119) {
		return c + (0x6c - 119);
	} else {
		return c;
	}
}

void Draw_Char_Hankaku(int x,int y,unsigned char ch,int col,int backcol,int fill) {
	unsigned short *vr;
	unsigned char  *fnt;
	unsigned char  pt;
	int y1;

	// mapping
	if (ch<0x20) {
		ch = 0;
	} else if (ch<0x80) {
		ch -= 0x20;
	} else if (ch<0xa0) {
		ch = 0;
	} else {
		ch -= 0x40;
	}
	fnt = (unsigned char *)&hankaku_font10[ch*10];

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		vr += LINESIZE-5;
	}
}

void Draw_Char_Zenkaku(int x,int y,unsigned char u,unsigned char d,int col,int backcol,int fill) {
	unsigned short *vr;
	unsigned short *fnt;
	unsigned short pt;
	int y1;

	// mapping
	if (d > 0x7F) d--;
	if (u > 0x9F) u-=0x40;
	d -= 0x40; u -= 0x81;

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		if (pt & 1) { *vr = col; } else { if (fill) *vr = backcol; } vr++; pt = pt >> 1;
		vr += LINESIZE-10;
	}
}

void text_print(int x,int y,char *str,int col,int backcol,int fill) {
unsigned char ch = 0,bef = 0;

	while(*str != 0) {
		ch = (unsigned char)(*str++);
		if (bef!=0) {
			x+=10;
			bef=0;
		} else {
			if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
				bef = ch;
			} else {
				Draw_Char_Hankaku(x,y,ch,col,backcol,fill);
				x+=5;
			}
		}
	}
}

// ---------------------------------------------------------------------------------
void	text_counter(int x,int y,unsigned int c,int k,int col)
{
	char m[2];
	m[1]=0;
	do{
		m[0]='0'+(c%10);
		text_print(x-=5,y,m,col,0,1);
		c/=10;
		if(!(--k))c=0;
	}while(c);
	while(k--){
		text_print(x-=5,y," ",col,0,1);
	}

}
void	text_counterh(int x,int y,unsigned int c,int k,int col)
{
	char m[2];
	static char m2[]="0123456789ABCDEF";
	m[1]=0;
	while(k--){
		m[0]=m2[(c&15)];
		text_print(x-=5,y,m,col,0,1);
		c/=16;
	}
	while(k--){
		text_print(x-=5,y," ",col,0,1);
	}

}



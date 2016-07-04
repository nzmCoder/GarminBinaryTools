/****************************************************************************
GAR2RNX CONVERTER converts Garmin binary G12 files to Rinex

Copyright (C) 2000-2002 Antonio Tabernero
Copyright (C) 2016 Norm Moulton
Created 28 Jun 2000, Last modified 23 Mar 2002, Antonio Tabernero.
Modifications started 30 May 2016, Norm Moulton

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

****************************************************************************/

/****************************************************************************

1.50 * Bring header up to 2.11 format
     * Fix all compile warnings

1.49 * Changes 2016 by N. Moulton
     * Reformated source code

1.48 * Version published. Minor cosmetic changes from 1.45

1.45 * Added -monitor, -V, -sf  options

1.4  * Added generation of RINEX ephemeris files (-nav option)
     * Added decoding of the navigation message
     * Added option to get input from stdin
     * Some bugs corrected

1.3  * Can use records 0x0e and 0x11 to get aprox time and position
       instead of requiring record 0x33 (which seems to be missing
       in some models/firmwares)
     * Output Doppler data (using the flag +doppler) when applied to
       binary files logged with the 1.2 (or newer) version of async.

****************************************************************************/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>

#define VERSION 1.50


#define AS_BYTE   0
#define AS_CHAR   1
#define AS_UINT   2
#define AS_ULONG  3
#define AS_FLOAT  4
#define AS_DOUBLE 5


#define NEVER_USED 0
#define NO_DUMP    1
#define DUMP       2
#define DUMP_PHASE_ONLY 3


#define c 2.99792458e8  // WGS84 Speed of light
#define L1 1575420000   // L1 frequency
#define lambda (c/L1)   // L1 wavelength
#define WGS84_PI 3.1415926535898


typedef unsigned char BOOLEAN;
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned short int UINT;
typedef short int INT;


BOOLEAN VC_format=1;
BOOLEAN STDIN=0;

BOOLEAN ETREX;
BOOLEAN RESET_CLOCK;
BYTE ONLY_STATS, SELECTED_SV, ONE_SAT, DIF_RECORDS;
int SELECTED_SF,SELECTED_PAGE;
BYTE VERBOSE,VERBOSE_NAV;
BYTE NAV_GENERATION,MONITOR_NAV,PARSE_RECORDS,RINEX_GENERATION,VERIFY_TIME_TAGS,NO_SNR;
BYTE OPT1,RELAX;

double USER_XYZ[3];
int USER_DATE[3];
BOOLEAN GIVEN_XYZ,GIVEN_DATE;


int L1_FACTOR,GET_THIS;
int SELECTED_RECORDS[16];
BYTE RINEX_FILE;
BYTE OBS_MASK,N_OBS;
UINT INTERVAL;
long START,LAST,ELAPSED;

char DATAFILE[60];
char COMMAND_LINE[256];
char location[6];
char marker[32];


BYTE LSB[32] = { 7, 6, 5, 4, 3, 2, 1, 0,
                 15,14,13,12,11,10, 9, 8,
                 23,22,21,20,19,18,17,16,
                 31,30,29,28,27,26,25,24
               };

ULONG NAV_WORD;

// data bits: bits 30 -> 7 within ULONG NAV_WORD
#define d1  ( (NAV_WORD>>29) & 1 )
#define d2  ( (NAV_WORD>>28) & 1 )

#define d3  ( (NAV_WORD>>27) & 1 )
#define d4  ( (NAV_WORD>>26) & 1 )
#define d5  ( (NAV_WORD>>25) & 1 )
#define d6  ( (NAV_WORD>>24) & 1 )
#define d7  ( (NAV_WORD>>23) & 1 )
#define d8  ( (NAV_WORD>>22) & 1 )
#define d9  ( (NAV_WORD>>21) & 1 )
#define d10  ( (NAV_WORD>>20) & 1 )

#define d11  ( (NAV_WORD>>19) & 1 )
#define d12  ( (NAV_WORD>>18) & 1 )
#define d13  ( (NAV_WORD>>17) & 1 )
#define d14  ( (NAV_WORD>>16) & 1 )
#define d15  ( (NAV_WORD>>15) & 1 )
#define d16  ( (NAV_WORD>>14) & 1 )
#define d17  ( (NAV_WORD>>13) & 1 )
#define d18  ( (NAV_WORD>>12) & 1 )
#define d19  ( (NAV_WORD>>11) & 1 )
#define d20  ( (NAV_WORD>>10) & 1 )
#define d21  ( (NAV_WORD>>9) & 1 )
#define d22  ( (NAV_WORD>>8) & 1 )
#define d23  ( (NAV_WORD>>7) & 1 )
#define d24  ( (NAV_WORD>>6) & 1 )

// Parity bits: bits 6 -> 1 within ULONG NAV_WORD
#define D25  ( (NAV_WORD>>5) & 1 )
#define D26  ( (NAV_WORD>>4) & 1 )
#define D27  ( (NAV_WORD>>3) & 1 )
#define D28  ( (NAV_WORD>>2) & 1 )
#define D29  ( (NAV_WORD>>1) & 1 )
#define D30  ( (NAV_WORD>>0) & 1 )

// Previous word parity bits 29-30: bits 32 and 31 of ULONG NAV_WORD
#define P29  ( (NAV_WORD>>31) & 1 )
#define P30  ( (NAV_WORD>>30) & 1 )


BYTE OK[3]= {1,1,1};
BYTE RESET[30]= { 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0};

BYTE frame[32][30];
BOOLEAN check_frame[32][30];

int pages[64] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,
                 16,17,18,19,20,21,22,23,24, 2, 3, 4, 5, 7, 8, 9,
                 10,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
                 -1,-1,-1,25,13,14,15,17,18, 1,19,20,22,23,12,25
                };

////// STRUCTS  of RECORDS with (more or less) known fields

typedef struct
{
    double llh[3];
    double xyz[3];
} type_rec0x11;

typedef struct
{
    int week;
    int garmin_wdays;
    double tow;
} type_rec0x0e;

typedef struct
{
    float delta_pr;
    float f1;
    double pr;
    float f2;
    BYTE sv;
}
type_rec0x16;

typedef struct
{
    BYTE sv;
    BYTE elev;
    UINT frac;
    UINT db;
    BYTE tracked;
    BYTE flag;
}
type_rec0x1a;

typedef struct
{
    float altitud;
    float epe[3];
    UINT fix;
    double tow;
    double pos[2];
    float vel[3];
    float h_ellip;
    UINT leap;
    ULONG wdays;
}
type_rec0x33;


typedef struct
{
    ULONG c50;
    BYTE uk[4];
    BYTE sv;
}
type_rec0x36;


typedef struct
{
    ULONG c_phase;
    long int tracked;   //BYTE  tracked; UINT cc2; BYTE flag;
    UINT  delta_f;
    ULONG  int_phase;
    double pr;
    ULONG  c511;
    UINT  db;
    double tow;
    BYTE   sv;
}
type_rec0x38;


typedef struct
{
    BYTE used;
    UINT db;
    double prange;
    double phase;
    float doppler;
    BYTE elev;
    long int tracked;
    ULONG c511;
    float last36;
}
rinex_obs;


// Global variables used to study rates of change, etc.

type_rec0x38 rec38,last38;
float mean,sigma,mean_q;
float phase_rate;
float mean2=0;
float sigma2=0;
int n;
int n2=0;


// function declarations
void llh2xyz(double*,double*);
int get_messages(char h[32][32]);
int show_alm();


type_rec0x11 process_0x11(BYTE *record) // Position
{
    type_rec0x11 rec;

    memcpy(&rec.llh[0],record,8);
    memcpy(&rec.llh[1],record+8,8);


    rec.llh[0]*=(180/WGS84_PI);
    rec.llh[1]*=(180/WGS84_PI);
    rec.llh[2]=0.0;
    llh2xyz(rec.llh,rec.xyz);

    if(VERBOSE)
    {
        printf("Record 0x11 :: "),
               printf("Lat %.4f  Long %.4f\n",rec.llh[0],rec.llh[1]);
        printf("XYZ(h=0.0) = [ %.0f %.0f %.0f]\n",rec.xyz[0],rec.xyz[1],rec.xyz[2]);
    }

    return rec;
}

double julday(year,month,day, hour, min, sec)
UINT  year,month,day,hour,min;
double sec;
{
    double jd;

    if(month <= 2)
    {
        year-=1;
        month+=12;
    }
    jd = floor(365.25*year)+floor(30.6001*(month+1))+day;
    jd +=(hour+(min+sec/60.0)/60.0)/24+1720981.5;
    return jd;
}

double gps_time(double julday, int *week)
{
    double tow,days_gps;
    int wn;
//int weekday;

//weekday=floor(julday+0.5)%7  + 1 ;  // 0 Sunday, 1 Monday, ...

// Correct at least for the 13 leap seconds as of writting this
    julday+= (13.0/86400.0);

    days_gps=(julday-2444244.5);
    wn = (int)floor(days_gps/7);

    tow = (days_gps- wn*7)*86400;

    *week=wn;

    return tow;
}

type_rec0x0e process_0x0e(BYTE *record)
{
    type_rec0x0e rec;
    UINT month,day,year,hour,min,sec;
    double jd;

    /*
     typedef struct {BYTE month;BYTE day;UINT year;UINT hour;BYTE min;BYTE sec;} UTC;
     UTC utc;
     utc.month=record[0];
     utc.day=record[1];
     utc.year=*((short int*)(record+2));
     utc.hour=*((short int*)(record+4));
     utc.min=record[6];
     utc.sec=record[7];
    */

    month=(UINT)record[0];
    day=(UINT)record[1];
    year=*((short int*)(record+2));
    hour=*((short int*)(record+4));
    min=(UINT)record[6];
    sec=(UINT)record[7];

    jd=julday(year,month,day,hour,min,(double)sec);
    rec.tow=gps_time(jd,&rec.week);
    rec.garmin_wdays=(rec.week-521)*7;


    if(VERBOSE)
    {
        printf("%02u/%02u/%4d ",day,month,year);
        printf("%02d:%02d:%02d -> JD %f -> ",hour,min,sec,jd);
        printf("GPS Week %4d ToW %6.0f sec  Garmin Weekdays %d\n",rec.week,rec.tow,rec.garmin_wdays);
    }

    return rec;
}

void process_0x12_gps38(BYTE *record)
{
    printf("PRN %2d ",record[3]);
    printf("TOW %14.7f ",*((double*)(record+18)));
    printf("Pseudorange %14.3f\n",*((double*)(record+26)));
}




void process_0x14(BYTE *record)
{

    printf("Fix %d %d\n",*((UINT*)record),*((UINT*)(record+2)));
    printf("TOW4 %.7f  TOW12 %.7f\n",*((double*)(record+4)),*((double*)(record+12)));

    printf("Posicion %.4f %.4f\n",*((double*)(record+28))*180/WGS84_PI,*((double*)(record+36))*180/WGS84_PI);

    printf("Vel  %f %f %f\n",*((float*)(record+48)),*((float*)(record+52)),*((float*)(record+56)));
}



type_rec0x36 process_0x36(BYTE *record)
{
    type_rec0x36 rec;
    int i;

    rec.c50=*((ULONG*)(record));
    for(i=0; i<4; i++) rec.uk[i]=record[4+i];
    rec.sv=record[8];

    /*
     memcpy(&rec.c50,record,4);
     memcpy(&rec.uk,record+4,4);
     memcpy(&rec.sv,record+8,1);
     */
    if(ONE_SAT && rec.sv!=SELECTED_SV) return rec;

    if(VERBOSE)
    {
        printf("0x36 ----------------------------------------------------------------\n");
        printf("PRN %02d: C (50 Hz) %8lu = TOW %8.1f ",rec.sv+1,rec.c50,rec.c50/50.0);
        for(i=0; i<4; i++) printf("%02x ",rec.uk[i]);
        printf("\n");
    }

    return rec;
}


type_rec0x33 process_0x33(BYTE *record)
{
    type_rec0x33 rec;

    memcpy(&rec.altitud,record,4);
    record+=4;
    memcpy(&rec.epe,record,12);
    record+=12;
    memcpy(&rec.fix,record,2);
    record+=2;
    memcpy(&rec.tow,record,8);
    record+=8;
    memcpy(&rec.pos,record,16);
    record+=16;
    memcpy(&rec.vel,record,12);
    record+=12;
    memcpy(&rec.h_ellip,record,4);
    record+=4;
    memcpy(&rec.leap,record,2);
    record+=2;
    memcpy(&rec.wdays,record,4);
    record+=4;


    if(VERBOSE)
    {
        printf("0x33 ----------------------------------------------------------------\n");
        printf("Pos:  (N,E,H )  (%f %f %f)  H_ellip=%.0f  FIX %dD\n",rec.pos[0]*180/WGS84_PI,rec.pos[1]*180/WGS84_PI,rec.altitud,rec.h_ellip,rec.fix);
        printf("Vel:  (N,E,Up)  (%f %f %f)  ",rec.vel[0],rec.vel[1],rec.vel[2]);
        printf("Epe  (dX,dY,dH) (%.1f %.1f %.1f)\n",rec.epe[0],rec.epe[1],rec.epe[2]);
        printf("Wdays %ld  TOW %13.6f  Leap %d\n",rec.wdays,rec.tow,rec.leap);
    }

    return rec;
}


void process_0x37(BYTE *record)
{
    double pr,tow;
    ULONG c511;
    BYTE svid,unknown[4];
    UINT cc,countdown,c1,c2;
    float TR;

    memcpy(&c1,record,2);
    memcpy(unknown,record+2,2);
    memcpy(&c2,record+4,2);
    memcpy(&cc,record+6,2);
    memcpy(&countdown,record+8,2);
    TR=(float)((1024.0-countdown)/16.0);
    memcpy(&pr,record+10,8);
    memcpy(&c511,record+18,4);
    memcpy(&tow,record+22,8);
    memcpy(unknown+2,record+30,2);
    svid=record[32];


    if(ONE_SAT && svid!=SELECTED_SV) return;

    printf("0x37 ----------------------------------------------------------------\n");
    printf("PRN %02d: PR %10.1f TOW %10.3f (c511 %10lu) CC %5d\n",svid+1,pr,tow,c511,cc);
    printf("       Countdown %5d (%2.0f'') C1 %04x C2 %04x\n",countdown,TR,c1,c2);
    printf("       UNKNOWN: ");
    printf("%02x %02x | %02x %02x\n",unknown[0],unknown[1],unknown[2],unknown[3]);
}

void get_0x1a_info(BYTE *record, type_rec0x1a chan[])
{
    BYTE *ptr;
    int k;

    for(k=0,ptr=record; k<12; k++,ptr+=8)
    {
        if(ETREX)
        {
            chan[k].sv=ptr[5];
            chan[k].elev=ptr[4];
            chan[k].frac=*((UINT*)(ptr+0));
            chan[k].db=*((UINT*)(ptr+2));
            chan[k].tracked=ptr[6];
            chan[k].flag=ptr[7];
        }
        else
        {
            chan[k].sv=ptr[0];
            chan[k].elev=ptr[1];
            chan[k].frac=*((UINT*)(ptr+2));
            chan[k].db=*((UINT*)(ptr+4));
            chan[k].tracked=ptr[6];
            chan[k].flag=ptr[7];
        }
    }

    return;
}

void process_0x1a(BYTE *record)
{
    int k;
    BYTE *rec;
    BYTE svid,elev,trk,flag;
    unsigned int sq;
    float frac_phase;


    if(!ONE_SAT)
    {
        printf("0x1a ---------------------------------------------------------------\n");
        printf("Chann -> PRN | Elev | Q Sgnl |  Phase  | Track | Flag |\n");
    }

    for(k=0,rec=record; k<12; k++,rec+=8)
    {
        if(ETREX)
        {
            svid=rec[5];
            elev=rec[4];
            sq=*((UINT*)(rec+2));
            frac_phase=(float)(*((UINT*)(rec+0))/2048.0);
            trk=rec[6];
            flag=rec[7];
        }
        else
        {
            svid=rec[0];
            elev=rec[1];
            sq=*((UINT*)(rec+4));
            frac_phase=(float)(*((UINT*)(rec+2))/2048.0);
            trk=rec[6];
            flag=rec[7];
        }
        if(ONE_SAT)
        {
            if(svid!=SELECTED_SV) continue;
            printf("0x1a ----------------------------------------------------------------\n");
            printf("PRN %02d: Chann %02d  Elev %2d  Q %5d ",svid+1,k,elev,sq);
            printf("( Phase ) %5.3f  Trk %d Flag %d\n",frac_phase,trk,flag);
        }
        else
        {
            printf("  %02d  -> ",k);
            if(svid==0xff) printf("not used\n");
            else
            {
                printf("%02d  |  %02d  | %6d |",svid+1,elev,sq);
                printf("  %05.3f |  %2u  |  %2u  |\n",frac_phase,trk,flag);
            }
        }
    }
//printf("----------------------------------------------------------\n");
//getch();
}

type_rec0x16 process_0x16(BYTE *record)
{
    type_rec0x16 rec;
    float dif;

    rec.sv=record[20];
    if(ONE_SAT && rec.sv != SELECTED_SV) return rec;

    if(ETREX)
    {
        memcpy(&rec.pr,record,8);
        memcpy(&rec.delta_pr,record+8,4);
        memcpy(&rec.f1,record+12,4);
        memcpy(&rec.f2,record+16,4);
    }
    else
    {
        memcpy(&rec.delta_pr,record,4);
        memcpy(&rec.f1,record+4,4);
        memcpy(&rec.pr,record+8,8);
        memcpy(&rec.f2,record+16,4);
    }

    if(VERBOSE)
    {
        printf("0x16 ---------------------------------------------------------\n");
        printf("PRN %02d: ",rec.sv+1);
        printf("Pseudorange %13.3f ",rec.pr);
        printf("Doppler(m/s)  %7.2f -> %.3f Hz ",rec.delta_pr,rec.delta_pr/lambda);
        printf("F1 %.2f F2 %.2f\n",rec.f1,rec.f2);
        if(ETREX) printf("Unknown %d %d %d\n",record[21],record[22],record[23]);

        if((DIF_RECORDS) && (ONE_SAT) && (rec.pr==rec38.pr))
        {
            dif=(rec.delta_pr-phase_rate)*100;
            if(fabs(dif)<5000)
            {
                mean2+=dif;
                sigma2+=(dif*dif);
                n2++;
                printf("\t(delta_pr-phase_rate) -> ");
                printf("Mean %.1f std %.1f\n",mean2/n2,sqrt((sigma2-mean2*mean2/n2)/n2));
            }
        }
    }

    return rec;
}

BYTE process_0x39(BYTE *record)
{
    BYTE sv;

    sv=record[34];
    if(ONE_SAT && (sv != SELECTED_SV))  return sv;

    mean=sigma=0.0;
    n=0;

    if(VERBOSE)
    {
        printf("0x39 ---------------------------------------------------------\n");
        printf("PRN %02d: ACQUIRED PSEUDORANGE\n",record[34]+1);
    }

    return sv;
}


ULONG tobin(BYTE b)
{
    ULONG res;
    BYTE mask=128;
    int k;

    res=0;
    for(k=0,mask=128; k<8; k++,mask/=2)
    {
        res=(res*10)+((b&mask)>>(7-k)) ;
    }

    return res;
}

type_rec0x38 process_0x38(BYTE* record)
{
    int nsec,doppler;
    double dt,slip,d_pr,drift,d_phase,phase;
    ULONG d_cphase,d_intphase,d_c511;
    type_rec0x38 rec;
    long dtracked;
    long itow,dtow;

    last38=rec38;

    if(ONE_SAT && (record[36] != SELECTED_SV))
    {
        rec.sv=record[36];
        return rec;
    }

    if(ETREX)
    {
        memcpy(&(rec38.pr),record,8);
        memcpy(&(rec38.tow),record+8,8);
        memcpy(&(rec38.c_phase),record+16,4);
        memcpy(&(rec38.tracked),record+20,4);
        memcpy(&(rec38.int_phase),record+24,4);
        memcpy(&(rec38.c511),record+28,4);
        memcpy(&(rec38.delta_f),record+32,2);
        memcpy(&(rec38.db),record+34,2);
        memcpy(&(rec38.sv),record+36,1);
    }
    else
    {
        memcpy(&(rec38.c_phase),record,4);
        memcpy(&(rec38.tracked),record+4,4);
        memcpy(&(rec38.delta_f),record+8,2);
        memcpy(&(rec38.int_phase),record+10,4);
        memcpy(&(rec38.pr),record+14,8);
        memcpy(&(rec38.c511),record+22,4);

        memcpy(&(rec38.db),record+26,2);
        memcpy(&(rec38.tow),record+28,8);
        memcpy(&(rec38.sv),record+36,1);
    }

//rec38.delta_f=*((UINT*)(record+8));
//rec38.int_phase=*((ULONG*)(record+10));
//rec38.pr=*((double*)(record+14));
//rec38.c511=*((ULONG*)(record+22));
//rec38.db=*((UINT*)(record+26));
//rec38.tow=*((double*)(record+28));
//rec38.sv=record[36];

    doppler=rec38.delta_f-32768;
    phase=rec38.int_phase+(rec38.c_phase & 2047)/2048.0;


    if(VERBOSE)
    {
        // Check that tow is within limits
        itow=(long)floor(rec38.tow+0.5);
        if(START==-1) START=itow;
        dtow=itow-START;
        if((itow>=START) && (itow<=LAST) && (dtow<ELAPSED))
        {
            printf("0x38 ----------------------------------------------------------------\n");
            printf("PRN %02d: Sgn Q %5d ",rec38.sv+1,rec38.db);
            printf("TRACKED %08x %010d -> %.3f\n",rec38.tracked,(int)rec38.tracked,(float)rec38.tracked/256.0);
            printf("\tTOW %14.7f (Counter 0.5 Mhz = %8lu).\n",rec38.tow,rec38.c511);
            printf("\tPseudoRange  %14.3f  Integrated Phase %14.3f\n",rec38.pr,phase);

            printf("\tDoppler(\?): %5d Hz -> %.2f m/s\n",rec38.delta_f,rec38.delta_f*lambda);
        }
    }

    if(ONE_SAT && DIF_RECORDS)    // Differencing records is only valid when tracking one sat
    {
        dt=rec38.tow-last38.tow;
        nsec=(int)floor(dt+0.499999999);                  // GPS time exact increment
        drift = (dt-nsec)/nsec;                      // Clock drift from the 1'' increment
        d_pr=(rec38.pr-last38.pr);                   // Pseudorange increment
        d_cphase=rec38.c_phase-last38.c_phase;       // Phase counter increment
        d_phase=d_cphase/2048.0;                     // Phase increment
        d_intphase=(rec38.int_phase-last38.int_phase);  // Integer phase increment   phase_rate=d_phase*lambda/nsec;        // phase rate (in cm/s)
        d_c511=rec38.c511-last38.c511;
        slip= (d_pr - d_phase*lambda)*100;   // discrepancy between phase and pseudorange
        if(fabs(slip)<5000)
        {
            mean_q+=rec38.db;
            mean+=(float)(slip/nsec);
            sigma+=(float)(slip*slip/(nsec*nsec));
            n++;
        }
        dtracked=rec38.tracked-last38.tracked;

        if(VERBOSE)
        {
            printf("Deltas:");
            printf(" 0.5Mhz %7ld  Clk drift: %.1f musec/sec -> (m/s) %.1f\n",d_c511,drift*1e6,drift*c);
            printf("\tdTracked %8d\n",dtracked);
            printf("\tdPR(m) %.1f dintPhase %u  dPhase %.3f dPR(m/s) %.1f\n",d_pr,d_intphase,d_phase,d_pr/dt);
            printf("\td_PR-d_PHASE %.0f cm ",slip);
            printf("(mean %.1f, std %.1f) ",mean/n,sqrt((sigma-mean*mean/n)/n));
            printf("Mean Q %.0f\n",mean_q/n);
        }
    }

    return rec38;
}



void collect_stats(FILE *org)
{
    BYTE id,L,record[256];
    int k;
    BYTE lengths[256];
    ULONG cont[256];
    BYTE var[256];

    for(k=0; k<256; k++) cont[k]=var[k]=0;

    do
    {
        fread(&id,1,1,org);
        fread(&L,1,1,org);
        fread(record,1,L,org); //fseek(org,L,SEEK_CUR);

        if((cont[id])  && (lengths[id]!=L)) var[id]=1;
        cont[id]++;
        lengths[id]=L;
    }
    while(feof(org)==0);

    if(STDIN==0) fclose(org);


    for(k=0; k<256; k++)
    {
        if(cont[k]==0) continue;
        printf("Record 0x%02x  (%5ld) L=%3d bytes ",k,cont[k],lengths[k]);
        if(var[k]) printf("(VAR)");
        printf("\n");
    }

    exit(0);
    return;
}


void check(BYTE *record,BYTE L,BYTE type)
{
    int k,size;


    switch(type)
    {
    case AS_BYTE:
        for(k=0; k<L; k++) printf("%02x ",record[k]);
        printf("\n");
        break;
    case AS_CHAR:
        for(k=0; k<L; k++) printf("%c",record[k]);
        printf("\n");
        break;
    case AS_DOUBLE:
        size=sizeof(double);
        if(L<size) return;
        for(k=0; k<=L-size; k++) printf("%3d %f\n",k,*((double*)(record+k)));
        break;
    case AS_FLOAT:
        size=sizeof(float);
        if(L<size) return;
        for(k=0; k<=L-size; k++) printf("%3d %f\n",k,*((float*)(record+k)));
        break;
    case AS_ULONG:
        size=sizeof(ULONG);
        if(L<size) return;
        for(k=0; k<=L-size; k++) printf("%3d %lu\n",k,*((ULONG*)(record+k)));
        break;
    case AS_UINT:
        size=sizeof(UINT);
        if(L<size) return;
        for(k=0; k<=L-size; k++) printf("%3d %u\n",k,*((UINT*)(record+k)));
        break;
    default:
        break;
    }
    printf("------------------------------------------------------------------------------\n");

}


int found_in_list(int lista[], BYTE id)
{
    int k=0;
    int found=0;

    while(lista[k]!=-1)
    {
        if(lista[k]==id) found=1;
        k++;
    }

    return found;
}


void reset_epoch(rinex_obs list[])
{
    int k;

    for(k=0; k<32; k++)
    {
        if(list[k].used!=NEVER_USED) list[k].used=NO_DUMP;
        list[k].doppler=-1;
    }

}

BYTE get_q_code(UINT sgn)
{
//float temp;
//temp=(float)sgn/2000.0 + 3.0; if (temp>9) temp=9;
//return (BYTE)floor(temp);

    if(sgn>=9000) return 9;
    if(sgn>=6000) return 8;
    if(sgn>=4000) return 7;
    if(sgn>=2500) return 6;
    return 5;
}



UINT get_uint(BYTE* ptr)
{
    UINT x;

    x= (UINT)(*ptr++);
    x+=256*(UINT)(*ptr);

    return x;
}

void get_prod_id(org,description,prod,version)
FILE *org;
char *description;
UINT *prod;
float *version;
{
    BYTE id,L,record[256];
    UINT temp,k;
    BOOLEAN bad;

    do
    {
        fread(&id,1,1,org);
        fread(&L,1,1,org);
        fread(record,1,L,org);
        switch(id)
        {
        case 0xff:
            bad=0;
            k=4;   // Check if it is a good looking ID record
            while(record[k++] && !bad) bad=(record[k]>=128);
            if(!bad) // Probably  a good ff record
            {
                temp=get_uint(record);
                *prod=temp;
                temp=get_uint(record+2);
                *version=(float)temp/100;
                strcpy(description,record+4);
                rewind(org);
                return;
            }
            else break;
        default  :
            break;
        }
    }
    while(feof(org)==0);

    *prod=0;
    *version=0.0;
    strcpy(description,"Generic GPS12");
    rewind(org);
    return;
}


void llh2xyz(double llh[],double xyz[])
{
    double a,f,a_WGS84,f_WGS84;
    double e2,N,temp;
    double lat,lon,h;
    double ellip[2]= {0,0};
//double ellip[2]={-251.0, -0.14192702};  Hayford (European)

    a_WGS84=6378137;
    f_WGS84=1/298.257223563;
    a = a_WGS84 - ellip[0];
    f = f_WGS84 - ellip[1]*1e-4;

    e2=f*(2-f);

    lat=WGS84_PI*llh[0]/180;
    lon=WGS84_PI*llh[1]/180;
    h=llh[2];

    N=a/sqrt(1-e2*sin(lat)*sin(lat));

    temp=(N+h)*cos(lat);

    xyz[0]=temp*cos(lon);
    xyz[1]=temp*sin(lon);
    xyz[2]=(N*(1-e2)+h)*sin(lat);
}


void get_wdays_and_tow_from_user_date(ULONG* wdays,ULONG* week_secs)
{
    UINT month,day,year,hour,min,sec;
    int gps_week;
    double jd,tow;

    year=(UINT)USER_DATE[0];
    month=(UINT)USER_DATE[1];
    day=(UINT)USER_DATE[2];
    hour=(UINT)12;
    min=(UINT)0;
    sec=(UINT)0;

    jd=julday(year,month,day,hour,min,(double)sec);
    tow=(ULONG)gps_time(jd,&gps_week);
    *week_secs=(ULONG)tow;
    *wdays=(gps_week-521)*7;


    if(VERBOSE)
    {
        printf("%02u/%02u/%4d ",day,month,year);
        printf("%02d:%02d:%02d -> JD %f :: ",hour,min,sec,jd);
        printf("GPS Week %4d ToW %6.0f sec  Garmin Weekdays %d\n",gps_week,tow,*wdays);
    }

}


void get_aprox_location_and_wdays_and_tow(org,xyz,wdays,tow)
FILE *org;
double xyz[3];
ULONG *wdays;
ULONG *tow;
{
    BYTE id,L,record[256];
    type_rec0x33 rec;
    type_rec0x11 rec11;
    type_rec0x0e rec0e;
    int fix=0;
    int k=0;
    BYTE found_11,found_0e,found_33;

    found_33=found_11=found_0e=0;


    do
    {
        fread(&id,1,1,org);
        fread(&L,1,1,org);
        fread(record,1,L,org);

        switch(id)
        {
        case 0x33:
            rec=process_0x33(record);
            k++;
            if(k>GET_THIS) break;
            fix=rec.fix;

            xyz[0]=180.0*rec.pos[0]/WGS84_PI;
            xyz[1]=180.0*rec.pos[1]/WGS84_PI;
            xyz[2]=rec.altitud-rec.h_ellip;
            llh2xyz(xyz,xyz);

            *wdays=rec.wdays;
            *tow=(ULONG)floor(rec.tow+0.5);

            found_33=1;
            break;

        case 0x0e:
            rec0e=process_0x0e(record);
            found_0e=1;
            break;

        case 0x11:
            rec11=process_0x11(record);
            found_11=1;
            break;

        default  :
            break;
        }
    }
    while(feof(org)==0);


    rewind(org);


    if(found_33)   // Found 0x33 record
    {
        //printf("Obtained date and position from 0x33 record.Fix = %d.\n",fix);
        if(fix<3)
        {
            printf("Couldn't detect a 3D fix in this session\n");
            printf("You probably won't get a useful RINEX file, but let's try it\n");
        }
        //return;
    }
    else
    {
        if(found_0e && found_11)    // Found 0x11 AND 0x0e records
        {
            //printf("Using 0x0e and 0x11 records to get date & position\n");
            for(k=0; k<3; k++) xyz[k]=rec11.llh[k];
            llh2xyz(xyz,xyz);

            *wdays=rec0e.garmin_wdays;
            *tow=(ULONG)rec0e.tow;
            //printf("[%f %f %f] :: %d %u\n",pos[0],pos[1],pos[2],*wdays,*tow);
        }
        else         // No pertinent records found
        {
            if((GIVEN_DATE==0) || (GIVEN_XYZ==0))
            {
                printf("Couldn't find any records to get aproximate location and date\n");
                printf("Make sure that you are using the latest version of async to log your data\n");
                printf("You can also overcome this problem by providing the date of observation\n");
                printf("(using the -date DD MM YYYY option) AND your approximate position\n");
                printf("(using the -llh or -xyz options) in the command line\n");
                printf("-------------------------------------------------------------------------\n");
                printf("Exiting now\n");
                exit(0);
            }
        }
    }



// If position or date were provided in the command line
// they are always used with preference to those found

    if(GIVEN_XYZ) for(k=0; k<3; k++) xyz[k]=USER_XYZ[k];

    if(GIVEN_DATE) get_wdays_and_tow_from_user_date(wdays,tow);


}




/////////////////////////////////////////////////7

void get_current_date(ULONG wdays,double tow,struct tm *gmt)
{
    time_t week_start,current;
    ULONG seconds;
    struct tm *qq;

    ULONG TIME_START=631065600L;  // Difference beetween GPS time and UNIX time

    seconds=(ULONG)floor(tow);

    week_start=TIME_START+wdays*24*3600L;
    current=week_start+seconds;
    qq=gmtime(&current);
    *gmt=*qq;

}

char *padd(char *in, char *out, int maxL)
{
    size_t L,cop;
    int k;

    L=strlen(in);
    cop = ((int)L>maxL)? maxL:L;

    for(k=0; k<(int)cop; k++) out[k]=in[k];
    for(k=cop; k<maxL; k++) out[k]=' ';
    out[maxL]=0;

    return out;
}



void generate_rinex_header(xyz,wdays,first_obs,prod_id,version,description,fd)
double xyz[],first_obs;
float version;
ULONG wdays;
UINT prod_id;
char *description;
FILE *fd;
{
    BYTE mask;
    int k,l,lines,written,max_lines,nchars;
    char *header,*ptr;
    time_t tt;
    char *date,buffer[80];
    struct tm gmt;
    double dt,secs;
    char obs[3][3]= {"C1", "L1", "D1"};


    // I dont think I'll use more than 40 lines for the header
    max_lines=40;
    header=(char*)malloc(max_lines*80*sizeof(char));

    ptr=header;

    written=sprintf(ptr,"%9.2f%11c",2.11,32);
    ptr+=written;
    written=sprintf(ptr,padd("OBSERVATION DATA",buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("G (GPS)",buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("RINEX VERSION / TYPE",buffer,20));
    ptr+=written;

    sprintf(buffer,"GAR2RNX %4.2f",VERSION);
    written=sprintf(ptr,padd(buffer,buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("Garmin Owner",buffer,20));
    ptr+=written;
    time(&tt);
    date=ctime(&tt);
    written=sprintf(ptr,padd(date,buffer,20));
    ptr+=written;
    written=sprintf(ptr,"PGM / RUN BY / DATE ");
    ptr+=written;

    padd("** gar2rnx (Garmin to Rinex) generates Rinex2 files",buffer,60);
    written=sprintf(ptr,buffer);
    ptr+=written;
    written=sprintf(ptr,padd("COMMENT",buffer,20));
    ptr+=written;

    /*
    padd("** from a GPS12 (or XL) (Copyright Antonio Tabernero)",buffer,60);
    written=sprintf(ptr,"%s",buffer);
    ptr+=written;
    written=sprintf(ptr,"COMMENT             ");
    ptr+=written;

    sprintf(buffer,"** Generated from G12 data file: %s ",DATAFILE);
    padd(buffer,buffer,60);
    written=sprintf(ptr,"%s",buffer);
    ptr+=written;
    written=sprintf(ptr,"%s",padd("COMMENT",buffer,20));
    ptr+=written;
    */

    written=sprintf(ptr,"** Options: ");
    ptr+=written;
    written=sprintf(ptr,padd(COMMAND_LINE,buffer,60-written));
    ptr+=written;
    written=sprintf(ptr,"COMMENT             ");
    ptr+=written;

    sprintf(buffer,"%s",marker);
    written=sprintf(ptr,padd(buffer,buffer,60));
    ptr+=written;
    written=sprintf(ptr,padd("MARKER NAME",buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("A001",buffer,60));
    ptr+=written;
    written=sprintf(ptr,padd("MARKER NUMBER",buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("Unknown",buffer,20));
    ptr+=written;
    written=sprintf(ptr,padd("Unknown",buffer,40));
    ptr+=written;
    written=sprintf(ptr,"OBSERVER / AGENCY   ");
    ptr+=written;

    written=sprintf(ptr,"%03d%17c",prod_id,32);
    if(written>20) written=20;
    ptr+=written;
    written=sprintf(ptr,"%s  ",padd(description,buffer,18));
    ptr+=20;
    written=sprintf(ptr,"%4.2f%16c",version,32);
    if(written>20) written=20;
    ptr+=written;
    written=sprintf(ptr,"REC # / TYPE / VERS ");
    ptr+=written;


    written=sprintf(ptr,padd("NONE",buffer,20)); // ANT # (Alphanumeric)
    ptr+=written;
    written=sprintf(ptr,padd("NONE",buffer,20)); // ANT TYPE
    ptr+=written;
    written=sprintf(ptr,padd(" ",buffer,20)); // blank field
    ptr+=written;
    written=sprintf(ptr,padd("ANT # / TYPE",buffer,20));
    ptr+=written;

    if(GIVEN_XYZ) padd("** Position provided by user in command line",buffer,60);
    else padd("** Position computed internally by GPS receiver",buffer,60);
    written=sprintf(ptr,buffer);
    ptr+=written;
    written=sprintf(ptr,"COMMENT             ");
    ptr+=written;

    written=sprintf(ptr,"%14.4f%14.4f%14.4f%18c",xyz[0],xyz[1],xyz[2],32);
    ptr+=written;
    written=sprintf(ptr,"APPROX POSITION XYZ ");
    ptr+=written;

    written=sprintf(ptr,"%14.4f%14.4f%14.4f%18c",0.0,0.0,0.0,32);
    ptr+=written;
    written=sprintf(ptr,"ANTENNA: DELTA H/E/N");
    ptr+=written;


    written=sprintf(ptr,"%6d%6d%48cWAVELENGTH FACT L1/2",L1_FACTOR,0,32);
    ptr+=written;



    written=sprintf(ptr,"%6d",N_OBS);
    ptr+=written;

    for(k=0,mask=1; k<3; k++,mask*=2)
    {
        if(OBS_MASK & mask)
        {
            written=sprintf(ptr,"%6s",obs[k]);
            ptr+=written;
        }
    }
    for(k=N_OBS; k<9; k++)
    {
        written=sprintf(ptr,"%6c",32);
        ptr+=written;
    }
    written=sprintf(ptr,"# / TYPES OF OBSERV ");
    ptr+=written;


    if(RESET_CLOCK)
    {
        dt=first_obs-floor(first_obs+0.5);
        dt=floor(dt*1e9)/1e9;
        first_obs-=dt;
    }

    get_current_date(wdays,first_obs,&gmt);
    written=sprintf(ptr,"%6d%6d",1900+gmt.tm_year,gmt.tm_mon+1);
    ptr+=written;
    written=sprintf(ptr,"%6d%6d%6d",gmt.tm_mday,gmt.tm_hour,gmt.tm_min);
    ptr+=written;
    secs=(gmt.tm_sec)+first_obs-floor(first_obs);
    written=sprintf(ptr,"%12.6f%6cGPS%9c",secs,32,32);
    ptr+=written;
    written=sprintf(ptr,"TIME OF FIRST OBS   ");
    ptr+=written;

    written=sprintf(ptr,"%10.3f%50cINTERVAL%12c",(float)INTERVAL,32,32);
    ptr+=written;
    written=sprintf(ptr,"%60cEND OF HEADER       ",32);
    ptr+=written;


    nchars=(ptr-header);
    lines=nchars/80;
// printf("Number of chars %d -> lines %d\n",nchars,lines);

    for(l=0; l<lines; l++,fprintf(fd,"\n"))
        for(k=0; k<80; k++)  fprintf(fd,"%c",header[l*80+k]);

    free((char*)header);
}



////////////////////////////////////////////////////


void print_rinex_info(ULONG wdays, double tow,rinex_obs epoch[],FILE *fd)
{
    int k,N_used;
    double frac,phase,pr;
    int snr;
    struct tm gmt;

    double dt;

// printf("TOW %.0f Start %d Last %d\n",tow,START,START+ELAPSED);

    for(k=0,N_used=0; k<32; k++) if(epoch[k].used>=DUMP) N_used++;

    if(N_used)
    {
        if(RESET_CLOCK)
        {
            dt=tow-floor(tow+0.5);
            dt=floor(dt*1e9)/1e9;
        }
        else dt=0.0;

        get_current_date(wdays,tow-dt,&gmt);
        frac=(tow-dt)-floor(tow-dt);
        fprintf(fd," %02d %02d",(1900+gmt.tm_year)%100,gmt.tm_mon+1);
        fprintf(fd," %02d %02d %02d",gmt.tm_mday,gmt.tm_hour,gmt.tm_min);
        fprintf(fd,"%11.7f",(double)gmt.tm_sec+frac);
        //fprintf(fd,"%11.7f",(double)gmt.tm_sec+frac,32,32);

        fprintf(fd,"%3d%3d",0,N_used);
        for(k=0; k<32; k++) if(epoch[k].used>=DUMP) fprintf(fd,"G%02d",k+1);
        for(k=0; k<12-N_used; k++) fprintf(fd,"%3c",32);

        //if(RESET_CLOCK) fprintf(fd,"%12.9f",dt);
        fprintf(fd,"\n");

        for(k=0; k<32; k++) if(epoch[k].used>=DUMP)
            {
                snr= (NO_SNR)? 0:get_q_code(epoch[k].db);

                pr=epoch[k].prange-c*dt;
                phase=epoch[k].phase-L1*dt;


                /*
                if(N_OBS>=1)
                 {
                  if(epoch[k].used==DUMP_PHASE_ONLY) fprintf(fd,"%16c",32);
                  else fprintf(fd,"%14.3f%1c%1d",pr,32,snr);
                 }
                if(N_OBS>=2)  fprintf(fd,"%14.3f%1c%1d",phase,32,snr);

                if( (N_OBS>=3) && (epoch[k].doppler!=-1))
                   fprintf(fd,"%14.3f%1c%1d",-epoch[k].doppler,32,snr);
                */

                if(OBS_MASK&1)  // Pseudoranges
                {
                    if(epoch[k].used==DUMP_PHASE_ONLY) fprintf(fd,"%16c",32);
                    else fprintf(fd,"%14.3f%1c%1d",pr,32,snr);
                }

                if(OBS_MASK&2)   // L1 Phase
                    fprintf(fd,"%14.3f%1c%1d",phase,32,snr);


                if((OBS_MASK&4) && (epoch[k].doppler!=-1))     //Doppler
                    fprintf(fd,"%14.3f%1c%1d",-epoch[k].doppler,32,snr);


                fprintf(fd,"\n");

            }

    }

}


void add_0x1a_to_epoch(rinex_obs epoch[], type_rec0x1a chan[])
{
    int k;
    BYTE sv;

    for(k=0; k<12; k++)
    {
        sv=chan[k].sv;
        if(sv>=32) continue;
        if(epoch[sv].db==chan[k].db)
            epoch[sv].elev=chan[k].elev;
    }
}




void get_session_number(char *name, char *ext)
{
    int k;
    FILE *fd;
    char fich[64];

    for(k=1; k<10; k++)
    {
        sprintf(fich,"%s%1d.%s%c",name,k,ext,0);
        fd=fopen(fich,"r");
        if(fd==NULL) break;
        fclose(fd);
    }

    strcpy(name,fich);

}

void get_rinex_file_name(station,week_days,week_secs,filename,type)
char *station,*filename,type;
ULONG week_days,week_secs;
{
    struct tm date;
    char ext[8];

//printf("Week Days %d  Week Sec %d\n",week_days,week_secs);


    get_current_date(week_days,(double)week_secs,&date);
    sprintf(filename,"%4s%03d%c",station,date.tm_yday+1,0);
    sprintf(ext,"%02d%c%c",(1900+date.tm_year)%100,type,0);
    get_session_number(filename,ext);

}




void verify_tt(FILE *org)
{
    ULONG check[32][2],cc;
    BYTE flag[32],ff;


    float tow[32],current_tow;
//BYTE flag38[32];

    BYTE id,L,record[256],sv;
    int k;
    type_rec0x38 rec;
    type_rec0x36 rec36;



    for(k=0; k<32; k++)
    {
        flag[k]=0xff;
        tow[k]=-1;
    }

    do
    {
        fread(&id,1,1,org);
        fread(&L,1,1,org);
        fread(record,1,L,org);

        switch(id)
        {
        case 0x36:
            rec36=process_0x36(record);
            sv=rec36.sv;
            if(sv>32) break;
            if(ONE_SAT && (sv!=SELECTED_SV)) break;

            cc=rec36.c50;
            ff=flag[sv];
            if(ff==0xff)
            {
                flag[sv]=0;
                check[sv][1]=cc;
                break;
            }

            if(cc==(check[sv][1]+30))   // OK
            {
                if(ff==1)     // print report
                {
                    printf("PRN %02d: 0x36 gap ",sv+1);
                    printf("(%.1f -> %.1f)\n",check[sv][0]/50.0,check[sv][1]/50.0);
                    flag[sv]=0;
                }
                check[sv][0]=check[sv][1];
            }
            else  flag[sv]=1;       // Lost 0x36 record
            check[sv][1]=cc;

            break;

        case 0x38:
            rec=process_0x38(record);
            sv=rec.sv;

            if(sv>32) break;
            if(ONE_SAT && (sv!=SELECTED_SV)) break;

            current_tow=(float)floor(rec.tow+0.5);

            //printf("%.0f %.0f\n",tow[sv],current_tow);

            if(tow[sv]==-1)
            {
                tow[sv]=current_tow;
                break;
            }


            if(current_tow != (tow[sv]+1))
            {
                printf("PRN %02d: ",sv+1);
                if(current_tow<tow[sv])
                    printf("Anomalous tow %.0f -> %.0f\n",tow[sv],current_tow);
                else if(current_tow==tow[sv]) printf("%.0f Duplicado\n",current_tow);
                else
                {
                    printf("Missing %.0f",tow[sv]+1);
                    if(current_tow == (tow[sv]+2)) printf("\n");
                    else printf(" -> %.0f)\n",current_tow-1);
                    tow[sv]=current_tow;
                }
            }
            else tow[sv]=current_tow;

            break;

        default  :
            break;
        }
    }
    while(feof(org)==0);

    if(STDIN==0) fclose(org);
    exit(0);
}

type_rec0x38 choose(type_rec0x38 rec[],int nn, rinex_obs last)
{
    int k;
    type_rec0x38 best;
    float dif[2],DIF,dif_min;
    long int dt;

// Only one case
    if(nn==1)
    {
        best=rec[0];
        return best;
    }

// No previous references (any will do)
    if(last.used==NEVER_USED)  best=rec[nn-1];
    else  // Choose between several
    {
        dif_min=(float)1e30;
        for(k=0; k<nn; k++)
        {
            dt=rec[k].tracked-last.tracked;
            dif[0]=(float)(dt/256.0);
            dif[1]=(float)rec[k].delta_f-last.doppler;
            DIF=(float)(fabs(dif[0])+fabs(dif[1]));
            if(DIF<dif_min)
            {
                dif_min=DIF;
                best=rec[k];
            }
        }
    }
    return best;
}

int verify_c511(type_rec0x38 allrec[],int N,double last_tow,ULONG next_c511)
{
    int k,j,nsat,nmax;
    int n_511,nn[40],test;
    ULONG c511[40],cc,c511_ok;
    float dif;

    n_511=0;
    for(j=0; j<40; j++) nn[j]=0;

    for(k=0; k<N; k++)
    {
        cc=allrec[k].c511;
        test=0;
        for(j=0; j<n_511; j++) if(cc==c511[j])
            {
                nn[j]++;
                test=1;
                break;
            }
        if(test==0)
        {
            c511[n_511]=cc;
            nn[n_511]=1;
            n_511++;
        }
    }

    nmax=0;
    for(k=0; k<n_511; k++) if(nn[k]>nmax)
        {
            c511_ok=c511[k];
            nmax=nn[k];
        }
    if(nmax==0) return 0;

    dif=(float)next_c511-(float)c511_ok;
    if(last_tow!=-1) if(abs((int)dif)>20) return 0;

    nsat=0;
    for(k=0; k<N; k++) if(allrec[k].c511==c511_ok) allrec[nsat++]=allrec[k];


    //printf("Verifying c511: %2d records: Expecting %12u\n",N,next_c511);
    //for(k=0;k<n_511;k++) printf("C511   %16u -> %2d veces\n",c511[k],nn[k]);
    //                     printf("Chosen %16u con %2d veces Dif %.0f\n",c511_ok,nmax,dif);

    return nsat;
}

int remove_duplicates(type_rec0x38 allrec[],int N,rinex_obs epoch[])
{
    int k,j,nsat,sv,used[32],nn;
    type_rec0x38 rec[20];
    double tow,phase;

    tow=allrec[0].tow;

    // Find SVs in sight and possibly duplicated records

    for(k=0; k<32; k++) used[k]=0;
    for(k=0; k<N; k++) used[allrec[k].sv]++;

    //printf("Remove: Tow %14.6f -> ",tow);
    //printf("NREC %2d :: ",N); for(k=0;k<N;k++) printf("%2d ",allrec[k].sv+1);
    //printf("\n");


    nsat=0;
    for(k=0; k<N; k++)
    {
        nn=0;
        rec[nn++]=allrec[k];
        sv=rec[0].sv;
        if(used[sv]<0) continue;         // This sv has already been processed

        if(used[sv]>1)
        {
            for(j=k+1; j<N; j++)
            {
                if(allrec[j].sv==sv) rec[nn++]=allrec[j];
                if(nn==used[sv]) break;
            }
            used[sv]=-1;
        }
        allrec[nsat++]=choose(rec,nn,epoch[sv]);
    }

    return nsat;

    //printf("N SV %2d :: ",nsat); for(k=0;k<nsat;k++) printf("%2d ",allrec[k].sv+1);
    //printf("\n");

    for(k=0; k<nsat; k++)
    {
        if((allrec[k].sv+1)!=21) continue;
        phase=allrec[k].int_phase+(allrec[k].c_phase & 2047)/2048.0;
        printf("0x38 ----------------------------------------------------------------\n");
        printf("PRN %02d: Sgn Q %5d ",allrec[k].sv+1,allrec[k].db);
        printf("TRACKED %08x -> %8d\n",allrec[k].tracked,(int)allrec[k].tracked);
        printf("\tTOW %12.5f (Counter 0.5 Mhz = %u).\n",allrec[k].tow,allrec[k].c511);
        printf("\tPseudoRange  %14.3f  Integrated Phase %14.3f\n",allrec[k].pr,phase);
        printf("\tDoppler(Hz): %5d \n",allrec[k].delta_f);
    }

    return nsat;
}



void add_0x16_to_epoch(rinex_obs epoch[], type_rec0x16 rec)
{
    BYTE sv=rec.sv;

//printf("%d %14.3f %14.3f %f\n",sv,epoch[sv].prange,rec.pr,epoch[sv].prange-rec.pr);

    if(epoch[sv].prange==rec.pr) epoch[sv].doppler=(float)(rec.delta_pr/lambda);

}

void add_0x38_to_epoch(rinex_obs epoch[], type_rec0x38 rec)
{
    BYTE sv;
    int action;
    BYTE check;
    long  dtracked;

    sv=rec.sv;

    check=rec.tracked&0xff;

    if(epoch[sv].used==NEVER_USED)  action= (check==0)? NO_DUMP:DUMP;
    else
    {
        dtracked=(epoch[sv].tracked-rec.tracked);
        action = (abs(dtracked)<256)?  DUMP:NO_DUMP;
    }

    if(RELAX) action=DUMP;


// Check if there have been a recent 0x36 record for that sat

    if((rec.tow>(epoch[sv].last36+2.0)) && (action==DUMP))
        action = (OPT1) ? DUMP_PHASE_ONLY:NO_DUMP;

//printf("SV %2d -> %08x %08x : ",sv+1,epoch[sv].tracked,rec.tracked,dtracked);
//printf("dTracked %8d :",dtracked);
//printf("check %02x action %d\n",check,action);

// Check for meaningless values
    if((rec.pr<=10.0) || (rec.pr>=1e9)) return;
//if (rec.db>15000)  return;

//epoch[sv].doppler=rec.delta_f;
    epoch[sv].prange=rec.pr;
    epoch[sv].phase=(double)rec.int_phase+(rec.c_phase&2047)/2048.0;
    epoch[sv].db=rec.db;

    epoch[sv].tracked=rec.tracked;
    epoch[sv].c511=rec.c511;

    epoch[sv].used=action;

}


int process_tow(rec,N,rec16,N16,epoch,last_tow)
type_rec0x38 *rec;
int N;
type_rec0x16 *rec16;
int N16;
rinex_obs *epoch;
double last_tow;
{
    double current_tow=rec[0].tow;
    int n_sat;
    int k;


// Check for current_tow < last_tow (problems during week rollover)
//if (current_tow<last_tow) return 0;

// Check for tow=0.000000
    if(current_tow==0.0) return 0;


//Eliminates duplicated records
    n_sat=remove_duplicates(rec,N,epoch);


//printf("Tow %14.6f -> %2d records -> %2d sats\n",current_tow,N,n_sat);

// Add (or not) records to rinex obs.
    for(k=0; k<n_sat; k++) add_0x38_to_epoch(epoch,rec[k]);

// Add doppler data
    for(k=0; k<N16; k++) add_0x16_to_epoch(epoch,rec16[k]);

    return n_sat;
}






void generate_rinex(FILE *org)
{
    BYTE id,L,record[256],sv;
    int k,n_records,nr,n_16;
    type_rec0x16 r16,rec16[48];
    type_rec0x38 rec,allrec[48];
    type_rec0x36 rec36;
//type_rec0x1a chan[12];
    double current_tow,last_tow;
    ULONG next_c511,last_c511=0;
    rinex_obs epoch[32];
    ULONG week_days,week_secs;
    double aprox_xyz[3];
    char description[256];
    UINT prod_number;
    long itow,dtow;
    float version;

    BOOLEAN found=0;
    long fptr;

    FILE *dest;
    char name[128];


    get_prod_id(org,description,&prod_number,&version);
    get_aprox_location_and_wdays_and_tow(org,aprox_xyz,&week_days,&week_secs);


    if(RINEX_FILE)
    {
        get_rinex_file_name(location,week_days,week_secs,name,'O');
        dest=fopen(name,"w");
    }
    else dest=stdout;


//printf("Week days %d TOW %d -> File %s\n",week_days,week_secs,name);
//printf("Aprox location %f %f %f\n",aprox_llh[0],aprox_llh[1],aprox_llh[2]);
//printf("Aprox XYZ  %f %f %f\n",aprox_xyz[0],aprox_xyz[1],aprox_xyz[2]);
//printf("ID %d.\n Desc: %s .\n Soft %.2f\n",prod_number,description,version);
//exit(0);

    rewind(org);

    n_records=0;
    n_16=0;
    last_tow=-1;

    reset_epoch(epoch);
    for(k=0; k<32; k++)
    {
        epoch[k].used=NEVER_USED;
        epoch[k].last36=-1.0;
    }

    do
    {
        fptr=ftell(org);
        fread(&id,1,1,org);
        fread(&L,1,1,org);
        fread(record,1,L,org);
        fptr-=ftell(org);

        switch(id)
        {
        case 0x1a:
            break;

        case 0x16:

            r16=process_0x16(record);
            sv=r16.sv;
            if(sv>=32) break;


            // Option A: keep all of them and discard them later
            if(n_16<48) rec16[n_16++]=r16;
            //if (n_16>=12) {printf("N16 %d Tow %.0f\n",n_16,current_tow); getchar();}


            // OPtion B: check to see if there is already a 0x16 record for that sv
            //found=0; for (k=0;k<n_16;k++) if (sv==rec16[k].sv) {found=1; break;}
            //if (found==0) rec16[n_16++]=r16;
            //else { printf("Repeated 0x16 for SV %d, tow %.0f. Not added\n",sv,current_tow);}


            break;

        case 0x36:
            rec36=process_0x36(record);
            sv=rec36.sv;
            if(sv>=32) break;
            epoch[sv].last36=(float)(rec36.c50/50.0);
            break;

        case 0x38:

            rec=process_0x38(record);
            sv=rec.sv;
            if(sv>=32) break;

            if(n_records==0) current_tow=rec.tow;

            if(rec.tow==current_tow) allrec[n_records++]=rec;
            else                             // End of epoch detected
            {
                fseek(org,fptr,SEEK_CUR);      // Reset file pointer for next epoch
                nr=n_records;
                n_records=0;

                //printf("End TOW:  0x38 records: %d  ",nr);

                // Check that tow is within limits
                itow=(long)floor(current_tow+0.5);
                if(START==-1) START=itow;
                dtow=itow-START;
                if((itow<START) || (itow>LAST) || (dtow>ELAPSED))
                {
                    n_16=0;
                    break;
                }

                // Verifies c511 fields

                //printf("%10.3f (%2d) ->\n ",current_tow,nr);

                next_c511 = last_c511 + (ULONG)floor((current_tow-last_tow)*511500.0 +0.5);
                nr=verify_c511(allrec,nr,last_tow,next_c511);
                if(nr==0)
                {
                    n_16=0;
                    break;
                }

                //printf("After c511 check = %2d. 0x16 records %d\n",nr,n_16);

                nr=process_tow(allrec,nr,rec16,n_16,epoch,last_tow);


                //printf("Final %2d:  ",nr);
                //for(k=0;k<nr;k++) printf("%02d ",allrec[k].sv+1); printf("\n");

                if(nr==0)
                {
                    n_16=0;
                    break;
                }


                // If first epoch, creates header
                if(last_tow==-1)
                    generate_rinex_header(aprox_xyz,week_days,current_tow,\
                                          prod_number,version,description,dest);

                // If multiple of interval, dump to rinex file
                if((INTERVAL==1) || (itow%INTERVAL)==0)
                    print_rinex_info(week_days,current_tow,epoch,dest);

                //for(k=0;k<n_16;k++) printf("%02d %14.3f\n",rec16[k].sv+1,rec16[k].pr);
                //getchar();

                n_16=0;  // Reset number of 0x16 records per epoch

                last_c511=allrec[0].c511;

                //printf("Expected c511 %u -> seen %u\n",next_c511,last_c511);
                last_tow=current_tow;
                reset_epoch(epoch);
            }

            break;

        default  :
            break;
        }
    }
    while(feof(org)==0);

    if(STDIN==0) fclose(org);
    if(RINEX_FILE) fclose(dest);

    exit(0);
}








void parse_records(FILE *org)
{
    BYTE record[256],id,L;

    fread(&id,1,1,org);
    fread(&L,1,1,org);
    fread(record,1,L,org);

    if(!found_in_list(SELECTED_RECORDS,id)) return;

    switch(id)
    {
    case 0x12:
        process_0x12_gps38(record);
        break;

    case 0x0e:
        process_0x0e(record);
        break;
    case 0x11:
        process_0x11(record);
        break;
    case 0x14:
        process_0x14(record);
        break;
    case 0x16:
        process_0x16(record);
        break;
    case 0x1a:
        process_0x1a(record);
        break;
    case 0x33:
        process_0x33(record);
        break;
    case 0x36:
        process_0x36(record);
        break;
    case 0x37:
        process_0x37(record);
        break;
    case 0x38:
        process_0x38(record);
        break;
    case 0x39:
        process_0x39(record);
        break;
    default :
        printf("Record %02x ------------------------------------------\n",id);
        check(record,L,AS_BYTE);
        getchar();
        break;
    }

}


void original_parsing(FILE *fd)
{
    do parse_records(fd);
    while(feof(fd)==0);

    if(DIF_RECORDS)
    {
        printf("%2d    %5.0f        ",SELECTED_SV+1,(float)mean_q/n);
        printf("%4.1f  %5.1f   ",mean/n,sqrt((sigma-mean*mean/n)/n));
        printf("%4.1f  %5.1f\n",mean2/n2,sqrt((sigma2-mean2*mean2/n2)/n2));
    }
    if(STDIN==0) fclose(fd);
    exit(0);
}



//////////////////PARSE NAV RELATED FUNCTIONS ///////////////////////////

BYTE current_sat;

typedef struct
{
    \
    BYTE prn;
    double acc;
    BYTE health;
    double af[3];
    double tgd;
    \
    double crs;
    double crc;
    double cuc;
    double cus;
    double cic;
    double cis;
    \
    double M0;
    double dn;
    double ecc;
    double roota;
    \
    double W0;
    double  Wdot;
    double i0;
    double idot;
    double w;
    \
    double toc;
    int week;
    double toe;
    double tom;
    \
    int iodc;
    int iode2;
    int iode3;
    int last_iode;
    \
    BYTE L2_code;
    BYTE L2_data;
    BYTE fit_flag;
    UINT aodo;
    \
} EPHEM;

EPHEM eph[32];

double URA_TABLE[16]= {2,2.8,4,5.7,8,11.3,16,32,64,128,256,512,1024,2048,4096,-1};



void dump_eph_ok(FILE* fd)
{
    EPHEM ep=eph[current_sat];
    char msg[700],ch;
    char *ptr=msg;
    //int toc,sec,min,hour;
    double toc;
    ULONG garmin_wdays,week;
    struct tm gmt;
    double frac;
    int k;
    BYTE health;
    double cero=0.0;
    int out;
    char f1[10],f4[40];


    if(VC_format) strcpy(f1,"%20.12e");
    else strcpy(f1,"%19.12e");
    strcpy(f4,"   ");
    for(k=0; k<4; k++) strcat(f4,f1);
    strcat(f4,"\n");


    health=ep.health>>5;

    // Convert toc time
    week=(ULONG)ep.week;
    garmin_wdays=(week-521)*7;
    toc=ep.toc;
    frac=toc-floor(toc);
    toc=floor(toc);
    get_current_date(garmin_wdays,(ULONG)toc,&gmt);

    ptr+=sprintf(ptr,"%2d %02d %2d ",ep.prn,(1900+gmt.tm_year)%100,gmt.tm_mon+1);
    ptr+=sprintf(ptr,"%02d %2d %2d",gmt.tm_mday,gmt.tm_hour,gmt.tm_min);
    ptr+=sprintf(ptr,"%5.1f",(double)gmt.tm_sec+frac);

    //sec=toc%86400; hour=sec/3600; sec-=hour*3600; min=sec/60;    sec-=min*60;
    //ptr+=sprintf(ptr,"TOC %d(%02d:%02d:%02d): ",toc,hour,min,sec);

    for(k=0; k<3; k++) ptr+=sprintf(ptr,f1,ep.af[k]);
    ptr+=sprintf(ptr,"\n");

    ptr+=sprintf(ptr,f4,(double)ep.iode2,ep.crs,ep.dn,ep.M0);
    ptr+=sprintf(ptr,f4,ep.cuc,ep.ecc,ep.cus,ep.roota);
    ptr+=sprintf(ptr,f4,ep.toe,ep.cic,ep.W0,ep.cis);
    ptr+=sprintf(ptr,f4,ep.i0,ep.crc,ep.w,ep.Wdot);
    ptr+=sprintf(ptr,f4,ep.idot,(double)ep.L2_code,(double)week,(double)ep.L2_data);
    ptr+=sprintf(ptr,f4,ep.acc,(double)health,ep.tgd,(double)(ep.iodc));
    ptr+=sprintf(ptr,f4,ep.tom,cero,cero,cero);

    /*
      for(k=0;k<3;k++) ptr+=sprintf(ptr,"%20.12e",ep.af[k]);   ptr+=sprintf(ptr,"\n");

      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",(double)ep.iode2,ep.crs,ep.dn,ep.M0);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.cuc,ep.ecc,ep.cus,ep.roota);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.toe,ep.cic,ep.W0,ep.cis);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.i0,ep.crc,ep.w,ep.Wdot);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.idot,(double)ep.L2_code,(double)week,(double)ep.L2_data);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.acc,(double)health,ep.tgd,(double)(ep.iodc));
      //ptr+=sprintf(ptr,"   %20.12e\n",ep.tom);
      ptr+=sprintf(ptr,"   %20.12e%20.12e%20.12e%20.12e\n",ep.tom,cero,cero,cero);
      */




    if(VC_format)
    {
        out=0;
        k=0;
        while(ch=msg[k])
        {
            if(ch=='e')
            {
                msg[k-out]='D';
                k++;
                msg[k-out]=msg[k];
                k++;
                out++;
            }
            else msg[k-out]=ch;
            k++;
        }
        msg[k-out]=0;
    }
    else for(k=0; k<(ptr-msg); k++) if(msg[k]=='e') msg[k]='D';


    //printf("%d\n",strlen(msg));



    fprintf(fd,"%s",msg);

    // getchar();
}



int format_double_old(char *dest,double a)
{
    char ptr[32];
    int ND,k,exp,index;
    int pos_coma,pos_e;
    char sgn,first;


    sprintf(ptr,"%+20.12e",a); //printf("Inicio:  %s\n",ptr);

    k=0;
    while(ptr[k]!='.') k++;
    pos_coma=k;
    while(ptr[k]!='e') k++;
    pos_e=k+1;
    exp=atoi(ptr+pos_e);


    first=ptr[pos_coma-1];
    sgn=ptr[pos_coma-2];

    if(sgn=='+') sgn=' ';

    index=0;
    dest[index++]=' ';
    dest[index++]=sgn;
    dest[index++]='.';

    if(first=='0') ND=12;
    else
    {
        dest[index++]=first;
        ND=11;
        exp++;
    }

    for(k=1; k<=ND; k++) dest[index++]=ptr[pos_coma+k];

    index+=sprintf(dest+index,"D%+03d",exp);

    //printf("Chars written %d\n",index);
    return index;

}



int format_double(char *dest,double a)
{
    char ptr[32];
    int ND,k,exp,index;
    int pos_coma,pos_e;
    char sgn,first;
    BOOLEAN leading_cero=1;


    sprintf(ptr,"%+20.11e",a); //printf("Inicio:  %s\n",ptr);

    k=0;
    while(ptr[k]!='.') k++;
    pos_coma=k;
    while((ptr[k]!='e') && (ptr[k]!='E')) k++;
    pos_e=k+1;
    exp=atoi(ptr+pos_e);

    first=ptr[pos_coma-1];
    sgn=ptr[pos_coma-2];

    if(sgn=='+') sgn=' ';

    index=0;
    //
    if(leading_cero)
    {
        dest[index++]=sgn;
        dest[index++]='0';
    }
    else
    {
        dest[index++]=' ';
        dest[index++]=sgn;
    }
    dest[index++]='.';

    dest[index++]=first;
    ND=11;
    exp++;
    for(k=1; k<=ND; k++) dest[index++]=ptr[pos_coma+k];

    index+=sprintf(dest+index,"D%+03d",exp);

    //printf("Chars written %d\n",index);
    return index;

}



int write_four(char* ptr,double one,double two,double three,double four)
{
    int index=0;

    index+=sprintf(ptr+index,"   ");
    index+=format_double(ptr+index,one);
    index+=format_double(ptr+index,two);
    index+=format_double(ptr+index,three);
    index+=format_double(ptr+index,four);

    index+=sprintf(ptr+index,"\n");

    return index;
}

void dump_eph(FILE* fd)
{
    EPHEM ep=eph[current_sat];
    char msg[700];
    char *ptr=msg;
    //int toc,sec,min,hour;
    double toc;
    ULONG garmin_wdays,week;
    struct tm gmt;
    double frac;
    int k;
    BYTE health;
    double cero=0.0;


    health=ep.health>>5;

    // Convert toc time
    week=(ULONG)ep.week;
    garmin_wdays=(week-521)*7;
    toc=ep.toc;
    frac=toc-floor(toc);
    toc=floor(toc);
    get_current_date(garmin_wdays,(ULONG)toc,&gmt);

    ptr+=sprintf(ptr,"%2d %02d %2d ",ep.prn,(1900+gmt.tm_year)%100,gmt.tm_mon+1);
    ptr+=sprintf(ptr,"%02d %2d %2d",gmt.tm_mday,gmt.tm_hour,gmt.tm_min);
    ptr+=sprintf(ptr,"%5.1f",(double)gmt.tm_sec+frac);

    //sec=toc%86400; hour=sec/3600; sec-=hour*3600; min=sec/60;    sec-=min*60;
    //ptr+=sprintf(ptr,"TOC %d(%02d:%02d:%02d): ",toc,hour,min,sec);

    for(k=0; k<3; k++) ptr+=format_double(ptr,ep.af[k]);
    ptr+=sprintf(ptr,"\n");


    ptr+=write_four(ptr,(double)ep.iode2,ep.crs,ep.dn,ep.M0);
    ptr+=write_four(ptr,ep.cuc,ep.ecc,ep.cus,ep.roota);
    ptr+=write_four(ptr,ep.toe,ep.cic,ep.W0,ep.cis);
    ptr+=write_four(ptr,ep.i0,ep.crc,ep.w,ep.Wdot);
    ptr+=write_four(ptr,ep.idot,(double)ep.L2_code,(double)week,(double)ep.L2_data);
    ptr+=write_four(ptr,ep.acc,(double)health,ep.tgd,(double)(ep.iodc));
    ptr+=write_four(ptr,ep.tom,cero,cero,cero);


    fprintf(fd,"%s",msg);

    // getchar();
}




void reset_frame()
{
    memcpy(frame[current_sat],RESET,30);
    memcpy(check_frame[current_sat],RESET,30);
}




void p_bits(BYTE b)
{
    int i;
    for(i=7; i>=0; i--) printf("%d",(b&(1<<i))>>i);
}

void p_nbits(BYTE b,int n)
{
    int i;
    for(i=n-1; i>=0; i--) printf("%d",(b&(1<<i))>>i);
}



// Move bits 2 to 25 from org to FRAME word
void strip_parity(BYTE* org,ULONG word)
{
    BYTE *dest;
    int k,index;
    BYTE d30;

    d30 = (org[3] >> 6) & 0x01;


    index=word*3;
    dest=frame[current_sat] + index;


    for(k=0; k<3; k++)
    {
        dest[k] = ((org[3-k] & 0x3f)<<2) + ((org[2-k] & 0xc0)>>6);
        if(d30==1) dest[k]= dest[k]^0xff;
        check_frame[current_sat][index+k]=1;
    }



// memcpy(check+index,OK,3);

#ifdef TRACE
    printf("D30 %d\n",d30);
    for(k=3; k>=0; k--) printf("%02x ",org[k]);
    printf("\n");
    for(k=3; k>=0; k--) p_bits(org[k]);
    printf("\n");
    printf("  ");
    for(k=0; k<3; k++) p_bits(dest[k]);
    printf("\n");
#endif


}



// Extract bit number nbit from byte stream ptr
BYTE peak_bit(int nbit,BYTE *ptr)
{
    int byte_pos=(nbit>>3);
    int bit_pos=7-(nbit&0x07);
    BYTE res = (ptr[byte_pos]&(1<<bit_pos)) >>bit_pos;
    return res;
}

// Place bit (0/1) in position nbit of byte stream dest
void poke_bit(BYTE bit,int nbit,BYTE *dest)
{
    int byte_pos=(nbit>>3);
    int bit_pos=(nbit&0x07);
    BYTE mask;
    BYTE org=dest[byte_pos];

// printf("ORG %d  byte %d bit %d\n",org,byte_pos,bit_pos);


    bit_pos=7-bit_pos;


    mask=0xff-(1<<bit_pos);
    org &= mask;

    //p_bits(org); printf("\n");
    //p_bits(mask); printf("\n");

    bit&=0x01;
    org = org | (bit<<bit_pos);

    //p_bits(org); printf("\n");


    dest[byte_pos]=org;
}


//Extract L bits starting from frame[ini-1] as unsigned long int (32 bits max)
ULONG extrae_ulong(int ini, int L)
{
    ULONG res=0;
    BYTE *dest=(BYTE*)&res;
    int k,pos_lsb;
    BYTE data;

    ini=ini-1;
    pos_lsb = ini+L-1;
    for(k=0,pos_lsb=ini+L-1; k<L; k++,pos_lsb--)
    {
        data=peak_bit(pos_lsb,frame[current_sat]);
        poke_bit(data,LSB[k],dest);
    }
    return res;
}



double  get_real(ULONG data,int L, BYTE signo, int scale)
{
    double res,factor;
    ULONG limit;

    res=(double)data;
    if(signo)
    {
        limit=1<<(L-1);
        if(data>=limit)
            res = res-pow((double)2.0,(double)L);
    }

    factor = pow(2.0,(double)scale);
    res=res*factor;


    return res;
}

void fill_subframe1()
{
    ULONG temp;
    double d;
    int L;
    BYTE alert,antispoof;
    char on_off[2][4];
    char *tab="        ";

    strcpy(on_off[0]," ON");
    strcpy(on_off[1],"OFF");

    eph[current_sat].prn=current_sat+1;


    alert=(BYTE)extrae_ulong(42,1);
    antispoof=(BYTE)extrae_ulong(43,1);


// Possible rollover problems ca. 2020
    temp = extrae_ulong(49,10);
    eph[current_sat].week=temp+1024;
    temp = extrae_ulong(59,2);
    eph[current_sat].L2_code=(BYTE)temp;
    temp = extrae_ulong(61,4);
    eph[current_sat].acc=URA_TABLE[temp];
    temp = extrae_ulong(71,2);
    temp = (temp << 8) + extrae_ulong(169,8);
    eph[current_sat].iodc=temp;
    temp = extrae_ulong(73,1);
    eph[current_sat].L2_data=(BYTE)temp;
    temp = extrae_ulong(65,6);
    eph[current_sat].health=(BYTE)temp;

    L=8;
    temp = extrae_ulong(161,L);
    d=get_real(temp,L,1,-31);
    eph[current_sat].tgd=d;

    temp = extrae_ulong(177,16);
    temp<<=4;
    eph[current_sat].toc=temp;

    L=22;
    temp = extrae_ulong(217,L);
    d=get_real(temp,L,1,-31);
    eph[current_sat].af[0]=d;
    L=16;
    temp = extrae_ulong(201,L);
    d=get_real(temp,L,1,-43);
    eph[current_sat].af[1]=d;
    L=8;
    temp = extrae_ulong(193,8);
    d=get_real(temp,L,1,-55);
    eph[current_sat].af[2]=d;



    if(VERBOSE_NAV==0) return;
    if((SELECTED_SF!=-1) && (SELECTED_SF!=1)) return;


// printf("------------------------------------------------------------------\n");
    printf("\nPRN %02d: ",eph[current_sat].prn);
    printf("Issue Of Data Clock (IODC) %04u -> IODE %3d. ",eph[current_sat].iodc,eph[current_sat].iodc&0xff);
    printf("Week GPS %lu.\n",eph[current_sat].week);

    printf("%s",tab);
    switch(eph[current_sat].L2_code)
    {
    case 0:
        printf("L2code flag 00 (reserved). ");
        break;
    case 1:
        printf("P code ON in L2. ");
        break;
    case 2:
        printf("C/A code ON in L2. ");
        break;
    case 3:
        printf("L2code flag 11 (?). ");
        break;
    }
    printf("NAV data on L2:%s\n",on_off[eph[current_sat].L2_data]);

    printf("%s",tab);
    if(eph[current_sat].health)
    {
        printf("Unhealthy satellite. Details: ");
        p_nbits(eph[current_sat].health,5);
        printf("\n");
    }
    else
        printf("Healthy satellite\n");

    printf("%s",tab);
    printf("User Range Accuracy: %.1f mt.  ",eph[current_sat].acc);
    printf("Alert flag %d. ",alert);
    printf("Anti-spoof:%s\n",on_off[antispoof^1]);

//printf("    Clock parameters:\n");
    printf("%s",tab);
    printf("Group Delay Differential (tgd): %.4g microsec (usec)\n",1e6*eph[current_sat].tgd);
    printf("%s",tab);
    printf("Time of Clock (toc) %8.1f sec. ",eph[current_sat].toc);
    printf("Clock error: %6.1f usec\n",eph[current_sat].af[0]*1e6);
    printf("%s",tab);
    printf("Clock drift: %5.2f usec/day. ",24*3600*eph[current_sat].af[1]*1e6);
    printf("Rate of drift: %.2f sec/sec^2\n",eph[current_sat].af[2]);
// printf("------------------------------------------------------------------\n");
//getchar();
}



void fill_subframe2()
{
    ULONG temp;
    double d;
    int L;
    char *tab="        ";

    double A,n0,mu;


    temp = extrae_ulong(49,8);
    eph[current_sat].iode2=temp;


    L=16;
    temp = extrae_ulong(169,L);
    d=get_real(temp,L,1,-29);
    eph[current_sat].cus=d;
    L=16;
    temp = extrae_ulong(121,L);
    d=get_real(temp,L,1,-29);
    eph[current_sat].cuc = d;
    L=16;
    temp=extrae_ulong(57,L);
    d=get_real(temp,L,1,-5);
    eph[current_sat].crs=d;

    L=32;
    temp = extrae_ulong(89,L);
    d=get_real(temp,L,1,-31)*WGS84_PI;
    eph[current_sat].M0 = d;
    L=16;
    temp = extrae_ulong(73,L);
    d=get_real(temp,L,1,-43)*WGS84_PI;
    eph[current_sat].dn = d;

    L=32;
    temp = extrae_ulong(137,L);
    d=get_real(temp,L,0,-33);
    eph[current_sat].ecc=d;
    L=32;
    temp = extrae_ulong(185,L);
    d=get_real(temp,L,0,-19);
    eph[current_sat].roota = d;

    L=16;
    temp = extrae_ulong(217,L)<<4;
    eph[current_sat].toe=temp;

    L=1;
    temp = extrae_ulong(233,L);
    eph[current_sat].fit_flag=(BYTE)temp;
    L=5;
    temp = extrae_ulong(235,L);
    eph[current_sat].aodo=(unsigned short)(temp*900);


    if(VERBOSE_NAV==0) return;
    if((SELECTED_SF!=-1) && (SELECTED_SF!=2)) return;

//printf("------------------------------------------------------------------\n");
    printf("\nPRN %02d: ",current_sat+1);
    printf("Issue Of Data Ephemeris (IODE) %lu\n",eph[current_sat].iode2);

    printf("%s",tab);
    printf("Reference Time for Ephemeris (TOE): %6.0f sec\n",eph[current_sat].toe);

    mu=3.986005e14;
    A=pow(eph[current_sat].roota,2.0);

    printf("%s",tab);
    printf("Semi-major axis: %.7g km.   ",A/1000.0);
    printf("Orbit eccentricity: %.6g\n",eph[current_sat].ecc);


    n0 = sqrt(mu/pow(A,3.0));
    n0=n0+eph[current_sat].dn;

    printf("%s",tab);
    printf("Mean Anomaly (m0): %.4g deg.   ",eph[current_sat].M0*180/WGS84_PI);
    printf("Mean Motion: %.4g deg/hour\n",3600*n0*180/WGS84_PI);

    printf("%s",tab);
    printf("Cus: %11.4e rad.  ",eph[current_sat].cus);
    printf("Cuc: %11.4e rad.  ",eph[current_sat].cuc);
    printf("Crs: %.4g mt.\n",eph[current_sat].crs);

    printf("%s",tab);
    printf("Curve Fit Interval: ");
    if(eph[current_sat].fit_flag==1) printf("> ");
    printf("4 hours.  ");
    printf("AODO %d sec\n",eph[current_sat].aodo);
//printf("------------------------------------------------------------------\n");
//getchar();
}


void fill_subframe3()
{
    ULONG temp;
    double d;
    int L;
    char *tab="        ";

//eph[current_sat].tom=(N_FRAME-1)*6;

    L=16;
    temp = extrae_ulong(49,L);
    d=get_real(temp,L,1,-29);
    eph[current_sat].cic = d;

    L=32;
    temp = extrae_ulong(65,L);
    d=get_real(temp,L,1,-31)*WGS84_PI;
    eph[current_sat].W0=d;

    L=16;
    temp = extrae_ulong(97,L);
    d=get_real(temp,L,1,-29);
    eph[current_sat].cis = d;

    L=32;
    temp = extrae_ulong(113,L);
    d=get_real(temp,L,1,-31)*WGS84_PI;
    eph[current_sat].i0 = d;

    L=16;
    temp = extrae_ulong(145,L);
    d=get_real(temp,L,1,-5);
    eph[current_sat].crc= d;

    L=32;
    temp = extrae_ulong(161,L);
    d=get_real(temp,L,1,-31)*WGS84_PI;
    eph[current_sat].w = d;

    L=24;
    temp = extrae_ulong(193,L);
    d=get_real(temp,L,1,-43)*WGS84_PI;
    eph[current_sat].Wdot = d;

    L=8;
    temp = extrae_ulong(217,L);
    eph[current_sat].iode3=temp;

    L=14;
    temp = extrae_ulong(225,L);
    d=get_real(temp,L,1,-43)*WGS84_PI;
    eph[current_sat].idot = d;


    if(VERBOSE_NAV==0) return;
    if((SELECTED_SF!=-1) && (SELECTED_SF!=3)) return;

    printf("\nPRN %02d: ",current_sat+1);
    printf("Issue of Data Ephemeris (IODE) %lu .\n",eph[current_sat].iode3);

    printf("%s",tab);
    printf("Inclination angle (i0) : %.4g deg. ",eph[current_sat].i0*180/WGS84_PI);
    printf("Rate (idot): %5.2f ''/hour\n",3600*3600*eph[current_sat].idot*180/WGS84_PI);

    printf("%s",tab);
    printf("Right Ascension (W)    : %.4g deg. ",eph[current_sat].W0*180/WGS84_PI);
    printf("Rate (Wdot): %5.2f ''/hour\n",3600*3600*eph[current_sat].Wdot*180/WGS84_PI);

    printf("%s",tab);
    printf("Argument of Perigee (w): %.4g deg.\n",eph[current_sat].w*180/WGS84_PI);

    printf("%s",tab);
    printf("Cis: %11.4e rad.  ",eph[current_sat].cis);
    printf("Cic: %11.4e rad.  ",eph[current_sat].cic);
    printf("Crc: %.4g mt.\n",eph[current_sat].crc);



// printf("------------------------------------------------------------------\n");
//getchar();
}



void fill_subframe4()
{
    ULONG temp;
    BYTE page,sv_id,av;
    //BYTE as;
    double d;
    int L,k,bad;
    BYTE sv_conf[32];
    char health_msg[32][32];
    BYTE msg[30];
    double iono[8];
    BYTE sv_health[32],detail;
    int scale[8] = {-30, -27, -24, -24, 11, 14, 16, 16};
    char av_message[32];
    char *tab="             ";
    char on_off[2][4];
    strcpy(on_off[0]," ON");
    strcpy(on_off[1],"OFF");

    sv_id=(BYTE)extrae_ulong(51,6);
    page=pages[sv_id];


    if(VERBOSE_NAV==0) return;
    if((SELECTED_SF!=-1) && (SELECTED_SF!=4)) return;
    if((SELECTED_PAGE!=-1) && (SELECTED_PAGE!=page)) return;


//printf("------------------------------------------------------------------\n");
    printf("\n Subframe 4: ");

    switch(page)
    {
    case 0:
        printf("Dummy sat. No data\n");
        break;
    case 1:
        printf("Pages 1,6,11,16 or 21 (repeated). Reserved data\n");
        break;//Reserved
        break;
    case 12:
        printf("Pages 12 or 24 (repeated). Reserved data\n");
        break;//Reserved
        break;
    case 13:
        av=(BYTE)extrae_ulong(57,2);
        switch(av)
        {
        case 0:
            strcpy(av_message,"Available");
            break;
        case 1:
            strcpy(av_message,"Encrypted");
            break;
        case 2:
            strcpy(av_message,"Not available");
            break;
        case 3:
            strcpy(av_message,"?????");
            break;
        }
        printf("Navigation Message Correction Table: %s\n",av_message);
        break;
    case 14:
        printf("Page 14: Spare\n");
        break; //Spare
        break;//Reserved
    case 15:
        printf("Page 15: Spare\n");
        break; //Spare
        break;//Reserved
    case 17: // Special messages
        for(k=0; k<22; k++) msg[k]=frame[current_sat][7+k];
        msg[22]=0;
        // for(k=0;k<22;k++) printf("%02x ",msg[k]);
        printf("%s",tab);
        printf("ASCII message: %s\n",msg);
        // getchar();
        break;
    case 18: // IONO  & UTC
        printf("Page 18.\n");
        printf("      Iono parameters\n");
        L=8;
        for(k=0; k<8; k++) iono[k]=get_real(frame[current_sat][7+k],L,1,scale[k]);
        printf("%s",tab);
        printf("Alfa: ");
        for(k=0; k<4; k++) printf("%6.3e ",iono[k]);
        printf("\n");
        printf("%s",tab);
        printf("Beta: ");
        for(k=4; k<8; k++) printf("%6.3e ",iono[k]);
        printf("\n");

        printf("       UTC parameters\n");
        printf("%s",tab);
        L=32;
        temp = extrae_ulong(145,L);
        d=get_real(temp,L,1,-30);
        printf("A0 %.2f nanosec   ",d*1e9);
        L=24;
        temp = extrae_ulong(121,L);
        d=get_real(temp,L,1,-50);
        printf("A1 %12.5e nanosec/sec\n",d*1e9);

        printf("%s",tab);
        L=8;
        temp = extrae_ulong(177,L);
        temp<<=12;
        printf("Tot %d sec. ",temp);
        L=8;
        temp = extrae_ulong(185,L);
        printf("WN_t %d weeks. ",temp);
        L=8;
        temp = extrae_ulong(201,L);
        printf("WN_LSF %d weeks\n",temp);


        printf("%s",tab);
        L=8;
        temp = extrae_ulong(193,L);
        d=get_real(temp,L,1,0);
        printf("DT_LS %.0f sec. ",d);
        L=8;
        temp = extrae_ulong(217,L);
        d=get_real(temp,L,1,0);
        printf("DT_LSF %.0f sec\n",d);

        printf("%s",tab);
        L=8;
        temp = extrae_ulong(209,L);
        printf("Day Number %d\n",temp);

        //getchar();
        break;
    case 19:
        printf("Page 19. Reserved data\n");
        break;
    case 20:
        printf("Page 20. Reserved data\n");
        break;
    case 22:
        printf("Page 22. Reserved data\n");
        break;
    case 23:
        printf("Page 23. Reserved data\n");
        break;

    case 25:
        printf("Page 25. Sat config for all SVs and health for SVs 25-32\n");

        for(k=0; k<32; k++)  sv_conf[k] = (BYTE)extrae_ulong(57+k*4,4);

        /*           for (k=0;k<32;k++)
                     {
                      as=sv_conf[k]>>3;
                      printf("%02d: A/S %s ",k+1,on_off[1-as]);
                      if (sv_conf[k]&0x07) printf("B II"); else  printf("B I");
                      if((k%8)==7) printf("\n"); else printf(" ");
                     }
        */
        printf("\n   Satellite Constellation Configuration Report\n");
        for(L=0; L<2; L++)
        {
            printf(" PRN  :");
            for(k=16*L; k<16*(L+1); k++) printf(" %02d ",(k+1));
            printf("\n A/S  :");
            for(k=16*L; k<16*(L+1); k++) printf("%3s ",on_off[1-(sv_conf[k]>>3)]);
            printf("\n BLOCK:");
            for(k=16*L; k<16*(L+1); k++)
                if(sv_conf[k]&0x07) printf(" II ");
                else  printf("  I ");
            printf("\n");
        }

        get_messages(health_msg);
        printf("\n   Health report for SVs 25-32: ");
        for(k=24; k<32; k++) sv_health[k]=(BYTE)extrae_ulong(199+(k-24)*6,6);

        bad=0;
        for(k=24; k<32; k++)
        {
            if(sv_health[k]==0) continue;
            bad++;
            printf("\n      PRN %02d (",k+1);
            detail = sv_health[k] & 0x1f;
            p_nbits(sv_health[k],5);
            printf(") : %s",health_msg[detail]);
        }
        if(bad==0) printf(" all SVs healthy\n");
        else  printf("\n   All other SVs from 25 to 32 healthy\n");
        getchar();
        break;

    /*
             printf("\nSat Health for sats 25-32\n");
              for(k=24;k<32;k++) sv_health[k]=(BYTE)extrae_ulong(199+(k-24)*6,6);
              for(k=24;k<32;k++)
                {
                 printf("S%02d ",k); p_nbits(sv_health[k],6);
                 if ((k%4)==3) printf("\n"); else printf(" | ");
                }
              break;
    */
    default:
        if((sv_id>=25) && (sv_id<=32))
        {
            printf("Page %d Almanac data for sat %d\n",page,sv_id);
            show_alm();
        }

        else
            printf("d SV_ID %d. Almanac page devoted to other function\n",page,sv_id);
        break;
    }


//printf("------------------------------------------------------------------\n");
//getchar();

}




int get_messages(char h[32][32])
{
//    int k;

    strcpy(h[0],"All signals OK");

    strcpy(h[1],"All signals Weak");
    strcpy(h[2],"All signals Dead");
    strcpy(h[3],"All signals without Data");

    strcpy(h[4],"L1 P signal Weak");
    strcpy(h[5],"L1 P signal Dead");
    strcpy(h[6],"L1 P signal without Data");
    strcpy(h[7],"L2 P signal Weak");
    strcpy(h[8],"L2 P signal Dead");
    strcpy(h[9],"L2 P signal without Data");

    strcpy(h[10],"L1 C signal Weak");
    strcpy(h[11],"L1 C signal Dead");
    strcpy(h[12],"L1 C signal without Data");
    strcpy(h[13],"L2 C signal Weak");
    strcpy(h[14],"L2 C signal Dead");
    strcpy(h[15],"L2 C signal without Data");

    strcpy(h[16],"L1 & L2 P signal Weak");
    strcpy(h[17],"L1 & L2 P signal Dead");
    strcpy(h[18],"L1 & L2 P signal without Data");
    strcpy(h[19],"L1 & L2 C signal Weak");
    strcpy(h[20],"L1 & L2 C signal Dead");
    strcpy(h[21],"L1 & L2 C signal without Data");

    strcpy(h[22],"L1 signal Weak");
    strcpy(h[23],"L1 signal Dead");
    strcpy(h[24],"L1 signal without Data");
    strcpy(h[25],"L2 signal Weak");
    strcpy(h[26],"L2 signal Dead");
    strcpy(h[27],"L2 signal without Data");


    strcpy(h[28],"SV is temporarily OUT");
    strcpy(h[29],"SV will be temporarily OUT");
    strcpy(h[30],"???");
    strcpy(h[31],"Several Anomalies");

// for (k=0;k<32;k++) printf("%s\n",h[k]);

    return 1;

}


int show_alm()
{
    double ecc,inc,Wdot,W0,roota,w,M0,af0,af1;
//    ULONG wna
    ULONG toa;
    BYTE health;
    int L;
    ULONG temp;
    char *tab="             ";


    L=16;
    temp = extrae_ulong(57,L);
    ecc=get_real(temp,L,0,-21);
    L=8;
    temp = extrae_ulong(73,L);
    toa=temp<<12;
    L=16;
    temp = extrae_ulong(81,L);
    inc=(0.3+get_real(temp,L,1,-19))*WGS84_PI;
    L=16;
    temp = extrae_ulong(97,L);
    Wdot=get_real(temp,L,1,-38)*WGS84_PI;
    L=8;
    temp = extrae_ulong(113,L);
    health=(BYTE)temp;
    L=24;
    temp = extrae_ulong(121,L);
    roota=get_real(temp,L,0,-11);
    L=24;
    temp = extrae_ulong(145,L);
    W0=get_real(temp,L,1,-23)*WGS84_PI;
    L=24;
    temp = extrae_ulong(169,L);
    w=get_real(temp,L,1,-23)*WGS84_PI;
    L=24;
    temp = extrae_ulong(193,L);
    M0=get_real(temp,L,1,-23)*WGS84_PI;

    L=11;
    temp = (extrae_ulong(217,8)<<3) + extrae_ulong(236,3);
    af0=get_real(temp,L,1,-20);

    L=11;
    temp = extrae_ulong(225,L);
    af1=get_real(temp,L,1,-38);

    printf("%s",tab);
    printf("Almanac Reference Time (toa) %lu sec. ",toa);
    printf("Health ");
    p_bits(health);
    printf("\n");
    printf("%s",tab);
    printf("Semimajor axis: %.7g km.  ",pow(roota,2.0)/1000.0);
    printf("Eccentricity: %.6g\n",ecc);
    printf("%s",tab);
    printf("Right Ascension (W): %.2f deg. ",W0*180/WGS84_PI);
    printf("Rate (Wdot): %4.1f''/hour\n",3600*3600*Wdot*180/WGS84_PI);

    printf("%s",tab);
    printf("Inclination angle (i0) : %.2f deg.\n",inc*180/WGS84_PI);
    printf("%s",tab);
    printf("Argument of Perigee (w): %.2f deg.\n",w*180/WGS84_PI);
    printf("%s",tab);
    printf("Mean Anomaly (m0): %.2f deg.\n",M0*180/WGS84_PI);

    printf("%s",tab);
    printf("Clock error: %6.1f usec.  ",af0*1e6);
    printf("Clock drift: %5.2f usec/day.\n",24*3600*af1*1e6);

    return 1;
}


void fill_subframe5()
{
    BYTE page,sv_id;
    char msg[32][32];
//    ULONG temp;
//    double d;
    ULONG wna,toa;
    BYTE sv_health[24],detail;
    int k,bad;
    char *tab="             ";

    sv_id=(BYTE)extrae_ulong(51,6);
    page=pages[sv_id];

    if(VERBOSE_NAV==0) return;
    if((SELECTED_SF!=-1) && (SELECTED_SF!=5)) return;
    if((SELECTED_PAGE!=-1) && (SELECTED_PAGE!=page)) return;


    printf("\n Subframe 5: ");
//printf("------------------------------------------------------------------\n");

    switch(page)
    {
    case 0:
        printf("Dummy sat. Non-available data\n");
        break;
    case 25:
        get_messages(msg);
        printf("Page 25 (Almanac time & Health info for SV 1-24)\n");

        toa=extrae_ulong(57,8)<<12;
        wna=extrae_ulong(65,8);
        printf("   Time of Almanac (toa) %lu sec.  Almanac Week (mod 256) %d\n",toa,wna);

        printf("   Health report for SVs 1-24: ");
        for(k=0; k<24; k++) sv_health[k]=(BYTE)extrae_ulong(73+k*6,6);

        bad=0;
        for(k=0; k<24; k++)
        {
            if(sv_health[k]==0) continue;
            bad++;
            printf("\n      PRN %02d (",k+1);
            detail = sv_health[k] & 0x1f;
            p_nbits(sv_health[k],5);
            printf(") : %s",msg[detail]);
        }
        if(bad==0) printf(" all SVs healthy\n");
        else  printf("\n   All other SVs from 1 to 24 healthy\n");
        getchar();
        break;

    default: //Almanac Data
        printf("Page %2d (Almanac data for sat %d)\n",page,page);
        show_alm();

        break;
    }

//getchar();
//printf("------------------------------------------------------------------\n");


}




int verify_frame()
{
    int k,sum;
    sum=0;
    for(k=0; k<30; k++) sum+=check_frame[current_sat][k];

    k=(30-sum)/3;

//if (k==0) printf("Frame OK\n");
//else printf("Frame with %d words missing\n",k);

    return k;

}


void dump_frame(int type)
{
    int k;


    if(type==0)
    {
        //printf("Hex  ");
        for(k=0; k<30; k++)
        {
            if((k%3)==0) printf("  ");
            printf("%02x ",frame[current_sat][k]);
            if((k%15)==14) printf("\n");
        }
    }
    else
    {
        //printf("Bin  ");
        for(k=0; k<30; k++)
        {
            if((k%3)==0) printf("  ");
            p_bits(frame[current_sat][k]);
            printf("  ");
            if((k%6)==5) printf("\n");
        }
    }
}


BOOLEAN info_frame(ULONG N_frame,char* info_msg)
{
    BYTE page,sv_id;
    int n_bad;
    char *ptr=info_msg;
    BOOLEAN fail=0;
    BYTE sf_id,preamble;
    ULONG tow;
//    ULONG av;
//    char av_message[16];


    n_bad=verify_frame();
    if(n_bad)
    {
        ptr+=sprintf(ptr," %d missing words\n",n_bad);
        fail=1;
        return fail;
    }

    preamble=frame[current_sat][0];
    tow = extrae_ulong(25,17);

    if(preamble!=0x8b)
    {
        ptr+=sprintf(ptr,"Bad preamble ");
        fail=1;
    }
    if(tow!=N_frame)
    {
        ptr+=sprintf(ptr,"Tow missmatch ");
        fail=1;
    }


    if(fail)
    {
        ptr+=sprintf(ptr,"\n");
        //ptr+=sprintf(ptr,": skipping\n"); //printf("%s",info_msg);
        return fail;
    }


    sf_id = (BYTE)extrae_ulong(44,3);
    ptr+=sprintf(ptr," Subframe %d",sf_id);

    if(sf_id<=3)
    {
        ptr+=sprintf(ptr,"                Ephemeris data\n");
        return fail;
    }

    sv_id=(BYTE)extrae_ulong(51,6);
    page=pages[sv_id];

// printf("SV_ID %d  PAGE %d\n",sv_id,page);

    switch(sv_id)
    {
    case 0:
        ptr+=sprintf(ptr,", page --       ");
        break;
    //ptr+=sprintf(ptr,"Dummy sat:");  break;
    case 57:
        ptr+=sprintf(ptr,", page %2d*      ",page);
        break;
    //ptr+=sprintf(ptr,"(1,6,11,16,21) : ");  break;
    case 62:
        ptr+=sprintf(ptr,", page %2d*      ",page);
        break;
    //ptr+=sprintf(ptr,"(12,24)        : "); break;
    default:
        ptr+=sprintf(ptr,", page %2d       ",page);
        break;
    }

    if(sf_id==4)
    {
        switch(page)
        {
        case 0:
            ptr+=sprintf(ptr,"Dummy sat(no data)\n");
            break;
        case 1:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 12:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 13:
            /* av=extrae_ulong(57,2);
                          switch(av)
                           {
                            case 0: strcpy(av_message,"Available"); break;
                            case 1: strcpy(av_message,"Encrypted"); break;
                            case 2: strcpy(av_message,"Not available"); break;
                            case 3: strcpy(av_message,"?????"); break;
                           }
                          ptr+=sprintf(ptr,"NMCT : %s\n",av_message);
            */
            ptr+=sprintf(ptr,"NMCT\n");
            break;
        case 14:
            ptr+=sprintf(ptr,"Spare\n");
            break;
        case 15:
            ptr+=sprintf(ptr,"Spare\n");
            break;
        case 17:
            ptr+=sprintf(ptr,"ASCII messages\n");
            break;
        case 18:
            ptr+=sprintf(ptr,"Iono parameters\n");
            break;
        case 19:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 20:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 22:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 23:
            ptr+=sprintf(ptr,"Reserved\n");
            break;
        case 25:
            ptr+=sprintf(ptr,"Sat Config\n");
            break;
        default:
            if((sv_id>=25) && (sv_id<=32))
                ptr+=sprintf(ptr,"Almanac (sat %d)\n",sv_id);
            else ptr+=sprintf(ptr,"Other function\n");
            break;
        }
    }
    else
    {
        switch(page)
        {
        case 0:
            ptr+=sprintf(ptr,"Dummy sat(no data)\n");
            break;
        case -1:
            ptr+=sprintf(ptr,"\n");
            break;
        case 25:
            ptr+=sprintf(ptr,"Sat 1-24 health data\n");
            break;
        default:
            ptr+=sprintf(ptr,"Almanac (sat %d)\n",sv_id);
        }
    }

    //printf("%s",info_msg);



    return fail;
}


/////////////////////////////////////////////////////////////////////////
// void PAUSA(msec) Wait for msec milliseconds
/////////////////////////////////////////////////////////////////////////
void pausa(double msec)    // Pausa de msec msec
{
    double CPMS=CLOCKS_PER_SEC/1000.0;
    clock_t start=clock();

    while((clock()-start)/CPMS < msec)
    {
    }
}


void procesa_frame(ULONG N_frame)
{
    BOOLEAN fail;
    BYTE sf_id;
    char info[256];

    fail=info_frame(N_frame,info);
    if(MONITOR_NAV)
    {
        if(fail) printf(" ERROR:");
        printf("%s",info);
        if(VERBOSE_NAV==0) return;
    }

    if(fail) return;


    sf_id = (BYTE)extrae_ulong(44,3);  //(frame[5] >>2) & 0x07;

    //printf("Procesando frame %d\n",sf_id);

    if((NAV_GENERATION==1) && (sf_id>3))  return;

//    printf("SF_ID %d  FAIL %d\n",sf_id,fail);

    switch(sf_id)
    {
    case 1:
        fill_subframe1();
        break;
    case 2:
        fill_subframe2();
        break;
    case 3:
        fill_subframe3();
        break;
    case 4:
        fill_subframe4();
        break;
    case 5:
        fill_subframe5();
        break;
    }

}




void reset_eph()
{
    int k;
    for(k=0; k<32; k++)
    {
        eph[k].iodc=-1;
        eph[k].iode2=-1;
        eph[k].iode3=-1;
        eph[k].last_iode=-1;
    }
}


int check_info_eph()
{
    int iodc, iodcc, iode2,iode3;
    int LAST_IODE=eph[current_sat].last_iode;

    if(current_sat!=SELECTED_SV) return 1;
    iodc=eph[current_sat].iodc;
    iodcc=iodc&0xff;
    iode2=eph[current_sat].iode2;
    iode3=eph[current_sat].iode3;

//   printf("%d (%d) %d %d\n",iodc,iodcc,iode2,iode3);

    // Condiciones
    if(((iodcc!=iode2) | (iode2!=iode3))  && (iodc!=-1))
    {
        printf("CURRENT: %d. Change!! ",LAST_IODE);
        printf("%d (%d) %d %d\n",iodc,iodcc,iode2,iode3);
        getchar();
    }

    if((iodcc==iode2) && (iode2==iode3) && (iode3!=LAST_IODE))
    {
        printf("New Ephemeris IODE %d\n",iode3);
        getchar();
    }
    return 1;
}


int detect_new_ephemeris()
{
    int iodc,iodcc,iode2,iode3;


    iodc=eph[current_sat].iodc;
    iodcc=iodc&0xff;
    iode2=eph[current_sat].iode2;
    iode3=eph[current_sat].iode3;

    //printf("%d (%d) %d %d\n",iodc,iodcc,iode2,iode3);

    if((iodcc==iode2) && (iode2==iode3) && (iode3!=eph[current_sat].last_iode))
    {
        eph[current_sat].last_iode=iode3;
        return 1;
    }

    return 0;
}

void generate_nav_header(FILE* fd)
{
    int k,l,lines,written,max_lines,nchars;
    char *header,*ptr;
    time_t tt;
    char *date,buffer[80];
//struct tm gmt;
//double dt,secs;


// I dont think I'll use more than 15 lines for the header

    max_lines=15;
    header=(char*)malloc(max_lines*80*sizeof(char));

    ptr=header;

    ptr+=sprintf(ptr,"%9.2f%11c",2.1,32);
    ptr+=sprintf(ptr,padd("N: GPS NAV DATA",buffer,20));
    ptr+=sprintf(ptr,"%20c",32);
    ptr+=sprintf(ptr,padd("RINEX VERSION / TYPE",buffer,20));

    sprintf(buffer,"GAR2RNX %4.2f",VERSION);
    ptr+=sprintf(ptr,padd(buffer,buffer,20));
    ptr+=sprintf(ptr,padd("Any GPS12 Owner",buffer,20));
    time(&tt);
    date=ctime(&tt);
    ptr+=sprintf(ptr,padd(date,buffer,20));
    ptr+=sprintf(ptr,"PGM / RUN BY / DATE ");

    padd("** gar2rnx (GARmin TO RiNeX) generates ephemeris files",buffer,60);
    ptr+=sprintf(ptr,buffer);
    ptr+=sprintf(ptr,padd("COMMENT",buffer,20));

    padd("** from a GPS12 (and others) (Copyright Antonio Tabernero)",buffer,60);
    ptr+=sprintf(ptr,"%s",buffer);
    ptr+=sprintf(ptr,"COMMENT             ");


    sprintf(buffer,"** Generated from G12 data file: %s ",DATAFILE);
    padd(buffer,buffer,60);
    ptr+=sprintf(ptr,"%s",buffer);
    ptr+=sprintf(ptr,"%s",padd("COMMENT",buffer,20));

    written=sprintf(ptr,"** Options: ");
    ptr+=written;
    ptr+=sprintf(ptr,padd(COMMAND_LINE,buffer,60-written));
    ptr+=sprintf(ptr,"COMMENT             ");

// ptr+=sprintf(ptr,"  %12.4E%12.4E%12.4E%12.4E%10c",0.0,0.0,0.0,0.0,32);
// ptr+=sprintf(ptr,padd("ION ALFA",buffer,20));
// ptr+=sprintf(ptr,"  %12.4E%12.4E%12.4E%12.4E%10c",0.0,0.0,0.0,0.0,32);
// ptr+=sprintf(ptr,padd("ION BETA",buffer,20));
// ptr+=sprintf(ptr,"%60cDELTA-UTC: A0,A1,T,W",32);
// ptr+=sprintf(ptr,"%60cLEAP SECONDS        ",32);

    ptr+=sprintf(ptr,"%60cEND OF HEADER       ",32);

    nchars=(ptr-header);
    lines=nchars/80;
//printf("Number of chars %d -> lines %d\n",nchars,lines);

    for(l=0; l<lines; l++,fprintf(fd,"\n"))
        for(k=0; k<80; k++)  fprintf(fd,"%c",header[l*80+k]);

    free((char*)header);


}



void check_VC_format()
{
    double a=0.12345678901234;
    char msg[32];
    int k;

    sprintf(msg,"%11.3le",a);

    k=0;
    while(msg[k]!='e') k++;
    //printf("%s. Posicion %d\n",msg,k);

    if(k==7) VC_format=0;
}


#define MASK_DATA   0x3fffffc0
#define MASK_PARITY 0x0000003f

BOOLEAN parity(BYTE *ptr)
{
    BYTE P,T,*dest;
    //int k;

    dest=(BYTE*)&NAV_WORD;
    memcpy((void*)&NAV_WORD,(void*)ptr,4);


    //for(k=0;k<4;k++) { p_bits(dest[k]); printf("|"); } printf("\n");

    if(P30) NAV_WORD ^= MASK_DATA;   // source bits complement

    //for(k=0;k<4;k++) { p_bits(dest[k]); printf("|"); } printf("\n");


    P = (BYTE)(P29^d1^d2^d3^d5^d6^d10^d11^d12^d13^d14^d17^d18^d20^d23);
    P = (P<<1) + (BYTE)(P30^d2^d3^d4^d6^d7^d11^d12^d13^d14^d15^d18^d19^d21^d24);
    P = (P<<1) + (BYTE)(P29^d1^d3^d4^d5^d7^d8^d12^d13^d14^d15^d16^d19^d20^d22);
    P = (P<<1) + (BYTE)(P30^d2^d4^d5^d6^d8^d9^d13^d14^d15^d16^d17^d20^d21^d23);
    P = (P<<1) + (BYTE)(P30^d1^d3^d5^d6^d7^d9^d10^d14^d15^d16^d17^d18^d21^d22^d24);
    P = (P<<1) + (BYTE)(P29^d3^d5^d6^d8^d9^d10^d11^d13^d15^d19^d22^d23^d24);

    //printf("D25-30%d%d%d%d%d%d\n

    T=(BYTE)(NAV_WORD&MASK_PARITY);
//printf("Comp Parity %02x  Transmitted %02x\n",P,T); //getchar();

    return (P==T);
}


void generate_nav(FILE *fd)
{
    BYTE record[256],id,L;
    type_rec0x36 rec;
    ULONG N_frame,word;
    ULONG current_frame[32];
    BOOLEAN first=1;
    FILE *dest;
    ULONG week,garmin_wdays,tom;
    char name[128];
    BOOLEAN all_par;

    check_VC_format();

    reset_eph();
    for(current_sat=0; current_sat<32; current_sat++)
    {
        reset_frame();
        current_frame[current_sat]=0xffffffff;
    }

    all_par=1;
    while(feof(fd)==0)
    {
        fread(&id,1,1,fd);
        fread(&L,1,1,fd);
        fread(record,1,L,fd);

        if(id==0x36)
        {
            rec=process_0x36(record);
            current_sat=rec.sv;
            if(current_sat<32)
            {
                N_frame = (rec.c50-30)/300;
                if(current_frame[current_sat]==0xffffffff)
                    current_frame[current_sat]=N_frame;
                if(N_frame!=current_frame[current_sat])
                {
                    //printf("N_frame %d  Parity %d\n",N_frame,all_par);
                    if(all_par) procesa_frame(N_frame);
                    all_par=1;

                    reset_frame();
                    if(detect_new_ephemeris() && (eph[current_sat].health==0))
                    {
                        tom=6*(N_frame-1);
                        //printf("New Ephemeris -> Frame %d (Tom %d): ",N_frame,tom);
                        //printf("PRN %d. IODE %d\n",current_sat+1,eph[current_sat].iode3);
                        eph[current_sat].tom=tom;
                        if(first)
                        {
                            // Convert toc time
                            week=(ULONG)eph[current_sat].week;
                            garmin_wdays=(week-521)*7; //tom=floor(tom);
                            if(RINEX_FILE)
                            {
                                get_rinex_file_name(location,garmin_wdays,tom,name,'n');
                                dest=fopen(name,"w");
                            }
                            else dest=stdout;
                            generate_nav_header(dest);
                            first=0;
                        }
                        dump_eph(dest);  //current_sat);
                    }
                    current_frame[current_sat]=N_frame;
                }

                all_par &= parity(rec.uk);
                word=((rec.c50-30)%300)/30;
                strip_parity(rec.uk,word);
            }
        }
    }

    if(RINEX_FILE) fclose(dest);
    if(STDIN==0) fclose(fd);

    exit(0);
}


void monitor_nav(FILE *fd)
{
    BOOLEAN par,all_par;
    BYTE record[256],id,L;
    type_rec0x36 rec;
    ULONG N_frame,word,last_word;
    ULONG current_frame[32];
    int k;
    char *line="-------------------------------------------------------------------------\n";

    printf("%s",line);

    printf("     ***** Tracking navigation message from PRN %2d *****\n",SELECTED_SV+1);
    printf("%s",line);
    printf("Subframe HOW  Words in SF  Parity/Subframe ID/page      Contents\n");
    printf("%s",line);

    reset_eph();
    for(current_sat=0; current_sat<32; current_sat++)
    {
        reset_frame();
        current_frame[current_sat]=0xffffffff;
        eph[current_sat].last_iode=-1;
    }


    all_par=1;
    last_word=-1;

    while(feof(fd)==0)
    {
        fread(&id,1,1,fd);
        fread(&L,1,1,fd);
        fread(record,1,L,fd);

        if(id==0x36)
        {
            rec=process_0x36(record);
            current_sat=rec.sv;
            if(current_sat==SELECTED_SV)
            {
                N_frame = (rec.c50-30)/300;
                if(current_frame[current_sat]==0xffffffff)
                {
                    current_frame[current_sat]=N_frame;
                    printf(" HOW %6d [",6*N_frame);
                }
                if(N_frame!=current_frame[current_sat])
                {
                    printf("] ");
                    if(all_par)
                    {
                        //printf(" Parity OK  ");
                        procesa_frame(N_frame);
                    }
                    else printf(" BAD PARITY\n");
                    if(VERBOSE_NAV) printf("%s",line);
                    reset_frame();
                    last_word=-1;
                    current_frame[current_sat]=N_frame;
                    all_par=1;
                    printf(" HOW %6d [",6*N_frame);
                }

                //pausa(600);
                word=((rec.c50-30)%300)/30;

                for(k=1; k<(int)(word-last_word); k++) printf(" ");
                //for(k=1;k<(word-last_word);k++) ptr+=sprintf(ptr," ");

                par=parity(rec.uk);
                //if (par) ptr+=sprintf(ptr,"O"); else ptr+=sprintf(ptr,"X");
                if(par) printf("O");
                else printf("X");
                all_par=all_par&par;

                last_word=word;
                //printf(" HOW %6d  [%s%c",6*N_frame,line,13);
                fflush(stdout);
                strip_parity(rec.uk,word);
            }
        }
    }

    printf("\n");
    if(STDIN==0) fclose(fd);
    exit(0);
}



//////////////////////////////////////////////////////////////////////////


void print_help(char **argv)
{
    char help[8192];

    sprintf(help,
            "-----------------------------------------------------------------\n"\
            "Gar2rnx (Garmin to Rinex) generates rinex2 compliant files fromv \n"\
            "data obtained with some Garmin handhelds using the async logger  \n"\
            "-----------------------------------------------------------------\n"\
            "Version 1.48, Copyright 2000-2002 Antonio Tabernero Galan        \n"\
            "Version %4.2f, Copyright 2016 Norm Moulton                        \n"\
            "-----------------------------------------------------------------\n",VERSION);


    strcat(help,"\n\
USAGE: gar2rnx g12file [-stat]\n\
                       [-parse options]\n\
                       [-rinex options]\n\
                       [-nav]\n\
                       [-monitor option]\n\
\n\
  g12file is a file generated using the async logger utility.\n\
          It's the only mandatory argument. If this argument is\n\
          stdin, gar2rnx will expect the data coming from the\n\
          standard input.\n\
          On the data contained in such file, gar2rnx can perform\n\
          several basic functions, each one with their own options\n\
\n\n\
******************************************************************\n\n\
  -stat : shows statistics about the number, Identity byte and\n\
          length of received packets\n\n");


    strcat(help,"******************************************************************\n\n\
  -parse: this was the original functionality, designed to help us\n\
          interprete the async messages sent by the GPS12.\n\
\n\
   List of options that can be activated along with -parse:\n\
\n\
  -sats: shows records 0x1a (Track status, signal, elevation, etc.)\n\
         Default\n\
  -all : displays info about all sats. Default.\n\
  -s nn: for those fields with a SV ID field, displays only\n\
         those records regarding the satellite with PRN nn\n\
  -r 0xr1 0xr2 ... : displays info about records 0xr1 0xr2 (hexadecimal)\n\
         The fields (more or less) known are interpreted.\n\
         Unknown bytes are simply listed.\n\
  -dif : presents some additional info about the increment of several\n\
         fields in records 0x38 and 0x16 (Obsolete)\n\n");


    strcat(help,"******************************************************************\n\n\
  -rinex: this is the default functionality (hence the change of\n\
          name of the program from parser to gar2rnx).\n\
          Since it's the default you actually don't have to type it.\n\
\n\
   List of options that can be used along with -rinex:\n\
\n\
  -etrex    : when using an Etrex (or Emap) to log your data, you\n\
               should use this option to obtain a proper Rinex file.\n\
  -reset    : reset the time-tags to the nearest full second\n\
               modifying the observables accordingly\n\
  -f        : Instead of sending the RINEX file to standard output\n\
               (default) it creates a file using the RINEX conventions\n\
\n------------------------------------------------------------------\n\n\
   -start tow: starts the generation of the RINEX file from tow\n\
               (week_seconds). By default, it starts from the first\n\
               record logged in g12file\n\
   -stop  tow: don't include in the RINEX file those records with a\n\
               time-tag later than tow (week_seconds)\n\
   -time  tt : only tt seconds of observations go into the RINEX file\n\
   -int   tt : only those records from g12file that are time-tagged\n\
               with a multiple of tt seconds go into the RINEX file\n\
\n------------------------------------------------------------------\n");

    strcat(help,"\n\
   Observables dumped into the RINEX file:\n\
   By default, gar2rnx will output pseudoranges and L1 phases.\n\
   These observables can be modified using the following options:\n\
\n\
   -code -phase -doppler: eliminate the corresponding observables\n\
                          from the default set (code+phase)\n\
   +code +phase +doppler: add  the corresponding observables\n\
                          to the default set (code+phase)\n\
   For instance: -phase   : will output only pseudoranges\n\
                 +doppler : will dump code, phase and doppler\n\
                 -code +doppler: will output phase and doppler\n");


    strcat(help,"------------------------------------------------------------------\n\
   -date YYYY MM DD: date of the first observation (year month day)\n\
                     You should only use this option if you get\n\
                     an error message telling you that the date\n\
                     couldn't be obtained from the binary file.\n\
\n\
   -xyz X Y Z : our approximate position in ECEF XYZ coordinates (meters).\n\
   -llh lat long height: same but using lat-long (deg) and height (mt) (WGS84)\n\
\n\
   There are two situations where the above options are to be used:\n\
     * When no proper position could be obtained from the binary file.\n\
       Wait until you see an error message telling you so when running gar2rnx\n\
     * When you want to use that rinex file as data from a reference station\n\
       and you know the coordinates of your observing site with\n\
       a better precision than a navigation solution (+/- 10 mt)\n");



    strcat(help,"\n---------------------------------------------------------------\n\
   -area name: Name of the station or area where observations are taken.\n\
               If the option -f is given, the first 4 chars of this name\n\
               are used to create the RINEX filename.\n\
\n\
   -mark name: Marker name in the Rinex file. Name used to identify\n\
               a particular point of measurement.\n\n");


    strcat(help,"******************************************************************\n\n\
  -nav: (new with version 1.45)  creates a Rinex ephemeris file from\n\
        the binary data captured in the g12bin file.\n\n\
        gar2rnx g12bin -nav\n\n\
        will dump a Rinex ephemeris file, corresponding to the\n\
        session logged in g12bin to the standard output.\n\
        If we add the -f option, a properly named rinex\n\
        ephemeris file will be created, tipically brdcDDD1.YYN\n\
        We can change its name using the -area option:\n\n\
        gar2rnx g12bin -nav -f -area IUPM\n\n\
        will create a Rinex navigation file named IUPMDDD1.YYN\n\n");

    strcat(help,"******************************************************************\n\n\
  -monitor prn : (new with version 1.45)  This option followed by \n\
                 a prn number will monitor the navigation message \n\
                 coming from a particular satellite, check the \n\
                 parity of the received words, group them in a \n\
                 subframe, and identify the subframe type and page,\n\
                 indicating the contents of the data.\n\n\
   List of options that can be used along with -rinex:\n\n\
    -V     :   gar2rnx will interprete and present the information\n\
               received in the different subframes. Sure enough,\n\
               only those fields documented in the unclassified\n\
               documents will be shown.\n\
    -sf   N:   only subframes with id N (1 to 5) will be displayed\n\
    -page P:   only page number P out of subframe N will be shown\n\n\
******************************************************************\n\n");

    /*
    */

    printf("%s",help);
// printf("L %d\n",strlen(help));
    exit(0);


}


FILE* parse_arg(int argc, char **argv)
{
    int arg_num,j;
    FILE *fd;
//BOOLEAN first_obs;

    if(argc<2) print_help(argv);



//Check if stdin is desired
    if(strcmp(argv[1],"stdin")==0) STDIN=1;

    fd= (STDIN)? stdin:fopen(argv[1],"rb");

    strcpy(DATAFILE,argv[1]);

    if(fd==NULL)
    {
        printf("Cannot read data from %s (\?\')\n",DATAFILE);
        exit(0);
    }

    if(argc>2)
    {
        strcpy(COMMAND_LINE,argv[2]);
        for(j=3; j<argc; j++)
        {
            strcat(COMMAND_LINE," ");
            strcat(COMMAND_LINE,argv[j]);
        }
    }


// Defaults values: shows record 1a (sat info) for all satellites.

    SELECTED_PAGE=-1;
    SELECTED_SF=-1;
    SELECTED_SV=0x14;
    ONE_SAT=0;
    SELECTED_RECORDS[0]=0x1a;
    SELECTED_RECORDS[1]=-1;
    DIF_RECORDS=0;

    ETREX=0;
    RINEX_GENERATION=1;
    VERBOSE=0;
    VERBOSE_NAV=0;

    ONLY_STATS=0;
    PARSE_RECORDS=0;
    MONITOR_NAV=0;
    NAV_GENERATION=0;
    VERIFY_TIME_TAGS=0;
    NO_SNR=0;


    OBS_MASK=3;     // Dump code + phase
    RINEX_FILE=0;   // Send Rinex to stdout
    L1_FACTOR=1;
    GET_THIS=1;

    INTERVAL=1;
    START=-1;
    LAST=604800+1;
    ELAPSED=604800+1;

    RELAX=0;
    OPT1=0;

    GIVEN_XYZ=0;
    GIVEN_DATE=0;


    strcpy(location,"site");
    strcpy(marker,"Measured Point");

// User provided arguments
    arg_num=2;
    while(arg_num<argc)
    {
        if(strcmp(argv[arg_num],"-stat")==0)
        {
            ONLY_STATS=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-all")==0)
        {
            ONE_SAT=0;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-dif")==0)
        {
            DIF_RECORDS=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-reset")==0)
        {
            RESET_CLOCK=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-etrex")==0)
        {
            ETREX=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-parse")==0)
        {
            PARSE_RECORDS=1;
            RINEX_GENERATION=0;
            VERBOSE=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-nav")==0)
        {
            MONITOR_NAV=0;
            NAV_GENERATION=1;
            strcpy(location,"brdc");
            RINEX_GENERATION=0;
            VERBOSE=0;
            VERBOSE_NAV=0;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-monitor")==0)
        {
            MONITOR_NAV=1;
            RINEX_GENERATION=0;
            VERBOSE_NAV=VERBOSE=0;
            ONE_SAT=1;
            SELECTED_SV=atoi(argv[arg_num+1])-1;
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-sf")==0)
        {
            SELECTED_SF=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-page")==0)
        {
            SELECTED_PAGE=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-o1")==0)
        {
            OPT1=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-V")==0)
        {
            VERBOSE_NAV=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-o2")==0)
        {
            RELAX=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-verify")==0)
        {
            VERIFY_TIME_TAGS=1;
            RINEX_GENERATION=0;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-rinex")==0)
        {
            RINEX_GENERATION=1;
            VERBOSE=0;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-half")==0)
        {
            L1_FACTOR=2;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-f")==0)
        {
            RINEX_FILE=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-snroff")==0)
        {
            NO_SNR=1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-get33")==0)
        {
            GET_THIS=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-start")==0)
        {
            START=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-stop")==0)
        {
            LAST=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-time")==0)
        {
            ELAPSED=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-int")==0)
        {
            INTERVAL=atoi(argv[arg_num+1]);
            arg_num+=2;
        }
        //else if (strcmp(argv[arg_num],"-obs")==0) { N_OBS=atoi(argv[arg_num+1]); arg_num+=2; }
        else if(strcmp(argv[arg_num],"-code")==0)
        {
            OBS_MASK = OBS_MASK & 6;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-phase")==0)
        {
            OBS_MASK = OBS_MASK & 5;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-doppler")==0)
        {
            OBS_MASK = OBS_MASK & 3;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"+code")==0)
        {
            OBS_MASK = OBS_MASK | 1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"+phase")==0)
        {
            OBS_MASK = OBS_MASK | 2;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"+doppler")==0)
        {
            OBS_MASK = OBS_MASK | 4;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-area")==0)
        {
            strncpy(location,argv[arg_num+1],4);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-mark")==0)
        {
            strcpy(marker,argv[arg_num+1]);
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-sats")==0)
        {
            ONE_SAT=0;
            SELECTED_RECORDS[0]=0x1a;
            SELECTED_RECORDS[1]=-1;
            arg_num++;
        }
        else if(strcmp(argv[arg_num],"-s")==0)
        {
            ONE_SAT=1;
            SELECTED_SV=atoi(argv[arg_num+1])-1;
            arg_num+=2;
        }
        else if(strcmp(argv[arg_num],"-r")==0)
        {
            arg_num++;
            j=0;
            while((arg_num!=argc) && (argv[arg_num][0]!=45))
                SELECTED_RECORDS[j++]=strtoul(argv[arg_num++],(char**)NULL,16);
            SELECTED_RECORDS[j]=-1;
        }
        else if(strcmp(argv[arg_num],"-date")==0)
        {
            GIVEN_DATE=1;
            arg_num++;
            for(j=0; j<3; j++)
            {
                if((arg_num==argc) || (argv[arg_num][0]==45))
                {
                    printf("Correct usage of -date option: -date YYYY MM DD (date of observation)\n");
                    exit(0);
                }
                else USER_DATE[j]=atoi(argv[arg_num++]);
            }
        }
        else if(strcmp(argv[arg_num],"-xyz")==0)
        {
            GIVEN_XYZ=1;
            arg_num++;
            for(j=0; j<3; j++)
            {
                if((arg_num==argc))
                {
                    printf("Correct usage of -xyz option: -xyz X Y Z (ECEF position in meters)\n");
                    exit(0);
                }
                else USER_XYZ[j]=atof(argv[arg_num++]);
            }
        }
        else if(strcmp(argv[arg_num],"-llh")==0)
        {
            GIVEN_XYZ=1;
            arg_num++;
            for(j=0; j<3; j++)
            {
                if((arg_num==argc))
                {
                    printf("Correct usage of -llh option: -llh lat long h [deg deg mt])\n");
                    exit(0);
                }
                else USER_XYZ[j]=atof(argv[arg_num++]);
            }
            llh2xyz(USER_XYZ,USER_XYZ);
        }
        else
        {
            printf("Unknown Option %s\n",argv[arg_num]);
            arg_num++;
        }
    }

    if(VERBOSE)
    {
        if(GIVEN_DATE) printf("DATE: %2d/%2d/%4d\n",USER_DATE[2],USER_DATE[1],USER_DATE[0]);
        if(GIVEN_XYZ) printf("XYZ : %f %f %f\n",USER_XYZ[0],USER_XYZ[1],USER_XYZ[2]);
    }


    N_OBS=(OBS_MASK & 1) + ((OBS_MASK & 2)>>1) + ((OBS_MASK & 4)>>2);
    if(N_OBS==0)
    {
        printf("You have selected no observables for the RINEX file\n");
        printf("Exiting now\n");
        exit(0);
    }


    // If we just want code it doesnt make sense to allow phase_only output
    if(OBS_MASK==1) OPT1=0;


    //printf("Mask %d. N_obs %d. Observables: ",OBS_MASK,N_OBS);
    //if (OBS_MASK & 1) printf("code ");
    //if (OBS_MASK & 2) printf("phase ");
    //if (OBS_MASK & 4) printf("doppler ");
    //printf("\n");
    //exit(0);

    return fd;
}




int main(int argc,char**argv)
{
    FILE *fd;

    /*
    char number[24];
    double a=-1.234567890123456e-15;

    format_double(number,a);
    printf("%s\n",number);
    exit(0);
    */

    fd=parse_arg(argc,argv);

    if(ONLY_STATS) collect_stats(fd);
    if(PARSE_RECORDS) original_parsing(fd);
    if(VERIFY_TIME_TAGS) verify_tt(fd);
    if(NAV_GENERATION) generate_nav(fd);
    if(MONITOR_NAV) monitor_nav(fd);

    generate_rinex(fd);

    return 0;
}



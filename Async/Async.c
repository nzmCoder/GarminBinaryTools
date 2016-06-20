/****************************************************************************
ASYNC MESSAGE LOGGER for selected Garmin GPS Receivers

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

1.24   * Changes 2016 by N. Moulton
       * Removed Linux option
       * Reformated source code
       * Fixed all compile warnings
       * Attempted to fix serial problems in Windows

1.23   * Option -all to log the serial port stream directly to a file
       * Option -stdout to redirect the output to the standard output

1.21   * Some minor bugs corrected (thanks to Michal Hobot) and some
         parameters modified to improve comms with some Garmin models
         (thanks to Jim Harris).

1.20   * Get what I believe is Doppler shift info using the +doppler flag.

1.15   * Uses Garmin protocol to log position and UTC time for those
         units (i.e. Garmin 175) without a 0x33 record.
       * Tried to improve IO comms (dont know if it's any better, though)
       * Dumps IO comms to file "trace.txt" if TRACE_IO defined.

1.10   * Fix the DLE,ETX bug
       * Do not save packets with bad checksums
       * Relax the fix condition on 0x33 records (for eMap, etrex)

1.00   First version

****************************************************************************/

#define VERSION 1.24

//Uncomment this only for serial IO debugging  purposes
#define TRACE_IO

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <windows.h>
#include <winbase.h>

#ifdef TRACE_IO
#define TRACE_FILE "trace.txt"
FILE* TRACE;
#endif

/////////////////////////////////////////////////////////////////////////////
// Possible commands
/////////////////////////////////////////////////////////////////////////////
#define CHECK   1
#define IDENT   2
#define ASYNC   3
#define REQST   4
#define RINEX   5
#define DOPPLER 6
#define TEST    11
#define LOG_ALL 12

/////////////////////////////////////////////////////////////////////////////
// Default Arguments
/////////////////////////////////////////////////////////////////////////////
#define DEF_COMMAND         IDENT       // Get product ID
#define DEF_FLAG            0xffff      // All events
#define DEF_FLAG_REQUEST    0x0001      // Default Request
#define DEF_LOG_TIME        30          // 30 sec
#define DEF_VERBOSE_LEVEL   1           // Verbosity level

#define DEF_PORT  "COM1"
#define M_PI      3.141592653589793

static const char MONTH[12][4] = {"Jan", "Feb", "Mar", "Apr", "May",
                                  "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                                 };

/////////////////////////////////////////////////////////////////////////////


#define ID_PACKET   mInBuffer[0]    // ID Position in Packet
#define L_PACKET    mInBuffer[1]    // Length Position in Packet
#define DATA_PACKET (mInBuffer+2)   // Data Position in Packet
#define INI_PACKET  mInBuffer


#define MAXBUF 512

#define EOD 0x0C    // End of Data (in request commands)

#define DLE 0x10
#define ETX 0x03

#define ACK 0x06
#define NAK 0x15

#define DAT_ST 0
#define DLE_ST 1
#define ETX_ST 2

#define MAX_FAILED 5000

/////////////////////////////////////////////////////////////////////////////
// Error codes

#define E_OPEN            0
#define E_SETUP           1
#define E_GETCOMM         2
#define E_SETCOMM         3
#define E_GETTIME         4
#define E_SETTIME         5
#define E_PURGE           6
#define E_CLOSE           7

#define E_READ            8
#define E_TIMEOUT         9
#define E_EXPECTED_ACK    10
#define E_EXPECTED_PACKET 11

#define E_WRITE           16
#define E_WRITE_SHORT     17

#define E_OPTION          31


/////////////////////////////////////////////////////////////////////////////
// Types

typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef long int LONG;
typedef unsigned int UINT;
typedef struct
{
    double lat;
    double lon;
} D_POS;

/////////////////////////////////////////////////////////////////////////////
// Global variables

ULONG mCharsRead;

BOOLEAN mGoodChksum=0;
BOOLEAN mNeedAck=1;

int mRxState = DAT_ST;

BYTE mBuffer[MAXBUF];
BYTE mInBuffer[MAXBUF];

char mPort[32];
ULONG* mErrCodePtr;
ULONG mReadTimeOut=100;
BYTE mVerbose;

BOOLEAN mIsStdOut=0;
HANDLE  mComHnd;

/////////////////////////////////////////////////////////////////////////////
// Function Declarations

// Serial Port Functions
BOOLEAN open_port(char *ComPort);
BOOLEAN close_port();
void set_error(BYTE b);

// Serial Port Read Functions
UINT read_data(BOOLEAN responde);
UINT read_packet();
BOOLEAN read_char(BYTE *bptr);
UINT strip_packet(UINT n, BYTE *ack);
UINT send_ack(BYTE id,BYTE ok);
void show_in(UINT n);

// Serial Port Write Functions
UINT build_packet(BYTE *data);
UINT read_ack(BYTE id);
UINT send_packet(BYTE *data);
void show_out(UINT n);

// Auxilary functions
UINT get_uint(BYTE* ptr);
ULONG get_long(BYTE* ptr);

// GPS related functions
void ident();
BOOLEAN async(int flag_async);
void log_packets_0x33(FILE *fich);
void log_packets_async(int flag,double T_LOG,FILE *fich);
ULONG log_packets_request(UINT flag_request, FILE *fich);


/////////////////////////////////////////////////////////////////////////////
// Wait for msec milliseconds
/////////////////////////////////////////////////////////////////////////////
void pause(int msec)    // Pause in msec
{
    Sleep(msec);
}

/////////////////////////////////////////////////////////////////////////////
// Serial Port Open & Close Functions
/////////////////////////////////////////////////////////////////////////////
BOOLEAN open_port(char *ComPort)
{
    DCB      m_dcb;
    COMMTIMEOUTS m_CommTimeouts;

    mComHnd = CreateFile(ComPort, GENERIC_READ | GENERIC_WRITE,
                         0, // exclusive access
                         NULL, // no security
                         OPEN_EXISTING,
                         0, // no overlapped I/O
                         NULL); // null template

    if(mComHnd==INVALID_HANDLE_VALUE)
    {
#ifdef TRACE_IO
        fprintf(TRACE,"Can't open port %s\n",ComPort);
        fflush(TRACE);
#endif
        set_error(E_OPEN);
        return 0;
    }
    else
    {
#ifdef TRACE_IO
        fprintf(TRACE,"Port  %s Open\n",ComPort);
        fflush(TRACE);
#endif
    }

    // Input/Output buffer size
    if(SetupComm(mComHnd, MAXBUF, MAXBUF)==0)
    {
        close_port();
        set_error(E_SETUP);
        return 0;
    }

    // Port settings are specified in a Data Communication Block (DCB).
    // The easiest way to initialize a DCB is to call GetCommState
    // to fill the default values, override the values that you
    // wannna change and then call SetCommState to set the values.

    if(GetCommState(mComHnd, &m_dcb)==0)
    {
        close_port();
        set_error(E_GETCOMM);
        return 0;
    }

    m_dcb.BaudRate = 9600;
    m_dcb.ByteSize = 8;
    m_dcb.Parity = NOPARITY;
    m_dcb.StopBits = ONESTOPBIT;
    m_dcb.fAbortOnError = TRUE;

    if(SetCommState(mComHnd, &m_dcb)==0)
    {
        close_port();
        set_error(E_SETCOMM);
        return 0;
    }

    // Optional Communication timeouts can be set similarly:

    if(GetCommTimeouts(mComHnd, &m_CommTimeouts)==0)
    {
        close_port();
        set_error(E_GETTIME);
        return 0;
    }

    m_CommTimeouts.ReadIntervalTimeout = 1; //50;
    m_CommTimeouts.ReadTotalTimeoutConstant = 0; //50;
    m_CommTimeouts.ReadTotalTimeoutMultiplier = 0; //10;
    m_CommTimeouts.WriteTotalTimeoutConstant = 50;
    m_CommTimeouts.WriteTotalTimeoutMultiplier = 10;

    if(SetCommTimeouts(mComHnd, &m_CommTimeouts)==0)
    {
        close_port();
        set_error(E_SETTIME);
        return 0;
    }

    // Purging comm port at start

    if(PurgeComm(mComHnd,PURGE_TXCLEAR | PURGE_RXCLEAR)==0)
    {
        close_port();
        set_error(E_PURGE);
        return 0;
    }

    return 1;
}


BOOLEAN close_port()
{
    if(CloseHandle(mComHnd)==0)
    {
        set_error(E_CLOSE);
        return 0;
    }
#ifdef TRACE_IO
    fprintf(TRACE,"Port closed\n---------------------------------\n");
    fflush(TRACE);
#endif
    return 1;
}


/////////////////////////////////////////////////////////////////////////////
// Serial Port IO Debugging Functions
/////////////////////////////////////////////////////////////////////////////

void show_in(UINT n)
{
#ifdef TRACE_IO
    UINT k;
    fprintf(TRACE,"IN : ");
    for(k=0; k<n; k++) fprintf(TRACE,"%02X ",mInBuffer[k]);
    fprintf(TRACE,"\n");//  getchar();
    fflush(TRACE);
#endif
}

void show_out(UINT n)
{
#ifdef TRACE_IO
    UINT k;
    fprintf(TRACE,"OUT: ");
    for(k=0; k<n; k++) fprintf(TRACE,"%02X ",mBuffer[k]);
    fprintf(TRACE,"\n");//  getchar();
    fflush(TRACE);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// Eliminates start/end and repeated DLE bytes and check CHECKSUM
/////////////////////////////////////////////////////////////////////////////
UINT strip_packet(UINT n, BYTE *ack)
{
    UINT k,cont,maxn;
    BYTE x,chk,chk_rec,id;

    chk_rec=mInBuffer[n-3];
    maxn=n-3;
    if(chk_rec==DLE) maxn--;

    chk=0;
    for(k=1,cont=0; k<maxn; k++)
    {
        x=mInBuffer[k];
        chk+=x;
        mInBuffer[cont++]=x;
        if(x==DLE) k++;
    }
    chk=-chk;

    id=mInBuffer[0];

#ifdef TRACE_IO
    fprintf(TRACE,"ID of packet %02X\n",id);
    fflush(TRACE);
#endif

    *ack = (chk == chk_rec);

    return cont;
}


/////////////////////////////////////////////////////////////////////////////
// WAIT UNTIL A CHAR (*x) IS READ FROM SERIAL PORT OR A TIMEOUT OCCURS
// RETURN 0 when TIMEOUT, 1 if OK
/////////////////////////////////////////////////////////////////////////////
BOOLEAN read_char(BYTE *x)
{
    ULONG nb;
    double CPMS=CLOCKS_PER_SEC/1000.0;
    clock_t start=clock();
    BOOLEAN res;

    do
    {
        if((clock()-start)/CPMS> mReadTimeOut)
        {
            set_error(E_TIMEOUT); //close_port();
            return 0;
#ifdef TRACE_IO
            fprintf(TRACE,"TIMEOUT waiting for a char\n");
            fflush(TRACE);
#endif
        }
        res=ReadFile(mComHnd, x, 1, &nb, NULL);
        if(res==0)
        {
            set_error(E_READ);
            close_port();
            return 0;
        }
    }
    while(nb==0);

    mCharsRead++;
    return (BOOLEAN)nb;
}


/////////////////////////////////////////////////////////////////////////////
// READ SERIAL PORT UNTIL THE END OF PACKET is DETECTED
// THE PACKET (ALREADY STRIPPED) IS FOUND in INBUFFER[]
// ERRORS: READ ERROR in PORT
//         PACKET LONGER THAN MAXBUF
// RETURN number of bytes in packet (OK) or 0 (ERROR)
/////////////////////////////////////////////////////////////////////////////
UINT read_packet()
{
    UINT buf_ptr = 0;
    BYTE c;

    while(1)
    {
        if(read_char(&c)==0) return 0;

        switch(mRxState)
        {
        case DAT_ST:
            if(c==DLE) mRxState=DLE_ST;
            else mInBuffer[buf_ptr++]=c;
            break;
        case DLE_ST:
            if(c==ETX)
            {
                mRxState=ETX_ST;
                return buf_ptr;
            }
            else
            {
                mInBuffer[buf_ptr++]=c;
                mRxState=DAT_ST;
            }
            break;
        case ETX_ST:
            if(c==DLE) mRxState=DLE_ST;
            break;
        default:
            if(buf_ptr==MAXBUF) return 0;
        }
    }
}


void check_packet(UINT n)
{
    UINT k;
    BYTE chksum=0;

    for(k=0; k<n; k++) chksum+=mInBuffer[k];

    mGoodChksum=(BOOLEAN)(chksum==0);

#ifdef TRACE_IO
    if(mGoodChksum) fprintf(TRACE,"Checksum ok.\n");
    else fprintf(TRACE,"Bad checksum.\n");
    fflush(TRACE);
#endif
}


/////////////////////////////////////////////////////////////////////////////
// TRY TO READ AN EXPECTED PACKET FROM SERIAL PORT.
// IF answer==1, SEND AN ACK/NACK packet.
// IN case of error, it tries to read it again until it is ACKed.
// RETURN 0 (if error) or the number of bytes in the packet received.
// When done, the stripped packet is placed in mInBuffer[]
/////////////////////////////////////////////////////////////////////////////
UINT read_data(BOOLEAN answer)
{
    UINT nb;
    BYTE id;

    nb=read_packet();
    if(nb==0)
    {
        set_error(E_EXPECTED_PACKET);
        return 0;
    }
    show_in(nb);

    check_packet(nb);

    if(answer)
    {
        id=mInBuffer[0];
        send_ack(id,mGoodChksum);
        if(mGoodChksum==0) nb=read_data(1);
    }

    return nb;
}


/////////////////////////////////////////////////////////////////////////////
// Prepares the data contained in data[] as a proper Garmin IO packet
// Return the length of the resulting packet
/////////////////////////////////////////////////////////////////////////////
UINT build_packet(BYTE *data)
{
    BYTE chk;
    UINT k,cont,L;

    L=(UINT)data[1];
    chk=0;
    cont=0;

// Init
    mBuffer[cont++]=DLE;

// Data
    for(k=0; k<L+2; k++)
    {
        chk=chk+data[k];
        mBuffer[cont++]=data[k];
        if(data[k]==DLE) mBuffer[cont++]=DLE;
    }

// Checksum
    chk=-chk;
    mBuffer[cont++]=chk;
    if(chk==DLE) mBuffer[cont++]=DLE;

// End
    mBuffer[cont++]=DLE;
    mBuffer[cont++]=ETX;

    return cont;
}


/////////////////////////////////////////////////////////////////////////////
// SEND an ACK (ok=1) or NACK (ok=0) to a received packet of identity id
// Returns 0 (in error) or  number of bytes written to the comm port
/////////////////////////////////////////////////////////////////////////////
UINT send_ack(BYTE id,BYTE ok)
{
    UINT nbytes,r;
    ULONG n;
    BYTE data[4];

    data[0]= (ok)? ACK:NAK;
    data[1]=0x02;
    data[2]=id;
    data[3]=0x00;

#ifdef TRACE_IO
    if(ok) fprintf(TRACE,"Received correct packet ID %02X -> ACK sent back ",id);
    else fprintf(TRACE,"Received bad packet ID %02X -> NAK sent back ",id);
    fflush(TRACE);
#endif

    nbytes=build_packet(data);

    r = WriteFile(mComHnd,mBuffer,nbytes,&n,NULL);
    if(r==0)
    {
        set_error(E_WRITE);
        close_port();
        return 0;
    }

    if((UINT)n<nbytes)
    {
        set_error(E_WRITE_SHORT);
        close_port();
        return 0;
    }
    show_out((UINT)n);

    return n;
}


/////////////////////////////////////////////////////////////////////////////
// Wait WAIT_FOR_ACK msec listening for an ACK/NCK packet
/////////////////////////////////////////////////////////////////////////////
UINT read_ack(BYTE id)
{
    UINT BytesRead;
    BYTE ack;

    pause(50);
    BytesRead=read_packet();
    if(BytesRead==0)
    {
        set_error(E_EXPECTED_ACK);
        return 0;
    }
    check_packet(BytesRead);
    show_in(BytesRead);

    ack=mInBuffer[0];

    return ack;
}



/////////////////////////////////////////////////////////////////////////////
// Receives a data packet, wrapps it according to the protocol, and
// checksums, etc, sends it, and wait for ACK. If not ACKed, it is sent again.
// Returns 0 (if error) or numberofbytes written to comm port .
/////////////////////////////////////////////////////////////////////////////
UINT send_packet(BYTE *data)
{
    UINT nbytes;
    int r;
    BYTE id;
    ULONG BytesWritten;

    id=data[0];

    nbytes=build_packet(data);

    r = WriteFile(mComHnd,mBuffer,nbytes,&BytesWritten,NULL);
    if(r==0)
    {
        set_error(E_WRITE);
        close_port();
        return 0;
    }
    if(BytesWritten<nbytes)
    {
        set_error(E_WRITE_SHORT);
        close_port();
        return 0;
    }
    show_out(BytesWritten);

    if(mNeedAck==0)
    {
        mNeedAck=1;
        return r;
    }

    r=read_ack(id);
    if(r==0) return(0);
    else
    {
        if(r==NAK)
        {
#ifdef TRACE_IO
            fprintf(TRACE,"Packet ID %02X NAK was returned. Sending again.\n",id);
            fflush(TRACE);
#endif
            r = WriteFile(mComHnd,mBuffer,nbytes,&BytesWritten,NULL);
            if(r==0)
            {
                set_error(E_WRITE);
                close_port();
                return 0;
            }
            if(BytesWritten<nbytes)
            {
                set_error(E_WRITE_SHORT);
                close_port();
                return
                    0;
            }
            show_out((UINT)BytesWritten);
        }
        else
        {
#ifdef TRACE_IO
            fprintf(TRACE,"Packet ID %02X ACK was returned.\n",id);
            fflush(TRACE);
#endif
        }
        return BytesWritten;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Set the error code so that we can know what went astray.
/////////////////////////////////////////////////////////////////////////////
void set_error(BYTE n)
{
    *mErrCodePtr |= ((1L)<<n);
#ifdef TRACE_IO
    fprintf(TRACE,"Error# %2d -> Error Code %ld\n",n,*mErrCodePtr);
    fflush(TRACE);
#endif
    return;
}



/////////////////////////////////////////////////////////////////////////////
// Get uint or ulong in a byte string
/////////////////////////////////////////////////////////////////////////////
UINT get_uint(BYTE* ptr)
{
    UINT x;

    x= (UINT)(*ptr++);
    x+=256*(UINT)(*ptr);

    return x;
}

ULONG get_long(BYTE* ptr)
{
    ULONG x;

    x=(UINT)(*ptr++);
    x+=(UINT)256*(UINT)(*ptr++);
    x+=(UINT)65536*(UINT)(*ptr++);
    x+=(UINT)16777216*(UINT)(*ptr++);

    return x;
}

/////////////////////////////////////////////////////////////////////////////
// GPS related functions
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Displays Product ID, software version, etc
/////////////////////////////////////////////////////////////////////////////

void check_port()
{

    if(open_port(mPort))
    {
        printf("Opening port %s\n",mPort);
        if(close_port()) printf("Closing port %s\n",mPort);
    }

}

void ident()
{
    UINT prod,ver;
    BYTE *bptr=DATA_PACKET;

    BYTE PROD_ID[2] = {0xFE, 0x00};   // Command

    if(open_port(mPort)==0) return;
    if(send_packet(PROD_ID)==0) return;
    if(read_data(1)==0) return;
    if(close_port()==0) return;

    prod=get_uint(bptr);
    bptr+=2;
    ver=get_uint(bptr);
    bptr+=2;
    printf("Product ID: %d, \"%s\", V%4.2f\n",prod,(char*)bptr,(float)ver/100.0);

    return;
}

int get_pos(int p)
{
    double lat,lon;
    BYTE SEND_POS[4]  = {0x0A, 0x02, 0x02, 0x00};

    if(open_port(mPort)==0) return 0;
    if(send_packet(SEND_POS)==0) return 0;
    if(read_data(1)==0) return 0;

    if(p && mVerbose)
    {
        memcpy(&lat,DATA_PACKET,sizeof(double));
        lat*=(180/M_PI);
        memcpy(&lon,DATA_PACKET+8,sizeof(double));
        lon*=(180/M_PI);
        printf("Position  : Lat %.5f  Long %.5f\n",lat,lon);
    }

    close_port();
    return 1;
}

double julday(int year,int month,int day, int hour, int min, double sec)
{
    double jd;

    if(month <= 2)
    {
        year-=1;
        month+=12;
    }
    jd = floor(365.25*year)+floor(30.6001*(month+1))+day+(hour+(min+sec/60.0)/60.0)/24+1720981.5;
    return jd;
}

double gps_time(double julday, int *week)
{
    double tow,days_gps;
    int wn;

    days_gps=(julday-2444244.5);
    wn = (int)floor(days_gps/7);

    tow = (days_gps- wn*7)*86400;

    tow=tow+17;  // Correct for the leap seconds as of writing this
    if(tow>=604800)
    {
        tow-=604800;
        wn++;
    }

    *week=wn;

    return tow;
}

int get_date(int p)
{
    typedef struct
    {
        BYTE month;
        BYTE day;
        UINT year;
        UINT hour;
        BYTE min;
        BYTE sec;
    } UTC;
    UTC utc;
    BYTE SEND_TIME[4] = {0x0A, 0x02, 0x05, 0x00};
    double jd,tow;
    int week;

    if(open_port(mPort)==0) return 0;
    if(send_packet(SEND_TIME)==0) return 0;
    if(read_data(1)==0) return 0;

    if(p && mVerbose)
    {
        utc.month=mInBuffer[2];
        utc.day=mInBuffer[3];
        utc.year=*((short int*)(mInBuffer+4));
        utc.hour=*((short int*)(mInBuffer+6));
        utc.min=mInBuffer[8];
        utc.sec=mInBuffer[9];

        jd=julday((int)utc.year,(int)utc.month,(int)utc.day,(int)utc.hour,(int)utc.min,(double)utc.sec);
        tow=gps_time(jd,&week);

        printf("UTC Date  : ");
        printf("%02u-%s-%4d ",utc.day, MONTH[utc.month-1],utc.year);
        printf("%02d:%02d:%02d\n",utc.hour,utc.min,utc.sec);
        printf("GPS Time  : ");
        printf("GPS Week %4d ToW %6.0f sec. Garmin Weekdays %d\n",week,tow,(week-521)*7);
    }

    close_port();
    return 1;
}

BOOLEAN async(int flag_async)
{
    BYTE orden[4]= {0x1C,0x02,0x00,0x00};

    orden[2]=(BYTE)(flag_async%256);
    orden[3]=(BYTE)(flag_async/256);


    if(flag_async)    //Enabling async events with mask FLAG_ASYNC
    {
        if(open_port(mPort)==0) return 0;
        if(send_packet(orden)==0) return 0;
        if(read_data(0)==0) return 0 ;
        if(read_data(0)==0) return 0 ;
    }
    else             // Disabling async events
    {
        mNeedAck=0;
        if(send_packet(orden)==0) return 0;
        pause(2000);
        if(PurgeComm(mComHnd,PURGE_TXCLEAR | PURGE_RXCLEAR)==0) return 0;
        if(close_port()==0) return 0;
    }

    return 1;
}


int clear_line()
{
    BYTE disable[4]= {0x1c, 0x02, 0x00, 0x00};

#ifdef TRACE_IO
    fprintf(TRACE,"Clearing line\n");
    fflush(TRACE);
#endif

    if(open_port(mPort)==0) return 0;

    mNeedAck=0;
    if(send_packet(disable)==0) return 0;

    pause(600);
    if(PurgeComm(mComHnd,PURGE_TXCLEAR | PURGE_RXCLEAR)==0) return 0;

    if(close_port()==0) return 0;

    return 1;
}

void write_packet(FILE *dest)
{
    fwrite(INI_PACKET,1,L_PACKET+2,dest);
    fflush(dest);
}

void log_packets_0x33(FILE *f_bin)
{
    clock_t start=clock();
    double dt = 0;
    const double MAX_WAIT = 20;  // Wait seconds for a 3D fix in msg 0x33
    UINT n, fix;
    ULONG rec = 0;
    ULONG ok = 0;

    if(mVerbose)
    {
        printf("----------------------------------------------------------------------------\n");
        printf("Waiting to verify 3D fix  (%.0f secs at most)\n",MAX_WAIT);
    }
    start=clock();
    if(async(0xffff)==0) return;    // Enable Async messages

    mReadTimeOut=3000;
    do
    {
        n=read_data(0);
        dt=((double)clock()-start)/CLOCKS_PER_SEC;
        if(n && mGoodChksum && (ID_PACKET==0x33))
        {
            rec++;
            fix=get_uint(DATA_PACKET+16);    //Get type of fix
            if(fix>=3)
            {
                write_packet(f_bin);
                ok++;
            }

            //if (fix>=3)  { fwrite(INI_PACKET,1,L_PACKET+2,f_bin); fflush(f_bin); ok++; }
            if(mVerbose)
            {
                printf("%02.0f secs: %3d 0x33 packets received. %d with a 3D fix.%c",dt,rec,ok,13);
            }
        }
    }
    while((dt<MAX_WAIT) && (ok<5));
    mReadTimeOut=100;

    async(0);   // Stop Async messages

    if(mVerbose)
    {
        printf("                                                             %c",13);
        printf("%02.0f secs: %2d packets with 0x33 ID received. %d with a 3D fix.\n",dt,rec,ok);
    }

    if(dt>=MAX_WAIT)
    {
        printf("----------------------------------------------------------------------------\n");
        printf("%.0f seconds waiting for 0x33 records with a 3D fix.\n",MAX_WAIT);
        printf("Could not verify 3D fix. The data quality may be poor.\n");
        printf("Let's give it a try anyway, but RINEX file processing may fail.\n");
    }
}


int test_async_basic(double T_LOG, FILE *f_bin)
{
    clock_t start=clock();
    double dt;
    UINT n;
    ULONG cont=0;
    int failed=0;
    BYTE start_async[4]= {0x1C,0x02,0xff,0xff};
    BYTE stop_async[4]= {0x1C,0x02,0x00,0x00};

    if(mVerbose)
        printf("STARTING BASIC ASYNC TEST---------------------------------------\n");

    start=clock();

    if(open_port(mPort)==0) return 0;
    if(send_packet(start_async)==0) return 0;


    mCharsRead=0;
    mReadTimeOut=1000;  //One second timeout
    do
    {
        n=read_data(0);
        dt=((double)clock()-start)/CLOCKS_PER_SEC;

        if(n==0) failed++;
        else
        {
            *mErrCodePtr=0;
            if(mGoodChksum)
            {
                write_packet(f_bin);
                cont++;
            }
        }

        if(mVerbose)
            printf("%6.1f secs left, %6d rcvd pkts, %2d failed reads. Current 0x%02x%c",T_LOG-dt,cont,failed,ID_PACKET,13);
    }
    while(dt<T_LOG && failed<MAX_FAILED);
    mReadTimeOut=100;

    if(mVerbose)
    {
        printf("                                                                           %c",13);
        printf("Comm Stats: %ld chars (%.1f Kbit/s): %d records, %d failed.\n",mCharsRead,(double)mCharsRead*10/(1024*dt),cont,failed);
        if(failed>=MAX_FAILED) printf("Too many failed packets. Closing connection now\n");
    }


    if(*mErrCodePtr==2560)   // Timeout
    {
        close_port();
        clear_line();
        return 0;
    }
    else    // Stop Async messages
    {
        mNeedAck=0;

        if(send_packet(stop_async)==0) return 0;
        pause(1000);
        if(PurgeComm(mComHnd,PURGE_TXCLEAR | PURGE_RXCLEAR)==0) return 0;
        if(close_port()==0) return 0;
    }

    if(mVerbose)
        printf("ASYNC TESTING FINISHED WITHOUT PROBLEMS------------------------\n");

    return 1;
}



int log_stream(double T_LOG, FILE *f_bin)
{
    BYTE buffer[256];
    int index;
    clock_t start;
    double dt;
    int timeouts=0;
    BYTE start_async[4]= {0x1C,0x02,0xff,0xff};
    BYTE stop_async[4]= {0x1C,0x02,0x00,0x00};

    if(mVerbose)
        printf("DUMPING SERIAL PORT STREAM DIRECTLY TO FILE--------------------\n");

    start=clock();

    if(open_port(mPort)==0) return 0;
    mNeedAck=0;
    if(send_packet(start_async)==0) return 0;

    index=0;
    timeouts=0;
    mReadTimeOut=1000;  //One second timeout
    mCharsRead=0;
    do
    {
        if(index==256)
        {
            fwrite(buffer,1,256,f_bin);
            index=0;
        }

        if(read_char(buffer+index)==0) timeouts++;
        else index++;

        dt=((double)clock()-start)/CLOCKS_PER_SEC;

        if(mVerbose)
        {
            printf("%6.1f secs left: %8d rcvd bytes. ",T_LOG-dt,mCharsRead);
            printf("%4d timeouts.%c",timeouts,13);
        }
    }
    while(dt<T_LOG && timeouts<MAX_FAILED);
    mReadTimeOut=100;

    // Dump remaining bytes in buffer
    if(index) fwrite(buffer,1,index,f_bin);

    if(mVerbose)
    {
        printf("                                                                           %c",13);
        printf("Comm Stats: %ld chars ",mCharsRead);
        printf("(%.1f Kbit/s). ",(double)mCharsRead*8/(1024*dt));
        printf("%d timeouts.\n",timeouts);
        if(timeouts>=MAX_FAILED)
            printf("Too many timeouts. Closing connection now\n");
    }

    if(*mErrCodePtr==2560)   // Timeout
    {
        close_port();
        clear_line();
        return 0;
    }
    else    // Stop Async messages
    {
        mNeedAck=0;

        if(send_packet(stop_async)==0) return 0;
        pause(1000);
        if(PurgeComm(mComHnd,PURGE_TXCLEAR | PURGE_RXCLEAR)==0) return 0;
        if(close_port()==0) return 0;
    }

    if(mVerbose)
        printf("DUMPING OF SERIAL PORT STREAM FINISHED WITHOUT PROBLEMS---\n");

    return 1;
}

void log_packets_async(int flag, double T_LOG, FILE *f_bin)
{
    clock_t start=clock();
    double dt;
    UINT n;
    ULONG cont=0;
    int failed=0;

    if(mVerbose)
        printf("----------------------------------------------------------------------------\n");

    // This text will quickly be overwritten when GPS receives first async message.
    printf("Specified async messages not read. Is GPS compatible?%c", 13);

    start=clock();
    if(async(flag)==0) return;    // Enable Async messages

    mCharsRead=0;
    mReadTimeOut=1000;            // One second timeout

    do
    {
        n=read_data(0);
        dt=((double)clock()-start)/CLOCKS_PER_SEC;

        if(n==0) failed++;
        else
        {
            *mErrCodePtr=0;
            if(mGoodChksum)
            {
                write_packet(f_bin);
                cont++;
            }
        }

        if(mVerbose)
            printf("%6.1f secs left, %6d rcvd pkts, %2d failed reads. Current 0x%02x%c",T_LOG-dt,cont,failed,ID_PACKET,13);

    }
    while(dt<T_LOG && failed<MAX_FAILED);
    mReadTimeOut=100;

    if(mVerbose)
    {
        printf("                                                                           %c",13);
        printf("Comm Stats: %ld chars (%.1f Kbit/s): %d records, %d failed.\n",mCharsRead,(double)mCharsRead*10/(1024*dt),cont,failed);
        if(failed>=MAX_FAILED) printf("Too many failed packets. Closing connection now\n");
    }


    if(*mErrCodePtr==2560)   // Timeout
    {
        close_port();
        clear_line();
    }
    else async(0);   // Stop Async messages


    if(mVerbose)
        printf("----------------------------------------------------------------------------\n");

}


ULONG log_packets_request(UINT flag_request, FILE* dest)
{
    ULONG nrecords,k;
    BYTE orden[4]= {0x0A,0x02,0x07,0x00};

    orden[2]=(BYTE)(flag_request%256);
    orden[3]=(BYTE)(flag_request/256);


    if(open_port(mPort)==0) return 0;
    if(send_packet(orden)==0) return 0;
    if(read_data(1)==0) return 0;

    if(mVerbose)
        printf("----------------------------------------------------------------------------\n");

    write_packet(dest);

    if(ID_PACKET!=0x1B)
    {
        close_port();
        if(mVerbose)
        {
            printf("One record received\n");
            printf("----------------------------------------------------------------------------\n");
        }
        return 1;
    }

    nrecords=get_uint(DATA_PACKET);

    pause(80);
    k=0;
    do
    {
        pause(20);
        if(read_data(1)==0) return 0;
        if(mVerbose) printf("Records   : %3d expected. %3d rcvd%c",nrecords,k,13);
        write_packet(dest);
        k++;
    }
    while(ID_PACKET!=EOD);
    k--;

    if(mVerbose)
    {
        printf("Records   : %3d expected. %3d actually received\n",nrecords,k);
        printf("----------------------------------------------------------------------------\n");
    }

    close_port();

    return k;
}


/////////////////////////////////////////////////////////////////////////////
// Display Error Msg
/////////////////////////////////////////////////////////////////////////////
int check_bit(ULONG x, BYTE pos)
{
    ULONG mask = 1 << pos;
    return (x & mask);
}

void show_err_msg(ULONG n)
{
    if(n==0)
    {
        if(mVerbose==2) printf("No comm errors during process.\n");
        return;
    }

    printf("Serial Comm Error %ld -> 0x%08lx: ",n,n);
    if(check_bit(n,0)) printf("Can't open serial port.\nPossibly being used by another application.\n");
    if(check_bit(n,1)) printf("Error in SETUP COMM port.\n");
    if(check_bit(n,2)) printf("Error in GetComm.\n");
    if(check_bit(n,3)) printf("Error in SetComm.\n");
    if(check_bit(n,4)) printf("Error in GetTimeOuts.\n");
    if(check_bit(n,5)) printf("Error in SetTimeOuts.\n");
    if(check_bit(n,6)) printf("Error purging COM port.\n");
    if(check_bit(n,7)) printf("Error closing COM port: maybe it is already closed.\n");
    if(check_bit(n,8)) printf("Hardware Error reading COMM port.\n");
    if(check_bit(n,9) && check_bit(n,10))
    {
        printf("TIMEOUT waiting for ACK.\n");
        printf("GPS doesn't answer in %s: Is it on and in GRMN/GRMN mode?\n",mPort);
    }
    if(check_bit(n,9) && check_bit(n,11))
    {
        printf("TIMEOUT waiting for a PACKET.\n");
        printf("GPS doesn't answer in %s: Is it on and in GRMN/GRMN mode?\n",mPort);
    }
    if(check_bit(n,16)) printf("Hardware Error when sending data to COM port.\n");
    if(check_bit(n,17)) printf("Short Write in COM port.\n");
    if(check_bit(n,31)) printf("Command not recognized; You should not be seeing this.\n");
}


/////////////////////////////////////////////////////////////////////////////
// Set default values and parse user arguments
/////////////////////////////////////////////////////////////////////////////

void print_help()
{
    char help[2048];

    sprintf(help,
            "----------------------------------------------------------------------------\n"\
            "* Async Software logs raw GPS measurement data from some Garmin handhelds  *\n"\
            "* Version 1.23, Copyright 2000-2002 Antonio Tabernero Galan                *\n"\
            "* Version %4.2f, Copyright 2016 Norm Moulton                                *\n"\
            "----------------------------------------------------------------------------\n"\
            "Usage:\n"\
            "  async command [options]\n\n",VERSION);

    strcat(help,
           "------------------- ASYNC COMMANDS -----------------------------------------\n\n"\
           "  -h        : Shows this help text.\n"\
           "  -c        : Checks com port availability.\n"\
           "  -i        : Gets the GPS receiver identification string.\n"\
           "  -rinex    : Logs records needed to compute pseudorange and carrier phase.\n\n");

    strcat(help,
           "-------------------- ASYNC OPTIONS   ---------------------------------------\n\n"\
           "  -p port     : Selects serial comm port, (eg. COM1).\n"\
           "  -t ttt      : Sets logging time to ttt seconds. Default is 30 sec.\n"\
           "  -o filename : Specifies output filename.\n"\
           "                By default the output goes to week_second.g12\n\n"\
           "----------------------------------------------------------------------------\n");

// Experimental "undocumented" options:
//
// async -a 0xnnnn : Enable async events with hex mask nnnn.
// async -r 0xnnnn : Sends hex request type nnnn.
// async +doppler  : Logs Doppler shift data in addition to pseudorange
//                   and phase. Doppler data is not used in the PPP
//                   solution and this option sometimes causes data errors
//                   on start-up, and missed observations of other data.

    printf("%s",help);
    exit(0);
}

void parse_args(int argc,char** argv,BYTE *command,double* log_time,unsigned int* flag,char *fich)
{
    int k=1;
    ULONG temp;
    time_t tt;
    struct tm *gmt;
    ULONG  gps_time;

    if(argc==1) print_help();

    // Default values

    strcpy(mPort,DEF_PORT);   //printf("%s %s\n",mPort,DEF_PORT); getchar();
    *command=DEF_COMMAND;
    *log_time=DEF_LOG_TIME;
    *flag=DEF_FLAG;

    mReadTimeOut=100;
    mVerbose=1;

    time(&tt);
    gmt=gmtime(&tt);
    gps_time=(gmt->tm_sec+60*(gmt->tm_min+60*(gmt->tm_hour+24*gmt->tm_wday)));
    sprintf(fich,"%06u.g12",gps_time);

    // User provided arguments
    while(k<argc)
    {
        if(strcmp(argv[k],"-p")==0)
        {
            strcpy(mPort,argv[k+1]);
            k+=2;
        }
        else if(strcmp(argv[k],"-h")==0)
        {
            print_help();
            exit(0);
        }
        else if(strcmp(argv[k],"-i")==0)
        {
            *command=IDENT;
            k++;
        }
        else if(strcmp(argv[k],"-rinex")==0)
        {
            *command=RINEX;
            k++;
        }
        else if(strcmp(argv[k],"+doppler")==0)
        {
            *command=DOPPLER;
            k++;
        }
        else if(strcmp(argv[k],"-c")==0)
        {
            *command=CHECK;
            k++;
        }
        else if(strcmp(argv[k],"-stdout")==0)
        {
            mIsStdOut=1;
            mVerbose=0;
            k++;
        }
        else if(strcmp(argv[k],"-all")==0)
        {
            *command=LOG_ALL;
            *flag=0xffff;
            strcpy(fich,"test_all.bin");
            k++;
        }
        else if(strcmp(argv[k],"-test")==0)
        {
            *command=TEST;
            *flag=0xffff;
            strcpy(fich,"test.bin");
            k++;
        }
        else if(strcmp(argv[k],"-a")==0)
        {
            *command=ASYNC;
            temp=strtoul(argv[k+1],(char**)NULL,16);
            *flag =(UINT)temp;
            k+=2;
        }
        else if(strcmp(argv[k],"-r")==0)
        {
            *command=REQST;
            temp=strtoul(argv[k+1],(char**)NULL,16);
            *flag= (UINT)temp;
            k+=2;
        }
        else if(strcmp(argv[k],"-q")==0)
        {
            mVerbose=0;
            k++;
        }
        else if(strcmp(argv[k],"-V")==0)
        {
            mVerbose=2;
            k++;
        }
        else if(strcmp(argv[k],"-t")==0)
        {
            *log_time=atoi(argv[k+1]);
            k+=2;
        }
        else if(strcmp(argv[k],"-o")==0)
        {
            strcpy(fich,argv[k+1]);
            k+=2;
        }
        else
        {
            printf("Unknown Option %s\n",argv[k]);
            k++;
        }
    }

    if(mIsStdOut==0)
    {
        printf("----------------------------------------------------------------------------\n"\
               "* Async Software logs raw GPS measurement data from some Garmin handhelds  *\n"\
               "* Version 1.23, Copyright 2000-2002 Antonio Tabernero Galan                *\n"\
               "* Version %4.2f, Copyright 2016 Norm Moulton                                *\n"\
               "----------------------------------------------------------------------------\n"\
               "", VERSION);
    }
    // Display info
    if(mVerbose==2)  printf("GPS Time %6u. UTC %s",gps_time,asctime(gmt));

    if(mVerbose)
    {
        printf("Serial port: %s. Command: ",mPort);
        switch(*command)
        {
        case IDENT:
            printf("GPS Identification.\n");
            break;

        case CHECK:
            printf("Check Comm Port\n");
            break;

        case LOG_ALL:
            printf("Binary dump from serial port.\n");
            printf("Log-time %.0f sec. File: %s\n",*log_time,fich);
            break;

        case TEST:
            printf("Test async events (mask 0xffff)\nLog-time %.0f sec to file %s\n",*log_time,fich);
            break;

        case ASYNC:
            printf("Log async events (mask 0x%04x)\nLog-time %.0f sec. ",*flag,*log_time);
            printf("Output binary file: %s\n",fich);
            break;

        case REQST:
            printf("Request records (type 0x%04x).\n",*flag);
            printf("Output binary file: %s\n",fich);
            break;

        case RINEX:
            printf("Log pseudorange and phase.\nLog-time %.0f sec. ",*log_time);
            printf("Output binary file: %s\n",fich);
            break;

        case DOPPLER:
            printf("Log pseudorange, phase, and Doppler.\nLog-time %.0f sec. ",*log_time);
            printf("Output binary file: %s\n",fich);
            break;

        default:
            break;
        }
        printf("----------------------------------------------------------------------------\n");
    }

}

void test_eph()
{
    BYTE eph[6]= {0x0d,0x04,0x02,0x0c,0x00,0x00};
    UINT n;

    if(open_port(mPort)==0) return;
    if(send_packet(eph)==0) return;

    n=read_data(0);
    while(n)
    {
        printf("%d bytes read\n",n);
        n=read_data(0);
    }

    if(close_port()==0) return;
}


int main(int argc, char **argv)
{
    char fich[50];
    BYTE command;
    double log_time;
    unsigned int flag;
    ULONG err;
    FILE *fd;


#ifdef TRACE_IO
    TRACE=fopen(TRACE_FILE,"wt");
#endif


    parse_args(argc,argv,&command,&log_time,&flag,fich);

    // Set Initial error code to 0
    err=0;
    mErrCodePtr=(ULONG*)&err;


    if(clear_line()==0)
    {
        show_err_msg(err);
        exit(0);
    }

    // Print position & date
    if((command!=CHECK) && (command!=TEST) && (command!=LOG_ALL))
    {
        get_pos(1);
        get_date(1);
    }


    if((command!=IDENT) && (command!=CHECK))
    {
        fd = (mIsStdOut==0)? fopen(fich,"wb"): stdout;
    }

    switch(command)
    {
    case IDENT:
        ident();
        break;

    case CHECK:
        check_port();
        break;

    case TEST:
        if(test_async_basic(log_time,fd)==0)
            printf("THERE WAS SOME TROUBLE WHILE RUNNING THE TEST---------------\n");
        break;

    case LOG_ALL:
        log_stream(log_time,fd);
        break;

    case ASYNC:
        if(flag)
        {
            ident();
            write_packet(fd); // fwrite(INI_PACKET,1,L_PACKET+2,fd);
            get_pos(0);
            write_packet(fd);  //fwrite(INI_PACKET,1,L_PACKET+2,fd);
            get_date(0);
            write_packet(fd);  //fwrite(INI_PACKET,1,L_PACKET+2,fd);
            log_packets_async(flag,log_time,fd);
        }
        else async(flag);
        break;

    case REQST:
        log_packets_request(flag,fd);
        break;

    case RINEX:
        ident();
        write_packet(fd); // fwrite(INI_PACKET,1,L_PACKET+2,fd);
        get_pos(0);
        write_packet(fd); //fwrite(INI_PACKET,1,L_PACKET+2,fd);
        get_date(0);
        write_packet(fd); //fwrite(INI_PACKET,1,L_PACKET+2,fd);
        log_packets_0x33(fd);
        log_packets_async(0x0020,log_time,fd);
        break;

    case DOPPLER:
        ident();
        write_packet(fd); //fwrite(INI_PACKET,1,L_PACKET+2,fd);
        get_pos(0);
        write_packet(fd); //fwrite(INI_PACKET,1,L_PACKET+2,fd);
        get_date(0);
        write_packet(fd); //fwrite(INI_PACKET,1,L_PACKET+2,fd);
        log_packets_0x33(fd);
        log_packets_async(0x0028,log_time,fd);
        break;

    default:
        set_error(E_OPTION);
        printf("Unknown command option");
        break;
    }

    if((command!=IDENT) && (command!=CHECK)) if(mIsStdOut==0) fclose(fd);


    show_err_msg(err);

#ifdef TRACE_IO
    fclose(TRACE);
#endif

    return 1;
}



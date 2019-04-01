#include <stdint.h>
#include <math.h>
#include <string.h>
#include "xio.h"

const char* product_name_string   = "XIO";
const int   audio_sample_rate     = 48000;
const int   flexfx_unit_number    = 0;
const int   audio_clock_mode      = 1; // 0=internal/master,1=external/master,2=slave
const int   usb_output_chan_count = 2; // 2 USB audio class 2.0 output channels
const int   usb_input_chan_count  = 2; // 2 USB audio class 2.0 input channels
const int   i2s_channel_count     = 2; // Channels per SDIN/SDOUT wire (2,4,or 8)
const int   i2s_sync_word[8]      = { 0xFFFFFFFF,0x00000000,0,0,0,0,0,0 };

// FQ converts Q28 fixed-point to floating point, QF converts floating-point to Q28

#define QQ 28
#define FQ(hh) (((hh)<0.0)?((int)((double)(1u<<QQ)*(hh)-0.5)):((int)(((double)(1u<<QQ)-1)*(hh)+0.5)))
#define QF(xx) (((int)(xx)<0)?((double)(int)(xx))/(1u<<QQ):((double)(xx))/((1u<<QQ)-1))

// MAC performs 32x32 multiply and 64-bit accumulation, SAT saturates a 64-bit result, EXT converts
// a 64-bit result to a 32-bit value, LDn/STn loads/stores two 32-values from/to 64-bit aligned
// 32-bit data arrays at address PP[2*n]. All 32-bit fixed-point values are Q28 fixed-point values.
//
// AH (high) and AL (low) form the 64-bit signed accumulator
// XX, YY, and AA are 32-bit QQQ fixed point values
// PP is a ** 64-bit aligned **  pointer to two 32-bit QQQ values

#define DSP_LD00( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 0]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD01( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 1]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD02( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 2]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD03( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 3]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD04( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 4]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD05( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 5]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD06( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 6]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD07( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 7]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD08( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 8]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD09( pp, xx, yy )    asm volatile("ldd %0,%1,%2[ 9]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD10( pp, xx, yy )    asm volatile("ldd %0,%1,%2[10]":"=r"(xx),"=r"(yy):"r"(pp));
#define DSP_LD11( pp, xx, yy )    asm volatile("ldd %0,%1,%2[11]":"=r"(xx),"=r"(yy):"r"(pp));

#define DSP_ST00( pp, xx, yy )    asm volatile("std %0,%1,%2[ 0]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST01( pp, xx, yy )    asm volatile("std %0,%1,%2[ 1]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST02( pp, xx, yy )    asm volatile("std %0,%1,%2[ 2]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST03( pp, xx, yy )    asm volatile("std %0,%1,%2[ 3]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST04( pp, xx, yy )    asm volatile("std %0,%1,%2[ 4]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST05( pp, xx, yy )    asm volatile("std %0,%1,%2[ 5]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST06( pp, xx, yy )    asm volatile("std %0,%1,%2[ 6]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST07( pp, xx, yy )    asm volatile("std %0,%1,%2[ 7]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST08( pp, xx, yy )    asm volatile("std %0,%1,%2[ 8]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST09( pp, xx, yy )    asm volatile("std %0,%1,%2[ 9]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST10( pp, xx, yy )    asm volatile("std %0,%1,%2[10]"::"r"(xx), "r"(yy),"r"(pp));
#define DSP_ST11( pp, xx, yy )    asm volatile("std %0,%1,%2[11]"::"r"(xx), "r"(yy),"r"(pp));

#define DSP_MUL( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(0),"1"(0) );
#define DSP_MAC( ah, al, xx, yy ) asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(xx),"r"(yy),"0"(ah),"1"(al) );
#define DSP_EXT( ah, al, xx, qq ) asm volatile("lextract %0,%1,%2,%3,32":"=r"(xx):"r"(ah),"r"(al),"r"(QQ+qq));
#define DSP_SAT( ah, al )         asm volatile("lsats %0,%1,%2":"=r"(ah),"=r"(al):"r"(QQ),"0"(ah),"1"(al));

#define DSP_DIV( qq,rr,ah,al,xx ) asm volatile("ldivu %0,%1,%2,%3,%4":"=r"(qq):"r"(rr),"r"(ah),"r"(al),"r"(xx));
#define DSP_CRC( nxt,prv,seed,poly ) asm volatile("crc32 %0,%2,%3":"=r"(nxt):"0"(prv),"r"(seed),"r"(poly));

inline int dsp_multiply( int xx, int yy ) // RR = XX * YY
{
    int ah; unsigned al; DSP_MUL(ah,al,xx,yy); DSP_EXT(ah,al,xx,0); return xx;
}
inline int _multacc( int xx, int yy, int ah, unsigned al ) // RR = XX * YY + ZZ
{
    DSP_MAC(ah,al,xx,yy); DSP_EXT(ah,al,xx,0); return xx;
}
inline int dsp_extract( int ah, int al ) // RR = AH:AL >> (64-QQ)
{
    DSP_EXT( ah, al, ah,0 ); return ah;
}

int dsp_blend( int dry, int wet, int blend ) // 0 (100% dry) <= MM <= 1 (100% wet)
{
    int ah = 0; unsigned al = 1<<(QQ-1);
    asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(dry),"r"(FQ(1.0)-blend),"0"(ah),"1"(al) );
    asm volatile("maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(wet),"r"(blend),"0"(ah),"1"(al) );
    asm volatile("lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ));
    return ah;
}

inline int dsp_biquad( int xx, int* cc, int* ss )
{
    unsigned al; int ah, b0,b1,b2,a1,a2, x1,x2,y1,y2, tmp;
    DSP_LD00( cc, b1, b0 );
    DSP_MUL( ah, al, xx, b0 ); DSP_LD00( ss, x2, x1 ); DSP_ST00( ss, x1, xx );
    DSP_MAC( ah, al, x1, b1 ); DSP_LD01( cc, a1, b2 );
    DSP_MAC( ah, al, x2, b2 ); DSP_LD01( ss, y2, y1 );
    DSP_MAC( ah, al, y1, a1 ); DSP_LD02( cc, tmp, a2 );
    DSP_MAC( ah, al, y2, a2 ); DSP_EXT( ah, al, ah,0 ); DSP_ST01( ss, y1, ah );
    return ah;
}

inline int dsp_dcblock( int xx, int cc, int* ss )
{
    int ah; unsigned al;
    asm volatile( "maccs %0,%1,%2,%3":"=r"(ah),"=r"(al):"r"(cc),"r"(ss[1]),"0"(0),"1"(1<<(QQ-1)) );
    asm volatile( "lextract %0,%1,%2,%3,32":"=r"(ah):"r"(ah),"r"(al),"r"(QQ) );
    ss[1] = xx - ss[0] + ah; ss[0] = xx; xx = ss[1];
    return xx;
};

#define dsp_power24( ah, al, cc, ss ) \
{ \
    int c0, c1, s0, s1; \
    DSP_LD00( cc, c1, c0 ); DSP_LD00( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD01( cc, c1, c0 ); DSP_LD01( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD02( cc, c1, c0 ); DSP_LD02( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD03( cc, c1, c0 ); DSP_LD03( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD04( cc, c1, c0 ); DSP_LD04( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD05( cc, c1, c0 ); DSP_LD05( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD06( cc, c1, c0 ); DSP_LD06( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD07( cc, c1, c0 ); DSP_LD07( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD08( cc, c1, c0 ); DSP_LD08( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD09( cc, c1, c0 ); DSP_LD09( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD10( cc, c1, c0 ); DSP_LD10( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD11( cc, c1, c0 ); DSP_LD11( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
}

#define dsp_convolve24( ah, al, cc, ss, xx ) \
{ \
    s0 = xx; \
    DSP_LD00( cc, b1, b0 ); DSP_LD00( ss, s2, s1 ); DSP_ST00( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD01( cc, b1, b0 ); DSP_LD01( ss, s0, s3 ); DSP_ST01( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    DSP_LD02( cc, b1, b0 ); DSP_LD02( ss, s2, s1 ); DSP_ST02( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD03( cc, b1, b0 ); DSP_LD03( ss, s0, s3 ); DSP_ST03( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    DSP_LD04( cc, b1, b0 ); DSP_LD04( ss, s2, s1 ); DSP_ST04( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD05( cc, b1, b0 ); DSP_LD05( ss, s0, s3 ); DSP_ST05( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    DSP_LD06( cc, b1, b0 ); DSP_LD06( ss, s2, s1 ); DSP_ST06( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD07( cc, b1, b0 ); DSP_LD07( ss, s0, s3 ); DSP_ST07( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    DSP_LD08( cc, b1, b0 ); DSP_LD08( ss, s2, s1 ); DSP_ST08( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD09( cc, b1, b0 ); DSP_LD09( ss, s0, s3 ); DSP_ST09( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    DSP_LD10( cc, b1, b0 ); DSP_LD10( ss, s2, s1 ); DSP_ST10( ss, s1, s0 ); \
    DSP_MAC( ah, al, b0, s0 ); DSP_MAC( ah, al, b1, s1 ); \
    DSP_LD11( cc, b1, b0 ); DSP_LD11( ss, s0, s3 ); DSP_ST11( ss, s3, s2 ); \
    DSP_MAC( ah, al, b0, s2 ); DSP_MAC( ah, al, b1, s3 ); \
    xx = s0; \
}

int dsp_fir24( int xx, const int* cc, int* ss )
{
    int c0, c1, s0 = xx, s1, s2, s3, ah = 0; unsigned al = 1<<(QQ-1);

    DSP_LD00( cc, c1, c0 ); DSP_LD00( ss, s2, s1 ); DSP_ST00( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD01( cc, c1, c0 ); DSP_LD01( ss, s0, s3 ); DSP_ST01( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 ); \
    DSP_LD02( cc, c1, c0 ); DSP_LD02( ss, s2, s1 ); DSP_ST02( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD03( cc, c1, c0 ); DSP_LD03( ss, s0, s3 ); DSP_ST03( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 ); \
    DSP_LD04( cc, c1, c0 ); DSP_LD04( ss, s2, s1 ); DSP_ST04( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD05( cc, c1, c0 ); DSP_LD05( ss, s0, s3 ); DSP_ST05( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 ); \
    DSP_LD06( cc, c1, c0 ); DSP_LD06( ss, s2, s1 ); DSP_ST06( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD07( cc, c1, c0 ); DSP_LD07( ss, s0, s3 ); DSP_ST07( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 ); \
    DSP_LD08( cc, c1, c0 ); DSP_LD08( ss, s2, s1 ); DSP_ST08( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD09( cc, c1, c0 ); DSP_LD09( ss, s0, s3 ); DSP_ST09( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 ); \
    DSP_LD10( cc, c1, c0 ); DSP_LD10( ss, s2, s1 ); DSP_ST10( ss, s1, s0 ); \
    DSP_MAC( ah, al, c0, s0 ); DSP_MAC( ah, al, c1, s1 ); \
    DSP_LD11( cc, c1, c0 ); DSP_LD11( ss, s0, s3 ); DSP_ST11( ss, s3, s2 ); \
    DSP_MAC( ah, al, c0, s2 ); DSP_MAC( ah, al, c1, s3 );
   
    DSP_EXT( ah, al, xx,0 ); return xx;
}

int lagrange_interp( int ff, int y1, int y2, int y3 )
{
    unsigned al; int z1,z2,z3,ah, x0=ff, x1=ff-FQ(1.0), x2=ff-FQ(2.0);
    DSP_MUL( ah, al, x1, x2 ); DSP_EXT( ah, al, z1,1 );
    DSP_MUL( ah, al, x0,-x2 ); DSP_EXT( ah, al, z2,0 );
    DSP_MUL( ah, al, x0, x1 ); DSP_EXT( ah, al, z3,1 );
    DSP_MUL( ah, al, y1, z1 );
    DSP_MAC( ah, al, y2, z2 );
    DSP_MAC( ah, al, y3, z3 ); DSP_EXT( ah, al, ah,0 );
    return ah;
}

int dsp_negexp( int xx )
{
    static int lut[4][64] =
    {
      { FQ(1.000000000),FQ(0.882496903),FQ(0.778800783),FQ(0.687289279),FQ(0.606530660),FQ(0.535261429),FQ(0.472366553),FQ(0.416862020),
        FQ(0.367879441),FQ(0.324652467),FQ(0.286504797),FQ(0.252839596),FQ(0.223130160),FQ(0.196911675),FQ(0.173773943),FQ(0.153354967),
        FQ(0.135335283),FQ(0.119432968),FQ(0.105399225),FQ(0.093014489),FQ(0.082084999),FQ(0.072439757),FQ(0.063927861),FQ(0.056416140),
        FQ(0.049787068),FQ(0.043936934),FQ(0.038774208),FQ(0.034218118),FQ(0.030197383),FQ(0.026649097),FQ(0.023517746),FQ(0.020754338),
        FQ(0.018315639),FQ(0.016163495),FQ(0.014264234),FQ(0.012588142),FQ(0.011108997),FQ(0.009803655),FQ(0.008651695),FQ(0.007635094),
        FQ(0.006737947),FQ(0.005946217),FQ(0.005247518),FQ(0.004630919),FQ(0.004086771),FQ(0.003606563),FQ(0.003182781),FQ(0.002808794),
        FQ(0.002478752),FQ(0.002187491),FQ(0.001930454),FQ(0.001703620),FQ(0.001503439),FQ(0.001326780),FQ(0.001170880),FQ(0.001033298),
        FQ(0.000911882),FQ(0.000804733),FQ(0.000710174),FQ(0.000626727),FQ(0.000553084),FQ(0.000488095),FQ(0.000430743),FQ(0.000380129) },
      { FQ(1.000000000),FQ(0.998048781),FQ(0.996101369),FQ(0.994157758),FQ(0.992217938),FQ(0.990281904),FQ(0.988349647),FQ(0.986421161),
        FQ(0.984496437),FQ(0.982575469),FQ(0.980658249),FQ(0.978744770),FQ(0.976835025),FQ(0.974929006),FQ(0.973026706),FQ(0.971128118),
        FQ(0.969233234),FQ(0.967342048),FQ(0.965454552),FQ(0.963570739),FQ(0.961690602),FQ(0.959814133),FQ(0.957941325),FQ(0.956072172),
        FQ(0.954206666),FQ(0.952344800),FQ(0.950486567),FQ(0.948631959),FQ(0.946780971),FQ(0.944933594),FQ(0.943089822),FQ(0.941249647),
        FQ(0.939413063),FQ(0.937580062),FQ(0.935750638),FQ(0.933924784),FQ(0.932102492),FQ(0.930283756),FQ(0.928468569),FQ(0.926656924),
        FQ(0.924848813),FQ(0.923044231),FQ(0.921243169),FQ(0.919445622),FQ(0.917651583),FQ(0.915861044),FQ(0.914073998),FQ(0.912290440),
        FQ(0.910510361),FQ(0.908733756),FQ(0.906960618),FQ(0.905190939),FQ(0.903424714),FQ(0.901661934),FQ(0.899902594),FQ(0.898146687),
        FQ(0.896394207),FQ(0.894645145),FQ(0.892899497),FQ(0.891157254),FQ(0.889418412),FQ(0.887682962),FQ(0.885950898),FQ(0.884222214) },
      { FQ(1.000000000),FQ(0.999969483),FQ(0.999938967),FQ(0.999908451),FQ(0.999877937),FQ(0.999847424),FQ(0.999816911),FQ(0.999786400),
        FQ(0.999755889),FQ(0.999725380),FQ(0.999694871),FQ(0.999664363),FQ(0.999633856),FQ(0.999603350),FQ(0.999572845),FQ(0.999542341),
        FQ(0.999511838),FQ(0.999481336),FQ(0.999450834),FQ(0.999420334),FQ(0.999389835),FQ(0.999359336),FQ(0.999328839),FQ(0.999298342),
        FQ(0.999267846),FQ(0.999237352),FQ(0.999206858),FQ(0.999176365),FQ(0.999145873),FQ(0.999115382),FQ(0.999084892),FQ(0.999054402),
        FQ(0.999023914),FQ(0.998993427),FQ(0.998962940),FQ(0.998932455),FQ(0.998901970),FQ(0.998871487),FQ(0.998841004),FQ(0.998810522),
        FQ(0.998780042),FQ(0.998749562),FQ(0.998719083),FQ(0.998688605),FQ(0.998658128),FQ(0.998627652),FQ(0.998597176),FQ(0.998566702),
        FQ(0.998536229),FQ(0.998505756),FQ(0.998475285),FQ(0.998444814),FQ(0.998414344),FQ(0.998383876),FQ(0.998353408),FQ(0.998322941),
        FQ(0.998292475),FQ(0.998262010),FQ(0.998231546),FQ(0.998201083),FQ(0.998170621),FQ(0.998140159),FQ(0.998109699),FQ(0.998079240) },
      { FQ(1.000000000),FQ(0.999999523),FQ(0.999999046),FQ(0.999998569),FQ(0.999998093),FQ(0.999997616),FQ(0.999997139),FQ(0.999996662),
        FQ(0.999996185),FQ(0.999995708),FQ(0.999995232),FQ(0.999994755),FQ(0.999994278),FQ(0.999993801),FQ(0.999993324),FQ(0.999992847),
        FQ(0.999992371),FQ(0.999991894),FQ(0.999991417),FQ(0.999990940),FQ(0.999990463),FQ(0.999989986),FQ(0.999989510),FQ(0.999989033),
        FQ(0.999988556),FQ(0.999988079),FQ(0.999987602),FQ(0.999987125),FQ(0.999986649),FQ(0.999986172),FQ(0.999985695),FQ(0.999985218),
        FQ(0.999984741),FQ(0.999984264),FQ(0.999983788),FQ(0.999983311),FQ(0.999982834),FQ(0.999982357),FQ(0.999981880),FQ(0.999981404),
        FQ(0.999980927),FQ(0.999980450),FQ(0.999979973),FQ(0.999979496),FQ(0.999979019),FQ(0.999978543),FQ(0.999978066),FQ(0.999977589),
        FQ(0.999977112),FQ(0.999976635),FQ(0.999976158),FQ(0.999975682),FQ(0.999975205),FQ(0.999974728),FQ(0.999974251),FQ(0.999973774),
        FQ(0.999973297),FQ(0.999972821),FQ(0.999972344),FQ(0.999971867),FQ(0.999971390),FQ(0.999970913),FQ(0.999970437),FQ(0.999969960) },
    };
    unsigned al; int ah, yy = lut[0][(xx>>22)&63];
    DSP_MUL( ah, al, yy, lut[1][(xx>>16)&63] ); DSP_EXT( ah, al, yy,0 );
    DSP_MAC( ah, al, yy, lut[2][(xx>>10)&63] ); DSP_EXT( ah, al, yy,0 );
    DSP_MAC( ah, al, yy, lut[3][(xx>> 4)&63] ); DSP_EXT( ah, al, yy,0 );
    return yy;
}

int dsp_sin( int xx )
{
    xx *= 4;
    if( xx < FQ(1) )
    {
        // +0.5 * xx * (pi - xx*xx * ((2pi-5) - xx*xx * (pi-3)))
        // -0.5 * xx * (xx*xx * ((2pi-5) - xx*xx * (pi-3)) - pi)
        // -0.5 * xx * (yy * ((2pi-5) - yy * (pi-3)) - pi)
        int ah = 0, yy = dsp_multiply(xx,xx); unsigned al = 1<<(QQ-1);
        DSP_MUL( ah,al, yy, FQ(+1.28318530718) );
        DSP_MAC( ah,al, yy, FQ(-0.14159265359) );
        DSP_EXT( ah,al, yy,0 ); yy -= FQ(3.14159265359);
        DSP_MUL( ah,al, yy, xx ); DSP_EXT( ah,al, xx,0 ); xx = -(xx/2);
    }
    else if( xx < FQ(2) )
    {
        xx = FQ(2) - xx;
        int ah = 0, yy = dsp_multiply(xx,xx); unsigned al = 1<<(QQ-1);
        DSP_MUL( ah,al, yy, FQ(+1.28318530718) );
        DSP_MAC( ah,al, yy, FQ(-0.14159265359) );
        DSP_EXT( ah,al, yy,0 ); yy -= FQ(3.14159265359);
        DSP_MUL( ah,al, yy, xx ); DSP_EXT( ah,al, xx,0 ); xx = -(xx/2);
    }
    else if( xx < FQ(3) )
    {
        xx = xx - FQ(2);
        int ah = 0, yy = dsp_multiply(xx,xx); unsigned al = 1<<(QQ-1);
        DSP_MUL( ah,al, yy, FQ(+1.28318530718) );
        DSP_MAC( ah,al, yy, FQ(-0.14159265359) );
        DSP_EXT( ah,al, yy,0 ); yy -= FQ(3.14159265359);
        DSP_MUL( ah,al, yy, xx ); DSP_EXT( ah,al, xx,0 ); xx = +(xx/2);
    }
    else
    {
        xx = FQ(4) - xx;
        int ah = 0, yy = dsp_multiply(xx,xx); unsigned al = 1<<(QQ-1);
        DSP_MUL( ah,al, yy, FQ(+1.28318530718) );
        DSP_MAC( ah,al, yy, FQ(-0.14159265359) );
        DSP_EXT( ah,al, yy,0 ); yy -= FQ(3.14159265359);
        DSP_MUL( ah,al, yy, xx ); DSP_EXT( ah,al, xx,0 ); xx = +(xx/2);
    }
    return xx;
}

void calc_peaking_48( int* coeffs, double freq, double gain, double Q )
{
    double A  = sqrt( pow(10,(gain/20)) );
    double w0 = 2*3.14159265359*(freq/48000.0), alpha = sin(w0)/(2.0*Q);
    coeffs[0] = FQ( +(1+alpha*A)/(1+alpha/A) );
    coeffs[1] = FQ( +(-2*cos(w0))/(1+alpha/A) );
    coeffs[2] = FQ( +(1-alpha*A)/(1+alpha/A) );
    coeffs[3] = FQ( -(-2*cos(w0))/(1+alpha/A) );
    coeffs[4] = FQ( -(1-alpha/A)/(1+alpha/A) );
}

void calc_lowpass_192( int* coeffs, double freq, double Q )
{
    double w0 = 2.0 * 3.14159265359 * (freq/48000.0);
    double alpha = sin(w0) / (2.0 * Q);
    coeffs[0] = FQ( 1 * ((1-cos(w0))/2)/(1+alpha) );
    coeffs[1] = FQ( 1 * (1-cos(w0))/(1+alpha) );
    coeffs[2] = FQ( 1 * ((1-cos(w0))/2)/(1+alpha) );
    coeffs[3] = FQ( -(-2*cos(w0))/(1+alpha) );
    coeffs[4] = FQ( -(1-alpha)/(1+alpha) );
}

void calc_peaking_192( int* coeffs, double freq, double gain, double Q )
{
    double A  = sqrt( pow(10,(gain/20)) );
    double w0 = 2*3.14159265359*(freq/192000.0), alpha = sin(w0)/(2.0*Q);
    coeffs[0] = FQ( +(1+alpha*A)/(1+alpha/A) );
    coeffs[1] = FQ( +(-2*cos(w0))/(1+alpha/A) );
    coeffs[2] = FQ( +(1-alpha*A)/(1+alpha/A) );
    coeffs[3] = FQ( -(-2*cos(w0))/(1+alpha/A) );
    coeffs[4] = FQ( -(1-alpha/A)/(1+alpha/A) );
}

double calc_dcblock_192( double freq )
{
    return pow( 2.718281828459045, -2 * 3.141592653589793 * freq / 192000.0 );
}

void xio_startup( void )
{
}

double scale_linear( double val, double min, double max ) { return min+val*(max-min); }
double scale_decade( double val, double min, double max ) { return min*pow(10,val*log10(max/min)); }

double read_pots( int number )
{
    i2c_init(100000);
    i2c_start(); i2c_write(0xDA+0); i2c_write(0x60+1+2*number); i2c_stop();
    i2c_start(); i2c_write(0xDA+1); timer_delay(100);
    double knob = ((double)i2c_read())/256; i2c_ack(1);
    i2c_stop();
    return knob;
}

void xio_control( const int rcv_prop[6], int snd_prop[6], int dsp_prop[6] )
{
    static double knobs[8] = {0,0,0,0};

    knobs[0] = read_pots(0); knobs[1] = read_pots(1);
    knobs[2] = read_pots(2); knobs[3] = read_pots(3);
    knobs[4] = read_pots(4); knobs[5] = read_pots(5);
    knobs[6] = read_pots(6); knobs[7] = read_pots(7);

    knobs[0] = knobs[1] = knobs[2] = knobs[3] = 0.5;
    knobs[4] = knobs[5] = knobs[6] = knobs[7] = 0.5;

    static int state = 1; dsp_prop[0] = state;
    if( state == 1 ) // Master volume, preamp gain (Linear)
    {
        dsp_prop[1] = FQ( scale_linear( knobs[0], 0.000, 0.999 )); // Volume (Knob 0)
        dsp_prop[2] = FQ( scale_linear( knobs[1], 0.500, 0.999 )); // Gain (Knob 1)
    }
    else if( state == 2 ) // Pre-Gain Low-Cut (Knob 2, Linear)
    {
        // DC blocker (1-pole low pass filter) coefficients for 10Hz - 50/100/200 Hz. 
        dsp_prop[1] = FQ( calc_dcblock_192( scale_linear( knobs[2], 10, 50 )));
        dsp_prop[2] = FQ( calc_dcblock_192( scale_linear( knobs[2], 10,100 )));
        dsp_prop[3] = FQ( calc_dcblock_192( scale_linear( knobs[2], 10,200 )));
    }
    else if( state == 3 ) // Tone (Knob 3, Logarithmic)
    {
        // Low-pass filter, 5kHz to 15kHz, Q = 0.707
        calc_lowpass_192( dsp_prop+1, scale_decade(knobs[3],5000,15000), 0.707 );
    }
    else if( state == 4 ) // 3-Band EQ Bass, Linear
    {
        // Peaking filter, frequency = 100 Hz, min gain = -9dB, max gain = +9dB, Q = 0.5
        calc_peaking_48( dsp_prop+1, 100, scale_linear( 0.7, -9, +9 ), 0.5 ); // Knob = 0.7
    }
    else if( state == 5 ) // 3-Band EQ Middle, Linear
    {
        // Peaking filter, frequency = 900 Hz, min gain = -9dB, max gain = +9dB, Q = 0.5
        calc_peaking_48( dsp_prop+1, 900, scale_linear( 0.3, -9, +9 ), 0.5 ); // Knob = 0.3
    }
    else if( state == 6 ) // 3-Band EQ Treble, Linear
    {
        // Peaking filter, frequency = 3.5 kHz, min gain = -9dB, max gain = +9dB, Q = 0.5
        calc_peaking_192( dsp_prop+1, 3500, scale_linear( 0.7, -9, +9 ), 0.5 ); // Knob = 0.7
    }
    else if( state == 7 ) // Chorus depth, rate, blend (knobs 4/5/6, Linear)
    {
        dsp_prop[1] = FQ( scale_linear( knobs[4], 0.0, 0.9000 ));
        dsp_prop[2] = FQ( scale_linear( knobs[5], 0.0, 0.0999 ));
        dsp_prop[3] = FQ( scale_linear( knobs[6], 0.0, 1.3/48000 ));
    }
    else if( state == 8 ) // Reverb blend (knob 7)
    {
        dsp_prop[1] = FQ( knobs[7] );
    }
    if( ++state > 8 ) state = 1;
}

void xio_mixer( const int usb_output_q31[2], int usb_input_q31[2],
                const int adc_output_q31[2], int dac_input_q31[2],
                const int dsp_output_q28[8], int dsp_input_q28[8], const int property[6] )
{
    usb_input_q31[0] = adc_output_q31[0];
    usb_input_q31[1] = dsp_output_q28[0] * 8;
   
    dsp_input_q28[0] = dsp_input_q28[1] = adc_output_q31[0] / 8;
    dac_input_q31[0] = dac_input_q31[1] = dsp_output_q28[0] * 8;

    //dsp_input_q28[0] = dsp_input_q28[1] = usb_output_q31[0] / 8;
    //usb_input_q31[0] = usb_input_q31[1] = dsp_output_q28[0] * 8;
}

void xio_initialize( void ) {}

int preamp_upsample_ss[96], preamp_upsample_cc[96] = // FIR 192k 8k 24k -120 L=4
{
    FQ(-0.00000013861),FQ(-0.00000395176),FQ(+0.00004880348),FQ(-0.00009472574),
    FQ(-0.00028257212),FQ(+0.00141057733),FQ(-0.00154574644),FQ(-0.00307777293),
    FQ(+0.01148487239),FQ(-0.01044092883),FQ(-0.01994352140),FQ(+0.10359038142),
    FQ(+0.15873191137),FQ(+0.02977729401),FQ(-0.02845652307),FQ(+0.00774282738),
    FQ(+0.00422954934),FQ(-0.00440658988),FQ(+0.00105878279),FQ(+0.00045488241),
    FQ(-0.00033593334),FQ(+0.00005008748),FQ(+0.00001064877),FQ(-0.00000220923),
    FQ(-0.00000081734),FQ(+0.00000000000),FQ(+0.00006126299),FQ(-0.00022006004),
    FQ(+0.00000000000),FQ(+0.00150236214),FQ(-0.00320526118),FQ(+0.00000000000),
    FQ(+0.01155494591),FQ(-0.02106486211),FQ(+0.00000000000),FQ(+0.13637245991),
    FQ(+0.13637245991),FQ(+0.00000000000),FQ(-0.02106486211),FQ(+0.01155494591),
    FQ(+0.00000000000),FQ(-0.00320526118),FQ(+0.00150236214),FQ(+0.00000000000),
    FQ(-0.00022006004),FQ(+0.00006126299),FQ(+0.00000000000),FQ(-0.00000081734),
    FQ(-0.00000220923),FQ(+0.00001064877),FQ(+0.00005008748),FQ(-0.00033593334),
    FQ(+0.00045488241),FQ(+0.00105878279),FQ(-0.00440658988),FQ(+0.00422954934),
    FQ(+0.00774282738),FQ(-0.02845652307),FQ(+0.02977729401),FQ(+0.15873191137),
    FQ(+0.10359038142),FQ(-0.01994352140),FQ(-0.01044092883),FQ(+0.01148487239),
    FQ(-0.00307777293),FQ(-0.00154574644),FQ(+0.00141057733),FQ(-0.00028257212),
    FQ(-0.00009472574),FQ(+0.00004880348),FQ(-0.00000395176),FQ(-0.00000013861),
    FQ(-0.00000380062),FQ(+0.00002837621),FQ(-0.00000000000),FQ(-0.00037960809),
    FQ(+0.00098532369),FQ(-0.00000000000),FQ(-0.00452074445),FQ(+0.00854642470),
    FQ(-0.00000000000),FQ(-0.02902839806),FQ(+0.06603911930),FQ(+0.16666654447),
    FQ(+0.06603911930),FQ(-0.02902839806),FQ(-0.00000000000),FQ(+0.00854642470),
    FQ(-0.00452074445),FQ(-0.00000000000),FQ(+0.00098532369),FQ(-0.00037960809),
    FQ(-0.00000000000),FQ(+0.00002837621),FQ(-0.00000380062),FQ(-0.00000013861)
};

int preamp_dnsample_ss[96], preamp_dnsample_cc[96] = // FIR 192k 16.1k 24k -64
{
    FQ(-0.00002259174),FQ(-0.00010484502),FQ(-0.00018383221),FQ(-0.00019491985),
    FQ(-0.00008348342),FQ(+0.00015137688),FQ(+0.00042954297),FQ(+0.00060355573),
    FQ(+0.00052066002),FQ(+0.00011308998),FQ(-0.00052777077),FQ(-0.00114305887),
    FQ(-0.00139129064),FQ(-0.00100874034),FQ(+0.00002059925),FQ(+0.00137644799),
    FQ(+0.00246757626),FQ(+0.00266034491),FQ(+0.00159473782),FQ(-0.00055427402),
    FQ(-0.00302018495),FQ(-0.00467479548),FQ(-0.00449888068),FQ(-0.00211479922),
    FQ(+0.00187062036),FQ(+0.00592696847),FQ(+0.00812664928),FQ(+0.00698357282),
    FQ(+0.00226650833),FQ(-0.00460871878),FQ(-0.01088385800),FQ(-0.01346111665),
    FQ(-0.01027899858),FQ(-0.00149996418),FQ(+0.01007181996),FQ(+0.01968488480),
    FQ(+0.02235633970),FQ(+0.01505536461),FQ(-0.00149404482),FQ(-0.02221444951),
    FQ(-0.03872863434),FQ(-0.04190411335),FQ(-0.02511018430),FQ(+0.01296243836),
    FQ(+0.06702269608),FQ(+0.12606342993),FQ(+0.17627517089),FQ(+0.20510315433),
    FQ(+0.20510315433),FQ(+0.17627517089),FQ(+0.12606342993),FQ(+0.06702269608),
    FQ(+0.01296243836),FQ(-0.02511018430),FQ(-0.04190411335),FQ(-0.03872863434),
    FQ(-0.02221444951),FQ(-0.00149404482),FQ(+0.01505536461),FQ(+0.02235633970),
    FQ(+0.01968488480),FQ(+0.01007181996),FQ(-0.00149996418),FQ(-0.01027899858),
    FQ(-0.01346111665),FQ(-0.01088385800),FQ(-0.00460871878),FQ(+0.00226650833),
    FQ(+0.00698357282),FQ(+0.00812664928),FQ(+0.00592696847),FQ(+0.00187062036),
    FQ(-0.00211479922),FQ(-0.00449888068),FQ(-0.00467479548),FQ(-0.00302018495),
    FQ(-0.00055427402),FQ(+0.00159473782),FQ(+0.00266034491),FQ(+0.00246757626),
    FQ(+0.00137644799),FQ(+0.00002059925),FQ(-0.00100874034),FQ(-0.00139129064),
    FQ(-0.00114305887),FQ(-0.00052777077),FQ(+0.00011308998),FQ(+0.00052066002),
    FQ(+0.00060355573),FQ(+0.00042954297),FQ(+0.00015137688),FQ(-0.00008348342),
    FQ(-0.00019491985),FQ(-0.00018383221),FQ(-0.00010484502),FQ(-0.00002259174)
};

int preamp_transfer( int xx )
{
    int yy = FQ(1.0) - dsp_negexp(2*xx-FQ(1.0/8)) / 8;
    if( xx <= FQ(0.5/8) ) yy = 8 * xx;
    if( xx >= FQ(4.5/8) ) yy = FQ(1.0);
	return yy;
}

int preamp1_antialias_ss[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
int preamp2_antialias_ss[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
int preamp3_antialias_ss[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
int preamp4_antialias_ss[24] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

int preamp1_state[4] = { 0,0,0,0 };
int preamp2_state[4] = { 0,0,0,0 };
int preamp3_state[4] = { 0,0,0,0 };
int preamp4_state[4] = { 0,0,0,0 };

int preamp_hicut_cc[6] = { 0,0,0,0,0,0 }, preamp_hicut_ss[4] = { 0,0,0,0 };

void xio_thread1( int samples[32], const int property[6] )
{
    static int gain = 0, block1 = 0, block2 = 0, block3 = 0;

    if( property[0] == 1 ) gain = property[2];
    if( property[0] == 2 ) // DC-blocking filter coefficients for stages 1,2,3
    {
        block1 = property[1]; block2 = property[2], block3 = property[3];
    }
    
    // Upsample x4
    
    samples[3] = 4 * dsp_fir24( samples[0], preamp_upsample_cc+ 0, preamp_upsample_ss+ 0 );
    samples[2] = 4 * dsp_fir24( samples[0], preamp_upsample_cc+24, preamp_upsample_ss+24 );
    samples[1] = 4 * dsp_fir24( samples[0], preamp_upsample_cc+48, preamp_upsample_ss+48 );
    samples[0] = 4 * dsp_fir24( samples[0], preamp_upsample_cc+72, preamp_upsample_ss+72 );

    // Gain Stage 1
    
    samples[3] = dsp_multiply( samples[3], gain );
    samples[2] = dsp_multiply( samples[2], gain );
    samples[1] = dsp_multiply( samples[1], gain );
    samples[0] = dsp_multiply( samples[0], gain );
    
    if(  samples[3] < 0 ) samples[3] = +preamp_transfer( -samples[3] );
    else                  samples[3] = -preamp_transfer( +samples[3] );
    if(  samples[2] < 0 ) samples[2] = +preamp_transfer( -samples[2] );
    else                  samples[2] = -preamp_transfer( +samples[2] );
    if(  samples[1] < 0 ) samples[1] = +preamp_transfer( -samples[1] );
    else                  samples[1] = -preamp_transfer( +samples[1] );
    if(  samples[0] < 0 ) samples[0] = +preamp_transfer( -samples[0] );
    else                  samples[0] = -preamp_transfer( +samples[0] );
    
    samples[3] = dsp_dcblock( samples[3], block1, preamp1_state+2 );
    samples[2] = dsp_dcblock( samples[2], block1, preamp1_state+2 );
    samples[1] = dsp_dcblock( samples[1], block1, preamp1_state+2 );
    samples[0] = dsp_dcblock( samples[0], block1, preamp1_state+2 );
        
    // Gain Stage 2
    
    samples[3] = dsp_multiply( samples[3], gain );
    samples[2] = dsp_multiply( samples[2], gain );
    samples[1] = dsp_multiply( samples[1], gain );
    samples[0] = dsp_multiply( samples[0], gain );
    
    if(  samples[3] < 0 ) samples[3] = +preamp_transfer( -samples[3] );
    else                  samples[3] = -preamp_transfer( +samples[3] );
    if(  samples[2] < 0 ) samples[2] = +preamp_transfer( -samples[2] );
    else                  samples[2] = -preamp_transfer( +samples[2] );
    if(  samples[1] < 0 ) samples[1] = +preamp_transfer( -samples[1] );
    else                  samples[1] = -preamp_transfer( +samples[1] );
    if(  samples[0] < 0 ) samples[0] = +preamp_transfer( -samples[0] );
    else                  samples[0] = -preamp_transfer( +samples[0] );
    
    samples[3] = dsp_dcblock( samples[3], block2, preamp2_state+2 );
    samples[2] = dsp_dcblock( samples[2], block2, preamp2_state+2 );
    samples[1] = dsp_dcblock( samples[1], block2, preamp2_state+2 );
    samples[0] = dsp_dcblock( samples[0], block2, preamp2_state+2 );
    
    // Gain Stage 3
    
    samples[3] = dsp_multiply( samples[3], gain );
    samples[2] = dsp_multiply( samples[2], gain );
    samples[1] = dsp_multiply( samples[1], gain );
    samples[0] = dsp_multiply( samples[0], gain );
    
    if(  samples[3] < 0 ) samples[3] = +preamp_transfer( -samples[3] );
    else                  samples[3] = -preamp_transfer( +samples[3] );
    if(  samples[2] < 0 ) samples[2] = +preamp_transfer( -samples[2] );
    else                  samples[2] = -preamp_transfer( +samples[2] );
    if(  samples[1] < 0 ) samples[1] = +preamp_transfer( -samples[1] );
    else                  samples[1] = -preamp_transfer( +samples[1] );
    if(  samples[0] < 0 ) samples[0] = +preamp_transfer( -samples[0] );
    else                  samples[0] = -preamp_transfer( +samples[0] );
    
    samples[3] = dsp_dcblock( samples[3], block3, preamp3_state+2 );
    samples[2] = dsp_dcblock( samples[2], block3, preamp3_state+2 );
    samples[1] = dsp_dcblock( samples[1], block3, preamp3_state+2 );
    samples[0] = dsp_dcblock( samples[0], block3, preamp3_state+2 );

    // Downsample x4
    
    memmove( preamp_dnsample_ss+4, preamp_dnsample_ss, 4*(96-4) ); // 96 taps, decimate by 4
    preamp_dnsample_ss[0] = samples[0]; preamp_dnsample_ss[1] = samples[1];
    preamp_dnsample_ss[2] = samples[2]; preamp_dnsample_ss[3] = samples[3];
    int ah = 0; unsigned al = 1<<(QQ-1);
    dsp_power24( ah,al, preamp_dnsample_cc+ 0, preamp_dnsample_ss+ 0);
    dsp_power24( ah,al, preamp_dnsample_cc+24, preamp_dnsample_ss+24);
    dsp_power24( ah,al, preamp_dnsample_cc+48, preamp_dnsample_ss+48);
    dsp_power24( ah,al, preamp_dnsample_cc+72, preamp_dnsample_ss+72);
    DSP_EXT( ah, al, samples[0],0 );
}

int tone_hcut_cc[6] = { 0,0,0,0,0,0 }, tone_hcut_ss[4] = { 0,0,0,0 };

int tone_eq01_cc[6] = { 0,0,0,0,0,0 }, tone_eq01_ss[4] = { 0,0,0,0 };
int tone_eq02_cc[6] = { 0,0,0,0,0,0 }, tone_eq02_ss[4] = { 0,0,0,0 };
int tone_eq03_cc[6] = { 0,0,0,0,0,0 }, tone_eq03_ss[4] = { 0,0,0,0 };

int _ampsim_data[3][1440], _ampsim_num = 1;
int _ampsim_ir_ss[1440], _ampsim_ir_cc[1440] = { FQ(1.0) };

void xio_thread2( int samples[32], const int property[6] )
{
    // Tone (Low-Pass)
    
    if( property[0] == 3 ) memcpy( tone_hcut_cc, property+1, 5*sizeof(int) );
    samples[0] = dsp_biquad( samples[0], tone_hcut_cc, tone_hcut_ss );

    // 3-Band Equalizer
    
    if( property[0] == 4 ) memcpy( tone_eq01_cc, property+1, 5*sizeof(int) );
    if( property[0] == 5 ) memcpy( tone_eq02_cc, property+1, 5*sizeof(int) );
    if( property[0] == 6 ) memcpy( tone_eq03_cc, property+1, 5*sizeof(int) );

    samples[0] = dsp_biquad( samples[0], tone_eq01_cc, tone_eq01_ss );
    samples[0] = dsp_biquad( samples[0], tone_eq02_cc, tone_eq02_ss );
    samples[0] = dsp_biquad( samples[0], tone_eq03_cc, tone_eq03_ss );
}

void xio_thread3( int samples[32], const int property[6] )
{
    if( property[0] == 19 && property[1] < 6 ) _ampsim_num = property[2];

    unsigned al = 1<<(QQ-1); int ah = 0, b0,b1,s0,s1,s2,s3, xx=samples[0];

    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 0, _ampsim_ir_ss+24* 0, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 1, _ampsim_ir_ss+24* 1, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 2, _ampsim_ir_ss+24* 2, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 3, _ampsim_ir_ss+24* 3, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 4, _ampsim_ir_ss+24* 4, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 5, _ampsim_ir_ss+24* 5, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 6, _ampsim_ir_ss+24* 6, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 7, _ampsim_ir_ss+24* 7, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 8, _ampsim_ir_ss+24* 8, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24* 9, _ampsim_ir_ss+24* 9, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*10, _ampsim_ir_ss+24*10, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*11, _ampsim_ir_ss+24*11, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*12, _ampsim_ir_ss+24*12, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*13, _ampsim_ir_ss+24*13, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*14, _ampsim_ir_ss+24*14, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*15, _ampsim_ir_ss+24*15, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*16, _ampsim_ir_ss+24*16, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*17, _ampsim_ir_ss+24*17, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*18, _ampsim_ir_ss+24*18, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*19, _ampsim_ir_ss+24*19, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*20, _ampsim_ir_ss+24*20, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*21, _ampsim_ir_ss+24*21, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*22, _ampsim_ir_ss+24*22, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*23, _ampsim_ir_ss+24*23, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*24, _ampsim_ir_ss+24*24, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*25, _ampsim_ir_ss+24*25, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*26, _ampsim_ir_ss+24*26, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*27, _ampsim_ir_ss+24*27, xx );

    samples[1] = ah; samples[2] = al;
}

void xio_thread4( int samples[32], const int property[6] )
{
    unsigned al=samples[2]; int ah=samples[1], b0,b1,s0,s1,s2,s3, xx=samples[0];

    dsp_convolve24( ah, al, _ampsim_ir_cc+24*58, _ampsim_ir_ss+24*58, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*59, _ampsim_ir_ss+24*59, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*60, _ampsim_ir_ss+24*60, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*61, _ampsim_ir_ss+24*61, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*62, _ampsim_ir_ss+24*62, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*63, _ampsim_ir_ss+24*63, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*64, _ampsim_ir_ss+24*64, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*65, _ampsim_ir_ss+24*65, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*66, _ampsim_ir_ss+24*66, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*67, _ampsim_ir_ss+24*67, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*68, _ampsim_ir_ss+24*68, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*69, _ampsim_ir_ss+24*69, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*70, _ampsim_ir_ss+24*60, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*71, _ampsim_ir_ss+24*71, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*72, _ampsim_ir_ss+24*72, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*73, _ampsim_ir_ss+24*73, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*74, _ampsim_ir_ss+24*74, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*75, _ampsim_ir_ss+24*75, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*76, _ampsim_ir_ss+24*76, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*77, _ampsim_ir_ss+24*77, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*78, _ampsim_ir_ss+24*78, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*79, _ampsim_ir_ss+24*79, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*80, _ampsim_ir_ss+24*80, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*81, _ampsim_ir_ss+24*81, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*82, _ampsim_ir_ss+24*82, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*83, _ampsim_ir_ss+24*83, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*84, _ampsim_ir_ss+24*84, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*85, _ampsim_ir_ss+24*85, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*86, _ampsim_ir_ss+24*86, xx );
    dsp_convolve24( ah, al, _ampsim_ir_cc+24*87, _ampsim_ir_ss+24*87, xx );

    DSP_EXT( ah, al, samples[0],0 );
}

// DIFFUSE all-pass: v[n] = x[n] - A * v[n-1], y[n] = A * v[n] + v[n-1]

#define _REVERB_DIFFUSE( xx, ss, ii, gg ) \
{ \
    int dd = (ss)[ii]; \
    (ss)[ii] = (xx) - dsp_multiply( (gg), dd ); \
    (xx) = dd + dsp_multiply( (gg), (ss)[ii] ); \
    if( --ii < 0 ) ii += (sizeof(ss)/sizeof(ss[0])); \
}

#define _REVERB_DELAY_N( xx, ss, nn, ii ) \
{ \
    int yy = (ss)[ii]; (ss)[ii] = (xx); (xx) = yy; \
    if( --ii < 0 ) ii += (nn); \
}
#define _REVERB_DELAY( xx, ss, ii ) \
{ \
    int yy = (ss)[ii]; (ss)[ii] = (xx); (xx) = yy; \
    if( --ii < 0 ) ii += (sizeof(ss)/sizeof(ss[0])); \
}

// DAMPEN - single pole low pass: y[n] = (1-A) * x[n] - A * y[n-1]

#define _REVERB_DAMPEN( xx, ss, aa ) \
{ \
    int yy = dsp_multiply(aa,xx) + dsp_multiply(FQ(1.0)-(aa),(ss)[0]); \
    xx = (ss)[0] = yy; \
}

#define _REVERB_DECAY( xx, gg ) { (xx) = dsp_multiply( (xx), (gg) ); }

int _tankA_decay = FQ(0.5), _tankB_decay = FQ(0.5);
int _input_damping=FQ(0.5);
int _tankA_damping=FQ(0.5), _tankB_damping=FQ(0.5);

int _input_dampen_buf[1] = {0};
int _tankA_dampen_buf[1] = {0};
int _tankB_dampen_buf[1] = {0};

int _input_diff_buf1 [ 181], _input_diff_idx1 = 0, _input_diff1 = FQ(0.75);
int _input_diff_buf2 [ 143], _input_diff_idx2 = 0; 
int _input_diff_buf3 [ 499], _input_diff_idx3 = 0, _input_diff2 = FQ(0.63);
int _input_diff_buf4 [ 352], _input_diff_idx4 = 0;

int _tankA_diff_buf1 [ 907], _tankA_diff_idx1 = 0, _decay_diff1 = FQ(0.70);
int _tankA_diff_buf2 [2099], _tankA_diff_idx2 = 0;
int _tankB_diff_buf1 [1201], _tankB_diff_idx1 = 0, _decay_diff2 = FQ(0.50);
int _tankB_diff_buf2 [3406], _tankB_diff_idx2 = 0;

int _tankA_delay1_buf[5973], _tankA_delay1_idx = 0;
int _tankA_delay2_buf[2401], _tankA_delay2_idx = 0;
int _tankB_delay1_buf[3608], _tankB_delay1_idx = 0;
int _tankB_delay2_buf[1944], _tankB_delay2_idx = 0;

void xio_thread5( int samples[32], const int property[6] )
{
    // Chorus
    
    static int delay = FQ(0.5), rate = FQ(1.0/48000), depth = FQ(0.1), blend = FQ(0.5);
    if( property[0] == 7 ) { delay=property[1]; depth=property[2]; rate=property[3]; blend=property[4]; }

    static int angle = FQ(0.0); angle += rate; if(angle >= FQ(1.0)) angle -= FQ(1.0);
    static int delay_buf[2048], delay_idx = 0; delay_buf[--delay_idx & 2047] = samples[0];
    int mm = delay + dsp_multiply( depth, FQ(0.5)+dsp_sin(angle) );

    int ii, ff, x1, x2, x3;        
    ii = ((mm & 0x0FFE0000) >> 17) + delay_idx; ff = (mm & 0x0001FFFF ) << 11;
    x1 = delay_buf[ii&2047]; x2 = delay_buf[(ii+1)&2047]; x3 = delay_buf[(ii+2)&2047];
    samples[0] = dsp_blend( samples[0], lagrange_interp(ff,x1,x2,x3), blend );

    // Reverb

    static int tankA = 0, tankB = 0, tankC = 0, mix = 0;
    
    if( property[0] == 8 ) mix = property[4];

    _REVERB_DAMPEN ( samples[0], _input_dampen_buf, _input_damping );
    _REVERB_DIFFUSE( samples[0], _input_diff_buf1, _input_diff_idx1, +_input_diff1 );
    _REVERB_DIFFUSE( samples[0], _input_diff_buf2, _input_diff_idx2, +_input_diff1 );
    _REVERB_DIFFUSE( samples[0], _input_diff_buf3, _input_diff_idx3, +_input_diff2 );
    _REVERB_DIFFUSE( samples[0], _input_diff_buf4, _input_diff_idx4, +_input_diff2 );

    tankC = tankA; tankA = samples[0] + tankB; tankB = samples[0] + tankC;
    
    _REVERB_DIFFUSE( tankA, _tankA_diff_buf1,  _tankA_diff_idx1, -_decay_diff1 );
    _REVERB_DELAY  ( tankA, _tankA_delay1_buf, _tankA_delay1_idx );
    _REVERB_DAMPEN ( tankA, _tankA_dampen_buf, _tankA_damping ); 
    _REVERB_DECAY  ( tankA, _tankA_decay );
    _REVERB_DIFFUSE( tankA, _tankA_diff_buf2,  _tankA_diff_idx2, +_decay_diff1 );
    _REVERB_DELAY  ( tankA, _tankA_delay2_buf, _tankA_delay2_idx );
    _REVERB_DIFFUSE( tankB, _tankB_diff_buf1,  _tankB_diff_idx1, -_decay_diff2 );
    _REVERB_DELAY  ( tankB, _tankB_delay1_buf, _tankB_delay1_idx );
    _REVERB_DAMPEN ( tankB, _tankB_dampen_buf, _tankB_damping ); 
    _REVERB_DECAY  ( tankB, _tankA_decay );
    _REVERB_DIFFUSE( tankB, _tankB_diff_buf2,  _tankB_diff_idx2, +_decay_diff2 );
    _REVERB_DELAY  ( tankB, _tankB_delay2_buf, _tankB_delay2_idx );

    samples[0] = dsp_blend( samples[0], tankB, mix );

    // Volume

    static int volume = 0;
    if( property[0] == 1 ) volume = property[1];
    samples[0] = dsp_multiply( samples[0], volume );

    samples[0] = samples[0] / 64;
}

int _ampsim_data[3][1440] =
{
    {
    FQ(1.0),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    },{
    FQ(1.0),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    },{
    FQ(1.0),0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    },
};

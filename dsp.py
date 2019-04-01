import os, sys


import math


def QF(xx): return (1.0*xx) / ((2**28)-0)

print QF(0x08000000)

exit()



for ii in range(0,6):
    print math.e ** -(2**ii)

for ii in range(6,12):
    print math.e ** -(2**ii)

for ii in range(12,18):
    print math.e ** -(2**ii)

for ii in range(18,24):
    print math.e ** -(2**ii)


print math.e ** -(2**1)





if len(sys.argv) < 5:

    print( "" )
    print( "Usage: python dsp.py fir <samp_freq> <pass_freq> <stop_freq> <-attenuation>" )
    print( "       python dsp.py fir <samp_freq> <pass_freq> <stop_freq> <+tap_count>" )
    print( "" )
    print( "Usage: python dsp.py iir <samp_freq> <type> <cutoff_freq> <Q> <gain>" )
    print( "" )
    print( "       <type> filter type (notch, lowpass, highpass, allpass, bandpass," )
    print( "                           peaking, highshelf, or lowshelf" )
    print( "       <freq> is cutoff frequency relative to Fs (0 <= freq < 0.5)" )
    print( "       <Q> is the filter Q-factor" )
    print( "       <gain> is the filter positive or negative gain in dB" )
    print( "" )
    print( "Usage: python dsp.py wave file1 file2 ... fileN" )
    print( "" )
    print( "       Print floating point values of samples within a wave file." )
    print( "" )
    print( "Usage: python dsp.py plot <datafile> time" )
    print( "       python dsp.py plot <datafile> time [beg end]" )
    print( "       python dsp.py plot <datafile> freq lin" )
    print( "       python dsp.py plot <datafile> freq log" )
    print( "" )
    print( "       <datafile> Contains one sample value per line.  Each sample is an" )
    print( "                  ASCII/HEX value (e.g. FFFF0001) representing a fixed-" )
    print( "                  point sample value." )
    print( "       time       Indicates that a time-domain plot should be shown" )
    print( "       freq       Indicates that a frequency-domain plot should be shown" )
    print( "       [beg end]  Optional; specifies the first and last sample in order to" )
    print( "                  create a sub-set of data to be plotted" )
    print( "" )
    print( "       Create time-domain plot data in \'out.txt\' showing samples 100" )
    print( "       through 300 ... bash$ python plot.py out.txt 100 300" )
    print( "" )
    print( "       Create frequency-domain plot data in \'out.txt\' showing the Y-axis" )
    print( "       with a logarithmic scale ... bash$ python plot.py out.txt freq log" )
    print( "" )

    exit(0)

if sys.argv[1] == "fir":

    import numpy as np
    from scipy.signal import kaiserord, firwin, freqz
    import math

    fs = float( sys.argv[2] )
    passband_freq   = float( sys.argv[3] ) / fs
    stopband_freq   = float( sys.argv[4] ) / fs
    stopband_atten  = float( sys.argv[5] )

    width = abs(passband_freq - stopband_freq) / 0.5
    (tap_count,beta) = kaiserord( ripple = stopband_atten, width = width )

    #if len(sys.argv) > 6: tap_count = int( sys.argv[6] )

    taps = firwin( numtaps = tap_count, \
                   cutoff  = ((passband_freq+stopband_freq)/2)/0.5, \
                   window  = ('kaiser', beta) )
    
    #nn = 25
    #taps = []
    #for ii in range(0,nn): taps.append(1.0/nn)
        
    import matplotlib.pyplot as plt
    w, h = freqz( taps, worN=8000 )
    fig = plt.figure()
    plt.title('Digital filter frequency response')
    ax1 = fig.add_subplot(111)
    plt.plot(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
    plt.ylabel('Amplitude [dB]', color='b')
    plt.xlabel('Frequency [Normalized to Fs]')
    #ax2 = ax1.twinx()
    #angles = np.unwrap(np.angle(h)) / (2*np.pi) * 360.0
    #plt.plot(w/(2*np.pi)*1.001, angles, 'g')
    #plt.ylabel('Angle (degrees)', color='g')
    plt.grid()
    plt.axis('tight')
    plt.show()
    
    L = 0
    if len(sys.argv) >= 7: L = int(sys.argv[6])
    print L
    
    if L != 0:
        for nn in range(0,L):
            ii = 0
            for cc in taps[nn:nn+len(taps)-1:L]:
                if (ii % 4) == 0: sys.stdout.write('    ')
                sys.stdout.write( "FQ(%+1.11f)," % cc )
                ii += 1
                if (ii % 4) == 0: sys.stdout.write('\n')
            sys.stdout.write( "FQ(%+1.11f)\n" % taps[len(taps)-1] )
    
    else:
        ii = 0
        for cc in taps[0:len(taps)-1]:
            if (ii % 4) == 0: sys.stdout.write('    ')
            sys.stdout.write( "FQ(%+1.11f)," % cc )
            ii += 1
            if (ii % 4) == 0: sys.stdout.write('\n')
        sys.stdout.write( "FQ(%+1.11f)\n" % taps[len(taps)-1] )

    print( "%u Taps" % len(taps) )

def plot_response( bb, aa, xmin=None, xmax=None, ymin=-60.0, ymax=6.0 ):

    import matplotlib.pyplot as plt
    w, h = dsp.freqz(bb,aa)
    fig = plt.figure()
    plt.title('Digital filter frequency response')
    ax1 = fig.add_subplot(111)
    #plt.plot(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
    plt.semilogx(w/(2*np.pi)*1.001, 20 * np.log10(abs(h)), 'b')
    plt.ylabel('Amplitude [dB]', color='b')
    plt.xlabel('Frequency [Normalized to Fs]')
    plt.ylim( -30,+6 )
    ax2 = ax1.twinx()
    angles = np.unwrap(np.angle(h)) / (2*np.pi) * 360.0
    plt.plot(w/(2*np.pi)*1.001, angles, 'g')
    plt.ylabel('Angle (degrees)', color='g')
    plt.grid()
    plt.axis('tight')
    plt.show()

def _make_biquad_notch( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0; b1 = -2.0 * np.cos(w0); b2 = +1.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_biquad_lowpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2 * q_factor)

	b0 = (+1.0 - np.cos(w0)) / 2.0; b1 =  +1.0 - np.cos(w0); b2 = (+1.0 - np.cos(w0)) / 2.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_biquad_highpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2 * q_factor)

	b0 = (1.0 + np.cos(w0)) / 2.0; b1 = -(1.0 + np.cos(w0)); b2 = (1.0 + np.cos(w0)) / 2.0
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-60, ymax=6 )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_biquad_allpass( filter_freq, q_factor ):

	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0 - alpha; b1 = -2.0 * np.cos(w0); b2 = +1.0 + alpha
	a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

	plot_response( [b0,b1,b2], [a0,a1,a2], ymin=-6, ymax=6 )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

from scipy.signal import butter, lfilter

def butter_bandpass(lowcut, highcut, fs, order=5):
    nyq = 0.5 * fs
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def butter_bandpass_filter(data, lowcut, highcut, fs, order=5):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = lfilter(b, a, data)
    return y

# Constant 0 dB peak gain
# FIXME: Results in a peaking filter at freq1 rather than BP filter that's flat within the pass-band
#def _make_biquad_bandpass( filter_freq1, filter_freq2 ):

def _make_biquad_bandpass( filter_freq, q_factor ):

    #filter_freq = (filter_freq1 + filter_freq2) / 2
    #w0 = 2.0 * np.pi * filter_freq
    #BW = (filter_freq2 -filter_freq1) / filter_freq1
    #alpha = np.sin(w0) * np.sinh( np.log(2)/2 * BW * w0/np.sin(w0) )

    w0 = 2.0 * np.pi * filter_freq
    alpha = np.sin(w0)/(2.0 * q_factor)
    
    b0 = alpha; b1 = +0.0; b2 = -alpha
    a0 = +1.0 + alpha; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha

    plot_response( [b0,b1,b2], [a0,a1,a2] )
    
    b,a = dsp.iirfilter( N=1, Wn=[0.1,0.2], btype='bandpass' )
    b0 = b[0]; b1 = b[1]; a0 = a[0]; a1 = a[1];
    plot_response( b, a )

    print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

# gain can be + or -
def _make_biquad_peaking( filter_freq, q_factor, gain_db ):

	A  = np.sqrt( 10 ** (gain_db/20) )
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/(2.0 * q_factor)

	b0 = +1.0 + alpha * A; b1 = -2.0 * np.cos(w0); b2 = +1.0 - alpha * A
	a0 = +1.0 + alpha / A; a1 = -2.0 * np.cos(w0); a2 = +1.0 - alpha / A

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_biquad_lowshelf( filter_freq, q_factor, gain_db ):

	S = q_factor
	A  = 10.0 ** (gain_db / 40.0)
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/2 * np.sqrt( (A + 1/A)*(1/S - 1) + 2 )

	b0 = A*( (A+1) - (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha )
	b1 =  2*A*( (A-1) - (A+1)*np.cos(w0) )
	b2 = A*( (A+1) - (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha )
	a0 = (A+1) + (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha
	a1 = -2*( (A-1) + (A+1)*np.cos(w0) )
	a2 = (A+1) + (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_biquad_highshelf( filter_freq, q_factor, gain_db ):

	S = q_factor
	A  = 10.0 ** (gain_db / 40.0)
	w0 = 2.0 * np.pi * filter_freq
	alpha = np.sin(w0)/2 * np.sqrt( (A + 1/A)*(1/S - 1) + 2 )

	b0 = A*( (A+1) + (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha )
	b1 = -2*A*( (A-1) + (A+1)*np.cos(w0) )
	b2 = A*( (A+1) + (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha )
	a0 = (A+1) - (A-1)*np.cos(w0) + 2*np.sqrt(A)*alpha
	a1 = 2*( (A-1) - (A+1)*np.cos(w0) )
	a2 = (A+1) - (A-1)*np.cos(w0) - 2*np.sqrt(A)*alpha

	if gain_db == 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3 )
	if gain_db  < 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3+gain_db, ymax=3 )
	if gain_db  > 0: plot_response( [b0,b1,b2],[a0,a1,a2], ymin=-3, ymax=3+gain_db )

	print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (b0/a0,b1/a0,b2/a0,-a1/a0,-a2/a0) )

def _make_tonestack( filter_freq ):

    C1 = 0.25e-9
    C2 = 20.0e-9
    C3 = 20.0e-9
    R1 = 250e3
    R2 = 1e6
    R3 = 25e3
    R4 = 56e3

    Fs = 48000.0
    k = 2 * Fs
    l = 0.9; m = 0.1; t = 0.9;

    m2 = m * m
    
    b1 = t*C1*R1 + m*C3*R3 + l*(C1*R2 + C2*R2) + (C1*R3 + C2*R3)
    b2=t*(C1*C2*R1*R4+C1*C3*R1*R4)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+m*(C1*C3*R1*R3+C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R1*R2+C1*C2*R2*R4+C1*C3*R2*R4)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)+(C1*C2*R1*R3+C1*C2*R3*R4+C1*C3*R3*R4)
    b3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+t*C1*C2*C3*R1*R3*R4-t*m*C1*C2*C3*R3*R3*R4+t*l*C1*C2*C3*R1*R2*R4
    a0=1
    a1=(C1*R1+C1*R3+C2*R3+C2*R4+C3*R4)+m*C3*R3+l*(C1*R2+C2*R2)
    a2=m*(C1*C3*R1*R3-C2*C3*R3*R4+C1*C3*R3*R3+C2*C3*R3*R3)+l*m*(C1*C3*R2*R3+C2*C3*R2*R3)-m2*(C1*C3*R3*R3+C2*C3*R3*R3)+l*(C1*C2*R2*R4+C1*C2*R1*R2+C1*C3*R2*R4+C2*C3*R2*R4)+(C1*C2*R1*R4+C1*C3*R1*R4+C1*C2*R3*R4+C1*C2*R1*R3+C1*C3*R3*R4+C2*C3*R3*R4)
    a3=l*m*(C1*C2*C3*R1*R2*R3+C1*C2*C3*R2*R3*R4)-m2*(C1*C2*C3*R1*R3*R3+C1*C2*C3*R3*R3*R4)+m*(C1*C2*C3*R3*R3*R4+C1*C2*C3*R1*R3*R3-C1*C2*C3*R1*R3*R4)+l*C1*C2*C3*R1*R2*R4+C1*C2*C3*R1*R3*R4

    B0 =       -b1*k -b2*k*k   -b3*k*k*k
    B1 =       -b1*k +b2*k*k +3*b3*k*k*k
    B2 =       +b1*k +b2*k*k -3*b3*k*k*k
    B3 =       +b1*k -b2*k*k   +b3*k*k*k
    A0 = -a0   -a1*k -a2*k*k   -a3*k*k*k
    A1 = -3*a0 -a1*k +a2*k*k +3*a3*k*k*k
    A2 = -3*a0 +a1*k +a2*k*k -3*a3*k*k*k
    A3 = -a0   +a1*k -a2*k*k   +a3*k*k*k

    print B0,B1,B2,B3,A0,A1,A2,A3

    plot_response( [B0,B1,B2,B3],[A0,A1,A2,A3] )

    print( "FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f),FQ(%+1.9f)" % (B0/A0,B1/A0,B2/A0,B3/A0,-A1/A0,-A2/A0,-A3/A0) )

if sys.argv[1] == "iir":

    import numpy as np
    import scipy.signal as dsp
    import matplotlib.pyplot as plot

    np.seterr( all='ignore' )

    type = sys.argv[2]
    fs   = float( sys.argv[3] )
    freq = float( sys.argv[4] ) / fs
    Q    = float( sys.argv[5] )
    gain = float( sys.argv[6] )

    if type == "tonestack":  _make_tonestack       ( freq )
    if type == "notch":      _make_biquad_notch    ( freq, Q )
    if type == "lowpass":    _make_biquad_lowpass  ( freq, Q )
    if type == "highpass":   _make_biquad_highpass ( freq, Q )
    if type == "allpass":    _make_biquad_allpass  ( freq, Q )
    if type == "bandpass":   _make_biquad_bandpass ( freq, Q )
    if type == "peaking":    _make_biquad_peaking  ( freq, Q, gain )
    if type == "highshelf":  _make_biquad_highshelf( freq, Q, gain )
    if type == "lowshelf":   _make_biquad_lowshelf ( freq, Q, gain )

def find_range(f, x):
    lowermin = 0; uppermin = 0
    for i in arange(x+1, len(f)):
        if f[i+1] >= f[i]:
            uppermin = i
            break
    for i in arange(x-1, 0, -1):
        if f[i] <= f[i-1]:
            lowermin = i + 1
            break
    return (lowermin, uppermin)

def _assert( condition, message ):

    if not condition:
        print message
        exit( 0 )

def _parse_wave( wave_file ):

    rate = 0; samples = None
    (group_id,total_size,type_id) = struct.unpack( "<III", wave_file.read(12) )
    _assert( group_id == 0x46464952, "Unknown File Format" ) # Signature for 'RIFF'
    _assert( type_id  == 0x45564157, "Unknown File Format" ) # Signature for 'WAVE'
    while True:
        if total_size <= 8: break
        data = wave_file.read(8)
        if len(data) < 8: break;
        (blockid,blocksz) = struct.unpack( "<II", data )
        #print "WaveIn: BlockID=0x%04X BlockSize=%u" % (blockid,blocksz)
        #print "WaveIn: ByteCount=%u" % (blocksz)
        total_size -= 8
        if blockid == 0x20746D66: # Signature for 'fmt'
            if total_size <= 16: break
            (format,channels,rate,thruput,align,width) = struct.unpack( "<HHIIHH", wave_file.read(16) )
            #print "WaveIn: ByteCount=%u Channels=%u Rate=%u WordSize=%u" % (blocksz,channels,rate,width)
            #print "WaveIn: Format=%u Alignment=%u" % (format,align)
            total_size -= 16
        elif blockid == 0x61746164: # Signature for 'data'
            samples = [0] * (blocksz / (width/8))
            count = 0
            data = wave_file.read( blocksz )
            if channels == 1:
                while len(data) >= width/8:
                    if width == 8:
                        samples[count] = struct.unpack( "b", data[0:1] )[0] * 256 * 256 * 256
                        samples[count]
                        data = data[1:]
                    if width == 16:
                        samples[count]  = struct.unpack( "b", data[1:2] )[0] * 256 * 256 * 256
                        samples[count] += struct.unpack( "B", data[0:1] )[0] * 256 * 256
                        data = data[2:]
                    if width == 24:
                        samples[count]  = struct.unpack( "b", data[2:3] )[0] * 256 * 256 * 256
                        samples[count] += struct.unpack( "B", data[1:2] )[0] * 256 * 256
                        samples[count] += struct.unpack( "B", data[0:1] )[0] * 256
                        data = data[3:]
                    if width == 32:
                        samples[count]  = struct.unpack( "b", data[3:4] )[0] * 256 * 256 * 256
                        samples[count] += struct.unpack( "B", data[2:3] )[0] * 256 * 256
                        samples[count] += struct.unpack( "B", data[1:2] )[0] * 256
                        samples[count] += struct.unpack( "B", data[0:1] )[0]
                        data = data[4:]
                    count += 1
            total_size -= blocksz
    return (rate,samples)

if sys.argv[1] == "wave":

    data = []
    count = 99999999

    for name in sys.argv[2:]:

        try: file = open( name, "rb" )
        except IOError as err: file = None
        _assert( file != None, "Unable to open file" )
        if file != None:
            (rate,samples) = _parse_wave( file )
            if count > len(samples): count = len(samples)
            data.append( samples )
            file.close()

    for row in range(0,count):
        for col in range(0,len(data)):
            #sys.stdout.write( "%08x " % (data[col][row] & 0xFFFFFFFF) )
            val = float(data[col][row]) / (2 ** 31)
            sys.stdout.write( "%+1.8f " % val )
        sys.stdout.write( "\n" )

if sys.argv[1] == "plot":

    import numpy
    import matplotlib.pyplot

    numpy.seterr( all='ignore' )

    K = 1
    yy = [[0] * 65536, [0] * 65536, [0] * 65536]
    mm = [[0] * 65536, [0] * 65536, [0] * 65536]
    N = 0

    file = open( sys.argv[2], "rt" )
    while True:
        ll = file.readline();
        ll = ll.replace( "\n", "" );
        if len(ll) < 1: break
        ll = ll.split()
        K = len(ll)
        for ii in range(0,K):
            #xx = int(ll[ii],16) * 2
            #tt = xx
            #if xx & 0x80000000 == 0x80000000:
            #    xx = -(1 - float((xx & 0x7FFFFFFF)) / (2**31))
            #else:
            #    xx = float((xx & 0x7FFFFFFF)) / ((2**31)-1)
            #yy[ii][N] = xx
            yy[ii][N] = float(ll[ii])
        N += 1
        if N == 65536: break
    file.close()

    xx = [0] * N
    ff = [0] * N
    mm[0] = [0] * N
    mm[1] = [0] * N
    mm[2] = [0] * N
    yy[0] = yy[0][0:N]
    yy[1] = yy[1][0:N]
    yy[2] = yy[2][0:N]

    for ii in range( 0, N ):
        ff[ii] = ii
        xx[ii] = float(ii) / N

    matplotlib.pyplot.grid( b=True, which='both', color='0.65',linestyle='-' )

    if sys.argv[3] == "time":

        beg = 0; end = N
        if len(sys.argv) > 3: beg = int( sys.argv[4] )
        if len(sys.argv) > 4: end = int( sys.argv[5] )
        ff = ff[beg:end]

        matplotlib.pyplot.title( "Time Domain" )
        matplotlib.pyplot.xlabel( "Sample Number" )
        matplotlib.pyplot.ylabel( "Sample Amplitude" )
        matplotlib.pyplot.xticks( numpy.arange(min(ff), max(ff), (max(ff)-min(ff))/4 ))

        ymin = 0; ymax = 0

        for ii in range(0,K):
            yy[ii] = yy[ii][beg:end]
            ymin = min(ymin,min(yy[ii]))
            ymax = max(ymax,max(yy[ii]))
            matplotlib.pyplot.plot( ff, yy[ii] )

        matplotlib.pyplot.ylim( ymin, ymax )
        matplotlib.pyplot.yticks( numpy.arange(ymin, ymax, (ymax-ymin)/20 ))
        matplotlib.pyplot.show()

    from scipy.signal import kaiser

    if sys.argv[3] == "freq":

        fs = 1.0
        if len(sys.argv) > 5: fs = float( sys.argv[5] )

        print( N/2+1 )
        ff = numpy.array(ff[0:int(N/2+1)]) / float(N/2)
        ff *= fs / 2.0

        for ii in range(0,K):
            yy[ii] *= kaiser( M=len(yy[ii]), beta=8, sym=False )
            mm[ii] = numpy.fft.rfft( yy[ii] )
            mm[ii] = mm[ii][0:len(mm[ii])-0]
            #mm[ii] = numpy.abs(mm[ii]) * 2.0 / N
            mm[ii] = numpy.abs(mm[ii]) * 1.0 / N
            mm[ii] /= numpy.max(mm[ii])
            mm[ii] = mm[ii][0:int(N/2+1)]

        matplotlib.pyplot.title( "Frequency Domain" )
        matplotlib.pyplot.xlabel( "Frequency (Normalized to Fs)" )
        matplotlib.pyplot.ylabel( "FFT Amplitude (dBfs)" )
        matplotlib.pyplot.ylim( -120, 0 )

        for ii in range(0,K):
            mm[ii] = 20.0 * numpy.log( mm[ii] )
            matplotlib.pyplot.plot( ff, mm[ii] )

        matplotlib.pyplot.xticks( numpy.arange(min(ff), max(ff), (max(ff)-min(ff))/10 ))
        matplotlib.pyplot.yticks( numpy.arange(-200, 0, (20+130)/15 ))
        matplotlib.pyplot.show()

    from numpy import argmax, sqrt, mean, absolute, arange, log10
    from scipy.signal import blackmanharris
    from numpy.fft import rfft, irfft

    for signal in yy:

        if K == 0: break
        K -= 1

        # Get rid of DC and window the signal
        signal -= mean(signal) # TODO: Do this in the frequency domain, and take any skirts with it?
        windowed = signal * blackmanharris(len(signal))  # TODO Kaiser?

        # Measure the total signal before filtering but after windowing
        total_rms = sqrt(mean(absolute(windowed)**2))

        # Find the peak of the frequency spectrum (fundamental frequency), and
        # filter the signal by throwing away values between the nearest local minima
        f = rfft(windowed); i = argmax(abs(f))
        lowermin, uppermin = find_range(abs(f), i)
        f[lowermin: uppermin] = 0

        # Transform noise back into the signal domain and measure it
        # TODO: Could probably calculate the RMS directly in the frequency domain instead
        noise = irfft(f)
        rms_flat = sqrt(mean(absolute(noise)**2))
        THDN = rms_flat / total_rms
        print( "SNR = %05.1fdB, THD+N = %04.1f%%" % \
               (20 * log10(total_rms) - 20 * log10(rms_flat), \
               THDN * 100 ))

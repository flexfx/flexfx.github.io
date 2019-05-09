import sys, time, struct, rtmidi

def midi_list():

    midiout = rtmidi.MidiOut()
    midiin  = rtmidi.MidiIn ()
    sys.stdout.write( "MIDI Output Devices:" )
    index = 0
    for device in midiout.get_ports():
        sys.stdout.write( " %u=\'%s\'" % (index,device) )
        index += 1
    sys.stdout.write( "\nMIDI Input Devices: " )
    index = 0
    for device in midiin.get_ports():
        sys.stdout.write( " %u=\'%s\'" % (index,device) )
        index += 1
    sys.stdout.write( "\n" )

def midi_open( port_number ):

    midiout = rtmidi.MidiOut()
    midiin  = rtmidi.MidiIn ()
    if midiout == None and midiin == None: return
    midiin.ignore_types( sysex = False, timing = False, active_sense = True )
    midiout.open_port( port_number )
    midiin.open_port ( port_number )
    return (midiout, midiin)

def midi_read( midi_device ): return midi_device[1].get_message()

def midi_write( midi_device, midi_sysex_data ):

    midi_device[0].send_message( midi_sysex_data )

def midi_wait( midi_device ):

    while( True ):
        data = midi_read( midi_device )
        if data == None: continue
        data = data[0]
        #if data[0] != 240: continue
        break;
    return data

def midi_close( midi_device ): a = 1

if len(sys.argv) < 2: # Usage 1 - Show help message

    print "Usage 1: python xio.py"
    print "         Show this message and list MIDI device names and port numbers."
    print ""
    print "Usage 2: python xio.py <midi_port>"
    print ""
    print "Usage 3: python xio.py <midi_port> <firmware_image>.bin"
    print "         Burn a FlexFX firmware image into FLASH memory. The firmware image must"
    print "         have a filename extension of .bin"
    print ""
    print "Usage 4: python xio.py <midi_port> <properties_file>.txt"
    print "         Load FlexFX properties contained in text file to device via USB MIDI."
    print "         Each property consists of a 16-bit ID and five 32-bit values. The text"
    print "         file contains one property per line with property data rendered as"
    print "         ASCII/HEX (e.g. 8001 11111111 22222222 33333333 44444444 55555555)"
    print ""
    print "Usage 5: python xio.py <midi_port> <prop_id> <prop_values ...>"
    print "         Write one FlexFX property to the FlexFX device. Each property consists of"
    print "         a 16-bit ID and five 32-bit values. The <prop_id> and <prop_values>"
    print "         represent one property per line with property data rendered as ASCII/HEX"
    print "         (e.g. F001 11111111 22222222 33333333 44444444 55555555)"
    print ""

    midi_list()
    exit(0)

def property_to_midi_sysex( property ):

    midi_data = [0xF0]

    midi_data.append( (property[0] >> 28) & 15 )
    midi_data.append( (property[0] >> 24) & 15 )
    midi_data.append( (property[0] >> 20) & 15 )
    midi_data.append( (property[0] >> 16) & 15 )
    midi_data.append( (property[0] >> 12) & 15 )
    midi_data.append( (property[0] >>  8) & 15 )
    midi_data.append( (property[0] >>  4) & 15 )
    midi_data.append( (property[0] >>  0) & 15 )

    midi_data.append( (property[1] >> 28) & 15 )
    midi_data.append( (property[1] >> 24) & 15 )
    midi_data.append( (property[1] >> 20) & 15 )
    midi_data.append( (property[1] >> 16) & 15 )
    midi_data.append( (property[1] >> 12) & 15 )
    midi_data.append( (property[1] >>  8) & 15 )
    midi_data.append( (property[1] >>  4) & 15 )
    midi_data.append( (property[1] >>  0) & 15 )

    midi_data.append( (property[2] >> 28) & 15 )
    midi_data.append( (property[2] >> 24) & 15 )
    midi_data.append( (property[2] >> 20) & 15 )
    midi_data.append( (property[2] >> 16) & 15 )
    midi_data.append( (property[2] >> 12) & 15 )
    midi_data.append( (property[2] >>  8) & 15 )
    midi_data.append( (property[2] >>  4) & 15 )
    midi_data.append( (property[2] >>  0) & 15 )

    midi_data.append( (property[3] >> 28) & 15 )
    midi_data.append( (property[3] >> 24) & 15 )
    midi_data.append( (property[3] >> 20) & 15 )
    midi_data.append( (property[3] >> 16) & 15 )
    midi_data.append( (property[3] >> 12) & 15 )
    midi_data.append( (property[3] >>  8) & 15 )
    midi_data.append( (property[3] >>  4) & 15 )
    midi_data.append( (property[3] >>  0) & 15 )

    midi_data.append( (property[4] >> 28) & 15 )
    midi_data.append( (property[4] >> 24) & 15 )
    midi_data.append( (property[4] >> 20) & 15 )
    midi_data.append( (property[4] >> 16) & 15 )
    midi_data.append( (property[4] >> 12) & 15 )
    midi_data.append( (property[4] >>  8) & 15 )
    midi_data.append( (property[4] >>  4) & 15 )
    midi_data.append( (property[4] >>  0) & 15 )

    midi_data.append( (property[5] >> 28) & 15 )
    midi_data.append( (property[5] >> 24) & 15 )
    midi_data.append( (property[5] >> 20) & 15 )
    midi_data.append( (property[5] >> 16) & 15 )
    midi_data.append( (property[5] >> 12) & 15 )
    midi_data.append( (property[5] >>  8) & 15 )
    midi_data.append( (property[5] >>  4) & 15 )
    midi_data.append( (property[5] >>  0) & 15 )

    midi_data.append( 0xF7 )
    return midi_data

def midi_sysex_to_property( midi_data ):

    property = [0,0,0,0,0,0]

    if len(midi_data) < 50: return property;
    if midi_data[0] != 0xF0 or midi_data[49] != 0xF7: return property

    property[0] = (midi_data[ 1] << 28) + (midi_data[ 2] << 24) \
                + (midi_data[ 3] << 20) + (midi_data[ 4] << 16) \
                + (midi_data[ 5] << 12) + (midi_data[ 6] <<  8) \
                + (midi_data[ 7] <<  4) + (midi_data[ 8] <<  0)

    property[1] = (midi_data[ 9] << 28) + (midi_data[10] << 24) \
                + (midi_data[11] << 20) + (midi_data[12] << 16) \
                + (midi_data[13] << 12) + (midi_data[14] <<  8) \
                + (midi_data[15] <<  4) + (midi_data[16] <<  0)

    property[2] = (midi_data[17] << 28) + (midi_data[18] << 24) \
                + (midi_data[19] << 20) + (midi_data[20] << 16) \
                + (midi_data[21] << 12) + (midi_data[22] <<  8) \
                + (midi_data[23] <<  4) + (midi_data[24] <<  0)

    property[3] = (midi_data[25] << 28) + (midi_data[26] << 24) \
                + (midi_data[27] << 20) + (midi_data[28] << 16) \
                + (midi_data[29] << 12) + (midi_data[30] <<  8) \
                + (midi_data[31] <<  4) + (midi_data[32] <<  0)

    property[4] = (midi_data[33] << 28) + (midi_data[34] << 24) \
                + (midi_data[35] << 20) + (midi_data[36] << 16) \
                + (midi_data[37] << 12) + (midi_data[38] <<  8) \
                + (midi_data[39] <<  4) + (midi_data[40] <<  0)

    property[5] = (midi_data[41] << 28) + (midi_data[42] << 24) \
                + (midi_data[43] << 20) + (midi_data[44] << 16) \
                + (midi_data[45] << 12) + (midi_data[46] <<  8) \
                + (midi_data[47] <<  4) + (midi_data[48] <<  0)

    return property

def _assert( condition, message ):

    if not condition:
        print message
        exit( 0 )

if len(sys.argv) == 2: # Usage 2

    midi = midi_open( int(sys.argv[1]) )
    while True:
        midi_write( midi, property_to_midi_sysex( [0xFFFF,0,0,0,0,0] ))
        prop = midi_sysex_to_property( midi_wait( midi ))
        print prop
        time.sleep( 0.1 )
    

name = sys.argv[2]

if name[len(name)-4:] == ".bin": # Usage 3 - Burn firmware image to FLASH boot partition

    unit = 0
    if len(sys.argv) > 3: unit = int(sys.argv[3])

    midi = midi_open( int(sys.argv[1]) )
    file = open( sys.argv[2], "rb" )

    sys.stdout.write("Erasing...")
    sys.stdout.flush()
    midi_write( midi, property_to_midi_sysex( [0x0002+3*unit,0,0,0,0,0] ))
    while True:
        prop = midi_sysex_to_property( midi_wait( midi ))
        if prop[0] == 0x0002+3*unit: break

    sys.stdout.write("Writing")
    sys.stdout.flush()
    
    count = 0
    while True:
        line = file.read( 16 )
        if len(line) == 0: break
        while len(line) < 16: line += chr(0)
        data = [ 0x0003+3*unit, \
                 (ord(line[ 0])<<24)+(ord(line[ 1])<<16)+(ord(line[ 2])<<8)+ord(line[ 3]), \
                 (ord(line[ 4])<<24)+(ord(line[ 5])<<16)+(ord(line[ 6])<<8)+ord(line[ 7]), \
                 (ord(line[ 8])<<24)+(ord(line[ 9])<<16)+(ord(line[10])<<8)+ord(line[11]), \
                 (ord(line[12])<<24)+(ord(line[13])<<16)+(ord(line[14])<<8)+ord(line[15]), \
                 0, 0, 0, 0 ]
        sys.stdout.flush()
        midi_write( midi, property_to_midi_sysex( data ))
        while True:
            prop = midi_sysex_to_property( midi_wait( midi ))
            if prop[0] == 0x0003+3*unit: break
        count += 1
        if count == 256:
	        sys.stdout.write(".")
	        count = 0
        
    midi_write( midi, property_to_midi_sysex( [0x0004+3*unit,0,0,0,0,0] ))
        
    file.close()
    midi_close( midi )
    print( "Done." )

elif name[len(name)-4:] == ".txt": # Usage 4

    midi = midi_open( int(sys.argv[1]) )
    file = open( sys.argv[2], "rt" )
    while True:
        
        line = file.readline().split()
        if len(line) < 6: exit(0)
        data = [int(line[0],16),int(line[1],16),int(line[2],16),int(line[3],16), \
                int(line[4],16),int(line[5],16)]
        print "%08x %08x %08x %08x %08x %08x" % (data[0],data[1],data[2],data[3],data[4],data[5])
        
        midi_write( midi, property_to_midi_sysex( data ))
        while True:
            prop = midi_sysex_to_property( midi_wait( midi ))
            if prop[0] == data[0]: break
      
    file.close()
    midi_close( midi )
	
elif len(sys.argv) == 8: # Usage 5

    data = [int(sys.argv[2],16),int(sys.argv[3],16),int(sys.argv[4],16),int(sys.argv[5],16), \
            int(sys.argv[6],16),int(sys.argv[7],16)]
    print "SEND %04x %08x %08x %08x %08x %08x" % (data[0],data[1],data[2],data[3],data[4],data[5])
    midi = midi_open( int(sys.argv[1]) )
    midi_write( midi, property_to_midi_sysex( data ))
    while True:
        prop = midi_sysex_to_property( midi_wait( midi ))
        break #if prop[0] == data[0]: break
    midi_close( midi )
    print "RECV %04x %08x %08x %08x %08x %08x" % (prop[0],prop[1],prop[2],prop[3],prop[4],prop[5])
    
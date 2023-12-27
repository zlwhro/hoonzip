f = open('/home/hoon/fuzz/pdf.zip', 'rb')
data = f.read()
i = -22

while(data[i:i+4] != b'\x50\x4b\x05\x06'):
    i+= 1
nod = int.from_bytes(data[i+4:i+6], byteorder='little')
centstart = int.from_bytes(data[i+6:i+8], byteorder='little')
centnum = int.from_bytes(data[i+8:i+10], byteorder='little')
centnum2 = int.from_bytes(data[i+10:i+12], byteorder='little')
centsize = int.from_bytes(data[i+12:i+16], byteorder='little')
centoffset = int.from_bytes(data[i+16:i+20], byteorder='little')
comment_len = int.from_bytes(data[i+20:i+22], byteorder='little')

# print(f'number of dist: {hex(nod)}')
# print(f'cen start: {hex(centstart)}')
# print(f'number of central directort: {hex(centnum)}')
# print(f'size of central directory {hex(centsize)}')
# print(f'ofset of central directory: {hex(centoffset)}')

i = centoffset

for j in range(centnum):
    check_sig = data[i:i+4] == b'\x50\x4b\x01\x02'
    print(f'check central directory header: {check_sig}')
    i +=4
    version = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'version: {version}')
    i+=2
    min_version = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'min version: {min_version}')
    i+=2
    bit_flag = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'general purpose bit flag: {bit_flag}')
    i+=2

    comp_method = {}
    comp_method[8] = 'DEFLATE'
    comp_method[0] = 'STORE'

    c = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'compresseion method: {comp_method[c]}')
    i+=2

    modtime = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'last modification time: {modtime}')
    i+=2

    moddate = int.from_bytes(data[i:i+2], byteorder='little')
    print(f'last modification date: {moddate}')
    i+=2

    crc = int.from_bytes(data[i:i+4], byteorder='little')
    print(f'crc: {hex(crc)}')
    i+=4

    compsize = int.from_bytes(data[i:i+4], byteorder='little')
    print(f'compressed size: {hex(compsize)}')
    i+=4

    uncompsize = int.from_bytes(data[i:i+4], byteorder='little')
    print(f'uncompressed size: {hex(uncompsize)}')
    i+=4

    file_name_len = int.from_bytes(data[i:i+2], byteorder='little')
    i+=2


    extra_file_len = int.from_bytes(data[i:i+2], byteorder='little')
    i+=2

    file_comment_len = int.from_bytes(data[i:i+2], byteorder='little')
    i+=2

    dis_number = int.from_bytes(data[i:i+2], byteorder='little')
    i+=2

    internal_file_attribute = int.from_bytes(data[i:i+2], byteorder='little')
    i+=2
    print(f'ifa: {internal_file_attribute}')

    external_file_attribute = int.from_bytes(data[i:i+4], byteorder='little')
    i+=4
    print(f'efa: {external_file_attribute}')

    file_header_offset = int.from_bytes(data[i:i+4], byteorder='little')
    i+=4
    print(f'local file header offset: {hex(file_header_offset)}')

    file_name = data[i:i+file_name_len]
    print(f'name: {file_name}')
    i+= file_name_len

    extra_file = data[i:i+extra_file_len]
    i += extra_file_len

    file_comment = data[i:i+file_comment_len]
    i += file_comment_len
    
    print('=============================================================')

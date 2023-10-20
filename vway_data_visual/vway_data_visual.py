import matplotlib.pyplot as plt 
from matplotlib.ticker import MultipleLocator
import re
import sys
import os

def file_data_draw(fn_list, pos_pair):
    """
    fn_list: a list of file names indicating files that contain data in
        the same format: text file, each line is a string of hex bytes.
    pos_pair: a (start, end) tuple, with "start" indicating the first byte
        of intersted data, and "end" indicating the next one after
        the last intersted byte.  

    Return:
        Return a list, every element of which is a list of data
        which forms a plot.
    """
    rmx_ptn = re.compile(r'(0[x|X])|(\\[x|X])') #used to remove '0x', '0X', '\x', '\X'
    rmnh_ptn = re.compile(r'[^0-9a-fA-F]') #used to remove non-hex digits
    bs, be = pos_pair
    f_d_list = []
    for fn in fn_list:
        data_file = open(fn, "r")
        line_no = 1
        f_d_e_list = []
        for file_line in data_file:
            file_line = file_line.splitlines()[0]
            file_line = rmnh_ptn.sub('', rmx_ptn.sub('', file_line)).upper()
            line_len = len(file_line)
            if line_len %2:
                file_line = file_line[0 : line_len - 1] + '0' + file_line[line_len - 1]
            byte_num = int(len(file_line)/2)
            if (not(bs in range(byte_num))) or (not(be in range(byte_num+1))) or (be <= bs):
                err_s ='file {}, line {}, pos error: {} bytes, bs:{}, be:{}. Set to 0'.format(fn,
                        line_no, byte_num, bs, be)
                print(err_s)
                data_str = '00'
            else:
                data_str = file_line[(bs * 2) : (be * 2)]
            f_d_e_list.append(int(data_str, 16))
            line_no += 1
        f_d_list.append(f_d_e_list)
        data_file.close()

    return f_d_list 

def data_visual(d_list):
    x_nums = tuple(len(l) for l in d_list)
    x_num = max(x_nums)
    fig, ax = plt.subplots()
    ax.xaxis.set_major_locator(MultipleLocator(1.0))
    for d in d_list:
        if len(d) < x_num:
            d += [0 for i in range(x_num - len(d))]
        ax.plot(range(1, x_num+1), d, '.-')
        #ax.plot(d, '.-')
    plt.show()

help_str = "Usage: vway_data_visual.py folder|txt_file [bs be]"
if len(sys.argv) < 2 or (len(sys.argv) != 2 and len(sys.argv) != 4):
    print(help_str)
    sys.exit(0)

tgt = sys.argv[1]
fn_list = []
if os.path.isdir(tgt):
    triples = (list(os.walk(tgt))[0],)
    for pn, dn, fn in triples:
        for f in fn:
            fn_list.append(pn + '/' + f)
elif os.path.isfile(tgt):
    fn_list.append(tgt) 
else:
    print(help_str)
    sys.exit(-1)

byte_s = 8
byte_e = 11
if(len(sys.argv) == 4):
    byte_s = int(sys.argv[2])
    byte_e = int(sys.argv[3])

file_data_list = file_data_draw(fn_list, (byte_s, byte_e))
data_visual(file_data_list)

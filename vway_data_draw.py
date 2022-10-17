import os
import sys
import re

dect_pos_map = ['位置', '额头', '左脸颊', '右脸颊']
light_lambda_arr = [354, 376, 405, 450, 505, 560, 600, 645, 680, 705, 800, 850, 910, 930, 970, 1000, 1050, 1070, 1200]
_default_pos_pair = (8, 11) #the 8th to 10th byte in a line read from device.
def draw_data_from_file(fn, pos_pair = _default_pos_pair):
    rmx_ptn = re.compile(r'(0[x|X])|(\\[x|X])') #used to remove '0x', '0X', '\x', '\X'
    rmnh_ptn = re.compile(r'[^0-9a-fA-F]') #used to remove non-hex digits
    bs, be = pos_pair
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
    data_file.close()
    return f_d_e_list

_KEY_STR_TYPE = 'type'
_KEY_STR_NO = 'no.'
_KEY_STR_POS = 'pos'
_KEY_STR_DATE = 'date'
_KEY_STR_TIME = 'time'
_KEY_STR_ZZZ = 'zzz' #milion-second str.
_KEY_STR_DATA = 'data'
_KEY_STR_DATUM = 'datum'
def parse_file_name(fn):
    """
    two kinds of file name:
    1) data_no + "---" + YYYYMMDD + "-" + HHmmSS + "-" + zzz + ".txt"
    2) data_no + "_" + pos + "---" + YYYYMMDD + "-" + HHmmSS + "-" + zzz + ".txt"
    For 1), pos is taken as an empty string.

    Returned:
        A tuple of two element: the first is number string, the second is a dictory containing pos, date, time and ms.
    """
    no_pos_s, sep, remains = fn.rpartition('---')
    no_s, sep, pos_s = no_pos_s.partition('_')
    dtms_s = remains.split('.')[0]
    date_s, time_s, zzz_s = dtms_s.split('-')
    date_s = date_s[:4] + '/' + date_s[4:6] + '/' + date_s[6:]
    time_s = time_s[:2] + ':' + time_s[2:4] + ':' + time_s[4:]
    return (no_s, {_KEY_STR_POS: pos_s,
                   _KEY_STR_DATE: date_s,
                   _KEY_STR_TIME: time_s,
                   _KEY_STR_ZZZ: zzz_s
                   }
            )

def gen_datum_dict(pt, byte_pos_pair):
    """
    Params:
        pt: full folder path containing files from which data are drawn.
        byte_pos_paiir: start and end (last + 1) of byte postion in a line read from device.
    Returned:
        A dictionary:
            [_KEY_STR_TYPE]: the type of these data, e.g. 敏感型... we take this info from pt.
            [_KEY_STR_DATUM]: a dictionary containing datum from files:
                [_KEY_STR_NO]: a list containing all data of a specific number:
                    [i]: a dictionary containing information and data of one file:
                        [_KEY_STR_POS]: where is the data collected from. see dect_pos_map.
                        [_KEY_STR_DATE]: date of the data collected.
                        [_KEY_STR_TIME]: time of the data collected.
                        [_KEY_STR_ZZZ]: milion second of the data collected.
                        [_KEY_STR_DATA]: a list of integer, the actual data bytes read from device.
    """
    datum_dict = {} #[_KEY_STR_DATUM]
    pt = pt.replace('\\', '/')
    type_str = os.path.split(pt)[1]
    pth, folder_s_l, file_s_l = (list(os.walk(pt)))[0]
    file_s_l.sort()
    for file_s in file_s_l:
        no_s, rec_dict = parse_file_name(file_s)
        if('' == no_s): continue
        if(not(no_s in datum_dict.keys())):
            datum_dict[no_s] = []
        if("" == rec_dict[_KEY_STR_POS]):
            pos_idx = len(datum_dict[no_s]) + 1
            if(not(pos_idx in range(1, len(dect_pos_map)))):
                pos_str = '{}{:02d}'.format(dect_pos_map[0], pos_idx)
            else:
                pos_str = dect_pos_map[pos_idx]
            rec_dict[_KEY_STR_POS] = pos_str

        data_info = draw_data_from_file(pth + '/' + file_s, byte_pos_pair) 
        rec_dict[_KEY_STR_DATA] = data_info
        datum_dict[no_s].append(rec_dict)
    return {_KEY_STR_TYPE: type_str, _KEY_STR_DATUM : datum_dict}

def write_to_csv(pth, d_d):
    """
    Parameters:
        pth: full path to write files into.
        d_d: a dictionary in the format of value returned by gen_datum_dict.
    Action:
        Every [_KEY_STR_NO] datum are written into to one file named no_s.
    """
    type_str = d_d[_KEY_STR_TYPE]
    for (no_s, data_d) in d_d[_KEY_STR_DATUM].items():
        csv_name = no_s + '.csv'
        fpn = pth + '/' + csv_name
        f = open(fpn, 'w')
        print('样本编号,' + no_s + ',采集日期,' + data_d[0][_KEY_STR_DATE] + ',类型,' + type_str, file = f)
        print(',采集时间,', end = '', file = f)
        s = ''
        for d in data_d: s += d[_KEY_STR_TIME] + ','
        print(s, file = f)
        print('光源编号, 波长（nm）,额头,左脸颊,右脸颊', file = f)
        dl = [[i+1 for i in range(len(light_lambda_arr))]] \
            + [light_lambda_arr] \
            + [d[_KEY_STR_DATA] for d in data_d]
        d_lines = zip(*dl)
        for d_line in d_lines:
            s = ''
            for e in d_line: s += str(e) + ','
            print(s, file = f)
        f.close()

#############################################################################
help_str = "Usage: vway_data_draw.py folder [bs be]"
if len(sys.argv) < 2 or (len(sys.argv) != 2 and len(sys.argv) != 4):
    print(help_str)
    sys.exit(0)
tgt_path = sys.argv[1]
byte_pos_pair = _default_pos_pair 
if(len(sys.argv) == 4):
    byte_pos_pair = sys.argv[2:]
datum_drawn = gen_datum_dict(tgt_path, byte_pos_pair)
p_p, c_f = os.path.split(tgt_path)
p_p = p_p.replace('\\', '/')
result_folder_apx = "_数据"
c_f += result_folder_apx 
os.mkdir(c_f)
write_to_csv(p_p + '/' + c_f, datum_drawn)


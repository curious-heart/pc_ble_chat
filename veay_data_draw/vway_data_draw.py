import os
import sys
import re
import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.ticker import MultipleLocator
from cosmetic_db_headers import *
import sqlite3

mpl.rcParams['font.family'] = 'SimHei'  #For chinese display
plt.rcParams['axes.unicode_minus'] = False

dect_pos_map = ['位置', '额头', '左脸颊', '右脸颊']
light_label_str = '光源波长(nm)'
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
    parent_str, type_str = os.path.split(pt)
    if(os.path.isdir(pt)):
        pth, _, file_s_l = (list(os.walk(pt)))[0]
    elif (os.path.isfile(pt)):
        pth, file_s_l = parent_str, [type_str] 
    else:
        print("INVALID file/folder in command line: " + f + "----> skipped")
        return {_KEY_STR_TYPE: type_str, _KEY_STR_DATUM : datum_dict}

    file_s_l.sort()
    for file_s in file_s_l:
        print(file_s)
        no_s, rec_dict = parse_file_name(file_s)
        if('' == no_s): 
            print('{} --> has no number, skipped...'.format(file_s))
            continue
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

def write_to_csv(pth, d_d, single_file = False):
    """
    Parameters:
        pth: full path to write files into.
        d_d: a dictionary in the format of value returned by gen_datum_dict.
        single_file: indicating if d_d containing only single file. If so, the file name should contain pos str to
            avoid duplication in the same folder.
    Action:
        Every [_KEY_STR_NO] datum are written into to one file named no_s.
    """
    type_str = d_d[_KEY_STR_TYPE]
    for (no_s, data_d) in d_d[_KEY_STR_DATUM].items():
        csv_name = (no_s + '_' + data_d[0][_KEY_STR_POS] if single_file else no_s) + '.csv'
        fpn = pth + '/' + csv_name
        f = open(fpn, 'w')
        print('样本编号,' + no_s + ',采集日期,' + data_d[0][_KEY_STR_DATE] + ',类型,' + type_str, file = f)
        print(',采集时间,', end = '', file = f)
        s = ''
        for d in data_d: s += d[_KEY_STR_TIME] + ','
        print(s, file = f)
        print('光源编号,波长（nm）,', file = f, end = '')
        for d in data_d: print(d[_KEY_STR_POS] + ',', file = f, end = '')
        #print('光源编号, 波长（nm）,额头,左脸颊,右脸颊', file = f)
        print('', file = f)
        dl = [[i+1 for i in range(len(light_lambda_arr))]] \
            + [light_lambda_arr] \
            + [d[_KEY_STR_DATA] for d in data_d]
        d_lines = zip(*dl)
        for d_line in d_lines:
            s = ''
            for e in d_line: s += str(e) + ','
            print(s, file = f)
        f.close()

def point_data_display(d_arr_list, line_list, x_base = 0, y_base = 0):
    """
    Parameters:
        d_arr_list: a list, every element is a data array (list).
        line_list: a list containg Line2D obj.
        x_base, y_base: base coordinate for each text position.
    Assumation: all data element array has the same number of elements: len(light_lambda_arr)
    """
    x_off_s = [0, 0, 0]
    y_cords_base = [y + y_base for y in map(min, zip(*d_arr_list)) ]
    x_cords_base = [x + x_base for x in range(1, len(d_arr_list[0]) + 1)]
    arr_idx = 0
    for d_arr, line in zip(d_arr_list, line_list):
        t_color = line.get_color()
        e_idx = 0
        for x, y in zip(x_cords_base, d_arr):
            plt.text(x + x_off_s[arr_idx%len(x_off_s)], y, str(d_arr[e_idx]), color = t_color)
            e_idx += 1
        arr_idx += 1

def add_table_to_plot(no_dict, no_lines = [], font_size = 10):
    """
    Add table to plot.
    Parameters:
        no_dict: A list. refer to write_to_csv.
    """
    row_head = [light_label_str] + [str(l) for l in light_lambda_arr]
    tbl_rows = [row_head]
    for d in no_dict:
        pos_s = d[_KEY_STR_POS]
        tbl_rows.append([pos_s] + [str(e) for e in d[_KEY_STR_DATA]])
    the_table = plt.table(cellText = tbl_rows, loc='top', fontsize = font_size)
    the_table.auto_set_font_size(False)
    the_table.set_fontsize(font_size)
    if(len(no_lines) > 0):
        row_txt_colors = [l.get_color() for l in no_lines]
        for r in range(len(tbl_rows) - 1):
            for c in range(len(row_head)):
                the_table[r + 1, c].set_text_props(color = row_txt_colors[r])

fig_size_px_w = 1280
fig_size_px_h = 960
def resize_fig(fig):
    fig_dpi = fig.dpi
    fig.set_size_inches(fig_size_px_w/fig_dpi,fig_size_px_h/fig_dpi)

def paint_datum(pth, d_d, only_1 = False):
    """
    Refer to write_to_csv function for the meaning of parameters.
    Generate png image file for each element of no_s, each nos, and the whole d_d.
    Assumation: all data element array has the same number of elements: len(light_lambda_arr)
    """
    type_str = d_d[_KEY_STR_TYPE]
    xticks_labels = [str(e) for e in light_lambda_arr]
    ele_num = len(light_lambda_arr) 
    xticks = list(range(1, ele_num + 1))
    if not only_1:
        all_fig, all_ax = plt.subplots()
        all_ax.set_xlabel(light_label_str)
        resize_fig(all_fig)
        all_ax.xaxis.set_major_locator(MultipleLocator(1))
    for (no_s, data_d) in d_d[_KEY_STR_DATUM].items():
        if not only_1:
            no_fig, no_ax = plt.subplots()
            no_xlabel = no_ax.set_xlabel(light_label_str)
            resize_fig(no_fig)
            no_ax.xaxis.set_major_locator(MultipleLocator(1))
        x_off = y_off = 0
        no_lines = []
        no_data_arr_list = []
        for d in data_d:
            d_arr = d[_KEY_STR_DATA]
            d_fig, d_ax = plt.subplots()
            resize_fig(d_fig)
            d_ax.set_xlabel(light_label_str)
            plt.xticks(xticks, xticks_labels, rotation = 30)
            d_line = d_ax.plot(xticks, d_arr, '.-', label = d[_KEY_STR_POS])
            d_ax.legend()
            point_data_display([d_arr], d_line)
            #-------------------
            d_img_n = '{}-{}.png'.format(no_s, d[_KEY_STR_POS])
            d_fig.savefig(pth + '/' + d_img_n)
            plt.close(d_fig)
           
            if not only_1:
                no_data_arr_list.append(d_arr)
                no_line = no_ax.plot(range(1, ele_num + 1), d_arr, '.-', label = d[_KEY_STR_POS])
                no_lines.append(no_line[0])
                plt.xticks(xticks, xticks_labels, rotation = 30)
                no_ax.legend()
                point_data_display(no_data_arr_list, no_lines)
                y_off += 1
                all_ax.plot(range(1, ele_num + 1), d_arr, '.-', label = d[_KEY_STR_POS])
        if not only_1:
            add_table_to_plot(data_d, no_lines, no_xlabel.get_fontsize())
            #-------------------
            no_img_n = '{}.png'.format(no_s)
            no_fig.savefig(pth + '/' + no_img_n)
            plt.close(no_fig)

    if not only_1:
        all_img_n = '{}.png'.format(type_str)
        all_fig.savefig(pth + '/' + all_img_n)
        plt.close(all_fig)
def connect_db(db_fpn):
    """
    Return a connection and a cursor.
    """
    conn = sqlite3.connect(db_fpn)
    cur = conn.cursor()
    return (conn, cur)

def create_db_tbl(conn, cur, tbl_name, headers, pks = [], exist_check = True):
    """
    tbl_name: string
    headers: dictionary, col_name: data_type
    pks: list of string

    CREATE TABLE IF NOT EXISTS tbl_name (headers[i], PRIMARY KEY(pks));
    """
    db_cmd_str = "CREATE TABLE" + " " + ("IF NOT EXISTS" if exist_check else "") + " " + tbl_name
    col_name_type_str = ','.join([col_name + ((" " + data_type) if data_type else "") \
                                for (col_name, data_type) in headers.items()])
    primary_keys_str = ("PRIMARY KEY" + "(" + ",".join(pks) + ")") if pks else ""
    db_cmd_str = db_cmd_str + " (" + col_name_type_str + "," + primary_keys_str + ");" 
    #db_cmd_str = '"' + db_cmd_str + '"'
    cur.execute(db_cmd_str)

def write_to_db_tbl(conn, cur, tbl_name, cols, vals, commit = True):
    """
    cols: list of col names. may be empty.
    vals: list of list, each is a series of values.
    """
    if len(vals) <= 0: return
    db_cmd_str = "INSERT INTO" + " " + tbl_name
    col_str = ("(" + ",".join(cols) + ")") if cols else ""
    #value_str_arr = [("(" + (','.join(d)) + "),") for d in vals]
    value_str_arr = vals 
    place_h_str = "(" + ("?," * len(vals[0]))[:-1] + ")"
    db_cmd_str = db_cmd_str + " " + col_str + " " + "VALUES" + " " + place_h_str
    cur.executemany(db_cmd_str, value_str_arr)
    if commit: conn.commit()

def close_db(conn, commit = True):
    if commit: conn.commit()
    conn.close()

def write_dict_to_db(con, cur, tbl_name, tbl_headers, datum_drawn):
    """
    Return True or False
    """
    tbl_lines = []
    skin_type = datum_drawn[_KEY_STR_TYPE]
    for no_s, datum in datum_drawn[_KEY_STR_DATUM].items():
        for d in datum:
            tbl_line = [no_s, skin_type]
            tbl_line.append(d[_KEY_STR_POS])
            tbl_line = tbl_line + d[_KEY_STR_DATA]
            tbl_line = tbl_line + [d[_KEY_STR_DATE], d[_KEY_STR_TIME]]
            tbl_lines.append(tbl_line)
    write_to_db_tbl(con, cur, tbl_name, tbl_headers, tbl_lines)
#############################################################################
if __name__ == '__main__':
    print('begin...')
    print('')
    _ARG_S_DRAW_DATA = 'draw_data'
    _ARG_S_PAINT_DATA = 'paint_data'
    _ARG_S_FD_LIST = 'fd_list'
    _ARG_S_BYTE_POS_START = 'byte_pos_start'
    _ARG_S_BYTE_POS_END = 'byte_pos_end'
    _ARG_S_WRITE_DB = 'write_db'

    parser = argparse.ArgumentParser()
    parser.add_argument('-d', dest=_ARG_S_DRAW_DATA, action="store_true", help = "draw data from files list")
    parser.add_argument('-p', dest=_ARG_S_PAINT_DATA, action="store_true", help = "paint data from file list")
    parser.add_argument(_ARG_S_FD_LIST, nargs='+', help = "A list of files or/and folders, all files in that \
                                                        folder are processed")
    parser.add_argument('--' + _ARG_S_BYTE_POS_START,
            default = _default_pos_pair[0], help = "indicating the start byte to be drawn")
    parser.add_argument('--' + _ARG_S_BYTE_POS_END, 
            default = _default_pos_pair[1], help = "indicating the (end + 1) byte to be drawn")

    parser.add_argument('--' + _ARG_S_WRITE_DB, default = "", help = "write data into database")

    cmd_args = vars(parser.parse_args())
    draw_data = cmd_args[_ARG_S_DRAW_DATA]
    paint_data = cmd_args[_ARG_S_PAINT_DATA]
    fd_list = cmd_args[_ARG_S_FD_LIST]
    write_db = cmd_args[_ARG_S_WRITE_DB]
    if not (draw_data or paint_data or write_db): paint_data = True
    byte_pos_pair = (cmd_args[_ARG_S_BYTE_POS_START], cmd_args[_ARG_S_BYTE_POS_END])
    #print(cmd_args)
    #print(byte_pos_pair)

    result_data_fd_apx = "_数据"
    result_img_fd_apx = "_图像"
    result_db_apx = "_database"
    all_files_list = []
    plt.rcParams.update({'figure.autolayout': True}) 
    if write_db:
        db_fpn = write_db + '/' + cosmetic_db_name
        con, cur = connect_db(db_fpn)
        if not con or not cur:
            print("Error: connect to {} fail!".format(db_fpn))
            write_db = ""
        create_db_tbl(con, cur, full_datum_tbl_name, full_datum_tbl_headers,full_datum_tbl_pks)   

    for tgt in fd_list:
        if os.path.isdir(tgt):
            datum_drawn = gen_datum_dict(tgt, byte_pos_pair)
            if draw_data:
                result_pth = tgt + result_data_fd_apx
                if(not os.path.isdir(result_pth)):
                    os.mkdir(result_pth)
                write_to_csv(result_pth, datum_drawn)
            if paint_data:
                result_pth = tgt + result_img_fd_apx
                if(not os.path.isdir(result_pth)):
                    os.mkdir(result_pth)
                paint_datum(result_pth, datum_drawn)
            if write_db:
                write_dict_to_db(con, cur, full_datum_tbl_name, full_datum_tbl_headers,datum_drawn)
        elif os.path.isfile(tgt):
            datum_drawn = gen_datum_dict(tgt, byte_pos_pair)
            result_pth = os.path.split(tgt)[0]
            if draw_data:
                write_to_csv(result_pth, datum_drawn, True)
            if paint_data:
                paint_datum(result_pth, datum_drawn, True)
            if write_db:
                write_dict_to_db(con, cur, full_datum_tbl_name, full_datum_tbl_headers,datum_drawn)
        else:
            print("INVALID file/folder in command line: " + tgt + "----> skipped")
            continue

    if write_db:
        close_db(con)

    print('')
    print('end.')

# -*- coding: utf-8 -*-

import struct
import input_parser


REQUEST_PIPE = '../pipe_A'
RESPONSE_PIPE = '../pipe_B'


NEW_REQ = bytes(chr(0), encoding='ascii')
EXIST_REQ = bytes(chr(1), encoding='ascii')


OK = bytes(chr(0), encoding='ascii')
OK_PAYLOAD = bytes(chr(1), encoding='ascii')
BAD = bytes(chr(2), encoding='ascii')


MAX_CNT_REQUEST_VAR = 20


REQ_ID = dict()
NEXT_ID = 0


def get_response():
    fin = open(RESPONSE_PIPE, 'rb')

    buffer = fin.read()
    code = struct.unpack_from('c', buffer)[0]

    return_payload = None
    if code == OK:
        print('OK')
        return_payload = []
    elif code == OK_PAYLOAD:
        print('OK_PAYLOAD')
        size = struct.calcsize('=I')
        l = struct.unpack_from('=I', buffer, 1)[0]
        docId = [struct.unpack_from('=I', buffer, 1 + (i + 1) * size)[0] for i in range(l)]
        return_payload = docId
    elif code == BAD:
        print('BAD')
    else:
        print("Bad response code '{}'".format(code))
    
    fin.close()

    return return_payload


def do_request(s):
    if len(s) == 0:
        print('Empty string')
        return False

    global REQ_ID, NEXT_ID

    if s in REQ_ID:
        id = REQ_ID[s]
        data = struct.pack('=cI', EXIST_REQ, id)
    else:
        REQ_ID[s] = NEXT_ID
        NEXT_ID += 1
        s = bytes(s, encoding='utf-8')
        data = struct.pack('=cI{}s'.format(len(s)), NEW_REQ, len(s), s)

    fout = open(REQUEST_PIPE, 'wb')

    fout.write(data)

    fout.close()

    return True


if __name__ == '__main__':
    pass

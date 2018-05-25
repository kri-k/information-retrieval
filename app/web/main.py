#! /usr/bin/python3
# -*- coding: utf-8 -*-

import os
import time
from flask import Flask, request, render_template
import unicodedata
import pymorphy2

import client
import input_parser
from snippets import get_snippet


DOC_ID = dict()
RESPONSE_BLOCK_SIZE = 50


DOC_TITLES_FILE = '/home/forlabs/wiki_index/docs'
TERMS_FILE = '/home/forlabs/wiki_index/terms'
PATHS_FILE = '/home/forlabs/wiki_index/paths'


TERM_TO_ID = dict() 
ID_TO_DOC_TITLE = None
DOC_PATH = dict()


with open(DOC_TITLES_FILE, 'r', encoding='utf-8') as fin:
    ID_TO_DOC_TITLE = [line.strip('\n') for line in fin]
    print('LOADED DOC TITLES')


with open(TERMS_FILE, 'r', encoding='utf-8') as fin:
    index = 0
    for line in fin:
        line = line.strip('\n')
        TERM_TO_ID[line] = index
        index += 1
    print('LOADED TERMS LIST')


with open(PATHS_FILE, 'r', encoding='utf-8') as fin:
    for line in fin:
        f, p = line.strip('\n').split(' ')
        DOC_PATH[f] = os.path.join(p, f)
    print('LOADED DOC PATHS')


morph = pymorphy2.MorphAnalyzer()


def remove_accents(input_str):
    nfkd_form = unicodedata.normalize('NFKD', input_str)
    return u"".join([c for c in nfkd_form if not unicodedata.combining(c)])


def normalize(text, id=True):
    res = []
    for i in text:
        if i in ('&', '|', '!'):
            res.append(i)
        else:
            # i = remove_accents(morph.parse(i.strip('\n'))[0].normal_form).lower()
            i = remove_accents(i).lower();
            if id:
                if '"' == i[0] == i[-1]:
                    l = []
                    for j in i.split():
                        if j[0] not in '"/':
                            if j not in TERM_TO_ID:
                                return None
                            l.append(str(TERM_TO_ID[j]))
                        else:
                            l.append(j)
                    res.append(' '.join(l))
                else:
                    if i not in TERM_TO_ID:
                        return None
                    res.append(str(TERM_TO_ID[i]))
            else:
                res.append(i)
    return res


def generate_buttons(page, delta, text):
    btn = []
    start = max(0, page - delta // 2)
    text = text.replace('&', '%26').replace('|', '%7C')
    for i in range(start, start + delta):
        s = str(i + 1)
        btn.append((s, '/search?text={}&p={}'.format(text, i)))
        if len(btn) == delta:
            return btn
    return btn


app = Flask(__name__)

@app.route('/', methods=['GET'])
@app.route('/index', methods=['GET'])
def index():
    return render_template('index.html')


@app.route('/search', methods=['GET'])
def search():
    if request.method == 'GET':
        start_time = time.time()

        timing = lambda: '{:.6f}'.format(time.time() - start_time)

        text = request.args.get('text')

        err = ''
        if text is None or len(text) == 0:
            err = 'Задан пустой запрос'
        elif len(text) > 1000:
            err = 'Превышена максимальная длина в 1000 символов'

        if len(err):
            return render_template('search.html', error=err, time=timing())

        clear_text = ' '.join(normalize(input_parser.get_tokens(text), id=False))
        print(clear_text)

        err = ''
        cnt, text = input_parser.get_postfix_expression(text)
        if cnt is None:
            err = "Bad expression"
        elif cnt > client.MAX_CNT_REQUEST_VAR:
            err = "Max words count exceeded"

        if len(err):
            return render_template('search.html', error=err, time=timing())

        text = normalize(text)
        if text is None:
            return render_template('search.html', error="Ничего не найдено ┐('～`;)┌", time=timing())

        text = ' '.join(text)

        print(text)

        p = request.args.get('p')
        if p is None:
            p = 0
        else:
            p = int(p)

        if text not in DOC_ID:
            client.do_request(text)
            client.get_response()
            DOC_ID[text] = []

        l = DOC_ID[text]

        max_p = len(l) // RESPONSE_BLOCK_SIZE + (1 if len(l) % RESPONSE_BLOCK_SIZE else 0) - 1

        if p > max_p:
            if len(l) % RESPONSE_BLOCK_SIZE != 0:
                return render_template('search.html', error='Ничего не найдено (＞﹏＜)', time=timing())

            for _ in range(max_p + 1, p + 1):
                client.do_request(text)
                res = client.get_response()

                if res is None:
                    break
                if len(res) == 0:
                    break
                l.extend(res)

        print(l)

        max_p = len(l) // RESPONSE_BLOCK_SIZE + (1 if len(l) % RESPONSE_BLOCK_SIZE else 0) - 1
        if p > max_p:
            return render_template('search.html', error='Ничего не найдено ¯\_(ツ)_/¯', time=timing())

        request_list = list(s for s in clear_text.split() if s not in '&|"' and not s.startswith('/'))
        res = [
            {
                'title': ID_TO_DOC_TITLE[i],
                'snippet': get_snippet(request_list, DOC_PATH[ID_TO_DOC_TITLE[i]], lambda s: remove_accents(s).lower())
            } 
            for i in l[p * RESPONSE_BLOCK_SIZE:(p + 1) * RESPONSE_BLOCK_SIZE]
        ]

        return render_template(
            'search.html', 
            results=res,
            txt=clear_text, 
            btn=generate_buttons(p, 5, request.args.get('text')),
            time=timing())


@app.route('/archive/<title>')
def archive(title):
    if title not in DOC_PATH:
        return index()

    with open(DOC_PATH[title], 'r', encoding='utf-8') as fin:
        text = fin.read()

    return text


if __name__ == '__main__':
    app.run(debug=False)

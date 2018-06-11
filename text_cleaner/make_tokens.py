import sys
import os
import unicodedata
import re
import time


RE_DOC_TITLE = re.compile(r'<doc\s+id="(\d+?)"\s+url="(.+?)"\s+title="(.+?)">(.+?)<\/doc>', flags=(re.M | re.S | re.U))
FILES_PER_DIR = 10000


def remove_accents(input_str):
    nfkd_form = unicodedata.normalize('NFKD', input_str)
    return u''.join(c for c in nfkd_form if not unicodedata.combining(c))


def normalize(token):
    return token.lower().strip('\n\t\r ')


def tokenize(input_str):
    input_str = remove_accents(input_str)
    token = []
    for c in input_str:
        if c.isalnum():
            token.append(c)
        elif len(token):
            token = normalize(''.join(token))
            yield token
            token = []
    if len(token):
        token = normalize(''.join(token))
        yield token


def split_to_documents(input_str):
    heads = []
    docs = []
    for m_obj in RE_DOC_TITLE.finditer(input_str):
        id, link, title, text = m_obj.groups()
        heads.append({'id': id, 'link': link, 'title': title})
        docs.append(text)

    return heads, docs


if __name__ == '__main__':
    dst_base = sys.argv[1]
    src = sys.argv[2:]

    total_token_cnt = 0
    total_length = 0
    total_cnt = 0
    total_time = 0

    fout_meta = open(os.path.join(dst_base, 'meta'), 'w', encoding='utf-8')

    for cur_src in src:
        for path, dirs, files in os.walk(cur_src):
            start_t = time.time()
            proc_files = False

            for file in files:
                proc_files = True
                with open(os.path.join(path, file), 'r', encoding='utf-8') as fin:
                    heads, docs = split_to_documents(fin.read())
                
                for i, h in enumerate(heads):
                    if total_cnt % FILES_PER_DIR == 0:
                        cur_dst_dir = os.path.join(dst_base, str(total_cnt // FILES_PER_DIR))
                        os.mkdir(cur_dst_dir)

                    with open(os.path.join(cur_dst_dir, h['id']), 'w', encoding='utf-8') as fout:
                        for token in tokenize(docs[i]):
                            print(token, file=fout)
                            total_length += len(token)
                            total_token_cnt += 1

                    print(h['id'], h['link'], h['title'], os.path.join(path, file), file=fout_meta, sep='\t')

                    total_cnt += 1

            if proc_files:
                end_t = time.time()
                total_time += end_t - start_t
                print("{}: {} m".format(path, round((end_t - start_t) / 60, 3)))

    fout_meta.close()

    print('Total time: {} s'.format(total_time))
    print('Total docs:', total_cnt)
    print('Average token length:', total_length / total_token_cnt)
    print('Average tokens per doc:', total_token_cnt / total_cnt)

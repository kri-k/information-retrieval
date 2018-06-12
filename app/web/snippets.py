# -*- coding: utf-8 -*-
from collections import Counter, defaultdict
import re


MAX_SNIPPET_LENGTH = 300
DELTA_INIT = 1
DELTA_STEP = 5


def get_snippet(query, id, text_path, lemmatize):
    with open(text_path, 'r') as f:
        pure_text = f.read().replace('\n', ' ')

    pure_text = re.search(
        r'<doc\s+id="{0}"\s+url=".+?"\s+title=".+?">(.+?)<\/doc>'.format(id), 
        pure_text, 
        flags=(re.M | re.S | re.U)).group(1)

    cnt = Counter()
    for s in query:
        cnt[s] = 0

    text = []
    targets = []
    target_cnt = 0

    pos = 0
    word = []
    sep = False
    for c in pure_text:
        if c.isalnum():
            sep = False
            word.append(c)
        else:
            if not sep:
                sep = True
                s = lemmatize(''.join(word))
                if s in cnt:
                    if cnt[s] == 0:
                        target_cnt += 1
                        cnt[s] = target_cnt
                    # word.insert(0, '<b>')
                    # word.append('</b>')
                    targets.append((pos, cnt[s]))
            if len(word):
                text.append(''.join(word))
                word = []
                pos += 1
            text.append(c)
            pos += 1

    if len(word):
        s = lemmatize(''.join(word))
        if s in cnt:
            if cnt[s] == 0:
                target_cnt += 1
                cnt[s] = target_cnt
            # word.insert(0, '<b>')
            # word.append('</b>')
            targets.append((pos, cnt[s]))
        text.append(''.join(word))

    if target_cnt == 0:
        print('EMPTY SNIPPET for "{}"'.format(query))
        print('file', text_path)
        return ' '.join(text[:50] + ['...'])

    min_range = len(text) + 1
    best_l = -1
    best_r = -1

    cur_cnt = 0
    cnt = Counter()

    l = 0
    r = -1
    while True:
        if cur_cnt == target_cnt:
            if targets[r][0] - targets[l][0] < min_range:
                min_range = targets[r][0] - targets[l][0]
                best_l, best_r = l, r
            cnt[targets[l][1]] -= 1
            if cnt[targets[l][1]] == 0:
                cur_cnt -= 1
            l += 1
        else:
            if r < len(targets) - 1:
                r += 1
                if cnt[targets[r][1]] == 0:
                    cur_cnt += 1
                cnt[targets[r][1]] += 1
            else:
                break

    delta = DELTA_INIT
    best_snippet = []
    q = set(query)

    while True:
        snippet = []
        if targets[best_l][0] - delta > 0:
            snippet.append('...')
        snippet += text[max(targets[best_l][0] - delta, 0) : min(targets[best_r][0] + delta + 1, len(text))]
        if targets[best_r][0] + delta + 1 < len(text):
            snippet.append('...')

        cutted_snippet = []
        if targets[best_l][0] - delta > 0:
            cutted_snippet.append('...')
        d = delta
        for i in range(max(targets[best_l][0] - delta, 0), min(targets[best_r][0] + delta + 1, len(text))):
            if lemmatize(text[i]) in q:
                d = delta
                cutted_snippet.append(text[i])
                continue
            if d > 0:
                cutted_snippet.append(text[i])
                d -= 1
                if d == 0:
                    cutted_snippet.append('...')

        la = len(''.join(snippet))
        lb = len(''.join(cutted_snippet))
        if lb <= MAX_SNIPPET_LENGTH:
            best_snippet = cutted_snippet
        if la <= MAX_SNIPPET_LENGTH:
            best_snippet = snippet

        if lb >= min(MAX_SNIPPET_LENGTH, len(pure_text)):
            break

        delta += DELTA_STEP

    if len(best_snippet) == 0:
        print('EMPTY BEST_SNIPPET for "{}"'.format(query))
        print('file', text_path)
        return ' '.join(text[:50] + ['...'])

    return ''.join(best_snippet)


if __name__ == '__main__':
    pass
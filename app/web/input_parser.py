# -*- coding: utf-8 -*-

import re


QUOTE_RE = re.compile(r'\"(.+?)\"(?:\s*(/\d+))?')


def is_char(c):
    return c not in ' &|!()'


def add_explicit_conjunction(res):
    if len(res) and (is_char(res[-1][-1]) or res[-1][-1] == ')'):
        res.append('&')


def get_tokens(s):
    s = ' '.join(s.split()).replace('&&', '&').replace('||', '|')
    res = []

    while True:
        match = QUOTE_RE.search(s)
        if match is None:
            tokens = _get_tokens(s)
            if len(tokens) and (is_char(tokens[0][0]) or tokens[0][0] in '!('):
                add_explicit_conjunction(res)
            res.extend(tokens)
            break

        quote_token = ''.join(c if is_char(c) else ' ' for c in match.group(1).strip())
        quote_token = ' '.join(quote_token.split())

        if match.group(2) is not None:
            quote_token += ' ' + match.group(2)
        quote_token = ''.join(('" ', quote_token, ' "'))

        l, r = match.span()

        tokens = _get_tokens(s[:l])
        if len(tokens) and (is_char(tokens[0][0]) or tokens[0][0] in '!('):
            add_explicit_conjunction(res)
        res.extend(tokens)

        add_explicit_conjunction(res)
        res.append(quote_token)

        s = s[r:]

    return res


def _get_tokens(s):
    res = []
    b = []

    for c in s:
        if is_char(c):
            b.append(c)
        elif c in ' &|!()':
            if len(b):
                add_explicit_conjunction(res)
                res.append(''.join(b))
            b = []
            if c != ' ':
                if c in '!(':
                    add_explicit_conjunction(res)
                res.append(c)

    if len(b):
        add_explicit_conjunction(res)
        res.append(''.join(b))

    return res


def check_postfix_expr(s):
    stack = []
    cnt = 0
    for c in s:
        if c in '&|':
            if len(stack) < 2:
                return None
            a = stack.pop()
            b = stack.pop()
            if a == '@' and b == '@':
                stack.append(a)
            else:
                return None
        elif c == '!':
            if len(stack) < 1 or stack[-1] != '@':
                return None
        else:
            stack.append('@')
            if c[0] == '"' and c[-1] == '"':
                cnt += sum(1 for i in c.split() if i[0] not in '"/')
            else:
                cnt += 1
    if len(stack) == 1 and stack[0] == '@':
        return cnt
    return None


def to_postfix_notation(tokens):
    result = []
    stack = []

    priority = list('|&!')

    for t in tokens:
        if t in ('&', '|', '!'):
            if len(stack) == 0 or stack[-1] == '(':
                stack.append(t)
                continue

            pr_a = priority.index(stack[-1])
            pr_b = priority.index(t)
            if pr_b >= pr_a:
                stack.append(t)
                continue
            
            while len(stack):
                op = stack.pop()
                if op == '(':
                    stack.append(op)
                    break
                if priority.index(op) < pr_b:
                    stack.append(op)
                    break
                result.append(op)
            stack.append(t)

        elif t == '(':
            stack.append(t)
        elif t == ')':
            cur = ''
            while len(stack):
                cur = stack.pop()
                if cur == '(':
                    break
                result.append(cur)
            if cur != '(':
                return None
        else:
            result.append(t)

    while len(stack):
        if stack[-1] in '()':
            return None
        result.append(stack.pop())

    return result


def get_postfix_expression(s):
    p = to_postfix_notation(get_tokens(s))
    if p is None:
        return None, None
    cnt = check_postfix_expr(p)
    if cnt is None:
        return None, None
    return cnt, tuple(p)


if __name__ == '__main__':
    pass

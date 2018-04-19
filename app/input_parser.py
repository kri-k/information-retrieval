# -*- coding: utf-8 -*-


def is_char(c):
    return c not in ' &|!()'


def get_tokens(s):
    s = ' '.join(s.split()).replace('&&', '&').replace('||', '|')
    res = []
    b = []

    for c in s:
        if is_char(c):
            b.append(c)
        elif c in ' &|!()':
            if len(b):
                if len(res) and (is_char(res[-1][-1]) or res[-1][-1] == ')'):
                    res.append('&')
                res.append(''.join(b))
            b = []
            if c != ' ':
                if c in '!(' and len(res) and (is_char(res[-1][-1]) or res[-1][-1] == ')'):
                    res.append('&')
                res.append(c)

    if len(b):
        if len(res) and (is_char(res[-1][-1]) or res[-1][-1] == ')'):
            res.append('&')
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

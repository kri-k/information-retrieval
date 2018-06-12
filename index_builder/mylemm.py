import sys
import pymorphy2
import unicodedata


def remove_accents(input_str):
    nfkd_form = unicodedata.normalize('NFKD', input_str)
    return u"".join([c for c in nfkd_form if not unicodedata.combining(c)])

morph = pymorphy2.MorphAnalyzer()

file_in, file_out = sys.argv[1:]

with open(file_out, 'w', encoding='utf-8') as fout:
	with open(file_in, 'r', encoding='utf-8') as fin:
		for line in fin:
			line = line.strip('\n')
			term = remove_accents(morph.parse(line)[0].normal_form)
			print(term if len(term) else line, file=fout)

#!/usr/bin/env python
import collections

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2015 Axiomatic Systems, LLC.'

import sys
import os
import os.path as path
from subprocess import check_output, CalledProcessError
import json
import io
import struct
import operator
import hashlib
import fractions
import xml.sax.saxutils as saxutils

LanguageCodeMap = {
    'aar': 'aa', 'abk': 'ab', 'afr': 'af', 'aka': 'ak', 'alb': 'sq', 'amh': 'am', 'ara': 'ar', 'arg': 'an',
    'arm': 'hy', 'asm': 'as', 'ava': 'av', 'ave': 'ae', 'aym': 'ay', 'aze': 'az', 'bak': 'ba', 'bam': 'bm',
    'baq': 'eu', 'bel': 'be', 'ben': 'bn', 'bih': 'bh', 'bis': 'bi', 'bod': 'bo', 'bos': 'bs', 'bre': 'br',
    'bul': 'bg', 'bur': 'my', 'cat': 'ca', 'ces': 'cs', 'cha': 'ch', 'che': 'ce', 'chi': 'zh', 'chu': 'cu',
    'chv': 'cv', 'cor': 'kw', 'cos': 'co', 'cre': 'cr', 'cym': 'cy', 'cze': 'cs', 'dan': 'da', 'deu': 'de',
    'div': 'dv', 'dut': 'nl', 'dzo': 'dz', 'ell': 'el', 'eng': 'en', 'epo': 'eo', 'est': 'et', 'eus': 'eu',
    'ewe': 'ee', 'fao': 'fo', 'fas': 'fa', 'fij': 'fj', 'fin': 'fi', 'fra': 'fr', 'fre': 'fr', 'fry': 'fy',
    'ful': 'ff', 'geo': 'ka', 'ger': 'de', 'gla': 'gd', 'gle': 'ga', 'glg': 'gl', 'glv': 'gv', 'gre': 'el',
    'grn': 'gn', 'guj': 'gu', 'hat': 'ht', 'hau': 'ha', 'heb': 'he', 'her': 'hz', 'hin': 'hi', 'hmo': 'ho',
    'hrv': 'hr', 'hun': 'hu', 'hye': 'hy', 'ibo': 'ig', 'ice': 'is', 'ido': 'io', 'iii': 'ii', 'iku': 'iu',
    'ile': 'ie', 'ina': 'ia', 'ind': 'id', 'ipk': 'ik', 'isl': 'is', 'ita': 'it', 'jav': 'jv', 'jpn': 'ja',
    'kal': 'kl', 'kan': 'kn', 'kas': 'ks', 'kat': 'ka', 'kau': 'kr', 'kaz': 'kk', 'khm': 'km', 'kik': 'ki',
    'kin': 'rw', 'kir': 'ky', 'kom': 'kv', 'kon': 'kg', 'kor': 'ko', 'kua': 'kj', 'kur': 'ku', 'lao': 'lo',
    'lat': 'la', 'lav': 'lv', 'lim': 'li', 'lin': 'ln', 'lit': 'lt', 'ltz': 'lb', 'lub': 'lu', 'lug': 'lg',
    'mac': 'mk', 'mah': 'mh', 'mal': 'ml', 'mao': 'mi', 'mar': 'mr', 'may': 'ms', 'mkd': 'mk', 'mlg': 'mg',
    'mlt': 'mt', 'mon': 'mn', 'mri': 'mi', 'msa': 'ms', 'mya': 'my', 'nau': 'na', 'nav': 'nv', 'nbl': 'nr',
    'nde': 'nd', 'ndo': 'ng', 'nep': 'ne', 'nld': 'nl', 'nno': 'nn', 'nob': 'nb', 'nor': 'no', 'nya': 'ny',
    'oci': 'oc', 'oji': 'oj', 'ori': 'or', 'orm': 'om', 'oss': 'os', 'pan': 'pa', 'per': 'fa', 'pli': 'pi',
    'pol': 'pl', 'por': 'pt', 'pus': 'ps', 'que': 'qu', 'roh': 'rm', 'ron': 'ro', 'rum': 'ro', 'run': 'rn',
    'rus': 'ru', 'sag': 'sg', 'san': 'sa', 'sin': 'si', 'slk': 'sk', 'slo': 'sk', 'slv': 'sl', 'sme': 'se',
    'smo': 'sm', 'sna': 'sn', 'snd': 'sd', 'som': 'so', 'sot': 'st', 'spa': 'es', 'sqi': 'sq', 'srd': 'sc',
    'srp': 'sr', 'ssw': 'ss', 'sun': 'su', 'swa': 'sw', 'swe': 'sv', 'tah': 'ty', 'tam': 'ta', 'tat': 'tt',
    'tel': 'te', 'tgk': 'tg', 'tgl': 'tl', 'tha': 'th', 'tib': 'bo', 'tir': 'ti', 'ton': 'to', 'tsn': 'tn',
    'tso': 'ts', 'tuk': 'tk', 'tur': 'tr', 'twi': 'tw', 'uig': 'ug', 'ukr': 'uk', 'urd': 'ur', 'uzb': 'uz',
    'ven': 've', 'vie': 'vi', 'vol': 'vo', 'wel': 'cy', 'wln': 'wa', 'wol': 'wo', 'xho': 'xh', 'yid': 'yi',
    'yor': 'yo', 'zha': 'za', 'zho': 'zh', 'zul': 'zu', '```': 'und'
}

LanguageNames = {
    "aa": 'Qafara',
    "ab": '\xd0\x90\xd2\xa7\xd1\x81\xd1\x83\xd0\xb0',
    "ae": 'Avesta',
    "af": 'Afrikaans',
    "ak": 'Akana',
    "am": '\xe1\x8a\xa0\xe1\x88\x9b\xe1\x88\xad\xe1\x8a\x9b',
    "an": 'Aragon\xc3\xa9s',
    "ar": '\xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9',
    "as": '\xe0\xa6\x85\xe0\xa6\xb8\xe0\xa6\xae\xe0\xa7\x80\xe0\xa6\xaf\xe0\xa6\xbc\xe0\xa6\xbe',
    "av": '\xd0\xb0\xd0\xb2\xd0\xb0\xd1\x80 \xd0\xbc\xd0\xb0\xd1\x86\xd3\x80; \xd0\xbc\xd0\xb0\xd0\xb3\xd3\x80\xd0\xb0\xd1\x80\xd1\x83\xd0\xbb \xd0\xbc\xd0\xb0\xd1\x86\xd3\x80',
    "ay": 'Aymar Aru',
    "az": 'Az\xc9\x99rbaycanca',
    "ba": '\xd0\xb1\xd0\xb0\xd1\x88\xd2\xa1\xd0\xbe\xd1\x80\xd1\x82 \xd1\x82\xd0\xb5\xd0\xbb\xd0\xb5',
    "be": '\xd0\x91\xd0\xb5\xd0\xbb\xd0\xb0\xd1\x80\xd1\x83\xd1\x81\xd0\xba\xd0\xb0\xd1\x8f \xd0\xbc\xd0\xbe\xd0\xb2\xd0\xb0',
    "bg": '\xd0\xb1\xd1\x8a\xd0\xbb\xd0\xb3\xd0\xb0\xd1\x80\xd1\x81\xd0\xba\xd0\xb8 \xd0\xb5\xd0\xb7\xd0\xb8\xd0\xba',
    "bh": '\xe0\xa4\xad\xe0\xa5\x8b\xe0\xa4\x9c\xe0\xa4\xaa\xe0\xa5\x81\xe0\xa4\xb0\xe0\xa5\x80',
    "bi": 'Bislama',
    "bm": 'Bamanankan',
    "bn": '\xe0\xa6\xac\xe0\xa6\xbe\xe0\xa6\x82\xe0\xa6\xb2\xe0\xa6\xbe',
    "bo": '\xe0\xbd\x96\xe0\xbd\xbc\xe0\xbd\x91\xe0\xbc\x8b\xe0\xbd\xa1\xe0\xbd\xb2\xe0\xbd\x82',
    "br": 'Brezhoneg',
    "bs": 'Bosanski',
    "ca": 'Catal\xc3\xa0',
    "ce": '\xd0\xbd\xd0\xbe\xd1\x85\xd1\x87\xd0\xb8\xd0\xb9\xd0\xbd \xd0\xbc\xd0\xbe\xd1\x82\xd1\x82',
    "ch": 'Chamoru',
    "co": 'Corsu',
    "cr": '\xe1\x93\x80\xe1\x90\xa6\xe1\x90\x83\xe1\x94\xad\xe1\x90\x8d\xe1\x90\x8f\xe1\x90\xa3',
    "cs": '\xc4\x8de\xc5\xa1tina',
    "cu": '\xd1\xa9\xd0\xb7\xd1\x8b\xd0\xba\xd1\x8a \xd1\x81\xd0\xbb\xd0\xbe\xd0\xb2\xd1\xa3\xd0\xbd\xd1\x8c\xd1\x81\xd0\xba\xd1\x8a',
    "cv": '\xd1\x87\xd3\x91\xd0\xb2\xd0\xb0\xd1\x88 \xd1\x87\xd3\x97\xd0\xbb\xd1\x85\xd0\xb8',
    "cy": 'Cymraeg',
    "da": 'Dansk',
    "de": 'Deutsch',
    "dz": '\xe0\xbd\xa2\xe0\xbe\xab\xe0\xbd\xbc\xe0\xbd\x84\xe0\xbc\x8b\xe0\xbd\x81',
    "ee": '\xc6\x90\xca\x8b\xc9\x9bgb\xc9\x9b',
    "el": '\xce\x95\xce\xbb\xce\xbb\xce\xb7\xce\xbd\xce\xb9\xce\xba\xce\xac',
    "en": 'English',
    "eo": 'Esperanto',
    "es": 'Espa\xc3\xb1ol',
    "et": 'Eesti Keel',
    "eu": 'Euskara',
    "fa": '\xd9\x81\xd8\xa7\xd8\xb1\xd8\xb3\xdb\x8c',
    "ff": 'Fulfulde',
    "fi": 'Suomi',
    "fj": 'Vosa Vakaviti',
    "fo": 'F\xc3\xb8royskt',
    "fr": 'Fran\xc3\xa7ais',
    "fy": 'Frysk',
    "ga": 'Gaeilge',
    "gd": 'G\xc3\xa0idhlig',
    "gl": 'Galego',
    "gn": "Ava\xc3\xb1e'\xe1\xba\xbd",
    "gu": '\xe0\xaa\x97\xe0\xab\x81\xe0\xaa\x9c\xe0\xaa\xb0\xe0\xaa\xbe\xe0\xaa\xa4\xe0\xab\x80',
    "gv": 'Gaelg; Manninagh',
    "ha": 'Hausanc\xc4\xab; \xd9\x87\xd9\x8e\xd9\x88\xd9\x8f\xd8\xb3\xd9\x8e',
    "he": '\xd7\xa2\xd6\xb4\xd7\x91\xd6\xb0\xd7\xa8\xd6\xb4\xd7\x99\xd7\xaa; \xd7\xa2\xd7\x91\xd7\xa8\xd7\x99\xd7\xaa',
    "hi": '\xe0\xa4\xb9\xe0\xa4\xbf\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa6\xe0\xa5\x80',
    "ho": 'Hiri Motu',
    "hr": 'Hrvatski',
    "ht": 'Krey\xc3\xb2l ayisyen',
    "hu": 'Magyar',
    "hy": '\xd5\x80\xd5\xa1\xd5\xb5\xd5\xa5\xd6\x80\xd5\xa5\xd5\xb6 \xd5\xac\xd5\xa5\xd5\xa6\xd5\xb8\xd6\x82',
    "hz": 'Otjiherero',
    "ia": 'Interlingua',
    "id": 'Bahasa Indonesia',
    "ie": 'Interlingue',
    "ig": 'Igbo',
    "ii": '\xea\x86\x87\xea\x89\x99',
    "ik": 'I\xc3\xb1upiaq; I\xc3\xb1upiatun',
    "io": 'Ido',
    "is": '\xc3\xadslenska',
    "it": 'Italiano',
    "iu": '\xe1\x90\x83\xe1\x93\x84\xe1\x92\x83\xe1\x91\x8e\xe1\x91\x90\xe1\x91\xa6',
    "ja": '\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e',
    "ka": '\xe1\x83\xa5\xe1\x83\x90\xe1\x83\xa0\xe1\x83\x97\xe1\x83\xa3\xe1\x83\x9a\xe1\x83\x98 \xe1\x83\x94\xe1\x83\x9c\xe1\x83\x90 (kartuli ena)',
    "kg": 'Kikongo',
    "ki": 'G\xc4\xa9k\xc5\xa9y\xc5\xa9',
    "kj": 'Kuanyama',
    "kk": '\xd2\x9a\xd0\xb0\xd0\xb7\xd0\xb0\xd2\x9b \xd1\x82\xd1\x96\xd0\xbb\xd1\x96',
    "kl": 'Kalaallisut',
    "km": '\xe1\x9e\x97\xe1\x9e\xb6\xe1\x9e\x9f\xe1\x9e\xb6\xe1\x9e\x81\xe1\x9f\x92\xe1\x9e\x98\xe1\x9f\x82\xe1\x9e\x9a',
    "kn": '\xe0\xb2\x95\xe0\xb2\xa8\xe0\xb3\x8d\xe0\xb2\xa8\xe0\xb2\xa1',
    "ko": '\xed\x95\x9c\xea\xb5\xad\xec\x96\xb4 (\xe9\x9f\x93\xe5\x9c\x8b\xe8\xaa\x9e); \xec\xa1\xb0\xec\x84\xa0\xeb\xa7\x90 (\xe6\x9c\x9d\xe9\xae\xae\xe8\xaa\x9e)',
    "kr": 'Kanuri',
    "ks": '\xe0\xa4\x95\xe0\xa5\x89\xe0\xa4\xb6\xe0\xa5\x81\xe0\xa4\xb0; \xda\xa9\xd9\xb2\xd8\xb4\xd9\x8f\xd8\xb1',
    "ku": 'Kurd\xc3\xae; \xd9\x83\xd9\x88\xd8\xb1\xd8\xaf\xd9\x8a',
    "kv": '\xd0\xba\xd0\xbe\xd0\xbc\xd0\xb8 \xd0\xba\xd1\x8b\xd0\xb2',
    "kw": 'Kernewek',
    "ky": '\xd0\xba\xd1\x8b\xd1\x80\xd0\xb3\xd1\x8b\xd0\xb7 \xd1\x82\xd0\xb8\xd0\xbb\xd0\xb8',
    "la": 'latine; lingua Latina',
    "lb": 'L\xc3\xabtzebuergesch',
    "lg": 'Luganda',
    "li": 'Limburgs',
    "ln": 'Lingala',
    "lo": '\xe0\xba\x9e\xe0\xba\xb2\xe0\xba\xaa\xe0\xba\xb2\xe0\xba\xa5\xe0\xba\xb2\xe0\xba\xa7',
    "lt": 'Lietuvi\xc5\xb3 Kalba',
    "lv": 'Latvie\xc5\xa1u Valoda',
    "mg": 'Malagasy fiteny',
    "mh": 'Kajin M\xcc\xa7aje\xc4\xbc',
    "mi": 'Te Reo M\xc4\x81ori',
    "mk": '\xd0\xbc\xd0\xb0\xd0\xba\xd0\xb5\xd0\xb4\xd0\xbe\xd0\xbd\xd1\x81\xd0\xba\xd0\xb8 \xd1\x98\xd0\xb0\xd0\xb7\xd0\xb8\xd0\xba',
    "ml": '\xe0\xb4\xae\xe0\xb4\xb2\xe0\xb4\xaf\xe0\xb4\xbe\xe0\xb4\xb3\xe0\xb4\x82',
    "mn": '\xd0\xbc\xd0\xbe\xd0\xbd\xd0\xb3\xd0\xbe\xd0\xbb \xd1\x85\xd1\x8d\xd0\xbb',
    "mr": '\xe0\xa4\xae\xe0\xa4\xb0\xe0\xa4\xbe\xe0\xa4\xa0\xe0\xa5\x80',
    "ms": 'Bahasa Melayu; \xd8\xa8\xd9\x87\xd8\xa7\xd8\xb3 \xd9\x85\xd9\x84\xd8\xa7\xd9\x8a\xd9\x88',
    "mt": 'Malti',
    "my": '\xe1\x80\x99\xe1\x80\xbc\xe1\x80\x94\xe1\x80\xba\xe1\x80\x99\xe1\x80\xac\xe1\x80\x85\xe1\x80\xac',
    "na": 'Ekakair\xc5\xa9 Naoero',
    "nb": 'Bokm\xc3\xa5l',
    "nd": 'isiNdebele',
    "ne": '\xe0\xa4\xa8\xe0\xa5\x87\xe0\xa4\xaa\xe0\xa4\xbe\xe0\xa4\xb2\xe0\xa5\x80',
    "ng": 'Owambo',
    "nl": 'Nederlands',
    "nn": 'Nynorsk',
    "no": 'Norsk',
    "nr": 'isiNdebele',
    "nv": 'Din\xc3\xa9 bizaad; Din\xc3\xa9k\xca\xbceh\xc7\xb0\xc3\xad',
    "ny": 'chiChe\xc5\xb5a; chinyanja',
    "oc": 'Occitan',
    "oj": '\xe1\x90\x8a\xe1\x93\x82\xe1\x94\x91\xe1\x93\x87\xe1\x90\xaf\xe1\x92\xa7\xe1\x90\x8f\xe1\x90\xa3 (Anishinaabemowin)',
    "om": 'Afaan Oromoo',
    "or": '\xe0\xac\x93\xe0\xac\xa1\xe0\xac\xbc\xe0\xac\xbf\xe0\xac\x86',
    "os": '\xd0\xb8\xd1\x80\xd0\xbe\xd0\xbd \xd3\x95\xd0\xb2\xd0\xb7\xd0\xb0\xd0\xb3',
    "pa": '\xe0\xa8\xaa\xe0\xa9\xb0\xe0\xa8\x9c\xe0\xa8\xbe\xe0\xa8\xac\xe0\xa9\x80; \xd9\xbe\xd9\x86\xd8\xac\xd8\xa7\xd8\xa8\xdb\x8c',
    "pi": '\xe0\xa4\xaa\xe0\xa4\xbe\xe0\xa4\xb2\xe0\xa4\xbf',
    "pl": 'Polski',
    "ps": '\xd9\xbe\xda\x9a\xd8\xaa\xd9\x88',
    "pt": 'Portugu\xc3\xaas',
    "qu": 'Runa Simi; Kichwa',
    "rm": 'Rumantsch Grischun',
    "rn": 'Rundi',
    "ro": 'Rom\xc3\xa2n\xc4\x83',
    "ru": '\xd1\x80\xd1\x83\xd1\x81\xd1\x81\xd0\xba\xd0\xb8\xd0\xb9 \xd1\x8f\xd0\xb7\xd1\x8b\xd0\xba',
    "rw": 'Ikinyarwanda',
    "sa": '\xe0\xa4\xb8\xe0\xa4\x82\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\x95\xe0\xa5\x83\xe0\xa4\xa4\xe0\xa4\xae\xe0\xa5\x8d',
    "sc": 'Sardu',
    "sd": '\xd8\xb3\xd9\x86\xda\x8c\xd9\x8a\xd8\x8c \xd8\xb3\xd9\x86\xd8\xaf\xda\xbe\xdb\x8c; \xe0\xa4\xb8\xe0\xa4\xbf\xe0\xa4\xa8\xe0\xa5\x8d\xe0\xa4\xa7\xe0\xa5\x80',
    "se": 's\xc3\xa1mi; s\xc3\xa1megiella',
    "sg": 'y\xc3\xa2ng\xc3\xa2 t\xc3\xae s\xc3\xa4ng\xc3\xb6',
    "si": '\xe0\xb7\x83\xe0\xb7\x92\xe0\xb6\x82\xe0\xb7\x84\xe0\xb6\xbd',
    "sk": 'Sloven\xc4\x8dina',
    "sl": 'Sloven\xc5\xa1\xc4\x8dina',
    "sm": "Gagana fa'a Samoa",
    "sn": 'chiShona',
    "so": 'Soomaaliga; af Soomaali',
    "sq": 'Shqip',
    "sr": '\xd1\x81\xd1\x80\xd0\xbf\xd1\x81\xd0\xba\xd0\xb8 \xd1\x98\xd0\xb5\xd0\xb7\xd0\xb8\xd0\xba; srpski jezik',
    "ss": 'siSwati',
    "st": 'Sesotho',
    "su": 'Basa Sunda',
    "sv": 'Svenska',
    "sw": 'Kiswahili',
    "ta": '\xe0\xae\xa4\xe0\xae\xae\xe0\xae\xbf\xe0\xae\xb4\xe0\xaf\x8d',
    "te": '\xe0\xb0\xa4\xe0\xb1\x86\xe0\xb0\xb2\xe0\xb1\x81\xe0\xb0\x97\xe0\xb1\x81',
    "tg": '\xd1\x82\xd0\xbe\xd2\xb7\xd0\xb8\xd0\xba\xd3\xa3; \xd8\xaa\xd8\xa7\xd8\xac\xdb\x8c\xda\xa9\xdb\x8c',
    "th": '\xe0\xb8\xa0\xe0\xb8\xb2\xe0\xb8\xa9\xe0\xb8\xb2\xe0\xb9\x84\xe0\xb8\x97\xe0\xb8\xa2',
    "ti": '\xe1\x89\xb5\xe1\x8c\x8d\xe1\x88\xad\xe1\x8a\x9b',
    "tk": '\xd0\xa2\xd2\xaf\xd1\x80\xd0\xba\xd0\xbc\xd0\xb5\xd0\xbd',
    "tl": 'Wikang Tagalog; \xe1\x9c\x8f\xe1\x9c\x92\xe1\x9c\x83\xe1\x9c\x85\xe1\x9c\x94 \xe1\x9c\x86\xe1\x9c\x84\xe1\x9c\x8e\xe1\x9c\x93\xe1\x9c\x84\xe1\x9c\x94',
    "tn": 'Setswana',
    "to": 'Faka-Tonga',
    "tr": 'T\xc3\xbcrk\xc3\xa7e',
    "ts": 'Xitsonga',
    "tt": '\xd1\x82\xd0\xb0\xd1\x82\xd0\xb0\xd1\x80\xd1\x87\xd0\xb0; tatar\xc3\xa7a; \xd8\xaa\xd8\xa7\xd8\xaa\xd8\xa7\xd8\xb1\xda\x86\xd8\xa7',
    "tw": 'Twi',
    "ty": 'te reo Tahiti; te reo M\xc4\x81\xca\xbcohi',
    "ug": 'Uy\xc6\xa3urq\xc9\x99; Uy\xc4\x9fur\xc3\xa7e; \xd8\xa6\xdb\x87\xd9\x8a\xd8\xba\xdb\x87\xd8\xb1\xda\x86',
    "uk": '\xd1\x83\xd0\xba\xd1\x80\xd0\xb0\xd1\x97\xd0\xbd\xd1\x81\xd1\x8c\xd0\xba\xd0\xb0 \xd0\xbc\xd0\xbe\xd0\xb2\xd0\xb0',
    "ur": '\xd8\xa7\xd8\xb1\xd8\xaf\xd9\x88',
    "uz": "O'zbek; \xd0\x8e\xd0\xb7\xd0\xb1\xd0\xb5\xd0\xba; \xd8\xa3\xdb\x87\xd8\xb2\xd8\xa8\xdb\x90\xd9\x83",
    "ve": 'Tshiven\xe1\xb8\x93a',
    "vi": 'Ti\xe1\xba\xbfng Vi\xe1\xbb\x87t',
    "vo": 'Volap\xc3\xbck',
    "wa": 'Walon',
    "wo": 'Wolof',
    "xh": 'isiXhosa',
    "yi": '\xd7\x99\xd7\x99\xd6\xb4\xd7\x93\xd7\x99\xd7\xa9',
    "yo": 'Yor\xc3\xb9b\xc3\xa1',
    "za": 'Sa\xc9\xaf cue\xc5\x8b\xc6\x85; Saw cuengh',
    "zh": '\xe6\xbc\xa2\xe8\xaa\x9e; \xe6\xb1\x89\xe8\xaf\xad; \xe4\xb8\xad\xe6\x96\x87',
    "zu": 'isiZulu',
    "und": 'Unknown'
}

def PrintErrorAndExit(message):
    sys.stderr.write(message+'\n')
    sys.exit(1)

def XmlDuration(d):
    h  = int(d)/3600
    d -= h*3600
    m  = int(d)/60
    s  = d-m*60
    xsd = 'PT'
    if h:
        xsd += str(h)+'H'
    if h or m:
        xsd += str(m)+'M'
    if s:
        xsd += ('%.3fS' % (s))
    return xsd

def Bento4Command(options, name, *args, **kwargs):
    executable = path.join(options.exec_dir, name)
    cmd = [executable]
    for kwarg in kwargs:
        arg = kwarg.replace('_', '-')
        if isinstance(kwargs[kwarg], bool):
            cmd.append('--'+arg)
        else :
            if isinstance(kwargs[kwarg], list):
                for element in kwargs[kwarg]:
                    cmd.append('--'+arg)
                    cmd.append(element)
            else:
                cmd.append('--'+arg)
                cmd.append(kwargs[kwarg])

    cmd += args
    if options.debug:
        print 'COMMAND: ', " ".join(cmd), cmd
    try:
        try:
            return check_output(cmd)
        except OSError as e:
            if options.debug:
                print 'executable ' + executable + ' not found in exec_dir, trying with PATH'
            cmd[0] = path.basename(cmd[0])
            return check_output(cmd)
    except CalledProcessError as e:
        message = "binary tool failed with error %d" % e.returncode
        if options.verbose:
            message += " - " + str(cmd)
        raise Exception(message)
    except OSError as e:
        raise Exception('executable "'+name+'" not found, ensure that it is in your path or in the directory '+options.exec_dir)


def Mp4Info(options, filename, *args, **kwargs):
    return Bento4Command(options, 'mp4info', filename, *args, **kwargs)

def Mp4Dump(options, filename, *args, **kwargs):
    return Bento4Command(options, 'mp4dump', filename, *args, **kwargs)

def Mp4Split(options, filename, *args, **kwargs):
    return Bento4Command(options, 'mp4split', filename, *args, **kwargs)

def Mp4Fragment(options, input_filename, output_filename, *args, **kwargs):
    return Bento4Command(options, 'mp4fragment', input_filename, output_filename, *args, **kwargs)

def Mp4Encrypt(options, input_filename, output_filename, *args, **kwargs):
    return Bento4Command(options, 'mp4encrypt', input_filename, output_filename, *args, **kwargs)

def Mp42Hls(options, input_filename, *args, **kwargs):
    return Bento4Command(options, 'mp42hls', input_filename, *args, **kwargs)

def Mp4IframIndex(options, input_filename, *args, **kwargs):
    return Bento4Command(options, 'mp4iframeindex', input_filename, *args, **kwargs)
    
class Mp4Atom:
    def __init__(self, type, size, position):
        self.type     = type
        self.size     = size
        self.position = position

    def __str__(self):
        return 'ATOM: ' + self.type + ',' + str(self.size) + '@' + str(self.position)


def WalkAtoms(filename, until=None):
    cursor = 0
    atoms = []
    file = io.FileIO(filename, "rb")
    while True:
        try:
            size = struct.unpack('>I', file.read(4))[0]
            type = file.read(4)
            if type == until:
                break
            if size == 1:
                size = struct.unpack('>Q', file.read(8))[0]
            atoms.append(Mp4Atom(type, size, cursor))
            cursor += size
            file.seek(cursor)
        except:
            break

    return atoms


def FilterChildren(parent, type):
    if isinstance(parent, list):
        children = parent
    else:
        children = parent['children']
    return [child for child in children if child['name'] == type]

def FindChild(top, path):
    for entry in path:
        children = FilterChildren(top, entry)
        if len(children) == 0: return None
        top = children[0]
    return top

class Mp4Track:
    def __init__(self, parent, info):
        self.parent = parent
        self.info   = info
        self.default_sample_duration  = 0
        self.timescale                = 0
        self.moofs                    = []
        self.kid                      = None
        self.sample_counts            = []
        self.segment_sizes            = []
        self.segment_durations        = []
        self.segment_scaled_durations = []
        self.segment_bitrates         = []
        self.total_sample_count       = 0
        self.total_duration           = 0
        self.media_size               = 0
        self.average_segment_duration = 0
        self.average_segment_bitrate  = 0
        self.max_segment_bitrate      = 0
        self.bandwidth                = 0
        self.language                 = ''
        self.order_index              = 0
        self.id = info['id']
        if info['type'] == 'Audio':
            self.type = 'audio'
        elif info['type'] == 'Video':
            self.type = 'video'
        elif info['type'] == 'Subtitles':
            self.type = 'subtitles'
        else:
            self.type = 'other'

        sample_desc = info['sample_descriptions'][0]

        self.codec_family = sample_desc['coding']
        if 'codecs_string' in sample_desc:
            self.codec = sample_desc['codecs_string']
        else:
            self.codec = self.codec_family

        if self.type == 'video':
            # set the scan type (hardcoded for now)
            self.scan_type = 'progressive'

            # set the width and height
            self.width  = sample_desc['width']
            self.height = sample_desc['height']

            # add dolby vision signaling if present
            if 'dolby_vision' in sample_desc:
                dv_info = sample_desc['dolby_vision']
                if sample_desc['coding'] in ['dvav', 'dva1', 'dvhe', 'dvh1']:
                    # non-backward-compatible
                    self.codec = sample_desc['coding'] + ('.%02d.%02d' % (dv_info['profile'], dv_info['level']))
                else:
                    # backward-compatible
                    coding_map = {
                        'avc1': 'dva1',
                        'avc3': 'dvav',
                        'hev1': 'dvhe',
                        'hvc1': 'dvh1'
                    }
                    dv_coding = coding_map.get(sample_desc['coding'])
                    if dv_coding:
                        dv_string = dv_coding + ('.%02d.%02d' % (dv_info['profile'], dv_info['level']))
                        self.codec += ','+dv_string

        if self.type == 'audio':
            self.sample_rate = sample_desc['sample_rate']
            self.channels = sample_desc['channels']

        self.language = info['language']

    def update(self, options):
        # compute the total number of samples
        self.total_sample_count = reduce(operator.add, self.sample_counts, 0)

        # compute the total duration
        self.total_duration = reduce(operator.add, self.segment_durations, 0)

        # compute the average segment durations
        segment_count = len(self.segment_durations)
        if segment_count > 2:
            # do not count the last two segments, which could be shorter
            self.average_segment_duration = reduce(operator.add, self.segment_durations[:-2], 0)/float(segment_count-2)
        elif segment_count > 0:
            self.average_segment_duration = self.segment_durations[0]
        else:
            self.average_segment_duration = 0

        # compute the average segment bitrates
        self.media_size = reduce(operator.add, self.segment_sizes, 0)
        if self.total_duration:
            self.average_segment_bitrate = int(8.0*float(self.media_size)/self.total_duration)

        # compute the max segment bitrates
        if len(self.segment_bitrates) > 1:
            self.max_segment_bitrate = max(self.segment_bitrates[:-1])
        else:
            self.max_segment_bitrate = self.average_segment_bitrate

        # compute the bandwidth
        if options.min_buffer_time == 0.0:
            options.min_buffer_time = self.average_segment_duration
        self.bandwidth = ComputeBandwidth(options.min_buffer_time, self.segment_sizes, self.segment_durations)

        if self.type == 'video':
            # compute the frame rate
            if self.total_duration:
                self.frame_rate = self.total_sample_count / self.total_duration
                self.frame_rate_ratio = str(fractions.Fraction(str(self.frame_rate)).limit_denominator(100000))
            else:
                self.frame_rate = 0.0
                self.frame_rate_ratio = "0"

    def compute_kid(self):
        moov = FilterChildren(self.parent.tree, 'moov')[0]
        traks = FilterChildren(moov, 'trak')
        for trak in traks:
            tkhd = FindChild(trak, ['tkhd'])
            tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'encv', 'sinf', 'schi', 'tenc'))
            if tenc is None:
                tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'enca', 'sinf', 'schi', 'tenc'))
            if tenc and 'default_KID' in tenc:
                self.kid = tenc['default_KID'].strip('[]').replace(' ', '')

    def __repr__(self):
        return 'File '+str(self.parent.file_list_index)+'#'+str(self.id)

class Mp4File:
    def __init__(self, options, media_source):
        self.media_source    = media_source
        self.tracks          = {}
        self.file_list_index = 0 # used to keep a sequence number just amongst all sources

        filename = media_source.filename
        if options.debug:
            print 'Processing MP4 file', filename

        # by default, the media name is the basename of the source file
        self.media_name = os.path.basename(filename)

        # walk the atom structure
        self.atoms = WalkAtoms(filename)
        self.segments = []
        for atom in self.atoms:
            if atom.type == 'moov':
                self.init_segment = atom
            elif atom.type == 'moof':
                self.segments.append([atom])
            else:
                if len(self.segments):
                    self.segments[-1].append(atom)
        #print self.segments
        if options.debug:
            print '  found', len(self.segments), 'segments'

        # get the mp4 file info
        json_info = Mp4Info(options, filename, format='json', fast=True)
        self.info = json.loads(json_info, strict=False, object_pairs_hook=collections.OrderedDict)

        for track in self.info['tracks']:
            self.tracks[track['id']] = Mp4Track(self, track)

        # get a complete file dump
        json_dump = Mp4Dump(options, filename, format='json', verbosity='1')
        #print json_dump
        self.tree = json.loads(json_dump, strict=False, object_pairs_hook=collections.OrderedDict)

        # look for KIDs
        for track in self.tracks.itervalues():
            track.compute_kid()

        # compute default sample durations and timescales
        for atom in self.tree:
            if atom['name'] == 'moov':
                for c1 in atom['children']:
                    if c1['name'] == 'mvex':
                        for c2 in c1['children']:
                            if c2['name'] == 'trex':
                                self.tracks[c2['track id']].default_sample_duration = c2['default sample duration']
                    elif c1['name'] == 'trak':
                        track_id = 0
                        for c2 in c1['children']:
                            if c2['name'] == 'tkhd':
                                track_id = c2['id']
                        for c2 in c1['children']:
                            if c2['name'] == 'mdia':
                                for c3 in c2['children']:
                                    if c3['name'] == 'mdhd':
                                        self.tracks[track_id].timescale = c3['timescale']

        # partition the segments
        segment_index = 0
        track = None
        segment_size = 0
        segment_duration_sec = 0.0
        for atom in self.tree:
            segment_size += atom['size']
            if atom['name'] == 'moof':
                segment_size = atom['size']
                trafs = FilterChildren(atom, 'traf')
                if len(trafs) != 1:
                    PrintErrorAndExit('ERROR: unsupported input file, more than one "traf" box in fragment')
                tfhd = FilterChildren(trafs[0], 'tfhd')[0]
                track = self.tracks[tfhd['track ID']]
                track.moofs.append(segment_index)
                segment_duration = 0
                default_sample_duration = tfhd.get('default sample duration', track.default_sample_duration)
                for trun in FilterChildren(trafs[0], 'trun'):
                    track.sample_counts.append(trun['sample count'])
                    for (name, value) in trun.items():
                        if name[0] in '0123456789':
                            sample_duration = -1
                            fields = value.split(',')
                            for field in fields:
                                if field.startswith('d:'):
                                    sample_duration = int(field[2:])
                            if sample_duration == -1:
                                sample_duration = default_sample_duration
                            segment_duration += sample_duration
                track.segment_scaled_durations.append(segment_duration)
                segment_duration_sec = float(segment_duration) / float(track.timescale)
                track.segment_durations.append(segment_duration_sec)
                segment_index += 1

                # remove the 'trun' entries to save some memory
                for traf in trafs:
                    traf['children'] = [x for x in traf['children'] if x['name'] != 'trun']
            elif atom['name'] == 'mdat':
                # end of fragment on 'mdat' atom
                if track:
                    track.segment_sizes.append(segment_size)
                    if segment_duration_sec > 0.0:
                        segment_bitrate = int((8.0 * float(segment_size)) / segment_duration_sec)
                    else:
                        segment_bitrate = 0
                    track.segment_bitrates.append(segment_bitrate)
                segment_size = 0
                
        # parse the 'mfra' index if there is one and update segment durations.
        # this is needed to deal with input files that have an 'mfra' index that
        # does not exactly match the sample durations (because of rounding errors),
        # which will make the Smooth Streaming URL mapping fail since the IIS Smooth Streaming
        # server uses the 'mfra' index to locate the segments in the source .ismv file
        mfra = FindChild(self.tree, ['mfra'])
        if mfra:
            for tfra in FilterChildren(mfra, 'tfra'):
                track_id = tfra['track_ID']
                if track_id not in self.tracks:
                    continue
                track = self.tracks[track_id]
                moof_pointers = []
                for (name, value) in tfra.items():
                    if name.startswith('['):
                        attributes = value.split(',')
                        attribute_dict = {}
                        for attribute in attributes:
                            (attribute_name, attribute_value) = attribute.strip().split('=')
                            attribute_dict[attribute_name] = int(attribute_value)
                        if attribute_dict['traf_number'] == 1 and attribute_dict['trun_number'] == 1 and attribute_dict['sample_number'] == 1:
                            # this points to the first sample of the first trun of the first traf, use it as a start time indication
                            moof_pointers.append(attribute_dict)
                if len(moof_pointers) > 1:
                    for i in range(len(moof_pointers)-1):
                        if i+1 >= len(track.moofs):
                            break

                        moof1 = self.segments[track.moofs[i]][0]
                        moof2 = self.segments[track.moofs[i+1]][0]
                        if moof1.position == moof_pointers[i]['moof_offset'] and moof2.position == moof_pointers[i+1]['moof_offset']:
                            # pointers match two consecutive moofs
                            moof_duration = moof_pointers[i+1]['time'] - moof_pointers[i]['time']
                            moof_duration_sec = float(moof_duration) / float(track.timescale)
                            track.segment_durations[i] = moof_duration_sec
                            track.segment_scaled_durations[i] = moof_duration

        # compute the total numer of samples for each track
        for track_id in self.tracks:
            self.tracks[track_id].update(options)

        # print debug info if requested
        if options.debug:
            for track in self.tracks.itervalues():
                print 'Track ID                     =', track.id
                print '    Segment Count            =', len(track.segment_durations)
                print '    Type                     =', track.type
                print '    Sample Count             =', track.total_sample_count
                print '    Average segment bitrate  =', track.average_segment_bitrate
                print '    Max segment bitrate      =', track.max_segment_bitrate
                print '    Required bandwidth       =', int(track.bandwidth)
                print '    Average segment duration =', track.average_segment_duration

    def find_track_by_id(self, track_id_to_find):
        for track_id in self.tracks:
            if track_id_to_find == 0 or track_id_to_find == track_id:
                return self.tracks[track_id]

        return None

    def find_tracks_by_type(self, track_type_to_find):
        return [track for track in self.tracks.values() if track_type_to_find == '' or track_type_to_find == track.type]

class MediaSource:
    def __init__(self, name):
        self.name = name
        self.track_key_infos = {}
        if name.startswith('[') and ']' in name:
            try:
                params = name[1:name.find(']')]
                self.filename = name[2+len(params):]
                self.spec = dict([x.split('=') for x in params.split(',')])
                for int_param in ['track']:
                    if int_param in self.spec: self.spec[int_param] = int(self.spec[int_param])
            except:
                raise Exception('Invalid syntax for media file spec "'+name+'"')
        else:
            self.filename = name
            self.spec = {}

        if 'type'     not in self.spec: self.spec['type']     = ''
        if 'track'    not in self.spec: self.spec['track']    = 0
        if 'language' not in self.spec: self.spec['language'] = ''

        # check if we have an explicit format (default=mp4)
        if '+format' in self.spec:
            self.format = self.spec['+format']
        else:
            self.format = 'mp4'

        # keep a record of our original filename in case it gets changed later
        self.original_filename = self.filename

    def __repr__(self):
        return self.name

def ComputeBandwidth(buffer_time, sizes, durations):
    bandwidth = 0.0
    for i in range(len(sizes)):
        accu_size     = 0
        accu_duration = 0
        buffer_size = (buffer_time*bandwidth)/8.0
        for j in range(i, len(sizes)):
            accu_size     += sizes[j]
            accu_duration += durations[j]
            max_avail = buffer_size+accu_duration*bandwidth/8.0
            if accu_size > max_avail and accu_duration != 0:
                bandwidth = 8.0*(accu_size-buffer_size)/accu_duration
                break
    return int(bandwidth)

def MakeNewDir(dir, exit_if_exists=False, severity=None):
    if os.path.exists(dir):
        if severity:
            sys.stderr.write(severity+': ')
            sys.stderr.write('directory "'+dir+'" already exists\n')
        if exit_if_exists:
            sys.exit(1)
    else:
        os.mkdir(dir)

def MakePsshBox(system_id, payload):
    pssh_size = 12+16+4+len(payload)
    return struct.pack('>I', pssh_size)+'pssh'+struct.pack('>I',0)+system_id+struct.pack('>I', len(payload))+payload

def MakePsshBoxV1(system_id, kids, payload):
    pssh_size = 12+16++4+(16*len(kids))+4+len(payload)
    pssh = struct.pack('>I', pssh_size)+'pssh'+struct.pack('>I',0x01000000)+system_id+struct.pack('>I', len(kids))
    for kid in kids:
        pssh += kid.decode('hex')
    pssh += struct.pack('>I', len(payload))+payload
    return pssh

def GetEncryptionKey(options, spec):
    if options.debug:
        print 'Resolving KID and Key from spec:', spec
    if spec.startswith('skm:'):
        import skm
        return skm.ResolveKey(options, spec[4:])
    else:
        raise Exception('Key Locator scheme not supported')

# Compute the Dolby Digital AudioChannelConfiguration value
#
# (MSB = 0)
# 0 L
# 1 C
# 2 R
# 3 Ls
# 4 Rs
# 5 Lc/Rc pair
# 6 Lrs/Rrs pair
# 7 Cs
# 8 Ts
# 9 Lsd/Rsd pair
# 10 Lw/Rw pair
# 11 Vhl/Vhr pair
# 12 Vhc
# 13 Lts/Rts pair
# 14 LFE2
# 15 LFE
#
# Using acmod
# 000 Ch1, Ch2
# 001 C
# 010 L, R
# 011 L, C, R
# 100 L, R, S
# 101 L, C, R, S
# 110 L, R, SL, SR
# 111 L, C, R, SL, SR
#
# chan_loc
# 0 Lc/Rc pair
# 1 Lrs/Rrs pair
# 2 Cs
# 3 Ts
# 4 Lsd/Rsd pair
# 5 Lw/Rw pair
# 6 Lvh/Rvh pair
# 7 Cvh
# 8 LFE2
#
# The Digital Cinema specification, which is also referenced from the
# Blu-ray Disc Specification, Specifies this speaker layout:
#
#       +---+       +---+       +---+
#       |Vhl|       |Vhc|       |Vhr|        "High" speakers
#       +---+       +---+       +---+
# +---+ +---+ +---+ +---+ +---+ +---+ +---+
# |Lw | | L | |Lc | | C | |Rc | | R | |Rw |
# +---+ +---+ +---+ +---+ +---+ +---+ +---+
#             +----+     +----+
#             |LFE1]     |LFE2|
# +---+       +----++---++----+       +---+
# |Ls |             |Ts |             |Rs |
# +---+             +---+             +---+
#
# +---+                               +---+
# |Lsd|                               |Rsd|
# +---+ +---+       +---+       +---+ +---+
#       |Rls|       |Cs |       |Rrs|
#       +---+       +---+       +---+
#
# Other names:
# Constant              | HDMI  | Digital Cinema | DTS extension
# ==============================|================|==============
# FRONT_LEFT            | FL    | L              | L
# FRONT_RIGHT           | FR    | R              | R
# FRONT_CENTER          | FC    | C              | C
# LOW_FREQUENCY         | LFE   | LFE            | LFE
# BACK_LEFT             | (RLC) | Rls            | Lsr
# BACK_RIGHT            | (RRC) | Rrs            | Rsr
# FRONT_LEFT_OF_CENTER  | FLC   | Lc             | Lc
# FRONT_RIGHT_OF_CENTER | FRC   | Rc             | Rc
# BACK_CENTER           | RC    | Cs             | Cs
# SIDE_LEFT             | (RL)  | Ls             | Lss
# SIDE_RIGHT            | (RR)  | Rs             | Rss
# TOP_CENTER            | TC    | Ts             | Oh
# TOP_FRONT_LEFT        | FLH   | Vhl            | Lh
# TOP_FRONT_CENTER      | FCH   | Vhc            | Ch
# TOP_FRONT_RIGHT       | FRH   | Vhr            | Rh
# TOP_BACK_LEFT         |       |                | Chr
# TOP_BACK_CENTER       |       |                | Lhr
# TOP_BACK_RIGHT        |       |                | Rhr
# STEREO_LEFT           |       |                |
# STEREO_RIGHT          |       |                |
# WIDE_LEFT             | FLW   | Lw             | Lw
# WIDE_RIGHT            | FRW   | Rw             | Rw
# SURROUND_DIRECT_LEFT  |       | Lsd            | Ls
# SURROUND_DIRECT_RIGHT |       | Rsd            | Rs

DolbyDigital_chan_loc = {
    0: 'Lc/Rc',
    1: 'Lrs/Rrs',
    2: 'Cs',
    3: 'Ts',
    4: 'Lsd/Rsd',
    5: 'Lw/Rw',
    6: 'Vhl/Vhr',
    7: 'Vhc',
    8: 'LFE2'
}

DolbyDigital_acmod = {
    0: ['L', 'R'],  # in theory this is not supported but we'll pick a reasonable value
    1: ['C'],
    2: ['L', 'R'],
    3: ['L', 'C', 'R'],
    4: ['L', 'R', 'Cs'],
    5: ['L', 'C', 'R', 'Cs'],
    6: ['L', 'R', 'Ls', 'Rs'],
    7: ['L', 'C', 'R', 'Ls', 'Rs']
}

def GetDolbyDigitalChannels(track):
    sample_desc = track.info['sample_descriptions'][0]
    if 'dolby_digital_info' not in sample_desc:
        return (track.channels, [])
    dd_info = sample_desc['dolby_digital_info']['substreams'][0]
    channels = DolbyDigital_acmod[dd_info['acmod']][:]
    if dd_info['lfeon'] == 1:
        channels.append('LFE')
    if dd_info['num_dep_sub'] and 'chan_loc' in dd_info:
        chan_loc_value = dd_info['chan_loc']
        for i in range(9):
            if chan_loc_value & (1<<i):
                channels.append(DolbyDigital_chan_loc[i])
    channel_count = 0
    for channel in channels:
        if '/' in channel:
            channel_count += 2
        else:
            channel_count += 1
    return (channel_count, channels)

def ComputeDolbyDigitalAudioChannelConfig(track):
    flags = {
        'L':       1<<15,
        'C':       1<<14,
        'R':       1<<13,
        'Ls':      1<<12,
        'Rs':      1<<11,
        'Lc/Rc':   1<<10,
        'Lrs/Rrs': 1<<9,
        'Cs':      1<<8,
        'Ts':      1<<7,
        'Lsd/Rsd': 1<<6,
        'Lw/Rw':   1<<5,
        'Vhl/Vhr': 1<<4,
        'Vhc':     1<<3,
        'Lts/Rts': 1<<2,
        'LFE2':    1<<1,
        'LFE':     1<<0
    }
    (channel_count, channels) = GetDolbyDigitalChannels(track)
    if len(channels) == 0:
        return str(channel_count)
    config = 0
    for channel in channels:
        if channel in flags:
            config |= flags[channel]
    return hex(config).upper()[2:]

def ComputeDolbyDigitalAudioChannelMask(track):
    masks = {
        'L':       0x1,             # SPEAKER_FRONT_LEFT
        'R':       0x2,             # SPEAKER_FRONT_RIGHT
        'C':	   0x4,             # SPEAKER_FRONT_CENTER
        'LFE':     0x8,             # SPEAKER_LOW_FREQUENCY
        'Ls':      0x10,            # SPEAKER_BACK_LEFT
        'Rs':      0x20,            # SPEAKER_BACK_RIGHT
        'Lc':      0x40,            # SPEAKER_FRONT_LEFT_OF_CENTER
        'Rc':      0x80,            # SPEAKER_FRONT_RIGHT_OF_CENTER
        'Cs':      0x100,           # SPEAKER_BACK_CENTER
        'Lrs':     0x200,           # SPEAKER_SIDE_LEFT
        'Rrs':     0x400,           # SPEAKER_SIDE_RIGHT
        'Ts':      0x800,           # SPEAKER_TOP_CENTER
        'Vhl/Vhr': 0x1000 | 0x4000, # SPEAKER_TOP_FRONT_LEFT/SPEAKER_TOP_FRONT_RIGHT
        'Vhc':     0x2000,          # SPEAKER_TOP_FRONT_CENTER
    }
    (channel_count, channels) = GetDolbyDigitalChannels(track)
    if len(channels) == 0:
        return (channel_count, 3)
    channel_mask = 0
    for channel in channels:
        if channel in masks:
            channel_mask |= masks[channel]
        else:
            (channel1, channel2) = channel.split('/')
            if channel1 in masks:
                channel_mask |= masks[channel1]
            if channel2 in masks:
                channel_mask |= masks[channel2]
    return (channel_count, channel_mask)

def ComputeDolbyDigitalSmoothStreamingInfo(track):
    (channel_count, channel_mask) = ComputeDolbyDigitalAudioChannelMask(track)
    info = "0006" # 1536 in little-endian
    mask_hex_be = "{0:0{1}x}".format(channel_mask, 4)
    info += mask_hex_be[2:4]+mask_hex_be[0:2]+'0000'
    info += "af87fba7022dfb42a4d405cd93843bdd"
    info += track.info['sample_descriptions'][0]['dolby_digital_info']['dec3_payload']
    return (channel_count, info.lower())

def ComputeMarlinPssh(options):
    # create a dummy (empty) Marlin PSSH
    return struct.pack('>I4sI4sII', 24, 'marl', 16, 'mkid', 0, 0)

def DerivePlayReadyKey(seed, kid, swap=True):
    if len(seed) < 30:
        raise Exception('seed must be  >= 30 bytes')
    if len(kid) != 16:
        raise Exception('kid must be 16 bytes')

    if swap:
        kid = kid[3]+kid[2]+kid[1]+kid[0]+kid[5]+kid[4]+kid[7]+kid[6]+kid[8:]

    seed = seed[:30]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha_A = [ord(x) for x in sha.digest()]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha_B = [ord(x) for x in sha.digest()]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha.update(kid)
    sha_C = [ord(x) for x in sha.digest()]

    content_key = ""
    for i in range(16):
        content_key += chr(sha_A[i] ^ sha_A[i+16] ^ sha_B[i] ^ sha_B[i+16] ^ sha_C[i] ^ sha_C[i+16])

    return content_key

def ComputePlayReadyChecksum(kid, key):
    import aes
    return aes.rijndael(key).encrypt(kid)[:8]

def WrapPlayreadyHeaderXml(header_xml):
    # encode the XML header into UTF-16 little-endian
    header_utf16_le = header_xml.encode('utf-16-le')
    rm_record = struct.pack('<HH', 1, len(header_utf16_le))+header_utf16_le
    return struct.pack('<IH', len(rm_record)+6, 1)+rm_record

def ComputePlayReadyHeader(header_spec, kid_hex, key_hex):
    # construct the base64 header
    if header_spec is None:
        header_spec = ''
    if header_spec.startswith('#'):
        header_b64 = header_spec[1:]
        header = header_b64.decode('base64')
        if len(header) == 0:
            raise Exception('invalid base64 encoding')
        return header
    elif header_spec.startswith('@') or os.path.exists(header_spec):
        # check that the file exists
        if header_spec.startswith('@'):
            header_spec = header_spec[1:]
            if not os.path.exists(header_spec):
                raise Exception('header data file does not exist')

        # read the header from the file
        header = open(header_spec, 'rb').read()
        header_xml = None
        if (ord(header[0]) == 0xff and ord(header[1]) == 0xfe) or (ord(header[0]) == 0xfe and ord(header[1]) == 0xff):
            # this is UTF-16 XML
            header_xml = header.decode('utf-16')
        elif header[0] == '<' and ord(header[1]) != 0x00:
            # this is ASCII or UTF-8 XML
            header_xml = header.decode('utf-8')
        elif header[0] == '<' and ord(header[1]) == 0x00:
            # this UTF-16LE XML without charset header
            header_xml = header.decode('utf-16-le')
        if header_xml is not None:
            header = WrapPlayreadyHeaderXml(header_xml)
        return header
    else:
        try:
            pairs = header_spec.split('#')
            fields = {}
            for pair in pairs:
                if len(pair) == 0: continue
                name, value = pair.split(':', 1)
                fields[name] = value
        except:
            raise Exception('invalid syntax for argument')

        header_xml = '<WRMHEADER xmlns="http://schemas.microsoft.com/DRM/2007/03/PlayReadyHeader" version="4.0.0.0"><DATA><PROTECTINFO><KEYLEN>16</KEYLEN><ALGID>AESCTR</ALGID></PROTECTINFO>'

        kid = kid_hex.decode('hex')
        kid = kid[3]+kid[2]+kid[1]+kid[0]+kid[5]+kid[4]+kid[7]+kid[6]+kid[8:]
        header_xml += '<KID>'+kid.encode('base64').replace('\n', '')+'</KID>'
        if key_hex:
            header_xml += '<CHECKSUM>'+ComputePlayReadyChecksum(kid, key_hex.decode('hex')).encode('base64').replace('\n', '')+'</CHECKSUM>'

        if 'CUSTOMATTRIBUTES' in fields:
            header_xml += '<CUSTOMATTRIBUTES>'+fields['CUSTOMATTRIBUTES'].decode('base64').replace('\n', '')+'</CUSTOMATTRIBUTES>'
        if 'LA_URL' in fields:
            header_xml += '<LA_URL>'+saxutils.escape(fields['LA_URL'])+'</LA_URL>'
        if 'LUI_URL' in fields:
            header_xml += '<LUI_URL>'+saxutils.escape(fields['LUI_URL'])+'</LUI_URL>'
        if 'DS_ID' in fields:
            header_xml += '<DS_ID>'+saxutils.escape(fields['DS_ID'])+'</DS_ID>'

        header_xml += '</DATA></WRMHEADER>'
        return WrapPlayreadyHeaderXml(header_xml)

    return ""

def ComputePrimetimeMetaData(metadata_spec, kid_hex):
    # construct the base64 header
    if metadata_spec is None:
        metadata_spec = ''
    if metadata_spec.startswith('#'):
        metadata_b64 = metadata_spec[1:]
        metadata = metadata_b64.decode('base64')
        if len(metadata) == 0:
            raise Exception('invalid base64 encoding')
    elif metadata_spec.startswith('@'):
        metadata_filename = metadata_spec[1:]
        if not os.path.exists(metadata_filename):
            raise Exception('data file does not exist')

        # read the header from the file
        metadata = open(metadata_filename, 'rb').read()

    amet_size = 12+4+16
    amet_flags = 0
    if len(metadata):
        amet_flags |= 2
        amet_size += 4+len(metadata)
    amet_box = struct.pack('>I4sII', amet_size, 'amet', amet_flags, 1)+kid_hex.decode("hex")
    if len(metadata):
        amet_box += struct.pack('>I', len(metadata))+metadata

    return amet_box

def WidevineVarInt(value):
    parts = [value % 128]
    value >>= 7
    while value:
        parts.append(value%128)
        value >>= 7
    varint = ''
    for i in range(len(parts)-1):
        parts[i] |= (1<<7)
    varint = ''
    for x in parts:
        varint += chr(x)
    return varint

def WidevineMakeHeader(fields):
    buffer = ''
    for (field_num, field_val) in fields:
        if type(field_val) == int and field_val < 256:
            wire_type = 0 # varint
            wire_val = WidevineVarInt(field_val)
        elif type(field_val) == str:
            wire_type = 2
            wire_val = WidevineVarInt(len(field_val))+field_val
        buffer += chr(field_num<<3 | wire_type) + wire_val
    return buffer

def ComputeWidevineHeader(header_spec, kid_hex):
    # construct the base64 header
    if header_spec.startswith('#'):
        header_b64 = header_spec[1:]
        header = header_b64.decode('base64')
        if len(header) == 0:
            raise Exception('invalid base64 encoding')
        return header
    else:
        try:
            pairs = header_spec.split('#')
            fields = {}
            for pair in pairs:
                name, value = pair.split(':', 1)
                fields[name] = value
        except:
            raise Exception('invalid syntax for argument')

        protobuf_fields = [(1, 1), (2, kid_hex.decode('hex'))]
        if 'provider' in fields:
            protobuf_fields.append((3, fields['provider']))
        if 'content_id' in fields:
            protobuf_fields.append((4, fields['content_id'].decode('hex')))
        if 'policy' in fields:
            protobuf_fields.append((6, fields['policy']))
        return WidevineMakeHeader(protobuf_fields)

    return ""

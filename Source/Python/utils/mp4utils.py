import collections
from functools import reduce

__author__    = 'Gilles Boccon-Gibod (bok@bok.net)'
__copyright__ = 'Copyright 2011-2020 Axiomatic Systems, LLC.'

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
import base64

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
    "aa": "Qafara",
    "ab": "Аҧсуа",
    "ae": "Avesta",
    "af": "Afrikaans",
    "ak": "Akana",
    "am": "አማርኛ",
    "an": "Aragonés",
    "ar": "العربية",
    "as": "অসমীয়া",
    "av": "авар мацӀ; магӀарул мацӀ",
    "ay": "Aymar Aru",
    "az": "Azərbaycanca",
    "ba": "башҡорт теле",
    "be": "Беларуская мова",
    "bg": "български език",
    "bh": "भोजपुरी",
    "bi": "Bislama",
    "bm": "Bamanankan",
    "bn": "বাংলা",
    "bo": "བོད་ཡིག",
    "br": "Brezhoneg",
    "bs": "Bosanski",
    "ca": "Català",
    "ce": "нохчийн мотт",
    "ch": "Chamoru",
    "co": "Corsu",
    "cr": "ᓀᐦᐃᔭᐍᐏᐣ",
    "cs": "čeština",
    "cu": "ѩзыкъ словѣньскъ",
    "cv": "чӑваш чӗлхи",
    "cy": "Cymraeg",
    "da": "Dansk",
    "de": "Deutsch",
    "dz": "རྫོང་ཁ",
    "ee": "Ɛʋɛgbɛ",
    "el": "Ελληνικά",
    "en": "English",
    "eo": "Esperanto",
    "es": "Español",
    "et": "Eesti Keel",
    "eu": "Euskara",
    "fa": "فارسی",
    "ff": "Fulfulde",
    "fi": "Suomi",
    "fj": "Vosa Vakaviti",
    "fo": "Føroyskt",
    "fr": "Français",
    "fy": "Frysk",
    "ga": "Gaeilge",
    "gd": "Gàidhlig",
    "gl": "Galego",
    "gn": "Avañe'ẽ",
    "gu": "ગુજરાતી",
    "gv": "Gaelg; Manninagh",
    "ha": "Hausancī; هَوُسَ",
    "he": "עִבְרִית; עברית",
    "hi": "हिन्दी",
    "ho": "Hiri Motu",
    "hr": "Hrvatski",
    "ht": "Kreyòl ayisyen",
    "hu": "Magyar",
    "hy": "Հայերեն լեզու",
    "hz": "Otjiherero",
    "ia": "Interlingua",
    "id": "Bahasa Indonesia",
    "ie": "Interlingue",
    "ig": "Igbo",
    "ii": "ꆇꉙ",
    "ik": "Iñupiaq; Iñupiatun",
    "io": "Ido",
    "is": "íslenska",
    "it": "Italiano",
    "iu": "ᐃᓄᒃᑎᑐᑦ",
    "ja": "日本語",
    "ka": "ქართული ენა (kartuli ena)",
    "kg": "Kikongo",
    "ki": "Gĩkũyũ",
    "kj": "Kuanyama",
    "kk": "Қазақ тілі",
    "kl": "Kalaallisut",
    "km": "ភាសាខ្មែរ",
    "kn": "ಕನ್ನಡ",
    "ko": "한국어 (韓國語); 조선말 (朝鮮語)",
    "kr": "Kanuri",
    "ks": "कॉशुर; کٲشُر",
    "ku": "Kurdî; كوردي",
    "kv": "коми кыв",
    "kw": "Kernewek",
    "ky": "кыргыз тили",
    "la": "latine; lingua Latina",
    "lb": "Lëtzebuergesch",
    "lg": "Luganda",
    "li": "Limburgs",
    "ln": "Lingala",
    "lo": "ພາສາລາວ",
    "lt": "Lietuvių Kalba",
    "lv": "Latviešu Valoda",
    "mg": "Malagasy fiteny",
    "mh": "Kajin M̧ajeļ",
    "mi": "Te Reo Māori",
    "mk": "македонски јазик",
    "ml": "മലയാളം",
    "mn": "монгол хэл",
    "mr": "मराठी",
    "ms": "Bahasa Melayu; بهاس ملايو",
    "mt": "Malti",
    "my": "မြန်မာစာ",
    "na": "Ekakairũ Naoero",
    "nb": "Bokmål",
    "nd": "isiNdebele",
    "ne": "नेपाली",
    "ng": "Owambo",
    "nl": "Nederlands",
    "nn": "Nynorsk",
    "no": "Norsk",
    "nr": "isiNdebele",
    "nv": "Diné bizaad; Dinékʼehǰí",
    "ny": "chiCheŵa; chinyanja",
    "oc": "Occitan",
    "oj": "ᐊᓂᔑᓇᐯᒧᐏᐣ (Anishinaabemowin)",
    "om": "Afaan Oromoo",
    "or": "ଓଡ଼ିଆ",
    "os": "ирон ӕвзаг",
    "pa": "ਪੰਜਾਬੀ; پنجابی",
    "pi": "पालि",
    "pl": "Polski",
    "ps": "پښتو",
    "pt": "Português",
    "qu": "Runa Simi; Kichwa",
    "rm": "Rumantsch Grischun",
    "rn": "Rundi",
    "ro": "Română",
    "ru": "Русский",
    "rw": "Ikinyarwanda",
    "sa": "संस्कृतम्",
    "sc": "Sardu",
    "sd": "سنڌي، سندھی; सिन्धी",
    "se": "sámi; sámegiella",
    "sg": "yângâ tî sängö",
    "si": "සිංහල",
    "sk": "Slovenčina",
    "sl": "Slovenščina",
    "sm": "Gagana fa'a Samoa",
    "sn": "chiShona",
    "so": "Soomaaliga; af Soomaali",
    "sq": "Shqip",
    "sr": "српски језик; srpski jezik",
    "ss": "siSwati",
    "st": "Sesotho",
    "su": "Basa Sunda",
    "sv": "Svenska",
    "sw": "Kiswahili",
    "ta": "தமிழ்",
    "te": "తెలుగు",
    "tg": "тоҷикӣ; تاجیکی",
    "th": "ภาษาไทย",
    "ti": "ትግርኛ",
    "tk": "Түркмен",
    "tl": "Wikang Tagalog; ᜏᜒᜃᜅ᜔ ᜆᜄᜎᜓᜄ᜔",
    "tn": "Setswana",
    "to": "Faka-Tonga",
    "tr": "Türkçe",
    "ts": "Xitsonga",
    "tt": "татарча; tatarça; تاتارچا",
    "tw": "Twi",
    "ty": "te reo Tahiti; te reo Māʼohi",
    "ug": "Uyƣurqə; Uyğurçe; ئۇيغۇرچ",
    "uk": "українська мова",
    "ur": "اردو",
    "uz": "O'zbek; Ўзбек; أۇزبېك",
    "ve": "Tshivenḓa",
    "vi": "Tiếng Việt",
    "vo": "Volapük",
    "wa": "Walon",
    "wo": "Wolof",
    "xh": "isiXhosa",
    "yi": "ייִדיש",
    "yo": "Yorùbá",
    "za": "Saɯ cueŋƅ; Saw cuengh",
    "zh": "漢語; 汉语; 中文",
    "zu": "isiZulu",
    "und": "Unknown"
}

def PrintErrorAndExit(message):
    sys.stderr.write(message+'\n')
    sys.exit(1)

def XmlDuration(d):
    h  = int(d) // 3600
    d -= h*3600
    m  = int(d) // 60
    s  = d-m*60
    xsd = 'PT'
    if h:
        xsd += str(h)+'H'
    if h or m:
        xsd += str(m)+'M'
    if s:
        xsd += ('%.3fS' % (s))
    return xsd

def BooleanFromString(string):
    if string is None:
        return False
    return string.lower() in ['yes', 'true', 'on', '1']

def Base64Encode(x):
    return base64.b64encode(x).decode('ascii')

def Base64Decode(x):
    return base64.b64decode(x)

def Bento4Command(options, name, *args, **kwargs):
    executable = path.join(options.exec_dir, name) if options.exec_dir != '-' else name
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
        print('COMMAND: ', " ".join(cmd), cmd)
    try:
        try:
            return check_output(cmd)
        except OSError as e:
            if options.debug:
                print('executable ' + executable + ' not found in exec_dir, trying with PATH')
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

def Mp4IframeIndex(options, input_filename, *args, **kwargs):
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
            type = file.read(4).decode('ascii')
            if type == until:
                break
            if size == 1:
                size = struct.unpack('>Q', file.read(8))[0]
            atoms.append(Mp4Atom(type, size, cursor))
            cursor += size
            file.seek(cursor)
        except Exception:
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
        if not children: return None
        top = children[0]
    return top

class Mp4Track:
    def __init__(self, parent, info):
        self.parent                   = parent
        self.info                     = info
        self.default_sample_duration  = 0
        self.timescale                = 0
        self.moofs                    = []
        self.sample_counts            = []
        self.segment_sizes            = []
        self.segment_durations        = []
        self.segment_scaled_durations = []
        self.segment_bitrates         = []
        self.total_sample_count       = 0
        self.total_duration           = 0
        self.total_scaled_duration    = 0
        self.media_size               = 0
        self.average_segment_duration = 0
        self.average_segment_bitrate  = 0
        self.max_segment_bitrate      = 0
        self.bandwidth                = 0
        self.language                 = ''
        self.language_name            = ''
        self.order_index              = 0
        self.key_info                 = {}
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
            # Set the default values for Dolby audio codec flags
            self.dolby_ddp_atmos = 'No'
            self.dolby_ac4_ims   = 'No'
            if self.codec_family == 'ec-3' and 'dolby_digital_plus_info' in sample_desc:
                self.channels = GetDolbyDigitalPlusChannels(self)[0]
                self.dolby_ddp_atmos = sample_desc['dolby_digital_plus_info']['Dolby_Atmos']
                if (self.dolby_ddp_atmos == 'Yes'):
                    self.complexity_index = sample_desc['dolby_digital_plus_info']['complexity_index']
                    self.channels =  str(self.info['sample_descriptions'][0]['dolby_digital_plus_info']['complexity_index']) + '/JOC'
            elif self.codec_family == 'ac-4' and 'dolby_ac4_info' in sample_desc:
                if sample_desc['dolby_ac4_info']['dsi version'] == 0:
                    raise Exception("AC4 dsi version 0 is deprecated.")
                elif sample_desc['dolby_ac4_info']['dsi version'] == 1:
                    if sample_desc['dolby_ac4_info']['bitstream version'] == 0:
                        raise Exception("AC4 bitstream version 0 is deprecated.")
                    if len(sample_desc['dolby_ac4_info']['presentations']):
                        stream_type = sample_desc['dolby_ac4_info']['presentations'][0]['Stream Type']
                        if stream_type == 'Immersive stereo':
                            self.dolby_ac4_ims = 'Yes'
                        self.channels = str(self.channels)

        self.language = info['language']
        self.language_name = LanguageNames.get(LanguageCodeMap.get(self.language, 'und'), '')

    def update(self, options):
        # compute the total number of samples
        self.total_sample_count = reduce(operator.add, self.sample_counts, 0)

        # compute the total duration
        self.total_duration = reduce(operator.add, self.segment_durations, 0)
        self.total_scaled_duration = reduce(operator.add, self.segment_scaled_durations, 0)

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
            if tkhd['id'] == self.id:
                tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'encv', 'sinf', 'schi', 'tenc'))
                if tenc is None:
                    tenc = FindChild(trak, ('mdia', 'minf', 'stbl', 'stsd', 'enca', 'sinf', 'schi', 'tenc'))
                if tenc and 'default_KID' in tenc:
                    self.key_info['kid'] = tenc['default_KID'].strip('[]').replace(' ', '')
                break

    def __repr__(self):
        return 'File '+str(self.parent.file_list_index)+'#'+str(self.id)

class Mp4File:
    def __init__(self, options, media_source):
        self.media_source    = media_source
        self.info            = media_source.mp4_info
        self.tracks          = {}
        self.file_list_index = 0 # used to keep a sequence number just amongst all sources

        filename = media_source.filename
        if options.debug:
            print('Processing MP4 file', filename)

        # by default, the media name is the basename of the source file
        self.media_name = path.basename(filename)

        # walk the atom structure
        self.atoms = WalkAtoms(filename)
        self.segments = []
        for atom in self.atoms:
            if atom.type == 'moov':
                self.init_segment = atom
            elif atom.type == 'moof':
                self.segments.append([atom])
            else:
                if self.segments:
                    self.segments[-1].append(atom)
        #print self.segments
        if options.debug:
            print('  found', len(self.segments), 'segments')

        for track in self.info['tracks']:
            self.tracks[track['id']] = Mp4Track(self, track)

        # get a complete file dump
        json_dump = Mp4Dump(options, filename, format='json', verbosity='1')
        self.tree = json.loads(json_dump, strict=False, object_pairs_hook=collections.OrderedDict)

        # look for KIDs
        for track in self.tracks.values():
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
                    for entry in trun['entries']:
                        sample_duration = int(entry.get('d', default_sample_duration))
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
                for (name, value) in list(tfra.items()):
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
            for track in self.tracks.values():
                print('Track ID                     =', track.id)
                print('    Segment Count            =', len(track.segment_durations))
                print('    Type                     =', track.type)
                print('    Sample Count             =', track.total_sample_count)
                print('    Average segment bitrate  =', track.average_segment_bitrate)
                print('    Max segment bitrate      =', track.max_segment_bitrate)
                print('    Required bandwidth       =', int(track.bandwidth))
                print('    Average segment duration =', track.average_segment_duration)

    def find_track_by_id(self, track_id_to_find):
        for track_id in self.tracks:
            if track_id_to_find == 0 or track_id_to_find == track_id:
                return self.tracks[track_id]

        return None

    def find_tracks_by_type(self, track_type_to_find):
        return [track for track in list(self.tracks.values()) if track_type_to_find == '' or track_type_to_find == track.type]

class MediaSource:
    def __init__(self, options, name):
        self.name = name
        self.mp4_info = None
        self.key_infos = {} # key infos indexed by track ID
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

        # if the file is an mp4 file, get the mp4 info now
        if self.format == 'mp4':
            json_info = Mp4Info(options, self.filename, format='json', fast=True)
            self.mp4_info = json.loads(json_info, strict=False)

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

def MakeNewDir(dir, exit_if_exists=False, severity=None, recursive=False):
    if path.exists(dir):
        if severity:
            sys.stderr.write(severity+': ')
            sys.stderr.write('directory "'+dir+'" already exists\n')
        if exit_if_exists:
            sys.exit(1)
    elif recursive:
        os.makedirs(dir)
    else:
        os.mkdir(dir)

def MakePsshBox(system_id, payload):
    pssh_size = 12+16+4+len(payload)
    return struct.pack('>I', pssh_size)+b'pssh'+struct.pack('>I',0)+system_id+struct.pack('>I', len(payload))+payload

def MakePsshBoxV1(system_id, kids, payload):
    pssh_size = 12+16+4+(16*len(kids))+4+len(payload)
    pssh = struct.pack('>I', pssh_size)+b'pssh'+struct.pack('>I',0x01000000)+system_id+struct.pack('>I', len(kids))
    for kid in kids:
        pssh += bytes.fromhex(kid)
    pssh += struct.pack('>I', len(payload))+payload
    return pssh

def GetEncryptionKey(options, spec):
    if options.debug:
        print('Resolving KID and Key from spec:', spec)
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

def GetDolbyDigitalPlusChannels(track):
    sample_desc = track.info['sample_descriptions'][0]
    if 'dolby_digital_plus_info' not in sample_desc and 'dolby_digital_info' not in sample_desc:
        return (track.channels, [])
    elif 'dolby_digital_plus_info' in sample_desc:
        if 'substreams' in sample_desc['dolby_digital_plus_info']:
            ddp_or_dd_info = sample_desc['dolby_digital_plus_info']['substreams'][0]
    elif 'dolby_digital_info' in sample_desc:
        if 'stream_info' in sample_desc['dolby_digital_info']:
            ddp_or_dd_info = sample_desc['dolby_digital_info']['stream_info']

    channels = DolbyDigital_acmod[ddp_or_dd_info['acmod']][:]
    if ddp_or_dd_info['lfeon'] == 1:
        channels.append('LFE')
    if 'num_dep_sub' in ddp_or_dd_info and ddp_or_dd_info['num_dep_sub'] and 'chan_loc' in ddp_or_dd_info:
        chan_loc_value = ddp_or_dd_info['chan_loc']
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

def ComputeDolbyDigitalPlusAudioChannelConfig(track):
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
    (channel_count, channels) = GetDolbyDigitalPlusChannels(track)
    if not channels:
        return str(channel_count)
    config = 0
    for channel in channels:
        if channel in flags:
            config |= flags[channel]
    return hex(config).upper()[2:]

# ETSI TS 102 366 V1.4.1 (2017-09) Table I.1.1
def DolbyDigitalWithMPEGDASHScheme(mask):
    available_mask_dict = {'4000': '1' , 'A000': '2' , 'E000': '3' , 'E100': '4' ,
                           'F800': '5' , 'F801': '6' , 'F821': '7' , 'A100': '9' ,
                           'B800': '10', 'E301': '11', 'FA01': '12', 'F811': '14',
                           'F815': '16', 'F89D': '17', 'E255': '19'}
    if mask in available_mask_dict:
        return (True, available_mask_dict[mask])
    else:
        return (False, mask)

def ComputeDolbyAc4AudioChannelConfig(track):
    sample_desc = track.info['sample_descriptions'][0]
    if 'dolby_ac4_info' in sample_desc:
        dolby_ac4_info = sample_desc['dolby_ac4_info']
        if 'presentations' in dolby_ac4_info and dolby_ac4_info['presentations']:
            presentation = dolby_ac4_info['presentations'][0]
            if 'presentation_channel_mask_v1' in presentation:
                return '%06x' % presentation['presentation_channel_mask_v1']

    return '000000'

# ETSI TS 103 190-2 V1.2.1 (2018-02) Table G.1
def DolbyAc4WithMPEGDASHScheme(mask):
    available_mask_dict = {
        '000002' : '1' , '000001' : '2' , '000003' : '3' , '008003' : '4' ,
        '000007' : '5' , '000047' : '6' , '020047' : '7' , '008001' : '9' ,
        '000005' : '10', '008047' : '11', '00004F' : '12', '02FF7F' : '13',
        '06FF6F' : '13', '000057' : '14', '040047' : '14', '00145F' : '15',
        '04144F' : '15', '000077' : '16', '040067' : '16', '000A77' : '17',
        '040A67' : '17', '000A7F' : '18', '040A6F' : '18', '00007F' : '19',
        '04006F' : '19', '01007F' : '20', '05006F' : '2'}
    if mask in available_mask_dict:
        return (True, available_mask_dict[mask])
    else:
        return (False, mask)

def ReGroupEC3Sets(audio_sets):
    regroup_audio_sets    = {}
    audio_adaptation_sets = {}
    for name, audio_tracks in audio_sets.items():
        if audio_tracks[0].codec_family == 'ec-3':
            for track in audio_tracks:
                if track.info['sample_descriptions'][0]['dolby_digital_plus_info']['atmos'] == 'Yes':
                    adaptation_set_name = ('audio', track.language, track.codec_family, track.channels, 'ATMOS')
                else:
                    adaptation_set_name = ('audio', track.language, track.codec_family, track.channels)
                adaptation_set = audio_adaptation_sets.get(adaptation_set_name, [])
                audio_adaptation_sets[adaptation_set_name] = adaptation_set
                adaptation_set.append(track)
        else:
            regroup_audio_sets[name] = audio_tracks

    for name, tracks in audio_adaptation_sets.items():
        regroup_audio_sets[name] = tracks

    return regroup_audio_sets

def ComputeDolbyDigitalPlusAudioChannelMask(track):
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
    (channel_count, channels) = GetDolbyDigitalPlusChannels(track)
    if not channels:
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

def ComputeDolbyDigitalPlusSmoothStreamingInfo(track):
    (channel_count, channel_mask) = ComputeDolbyDigitalPlusAudioChannelMask(track)
    info = "0006" # 1536 in little-endian
    mask_hex_be = "{0:0{1}x}".format(channel_mask, 4)
    info += mask_hex_be[2:4]+mask_hex_be[0:2]+'0000'
    info += "af87fba7022dfb42a4d405cd93843bdd"
    info += track.info['sample_descriptions'][0]['dolby_digital_info']['dec3_payload']
    return (channel_count, info.lower())

def ComputeMarlinPssh(options):
    # create a dummy (empty) Marlin PSSH
    return struct.pack('>I4sI4sII', 24, b'marl', 16, b'mkid', 0, 0)

def DerivePlayReadyKey(seed, kid, swap=True):
    if len(seed) < 30:
        raise Exception('seed must be  >= 30 bytes')
    if len(kid) != 16:
        raise Exception('kid must be 16 bytes')

    if swap:
        kid = kid[3:4]+kid[2:3]+kid[1:2]+kid[0:1]+kid[5:6]+kid[4:5]+kid[7:8]+kid[6:7]+kid[8:]

    seed = seed[:30]

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha_A = sha.digest()

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha_B = sha.digest()

    sha = hashlib.sha256()
    sha.update(seed)
    sha.update(kid)
    sha.update(seed)
    sha.update(kid)
    sha_C = sha.digest()

    content_key = bytes([sha_A[i] ^ sha_A[i+16] ^ sha_B[i] ^ sha_B[i+16] ^ sha_C[i] ^ sha_C[i+16] for i in range(16)])

    return content_key

def ComputePlayReadyChecksum(kid, key):
    import aes
    return aes.rijndael(key).encrypt(kid)[:8]

def WrapPlayReadyHeaderXml(header_xml):
    # encode the XML header into UTF-16 little-endian
    header_utf16_le = header_xml.encode('utf-16-le')
    rm_record = struct.pack('<HH', 1, len(header_utf16_le))+header_utf16_le
    return struct.pack('<IH', len(rm_record)+6, 1)+rm_record

def ComputePlayReadyKeyInfo(key_spec):
    (kid_hex, key_hex) = key_spec
    kid = bytes.fromhex(kid_hex)
    kid = kid[3:4]+kid[2:3]+kid[1:2]+kid[0:1]+kid[5:6]+kid[4:5]+kid[7:8]+kid[6:7]+kid[8:]

    xml_key_checksum = None
    if key_hex:
        xml_key_checksum = Base64Encode(ComputePlayReadyChecksum(kid, bytes.fromhex(key_hex)))

    return (Base64Encode(kid), xml_key_checksum)

def ComputePlayReadyXmlKid(key_spec, algid):
    (xml_kid, xml_key_checksum) = ComputePlayReadyKeyInfo(key_spec)

    # optional checkum
    xml_key_checksum_attribute = ''
    if xml_key_checksum:
        xml_key_checksum_attribute = 'CHECKSUM="' + xml_key_checksum + '" '

    # KID
    return '<KID ALGID="%s" %sVALUE="%s"></KID>' % (algid, xml_key_checksum_attribute, xml_kid)

def ComputePlayReadyHeader(version, header_spec, encryption_scheme, key_specs):
    # map the encryption scheme and an algorithm ID
    scheme_to_id_map = {
        'cenc': 'AESCTR',
        'cens': 'AESCTR',
        'cbc1': 'AESCBC',
        'cbcs': 'AESCBC'
    }
    if encryption_scheme not in scheme_to_id_map:
        raise Exception('Encryption scheme not supported by PlayReady')
    algid = scheme_to_id_map[encryption_scheme]

    # check that the algorithm is supported
    if algid == 'AESCBC' and version < "4.3":
        raise Exception('AESCBC requires PlayReady 4.3 or higher')

    # construct the base64 header
    if header_spec is None:
        header_spec = ''
    if header_spec.startswith('#'):
        header_b64 = header_spec[1:]
        header = Base64Decode(header_b64)
        if not header:
            raise Exception('invalid base64 encoding')
        return header
    elif header_spec.startswith('@') or path.exists(header_spec):
        # check that the file exists
        if header_spec.startswith('@'):
            header_spec = header_spec[1:]
            if not path.exists(header_spec):
                raise Exception('header data file does not exist')

        # read the header from the file
        header = open(header_spec, 'rb').read()
        header_xml = None
        if (header[0] == 0xff and header[1] == 0xfe) or (header[0] == 0xfe and header[1] == 0xff):
            # this is UTF-16 XML
            header_xml = header.decode('utf-16')
        elif header[0] == '<' and header[1] != 0x00:
            # this is ASCII or UTF-8 XML
            header_xml = header.decode('utf-8')
        elif header[0] == '<' and header[1] == 0x00:
            # this UTF-16LE XML without charset header
            header_xml = header.decode('utf-16-le')
        if header_xml is not None:
            header = WrapPlayReadyHeaderXml(header_xml)
        return header
    else:
        try:
            pairs = header_spec.split('#')
            fields = {}
            for pair in pairs:
                if not pair: continue
                name, value = pair.split(':', 1)
                fields[name] = value
        except:
            raise Exception('invalid syntax for argument')

        xml_protectinfo = ''
        xml_extras      = ''
        if version == "4.0":
            # 4.0 version only
            if len(key_specs) != 1:
                raise Exception("PlayReady 4.0 only supports 1 key")

            (xml_kid, xml_key_checksum) = ComputePlayReadyKeyInfo(key_specs[0])
            xml_protectinfo = '<KEYLEN>16</KEYLEN><ALGID>AESCTR</ALGID>'
            xml_extras = '<KID>' + xml_kid +'</KID>'
            if xml_key_checksum:
                xml_extras += '<CHECKSUM>' + xml_key_checksum + '</CHECKSUM>'
        else:
            # 4.1 and above
            if version == "4.1":
                # 4.1 only
                if len(key_specs) != 1:
                    raise Exception("PlayReady 4.1 only supports 1 key")

                # single KID
                xml_protectinfo += ComputePlayReadyXmlKid(key_specs[0], algid)
            else:
                # 4.2 and above

                # list of KIDS
                xml_protectinfo += '<KIDS>'
                for key_spec in key_specs:
                    xml_protectinfo += ComputePlayReadyXmlKid(key_spec, algid)
                xml_protectinfo += '</KIDS>'

        header_xml = '<WRMHEADER xmlns="http://schemas.microsoft.com/DRM/2007/03/PlayReadyHeader" version="%s.0.0"><DATA>' % version
        header_xml += '<PROTECTINFO>' + xml_protectinfo + '</PROTECTINFO>'
        header_xml += xml_extras
        if 'CUSTOMATTRIBUTES' in fields:
            header_xml += '<CUSTOMATTRIBUTES>'+Base64Decode(fields['CUSTOMATTRIBUTES']).decode('utf-8').replace('\n', '')+'</CUSTOMATTRIBUTES>'
        if 'LA_URL' in fields:
            header_xml += '<LA_URL>'+saxutils.escape(fields['LA_URL'])+'</LA_URL>'
        if 'LUI_URL' in fields:
            header_xml += '<LUI_URL>'+saxutils.escape(fields['LUI_URL'])+'</LUI_URL>'
        if 'DS_ID' in fields:
            header_xml += '<DS_ID>'+saxutils.escape(fields['DS_ID'])+'</DS_ID>'

        header_xml += '</DATA></WRMHEADER>'
        return WrapPlayReadyHeaderXml(header_xml)

def ComputePrimetimeMetaData(metadata_spec, kid_hex):
    # construct the base64 header
    if metadata_spec is None:
        metadata_spec = ''
    if metadata_spec.startswith('#'):
        metadata_b64 = metadata_spec[1:]
        metadata = Base64Decode(metadata_b64)
        if not metadata:
            raise Exception('invalid base64 encoding')
    elif metadata_spec.startswith('@'):
        metadata_filename = metadata_spec[1:]
        if not path.exists(metadata_filename):
            raise Exception('data file does not exist')

        # read the header from the file
        metadata = open(metadata_filename, 'rb').read()

    amet_size = 12+4+16
    amet_flags = 0
    if metadata:
        amet_flags |= 2
        amet_size += 4+len(metadata)
    amet_box = struct.pack('>I4sII', amet_size, 'amet', amet_flags, 1) + bytes.fromhex(kid_hex)
    if metadata:
        amet_box += struct.pack('>I', len(metadata))+metadata

    return amet_box

def WidevineVarInt(value):
    parts = [value % 128]
    value >>= 7
    while value:
        parts.append(value % 128)
        value >>= 7
    for i in range(len(parts) - 1):
        parts[i] |= (1<<7)
    return bytes(parts)

def WidevineMakeHeader(fields):
    buffer = b''
    for (field_num, field_val) in fields:
        if type(field_val) == int:
            wire_type = 0 # varint
            wire_val = WidevineVarInt(field_val)
        elif type(field_val) == str:
            wire_type = 2
            wire_val = WidevineVarInt(len(field_val)) + field_val.encode('ascii')
        elif type(field_val) == bytes:
            wire_type = 2
            wire_val = WidevineVarInt(len(field_val)) + field_val
        buffer += bytes([(field_num << 3) | wire_type]) + wire_val
    return buffer

def ComputeWidevineHeader(header_spec, encryption_scheme, kid_hex):
    try:
        pairs = header_spec.split('#')
        fields = {}
        for pair in pairs:
            name, value = pair.split(':', 1)
            fields[name] = value
    except:
        raise Exception('invalid syntax for argument')

    protobuf_fields = [(2, bytes.fromhex(kid_hex))]
    if 'provider' in fields:
        protobuf_fields.append((3, fields['provider']))
    if 'content_id' in fields:
        protobuf_fields.append((4, bytes.fromhex(fields['content_id'])))
    if 'policy' in fields:
        protobuf_fields.append((6, fields['policy']))

    if encryption_scheme == 'cenc':
        protobuf_fields.append((1, 1))

    four_cc = struct.unpack('>I', encryption_scheme.encode('ascii'))[0]
    protobuf_fields.append((9, four_cc))

    return WidevineMakeHeader(protobuf_fields)

#############################################
# Module Exports
#############################################
__all__ = [
    'LanguageCodeMap',
    'LanguageNames',
    'PrintErrorAndExit',
    'XmlDuration',
    'Base64Encode',
    'Base64Decode',
    'Bento4Command',
    'Mp4Info',
    'Mp4Dump',
    'Mp4Split',
    'Mp4Fragment',
    'Mp4Encrypt',
    'Mp42Hls',
    'Mp4IframeIndex',
    'WalkAtoms',
    'Mp4Track',
    'Mp4File',
    'MediaSource',
    'ComputeBandwidth',
    'MakeNewDir',
    'MakePsshBox',
    'MakePsshBoxV1',
    'GetEncryptionKey',
    'GetDolbyDigitalPlusChannels',
    'ComputeDolbyDigitalPlusAudioChannelConfig',
    'ComputeDolbyAc4AudioChannelConfig',
    'ComputeDolbyDigitalPlusSmoothStreamingInfo',
    'ComputeMarlinPssh',
    'DerivePlayReadyKey',
    'ComputePlayReadyHeader',
    'ComputePrimetimeMetaData',
    'ComputeWidevineHeader'
]

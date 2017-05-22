#!/usr/bin/env python

import ConfigParser, sys, re

# Mood invert index
mood_dic = {
"peaceful" : "65322",
"still" : "65322",
"romantic" : "65323",
"fantastic" : "65323",
"sentimental" : "65324",
"melting" : "65324",
"mushy": "65324",
"tender" : "42942",
"soft" : "42942",
"easygoing" : "42946",
"happy" : "42946",
"relaxed" : "42946",
"leisurable" : "42946" , 
"yearning" : "65325",
"eager":"65325",
"hungry": "65325",
"desired": "65325",
"anxious": "65325",
"sick": "65325",
"sophisticated" : "42954",
"experienced": "42954",
"sensual" : "42947",
"lay": "42947",
"cool" : "65326",
"calm": "65326",
"gritty" : "65327",
"somber" : "42948",
"severe": "42948",
"blue": "42948",
"dark": "42948",
"lowering": "42948",
"melancholy" : "42949",
"sad": "42949",
"serious" : "65328",
"solemn": "65328",
"brooding" : "65329",
"reflecting": "65329",
"fiery" : "42953",
"passionate": "42953",
"hotter": "42953",
"urgent" : "42955",
"instant": "42955",
"defiant" : "42951",
"challenging": "42951",
"aggressive" : "42958",
"invasive": "42958",
"enterprising": "42958",
"rowdy" : "65330",
"clamorous" : "65330",
"excited" : "42960",
"active": "42960",
"living": "42960",
"hot": "42960",
"energizing" : "42961",
"motivating": "42961",
"urging": "42961",
"empowering" : "42945",
"stirring" : "65331",
"engaged": "65331",
"lively" : "65332",
"alive": "65332",
"sincere": "65332",
"upbeat" : "65333",
"optimistic" : "65333",
"rising": "65333",
"other" : "42966"
}

# Scan the keyword from the dict
def keyword_scan(question):
	word = re.sub(r'[^a-zA-Z0-9]+', ' ', question).strip().lower()
	word_list = word.split()
	output = []
	for item in word_list:
		if item in mood_dic.keys():
			output.append(item)
	if len(output) == 0:
		return ""
	else:
		return output[0]

class FakeSecHead(object):
    def __init__(self, fp):
        self.fp = fp
        self.sechead = '[asection]\n'

    def readline(self):
        if self.sechead:
            try: 
                return self.sechead
            finally: 
                self.sechead = None
        else: 
            return self.fp.readline()

cp = ConfigParser.SafeConfigParser()
cp.readfp(FakeSecHead(open("../../config.properties")))
port_dic = dict(cp.items('asection'))
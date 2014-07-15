#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
from xml.etree.ElementTree import ElementTree
from optparse import OptionParser, OptionGroup
from collections import defaultdict

def main(options):
    alphas = ElementTree()
    try:
        alphas = alphas.parse(options.alphas).text
    except:
        raise Exception("Can't parse the file with warping factors!")

    alphas = alphas.strip().split()

    # parse log files and store best values in a dict "seg -> (score, alpha)"
    weights = {}
    tagname = "corpus-key-to-score-accumulator-map/score-accumulator"
    for a in alphas:
        scores = ElementTree()
        filename = "%s/%s%s" % (options.prefix, a, options.suffix)
        try:
            scores.parse(filename)
            for s in scores.findall(tagname):
                seg = s.attrib["corpus-key"]
                score = float(s.find("weighted-sum-of-scores").text)
                if seg in weights:
                    if score < weights[seg][0]:
                        weights[seg] = (score, a)
                else:
                    weights[seg] = (score, a)
        except:
            raise Exception("Can't parse log file '%s'!" % filename)

    # write map
    seen_alphas = defaultdict(int)
    try:
        m = open(options.warping_map, 'w')
        m.write("<?xml version='1.0' encoding='utf-8'?>\n")
        m.write("<coprus-key-map>\n")

        for seg in sorted(weights.keys()):
            a = weights[seg][1]
            m.write("  <map-item key='%s' value='%s'/>\n" % (seg, a))
            seen_alphas[a] += 1

        m.write("</coprus-key-map>\n")
        m.close()
    except:
        raise Exception("Failed writing the map to '%s'!" %
                        options.warping_map)

    # write reduced alphas
    try:
        m = open(options.reduced_alphas, 'w')
        m.write("<?xml version='1.0' encoding='utf-8'?>\n")
        m.write("<vector-string>\n")

        for a in sorted(seen_alphas.keys()):
            m.write("\t%s\n" % a)
        m.write("</vector-string>\n")
        m.close()
    except:
        raise Exception("Failed writing the reduced list to '%s'!" %
                        options.reduced_alphas)

###############################################################################

if __name__ == "__main__":
    parser = OptionParser()

    group = OptionGroup(parser, "Input", "The tool will walk through "
                        "<prefix>/<alpha><suffix> and select the best "
                        "warping factor for each segment.")
    parser.add_option_group(group)

    group.add_option("-p", "--prefix", dest="prefix", default="log/",
                     help="path prefix to the scoring logs")
    group.add_option("-s", "--suffix", dest="suffix", default=".score",
                     help="path suffix to the scoring logs")
    group.add_option("-a", "--alphas", dest="alphas", metavar="FILE",
                     help="path to the list of warping factors")

    group = OptionGroup(parser, "Output")
    parser.add_option_group(group)
    group.add_option("-r", "--reduced-alphas", dest="reduced_alphas",
                     metavar="FILE",
                     help="path to the list of seen warping factors")
    group.add_option("-m", "--map", dest="warping_map", metavar="FILE",
                     help="path to the output map of warping factors")
    
    (options, args) = parser.parse_args()
    if None in [options.alphas, options.reduced_alphas, options.warping_map]:
        parser.error("Need to set -a, -r and -m! (see -h)")
    main(options)

#!/usr/python

import os, glob, sys
from os.path import *

# [Video formats]
# [Audio formats]
# [Image formats]
# Supported -> Format(n)

field_map = {
    "RendererName":                    "General/Name",
    "RendererIcon":                    "General/Icon",
    "UserAgentSearch":                 "Recognize/UAMatch",
    "UserAgentAdditionalHeader":       "Recognize/AdditionalHeader",
    "UserAgentAdditionalHeaderSearch": "Recognize/AdditionalHeaderMatch",
    "Video":                           "Basic capabilites/Video",
    "Audio":                           "Basic capabilites/Audio",
    "Image":                           "Basic capabilites/Image",
    "SeekByTime":                      "MediaServer/SeekByTime",
    "DLNALocalizationRequired":        "MediaServer/ProtocolLocalization",
    "DLNAProfileChanges":              "MediaServer/ProfilePatches",
    "DLNAOrgPN":                       "MediaServer/Send_ORG_PN",
    "TranscodeVideo":                  "Transcode/Video",
    "TranscodeAudio":                  "Transcode/Audio",
    "MaxVideoBitrateMbps":             "Transcode/MaxVideoBitrateMbps",
    "MaxVideoWidth":                   "Transcode/MaxVideoWidth",
    "MaxVideoHeight":                  "Transcode/MaxVideoHeight",
    "H264Level41Limited":              "Transcode/MaxH264Level41",
    "TranscodeAudioTo441kHz":          "Transcode/441kHzAudio",
    "TranscodeFastStart":              "Transcode/FastStart",
    "TranscodedVideoFileSize":         "Transcode/VideoFileSize",
    "ForceJPGThumbnails":              "Transcode/ForceJPGThumbnails",
    "ThumbnailAsResource":             "Transcode/ThumbnailAsResource",
    "ChunkedTransfer":                 "Transcode/AllowChunkedTransfer",
    "AutoExifRotate":                  "Transcode/ExifAutoRotate"
}

def append(conf, key, value):
    k = key.split("/", 1)
    for c in conf:
        if c[0] == k[0]:
            c[1].append([k[1], value])
            return
    conf.append([k[0], [ [k[1], value] ]])

def translate(src, dst):
    print dst
    inf = open(src, "r")
    conf = []
    vids = []
    auds = []
    imgs = []
    for line in inf:
        line = line.strip().split("#", 1)[0]
        if line == "": continue
        line = line.split("=", 1)
        key = line[0].strip()
        value = line[1].strip()
        if key == "Supported":
            sup = {}
            subs = value.replace("\t", " ").split(" ")
            for sub in subs:
                if sub == "": continue
                sub = sub.split(':', 1)
                sup[sub[0].strip()] = sub[1].strip()
                pass
            if "m" in sup:
                m = sup["m"].split("/", 1)[0]
                if m == "video": vids.append(value); continue
                elif m == "audio": auds.append(value); continue
                elif m == "image": imgs.append(value); continue
            elif "v" in sup: vids.append(value); continue
            elif "a" in sup: auds.append(value); continue
            else: imgs.append(value)
            continue
        if key not in field_map: continue
        append(conf, field_map[key], value)
    inf.close()

    idx = 0
    for value in vids:
        append(conf, "Video formats/Format%s" % idx, value)
        idx+=1
    idx = 0
    for value in auds:
        append(conf, "Audio formats/Format%s" % idx, value)
        idx+=1
    idx = 0
    for value in imgs:
        append(conf, "Image formats/Format%s" % idx, value)
        idx+=1

    outf = open(dst, "w")
    for c in conf:
        print >>outf, "[%s]" % c[0]
        for e in c[1]:
            print >>outf, "%s = %s" % (e[0], e[1])
        print >>outf
    outf.close()

if len(sys.argv) < 3:
    print "pms-import.py [PMS DIR] [RESOURCES]"
    exit(1)

src = join(sys.argv[1], "renderers")
dst = join(sys.argv[2], "renderers")
src = glob.glob(join(src, "*.conf"))
for fname in src:
    here, conf = split(fname)
    translate(fname, join(dst, conf))

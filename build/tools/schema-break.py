import sys, os, re

def safe_name(path):
    path = os.path.split(path)[1]
    return re.sub("[._]+", "_", "".join(c for c in path if c.isalnum() or c in "._"))

def print_string(o, line):
    line = line.translate(None, "\r")
    line = re.sub("--.*\\n", "\n", line)
    line = re.sub("\\s+", " ", line).strip()
    text = line.split("\n")
    start = True
    for line in text:
        if start and line.strip() == "": continue
        start = False
        o.write("\n\t\"")
        o.write(line.replace("\\", "\\\\").replace("\"", "\\\""))
        o.write("\"")
    if not start: o.write(",")

if len(sys.argv) < 3:
    print "schema-break.py [INPUT] [OUTPUT]"
    exit(1)

try: f = open(sys.argv[1], "r")
except:
    print "Could not open", sys.argv[1], "for reading"
    exit(1)

try: o = open(sys.argv[2], "w")
except:
    print "Could not open", sys.argv[2], "for writing"
    exit(1)

o.write("static const char* %s[] = {" % safe_name(sys.argv[1]))
text = ''
for line in f:
    text += line

text = text.split(";")
for line in text:
    if line.strip() == "": continue
    print_string(o, line)

print >>o, "\n};"
#!/usr/bin/python2

import glob
import re


definitions = "";

for filename in glob.glob('static/*.html'):

  variable = filename.upper()
  variable = re.sub('[^A-Z0-9]', '_', variable)

  result = []
  for line in open(filename, 'r'):
    line = line.strip()

    if not line:
      continue

    line = re.sub('"', '\\"', line)
    result.append('"' + line + '"')

  definitions += "const PROGMEM char* " + variable + " = " + "\n".join(result) + ";\n"


result = """
#ifndef __STATIC_RESPONSE_H__
#define __STATIC_RESPONSE_H__

const PROGMEM char* TEXT_HTML = "text/html";

""" + definitions + """
#endif"""

print result

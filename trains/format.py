import sys
import re

pattern = re.compile("t\[(\d+)\]\[(\d+)\]\[(\d+)\] = (\d+)")

for raw_line in sys.stdin:
	lines = raw_line.split(';')
	for line in lines:
		match = pattern.match(line.strip())
		if match:
			sys.stdout.write("\ttrain->track_segment_ticks[%s][%s][%s] = %s;\n" % (match.group(1), match.group(2), match.group(3), match.group(4)))


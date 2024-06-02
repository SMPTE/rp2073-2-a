#!/usr/bin/env python
#
# Calculate the DPX encoding curve in ST 2073-7, section B.9.5.

from math import pow, log10

#r = 685.0
r = 985.0

p = 1.7

for x in range(1024):
	b = 1023.0 * pow(10.0, (1023.0 * (95.0/1023.0 - r/1023.0) * (p/1.7) * 0.002 / 0.6))
	x = x + b
	#if x < 0.001: x = 0.001
	x = max(x, 0.001) 
	y = (r/1023.0 + log10(x) / ((p/1.7) * 0.002 / 0.6)) / 1023.0
	print(x, y)
	
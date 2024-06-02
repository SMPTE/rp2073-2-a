#!/usr/bin/env python3
#
# Python script to display unformatted BYR4 image files

import os.path
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from PIL import Image

# Table that maps a pixel format to information that describes the format
pixel_info = {
	'byr4': {'type': np.uint16, 'count': 1},
	'rg48': {'type': np.uint16, 'count': 3},
	'b64a': {'type': np.uint16, 'count': 4},
	'ca32': {'type': np.uint32, 'count': 4},
	'nv12': {'type': np.uint8,  'count': 1}
}

def array_dimensions(height, width, format):
	"""Map the image dimensions and format to the tuple of array dimensions"""
	channel_count = pixel_info[format]['count']
	if channel_count > 1:
		return (height, width, channel_count)
	elif format == 'nv12':
		# The luma and chroma planes are stacked vertically
		return (height + height/2, width)
	else:
		return (height, width)

def read_array(template, width, height, format, count):
	"""Read the component array files into memory as a single array"""

	array = np.zeros((height, width, count), 'uint32')
	#array = np.zeros((height, width, count), 'uint16')

	for channel in range(count):
		pathname = template % channel
		data = open(pathname, "rb").read()
		type = pixel_info[format]['type']
		#shape = array_dimensions(height, width, format)
		array[:,:,channel] = np.reshape(np.frombuffer(data, dtype=type), (height, width))

	return array

def read_image(pathname, width, height, format):
	"""Read the entire image file into memory as a single array"""
	data = open(pathname, "rb").read()
	type = pixel_info[format]['type']
	shape = array_dimensions(height, width, format)
	#print(width, height, format, type, shape)
	return np.reshape(np.frombuffer(data, dtype=type), shape)

# Display an array as an image
def display(array, filename, channel, wavelet, band):
	"""Display the mage without interpolation"""
	scaled_array = array - np.min(array)
	scaled_array = (scaled_array/float(np.max(scaled_array))) * 255
	image = Image.fromarray(scaled_array.astype(np.uint8), 'L')
	#title = "%s-%d-%d-%d" % (filename, channel, wavelet, band)
	title = "%s channel: %d, wavelet: %d, band: %d" % (filename, channel, wavelet, band)
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

def display_byr4(array, title):
	"""Display an array of pixels in BYR4 format"""

	r1 = array[0::2,0::2]
	g1 = array[0::2,1::2]
	g2 = array[1::2,0::2]
	b2 = array[1::2,1::2]

	# Compute the dimensions of each component array
	width = array.shape[1] / 2
	height = array.shape[0] / 2

	# Average the two green component arrays
	g1 = (g1 + g2.astype(np.uint32)) / 2

	# Scale the input data to pixel range
	while np.max(r1) > 255 or np.max(g1) > 255 or np.max(b2) > 255:
		r1 >>= 1
		g1 >>= 1
		b2 >>= 1

	# Allocate an array for all three color components
	array = np.zeros((height, width, 3), 'uint8')

	# Combine the color planes into a single RGB image clipped to 8 bits
	array[:,:,0] = np.clip(r1, 0, 255)
	array[:,:,1] = np.clip(g1, 0, 255)
	array[:,:,2] = np.clip(b2, 0, 255)
	image = Image.fromarray(array)

	# Display the image without interpolation
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

def display_rg48(array, title):
	"""Display an array of pixels in RG48 format"""

	# Allocate an array for all three color components
	image = np.zeros(array.shape[:2] + (3,), 'uint8')

	# Scale the RGB pixel values to 8 bits
	image[:,:,0] = array[:,:,0] >> 8
	image[:,:,1] = array[:,:,1] >> 8
	image[:,:,2] = array[:,:,2] >> 8

	image = Image.fromarray(image)

	# Display the image without interpolation
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

def display_b64a(array, title):
	"""Display an array of pixels in B64A format"""

	# Allocate an array for all three color components
	image = np.zeros(array.shape[:2] + (3,), 'uint8')

	# Scale the RGB values to 8 bits
	#image[:,:,0] = array[:,:,2] >> 8
	#image[:,:,1] = array[:,:,1] >> 8
	#image[:,:,2] = array[:,:,0] >> 8
	image[:,:,0] = array[:,:,1]
	image[:,:,1] = array[:,:,2]
	image[:,:,2] = array[:,:,3]
	image = Image.fromarray(image)

	# Display the image without interpolation
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

def display_ca32(array, title):
	"""Display an array of pixels in CA32 format"""

	# Allocate an array for all three color components
	image = np.zeros(array.shape[:2] + (3,), 'uint8')

	# Average the green channels and scale the RGB values to 8 bits
	#shift = 4
	shift = 24
	image[:,:,0] = array[:,:,0] >> shift
	image[:,:,1] = (array[:,:,1] + array[:,:,2])/2 >> shift
	image[:,:,2] = array[:,:,3] >> shift

	image = Image.fromarray(image)

	# Display the image without interpolation
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

def display_nv12(array, title):
	"""Display an array of pixels in NV12 format"""

	# The height of the array is one and one-half times the image height
	(height, width) = array.shape[:2]
	height = (2 * height) / 3

	#print("Image height: %d" % height)

	# Allocate an array for all three color components
	image = np.zeros((height, width, 3), 'uint8')

	# This is a cheat -- need to use correct formulas for color conversion

	# Up sample the color difference components
	image[:,:,1] = array[:height,:]
	image[:,:,2] = np.repeat(np.repeat(array[height:,0::2], 2, axis=0), 2, axis=1)
	image[:,:,0] = np.repeat(np.repeat(array[height:,1::2], 2, axis=0), 2, axis=1)
	image = Image.fromarray(image)

	# Display the image without interpolation
	plt.imshow(image,interpolation='nearest')
	plt.title(title)
	plt.show()

# Table of image display routines indexed by pixel format
display_table = {
	'byr4': display_byr4,
	'rg48': display_rg48,
	'b64a': display_b64a,
	'ca32': display_ca32,
	'nv12': display_nv12,
}

if __name__ == "__main__":
	# Parse the command line arguments
	from argparse import ArgumentParser

	parser = ArgumentParser(conflict_handler='resolve')
	parser.add_argument('-w', '--width', type=int, help='number of samples per row')
	parser.add_argument('-h', '--height', type=int, help='number of rows')
	parser.add_argument('-v', '--verbose', action='store_true', help='enable verbose output for debugging')
	parser.add_argument('pathname', help='pathname to the image file')
	args = parser.parse_args()

	if args.width == None or args.height == None:
		print("Must provide the image width and height (in pixels)")
		exit(1)

	#print(args.pathname)
	pathname = args.pathname
	width = args.width
	height = args.height

	# Save the filename for creating plot titles
	(filename, extension) = os.path.splitext(os.path.basename(pathname))

	# Get the pixel format
	format = extension[1:]

	# Read the image array
	if format == 'ca32':
		array = read_array(pathname, width, height, format, pixel_info[format]['count'])
	else:
		array = read_image(pathname, width, height, format)

	# Call the display routine associated with the pixel format
	try:
		title = os.path.basename(pathname)
		display_table[format](array, title)
	except:
		print("Could not display image pixel format: %s" % format)

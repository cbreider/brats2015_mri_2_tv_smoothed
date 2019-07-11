"""
Lab Visualisation & Medical Image Analysis SS2019
Institute of Computer Science II

Author: Christian Breiderhoff
created on June 2019
"""

import nrrd
import SimpleITK as sitk
import os
import subprocess
import numpy as np
import png
import config as conf
import copy

def load_3d_volume_as_array(filename):
    if ('.mha' in filename):
        return load_mha_volume_as_array(filename)
    raise ValueError('{0:} unspported file format'.format(filename))


def load_mha_volume_as_array(filename):
    img = sitk.ReadImage(filename)
    nda = sitk.GetArrayFromImage(img)
    return nda


def save_3darray_to_pngs(data, filename, skip_emtpy=False, skip_if_exists=False):
	try:
		all = range(np.shape(data)[0])
		max_val = np.amax(data)
		if not conf.ground_truth_path_identifier.lower() in filename.lower():
			if float(max_val) != 0.0:
				data = data / max_val
				data = data * np.iinfo(np.uint8).max
			elif skip_emtpy:
				print("ERROR: values of {} are all 0!".format(filename))
				return
			data = data.astype(np.uint8)
		#else:
			 #_val = np.unique(data)
			#print(_val)
		for i in all:
			file_path = "{}_{}.png".format(filename, i)
			if os.path.exists(file_path) and skip_if_exists:
				continue
			sclice = data[i]
			if float(np.amax(sclice)) == 0.0 and skip_emtpy:
				#print("skipped sclice {} of {}".format(i, filename))
				continue
			with open(file_path, 'wb') as f:
				w = png.Writer(240, 240, greyscale=True, bitdepth=8)
				mha_slice = data[i]
				w.write(f, mha_slice)
				f.close()
			#if conf.ground_truth_path_identifier.lower() in filename.lower():
			#	r = png.Reader(filename=file_path)
			#	r_ar = r.read()
			#	print(np.unique(data))
	except Exception as e:
		print("type error: " + str(e))


def save_3darray_to_nrrd(array, file_path, skip_empty=False, skip_if_exists=False):
	try:
		all_slices = range(np.shape(array)[0])
		for i in all_slices:
			slice_arr = array[i]
			if float(np.amax(slice_arr)) == 0.0 and skip_empty:
				continue
			filename = "{}_{}{}".format(file_path, i, conf.in_ext)
			if os.path.exists(filename) and skip_if_exists:
				continue
			nrrd.write(filename, data=slice_arr)
	except Exception as e:
		print("type error: " + str(e))


def run_tvflow(command):
	popen = subprocess.call(command, stdout=subprocess.PIPE, shell=True)


def load_2d_nrrd_and_save_to_png(in_file, out_file, skip_emtpy=False):
	try:
		data, header = nrrd.read(in_file)
		max_val = np.amax(data)
		if float(max_val) != 0.0:
			data = data / max_val
			data = data * np.iinfo(np.uint8).max
		elif skip_emtpy:
			return
		data = data.astype(np.uint8)
		with open(out_file, 'wb') as f:
			w = png.Writer(240, 240, greyscale=True, bitdepth=8)
			w.write(f, data.copy())
			f.close()
	except Exception as e:
		print("type error: " + str(e))


def read_tvflow_arrs(folder):
	i = 0
	files = os.listdir(folder)
	files.sort()
	for f in files:
		in_file = os.path.join(folder, f)
		if os.path.isfile(in_file):
			readdata, header = nrrd.read(in_file)
			i+=1
import nrrd
import SimpleITK as sitk
import os
import subprocess
import numpy as np
import png
import config as conf


def load_3d_volume_as_array(filename):
    if ('.mha' in filename):
        return load_mha_volume_as_array(filename)
    raise ValueError('{0:} unspported file format'.format(filename))


def load_mha_volume_as_array(filename):
    img = sitk.ReadImage(filename)
    nda = sitk.GetArrayFromImage(img)
    return nda


def save_3darray_to_pngs(data, filename):
	try:
		all = range(np.shape(data)[0])
		max_val = np.amax(data)
		if float(max_val) != 0.0:
			data = data / max_val
			data = data * np.iinfo(np.uint8).max
		data = data.astype(np.uint8)
		for i in all:
			file_path = "{}_{}.png".format(filename, i)
			if os.path.exists(file_path):
				return
			with open(file_path, 'wb') as f:
				w = png.Writer(240, 240, greyscale=True, bitdepth=8)
				mha_slice = data[i]
				w.write(f, mha_slice)
				f.close()
	except Exception as e:
		print("type error: " + str(e))


def save_3darray_to_nrrd(array, file_path):
	try:
		all_slices = range(np.shape(array)[0])
		for i in all_slices:
			slice_arr = array[i]
			filename = "{}_{}{}".format(file_path, i, conf.in_ext)
			if os.path.exists(filename):
				return
			nrrd.write(filename, data=slice_arr)
	except Exception as e:
		print("type error: " + str(e))


def run_tvflow(command):
	popen = subprocess.call(command, stdout=subprocess.PIPE, shell=True)


def load_2d_nrrd_and_save_to_png(in_file, out_file):
	try:
		if os.path.exists(out_file):
			print("ret")
			return
		data, header = nrrd.read(in_file)
		max_val = np.amax(data)
		if float(max_val) != 0.0:
			data = data / max_val
			data = data * np.iinfo(np.uint8).max
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
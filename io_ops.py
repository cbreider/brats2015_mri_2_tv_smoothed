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
    all = range(np.shape(data)[0])
    for i in all:
        max_val = np.amax(data)
        if float(max_val) != 0.0:
            data = data / max_val
            data = data * np.iinfo(np.int16).max
        data = data.astype(np.int16)
        f = open("{}_{}.png".format(filename, i), 'wb')  # binary mode is important
        w = png.Writer(240, 240, greyscale=True, bitdepth=16)
        mha_slice = data[i]
        w.write(f, mha_slice)
        f.close()


def save_3darray_to_nrrd(array, file_path):
    all = range(np.shape(array)[0])
    for i in all:
        slice = array[i]
        nrrd.write("{}_{}{}".format(file_path, i, conf.in_ext), data=slice)


def run_tvflow(command):
    popen = subprocess.call(command, stdout=subprocess.PIPE, shell=True)


def load_2d_nrrd_and_save_to_png(in_file, out_file):
    data, header = nrrd.read(in_file)
    max_val = np.amax(data)
    if float(max_val) != 0.0:
        data = data / max_val
        data = data * np.iinfo(np.int16).max
    data = data.astype(np.int16)
    f = open(out_file, 'wb')  # binary mode is important
    w = png.Writer(240, 240, greyscale=True, bitdepth=16)
    w.write(f, data)
    f.close()


def read_tvflow_arrs(folder):
    i = 0
    files = os.listdir(folder)
    files.sort()
    for f in files:
        in_file = os.path.join(folder, f)
        if os.path.isfile(in_file):
            readdata, header = nrrd.read(in_file)
            i+=1
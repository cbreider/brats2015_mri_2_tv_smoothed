import nrrd
import SimpleITK as sitk
import os
import subprocess
import matplotlib.pyplot as plt
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


def save_3darray_to_pngs(array, folder, filename):
    all = range(np.shape(array)[0])
    max = np.amax(array)
    array = array / max *255
    max = np.amax(array)
    for i in all:
        f = open(os.path.join(folder, "{}_{}.png".format(filename, i)), 'wb')  # binary mode is important
        w = png.Writer(240, 240, greyscale=True, bitdepth=16)
        slice = array[i]
        plt.imshow(slice, 'gray')
        plt.show()
        w.write(f, slice)
        f.close()


def save_3darray_to_nrrd(array, file_path):
    all = range(np.shape(array)[0])
    for i in all:
        slice = array[i]
        #plt.imshow(slice, 'gray')
        #plt.show()
        nrrd.write("{}_{}{}".format(file_path, i, conf.in_ext), data=slice)


def run_tvflow(command):
    popen = subprocess.call(command, stdout=subprocess.PIPE, shell=True)


def read_tvflow_arrs(folder):
    i = 0
    files = os.listdir(folder)
    files.sort()
    for f in files:
        in_file = os.path.join(folder, f)
        if os.path.isfile(in_file):
            readdata, header = nrrd.read(in_file)
            plt.imshow(readdata, 'gray')
            plt.show()
            i+=1
"""
Lab Visualisation & Medical Image Analysis SS2019
Institute of Computer Science II

Author: Christian Breiderhoff
created on June 2019
"""


import os
import io_ops
import config as conf
import shutil

project_dir = os.path.dirname(os.path.realpath(__file__))

dirs = dict()
dirs["out_dir"] = os.path.join(project_dir, "test_in_out")
dirs["arr_dir"] = os.path.join(dirs["out_dir"], "nrrd")
dirs["png_dir"] = os.path.join(dirs["out_dir"], "png")
dirs["tvflow_png_out_dir"] = os.path.join(dirs["png_dir"], "tvflow")
dirs["tvflow_nrrd_out_dir"] = os.path.join(dirs["arr_dir"], "tvflow")
dirs["raw_png_dir"] = os.path.join(dirs["png_dir"], "raw")
dirs["raw_arr_dir"] = os.path.join(dirs["arr_dir"], "raw")

if os.path.exists(dirs["arr_dir"]):
	shutil.rmtree(dirs["arr_dir"])
if os.path.exists(dirs["png_dir"]):
	shutil.rmtree(dirs["png_dir"])

file_in = os.path.join(dirs["out_dir"], "test.mha")
file_in_gt = os.path.join(dirs["out_dir"], "test_gt.mha")
for k, v in dirs.items():
	if not os.path.exists(v):
		os.makedirs(v)


arr = io_ops.load_3d_volume_as_array(file_in)
io_ops.save_3darray_to_nrrd(arr, os.path.join(dirs["raw_arr_dir"], "test"))
io_ops.save_3darray_to_pngs(arr, os.path.join(dirs["raw_png_dir"], "test"))
for file in os.listdir(dirs["raw_arr_dir"]):
	in_path = os.path.join(dirs["raw_arr_dir"], file)
	out_file_nrrd = file.replace(".nrrd", "")
	out_path = os.path.join(dirs["tvflow_nrrd_out_dir"], out_file_nrrd)
	tvflow_cmd = conf.get_tvflow_arg_string(in_path, out_path)
	io_ops.run_tvflow(tvflow_cmd)


for file in os.listdir(dirs["tvflow_nrrd_out_dir"]):
	in_path = os.path.join(dirs["tvflow_nrrd_out_dir"], file)
	out_file = file.replace(".nrrd", ".png")
	out_path = os.path.join(dirs["tvflow_png_out_dir"], out_file)
	io_ops.load_2d_nrrd_and_save_to_png(in_path, out_path,)



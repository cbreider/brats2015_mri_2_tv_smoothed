
import os
import io_ops


project_dir = os.path.dirname(os.path.realpath(__file__))


out_dir = os.path.join(project_dir, "output")
tvflow_binary = os.path.join(out_dir, "tvflow/tvflow")
arr_dir = os.path.join(out_dir, "arr")
tvflow_out_dir = os.path.join(out_dir, "tvflow")
tvflow_nrrd_dir = os.path.join(tvflow_out_dir, "nrrd")
tvflow_png_dir = os.path.join(tvflow_out_dir, "png")

if not os.path.exists(out_dir):
    os.makedirs(out_dir)
if not os.path.exists(arr_dir):
    os.makedirs(arr_dir)
if not os.path.exists(tvflow_out_dir):
    os.makedirs(tvflow_out_dir)
if not os.path.exists(tvflow_nrrd_dir):
    os.makedirs(tvflow_nrrd_dir)
if not os.path.exists(tvflow_png_dir):
    os.makedirs(tvflow_png_dir)

filen = "test.mha"
files = os.listdir(input_folder)
files.sort()
for f in files:
    in_file = os.path.join(input_folder, f)
    if os.path.isfile(in_file):
        out_file = "{}/{}_{}".format(output_folder, "test", i)
        args = ("./tvflow/tvflow", in_file, out_file)

arr = io_ops.load_3d_volume_as_array(filen)
io_ops.save_3darray_to_nrrd(arr, arr_dir, "test")
io_ops.run_tvflow(tvflow_binary, arr_dir, tvflow_nrrd_dir)
io_ops.read_tvflow_arrs(tvflow_nrrd_dir)
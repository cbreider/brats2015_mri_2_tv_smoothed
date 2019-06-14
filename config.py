import os
import copy


eps = 0.005  # -e regularization eps (double); default: "0.001"
itr = 20  # maximum  iterations (unsigned int); default: "10"
gradeps = 0.005  # gradient eps (double); default: "0.001"
scl = "scale"  # scale output file (string); default: ""
sclMinItr = 0  # iteration to start measurement (unsigned int);default: "0"
tau = 1.0  # step size tau (double); default: "1.0"


tvflow_bin = "tvflow/tvflow"
in_ext = ".nrrd"
out_ext = ".nrrd"

high_grade_gliomas_folder = 'HGG'
low_grade_gliomas_folder = 'LGG'
test_gg_path = "{}_{}".format(high_grade_gliomas_folder, low_grade_gliomas_folder)
ground_truth_path_identifier = ".xx.o.ot."
project_dir = None
data_dir = "../dataset"
slice_dir = "2d_slices"
raw_train_dir = "train"
raw_test_dir = "test"
tv_flow_out_dir = "tvflow"
raw_dir = "raw"
brats_train_dir = "BRATS2015_Training"
brats_test_dir = "BRATS2015_Testing"
mha_to_nrrd_file_dict = None
ground_truth_file_list = None
initialized = False
base_file_list = None

def initialize():
    global project_dir
    global data_dir
    global slice_dir
    global raw_train_dir
    global raw_test_dir
    global tv_flow_out_dir
    global raw_dir
    global brats_train_dir
    global brats_test_dir
    global initialized
    global raw_dir
    global tvflow_bin

    project_dir = os.path.dirname(os.path.realpath(__file__))
    tvflow_bin = os.path.join(project_dir, tvflow_bin)
    data_dir = os.path.join(project_dir, data_dir)
    brats_train_dir = os.path.join(data_dir, brats_train_dir)
    brats_test_dir = os.path.join(data_dir, brats_test_dir)
    slice_dir = os.path.join(data_dir, slice_dir)
    raw_dir = os.path.join(slice_dir, raw_dir)
    tv_flow_out_dir = os.path.join(slice_dir, tv_flow_out_dir)
    raw_train_dir = os.path.join(raw_dir, raw_train_dir)
    raw_test_dir = os.path.join(raw_dir, raw_test_dir)

    if not os.path.exists(data_dir):
        return
    if not os.path.exists(brats_train_dir):
        return
    if not os.path.exists(brats_test_dir):
        return
    if not os.path.exists(slice_dir):
        os.makedirs(slice_dir)
    if not os.path.exists(tv_flow_out_dir):
        os.makedirs(tv_flow_out_dir)
    if not os.path.exists(raw_dir):
        os.makedirs(raw_dir)
    if not os.path.exists(tv_flow_out_dir):
        os.makedirs(tv_flow_out_dir)
    if not os.path.exists(raw_dir):
        os.makedirs(raw_dir)
    if not os.path.exists(raw_test_dir):
        os.makedirs(raw_test_dir)
    if not os.path.exists(raw_train_dir):
        os.makedirs(raw_train_dir)

    initialized = True


def get_tvflow_arg_string(in_file, out_file):
    global eps
    global itr
    global gradeps
    global scl
    global sclMinItr
    global tau
    global tvflow_bin
    global in_ext
    global out_ext

    args = "\"{}\" -e {} -i {} -g {} -s \"{}\" -smi {} -t {} \"{}\" \"{}\"".format(
        tvflow_bin, eps, itr, gradeps, "{}_{}".format(out_file, scl), sclMinItr, tau, in_file, out_file)
    # args = ["\"{}\"".format(tvflow_bin), "-e {}".format(eps), "-i {}".format(itr), "-g {}".format(gradeps),
    #        "-s \"{}_{}\"".format(out_file, scl), "-smi {}".format(sclMinItr), "-t {}".format(tau), "\"{}\"".format(in_file), "\"{}\"".format(out_file)]
    return args


def load_patient_folders(folder):
    patient_names = os.listdir('{}{}'.format(folder, high_grade_gliomas_folder))
    patient_names = [os.path.join(folder, high_grade_gliomas_folder, patient) for patient in patient_names]
    patient_names_tmp = os.listdir('{}{}'.format(folder, low_grade_gliomas_folder))
    patient_names.extend([os.path.join(folder, low_grade_gliomas_folder, patient) for patient in patient_names_tmp])

    return patient_names


def create_base_file_names():
    if not initialized:
        return None

    global base_file_list


def get_mha_file_path(path):
    for file in os.listdir(path):
        if file.endswith(".mha"):
            return os.path.join(path, file)


def get_mha_to_nrrd_file_paths_dict():
    if not initialized:
        return None

    global brats_train_dir
    global brats_test_dir
    global mha_to_nrrd_file_dict
    global high_grade_gliomas_folder
    global low_grade_gliomas_folder
    global raw_train_dir
    global raw_test_dir

    mha_to_nrrd_file_dict = dict()
    mha_to_nrrd_file_dict.update(get_paths_dict(brats_train_dir, raw_train_dir, gg=high_grade_gliomas_folder))
    mha_to_nrrd_file_dict.update(get_paths_dict(brats_train_dir, raw_train_dir, gg=low_grade_gliomas_folder))
    mha_to_nrrd_file_dict.update(get_paths_dict(brats_test_dir, raw_test_dir, gg=test_gg_path))

    return mha_to_nrrd_file_dict


def get_raw_to_tvflow_file_paths_dict():
    global tv_flow_out_dir
    global raw_dir
    global tv_flow_out_dir
    global high_grade_gliomas_folder
    global low_grade_gliomas_folder
    if not initialized:
        return None

    raw_to_tvflow_file_dict = dict()
    raw_to_tvflow_file_dict.update(get_paths_dict(
        base_path_key=raw_train_dir, base_path_vlaue=tv_flow_out_dir, ext_key=in_ext, ext_val="_tvflow", remove_ext=False, gg=high_grade_gliomas_folder, without_gt=True))
    raw_to_tvflow_file_dict.update(get_paths_dict(
        base_path_key=raw_train_dir, base_path_vlaue=tv_flow_out_dir, ext_key=in_ext, ext_val="_tvflow", remove_ext=False, gg=low_grade_gliomas_folder, without_gt=True))

    return raw_to_tvflow_file_dict


def get_paths_dict(base_path_key, base_path_vlaue, ext_key=".mha", ext_val=".nrrd", remove_ext=True, gg="HGG", without_gt=False):
    global ground_truth_path_identifier
    file_dict = dict()
    gg_path = os.path.join(base_path_key, gg)
    patient_names = os.listdir(gg_path)
    patient_paths = [os.path.join(gg_path, patient) for patient in patient_names]
    file_folders = [[os.path.join(path, tmp_path) for tmp_path in os.listdir(path)] for path in patient_paths]
    flat_list_flat = [item for sublist in file_folders for item in sublist]

    for path in flat_list_flat:
        if without_gt and ground_truth_path_identifier in path.lower():
            continue
        out_path = path.replace(base_path_key, base_path_vlaue)
        if not os.path.exists(out_path):
            os.makedirs(out_path)
        for file in os.listdir(path):
            if file.endswith(ext_key):
                file_path_key = os.path.join(path, file)
                file_path_val = file_path_key.replace(base_path_key, base_path_vlaue)
                if not remove_ext:
                    file_path_val = file_path_val.replace(ext_key, ext_val)
                else:
                    file_path_val = file_path_val.replace(ext_key, "")
                file_dict[file_path_key] = file_path_val

    return file_dict




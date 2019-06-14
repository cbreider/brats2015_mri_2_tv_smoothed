import config
import argparse
import io_ops
import sys
if sys.version_info > (3, 0):
    from _thread import start_new_thread, allocate_lock
else:
    from thread import start_new_thread, allocate_lock
import multiprocessing

lock = allocate_lock()
thread_finished = None
show_step_size = 50


def chunk_dict(dict_in, num_seq):
    avg = len(dict_in) / float(num_seq)
    out = []
    last = 0.0

    while last < len(dict_in):
        out.append(dict(dict_in.items()[int(last):int(last + avg)]))
        last += avg

    return out


def run_tv_flow_on_dict(tvflow_file_dict_part, thread_num):
    global show_step_size, threads_running
    length = len(tvflow_file_dict_part)
    tmp = 1
    for key, value in tvflow_file_dict_part.iteritems():
        command = config.get_tvflow_arg_string(key, value)
        io_ops.run_tvflow(command)
        if tmp % show_step_size == 0:
            print("Processed tvflow in tread {} on scan {} of {}".format(thread_num, tmp, length))
        tmp += 1
    threads_running[thread_num - 1] = False


def run_mha_to_nrrd_conversion(file_dict_part, thread_num):
    global show_step_size, threads_running
    length = len(file_dict_part)
    tmp = 1
    for key, value in file_dict_part.iteritems():
        arr = io_ops.load_mha_volume_as_array(key)
        io_ops.save_3darray_to_nrrd(arr, value)
        if tmp % show_step_size == 0:
            print("Saved scan {} of {} as nrrd in tread {}".format(tmp, length, thread_num))
        tmp += 1
    threads_running[thread_num - 1] = False


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--create_raw_files_first",
                        help="create nrrd 2d array files from mha files bevor run tvflow",
                        action='store_true')
    args = parser.parse_args()
    config.initialize()
    num_threads = int(multiprocessing.cpu_count())

    # convert mhs to nrrd
    if args.create_raw_files_first:
        mha_file_dict = config.get_mha_to_nrrd_file_paths_dict()
        complete_dict_len = len(mha_file_dict)
        chunked_dict = chunk_dict(mha_file_dict, num_threads)
        threads_running = [True for el in range(num_threads)]
        print("Starting conversion from mha to nrrd in {} threads on {} elements...".format(num_threads, complete_dict_len))
        for i in range(num_threads):
            print("Starting thread {} with {} elements".format(i+1, len(chunked_dict[i])))
            start_new_thread(run_mha_to_nrrd_conversion, (chunked_dict[i], i+1))

        while any(thread_running for thread_running in threads_running):
            pass
        print("Finished conversion from mha to nrrd!")

    tvflow_file_dict = config.get_raw_to_tvflow_file_paths_dict()
    complete_dict_len = len(tvflow_file_dict)
    chunked_dict_tvflow = chunk_dict(tvflow_file_dict, num_threads)
    threads_running = [True for el in range(num_threads)]
    print("Starting to run tvflow in {} threads on {} elements...".format(num_threads, complete_dict_len))
    for i in range(num_threads):
        print("Starting thread {} with {} elements".format(i+1, len(chunked_dict_tvflow[i])))
        start_new_thread(run_tv_flow_on_dict, (chunked_dict_tvflow[i], i+1))

    while any(thread_running for thread_running in threads_running):
        pass
    print("Finished tvflow!")


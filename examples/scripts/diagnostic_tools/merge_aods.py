#!/usr/bin/env python3


"""
Script to scan paths and merge AODs
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

from common import fatal_msg, get_default_parser, run_cmd, run_in_parallel, set_verbose_mode, verbose_msg, warning_msg, msg
import os
import multiprocessing

bunched_aod_names = multiprocessing.Manager().dict()


def run_merge(input_list_name):
    out_aod = bunched_aod_names[input_list_name]["out_aod"]
    file_index = bunched_aod_names[input_list_name]["file_index"]
    total_files = bunched_aod_names[input_list_name]["total_files"]
    input_size = bunched_aod_names[input_list_name]["input_size"]
    run_cmd(f"o2-aod-merger --input {input_list_name} --output {out_aod}",
            time_it=True, print_output=False)
    msg(f"Merged #{file_index}/{total_files-1} ({input_size} MB) to", out_aod,
        os.path.getsize(out_aod)*1E-6, "MB")


def main(input_files, do_merge=True,
         sanity_file=None, max_bunch_size=200,
         out_path="./", over_write_lists=False, jobs=1):
    msg("Merging to", out_path, "with maximum input size", max_bunch_size)
    out_path = os.path.normpath(out_path)
    if not os.path.exists(out_path):
        warning_msg("Output path", out_path, "does not exist")
        ans = input("Create it? (Y/[N])")
        if ans == "Y":
            os.makedirs(out_path)
        else:
            msg("Exit")
            return
    sane_files = None
    if sanity_file is not None:
        msg("Using sanity file", sanity_file)
        sane_files = []
        with open(sanity_file, "r") as f:
            for i in f:
                sane_files.append(os.path.abspath(os.path.normpath(i.strip())))
    size_of_files = {}
    for i in input_files:
        i = os.path.normpath(i.strip())
        if sane_files is not None and os.path.abspath(i) not in sane_files:
            msg("Skipping", i, "because not in sanity file")
            continue
        size_of_files[i] = os.path.getsize(i)*1e-6
    bunched_files = [[]]
    bunched_sizes = []
    bunch_size = []
    for i in size_of_files:
        verbose_msg("Checking file", i, "of size", size_of_files[i], "MB")
        if sum(bunch_size) > max_bunch_size:
            verbose_msg("Bunch size", sum(bunch_size), "reached limit with",
                        len(bunch_size), "files",
                        max_bunch_size, "MB",
                        "preparing next bunch!")
            bunched_files.append([])
            bunched_sizes.append(sum(bunch_size))
            bunch_size = []
        bunch_size.append(size_of_files[i])
        bunched_files[-1].append(i)
    bunched_sizes.append(sum(bunch_size))
    verbose_msg("Got", len(bunched_files), "bunches")
    for i, j in enumerate(bunched_files):
        verbose_msg(f"{i})", bunched_sizes[i], "MB, with", len(j), j)

    msg("Preparing", len(bunched_files), "bunched lists")
    bunched_aod_names.clear()
    for i, j in enumerate(bunched_files):
        fn = f"aod_merge_list_bunch{i}.txt"
        verbose_msg("Writing bunch", i, "to", fn)
        if not over_write_lists:
            if os.path.isfile(fn):
                fatal_msg(fn, "already present, remove it first")
        with open(fn, "w") as f:
            for k in j:
                f.write(k+"\n")
        if do_merge:
            out_aod = os.path.join(out_path, f"AO2D_Merge_{i}.root")
            if os.path.isfile(out_aod):
                fatal_msg(out_aod, "already present")
            bunched_aod_names[fn] = {"out_aod": out_aod, "file_index": i,
                                     "total_files": len(bunched_files), "input_size": bunched_sizes[i]}

    run_in_parallel(jobs, run_merge, list(bunched_aod_names.keys()), job_message="Running AOD merging",
                    linearize_single_core=True)


if __name__ == "__main__":
    parser = get_default_parser(__doc__)
    parser.add_argument("input_files",
                        type=str,
                        nargs="+",
                        help="Input files to merge")
    parser.add_argument("--max_bunch_size", "--max", "-m",
                        default=1000,
                        type=float,
                        help="Approximate maximum size of the bunch to merge in MB")
    parser.add_argument("--output_path", "-o",
                        default="./",
                        type=str,
                        help="Output path for merged AODs")
    parser.add_argument("--sanity_file", "-s",
                        default=None,
                        type=str,
                        help="Sanity file with the files to filter")
    parser.add_argument("--overwrite",
                        action="store_true", help="Flag to overwrite the lists of files that are to be merged")

    args = parser.parse_args()
    set_verbose_mode(args)

    main(args.input_files, max_bunch_size=args.max_bunch_size,
         out_path=args.output_path, sanity_file=args.sanity_file,
         over_write_lists=args.overwrite, jobs=args.njobs)

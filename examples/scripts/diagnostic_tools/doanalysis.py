#!/usr/bin/env python3

"""
Script to run o2 analyses on AODs.
This script is used to run the basic QA checks on the productions.
Several analyses are implemented already, you can pick yours and run e.g.:
`./doanalysis.py TrackQA -i ../AODRun5.0.root`
Results will be available for each batch of files in the `AnalysisResults` directory.
You can check the help of the script (i.e. `./doanalysis.py --h`) to have information on the available options and workflows.
"""

import argparse
import multiprocessing
from itertools import islice
import os
import sys
try:
    import tqdm
except ImportError as e:
    print("Module tqdm is not imported. Progress bar will not be available (you can install tqdm for the progress bar)")
from ROOT import TFile

# Global running flags
verbose_mode = False


class bcolors:
    # Colors for bash
    BOLD = "\033[1m"
    UNDERLINE = "\033[4m"
    HEADER = "\033[95m"
    OKBLUE = "\033[94m"
    BOKBLUE = BOLD + OKBLUE
    OKGREEN = "\033[92m"
    BOKGREEN = BOLD + OKGREEN
    WARNING = "\033[93m"
    BWARNING = BOLD + WARNING
    FAIL = "\033[91m"
    BFAIL = BOLD + FAIL
    ENDC = "\033[0m"


def verbose_msg(*args, color=bcolors.OKBLUE):
    if verbose_mode:
        print("** ", color, *args, bcolors.ENDC)


def msg(*args, color=bcolors.BOKBLUE):
    print(color, *args, bcolors.ENDC)


def fatal_msg(*args):
    msg("[FATAL]", *args, color=bcolors.BFAIL)
    raise RuntimeError("Fatal Error!")


def set_o2_analysis(o2_analyses=["o2-analysis-hf-task-d0 --pipeline qa-tracking-kine:4,qa-tracking-resolution:4"],
                    o2_arguments="--shm-segment-size 16000000000 --readers 4 --configuration json://$PWD/dpl-config_std.json",
                    input_file="listfiles.txt",
                    tag="QA",
                    output_files=["AnalysisResults.root",
                                  "AnalysisResults_trees.root",
                                  "QAResults.root"],
                    dpl_configuration_file=None):
    """
    Function to prepare everything you need for your O2 analysis.
    From the output folder to the script containing the O2 workflow.
    The output can be found in the same directory as the input data.
    """
    # Defining log file
    log_file = f"log_{tag.lower()}.log"
    verbose_msg("Configuring the tasks with O2", color=bcolors.BOKBLUE)
    # Creating output directory
    output_path = os.path.dirname(os.path.abspath(input_file))
    # Checking input file
    verbose_msg("Using", input_file, "as input file")
    if not input_file.endswith(".root"):
        input_file = f"@{os.path.join(os.getcwd(), input_file)}"

    # Creating the script to run O2
    tmp_script_name = os.path.join(output_path, f"tmpscript{tag}.sh")
    with open(tmp_script_name, "w") as tmp_script:

        verbose_msg("Writing o2 instructions to", f"'{tmp_script_name}'")

        def write_instructions(instructions):
            verbose_msg("--\t", instructions.strip())
            tmp_script.write(f"{instructions}")
        write_instructions(f"#!/bin/bash\n\n")
        write_instructions(f"cd {output_path} \n\n")  # Move to run dir
        write_instructions(f"pwd \n\n")  # Move to run dir

        for i in output_files:  # Removing old output
            write_instructions(f"rm -v {i} 2>&1\n")
        write_instructions("\n\n")

        o2_workflow = ""
        for i in o2_analyses:
            line = f"{i} {o2_arguments}"
            if i == o2_analyses[0]:
                line += f" --aod-file {input_file}"
            if dpl_configuration_file is not None:
                line += f" --configuration json://{dpl_configuration_file}"
            if len(o2_analyses) > 1 and i != o2_analyses[-1]:
                line = f"{line} | \\\n \t"
            else:
                line = f"{line}"
            o2_workflow += line
        log_line = "echo \"Running: \n \t"+o2_workflow.replace("\t", "")+"\""
        log_line += f" > {log_file}"
        write_instructions(log_line+" \n\n")
        write_instructions(o2_workflow + f" >> {log_file} \n \t")
        write_instructions("\n\n")

        for i in output_files:  # renaming output with tag
            r = i.replace(".root", f"_{tag}.root")
            write_instructions(f"mv {i} {r} 2>&1\n")

        write_instructions("\n\n")
    return tmp_script_name


def run_command(cmd):
    verbose_msg(f"Running '{cmd}'")
    try:
        content = os.popen(cmd).read()
        if content:
            for i in content.strip().split("\n"):
                verbose_msg("++", i)
    except:
        raise ValueError("Error!")


def run_o2_analysis(tmp_script_name, remove_tmp_script=False):
    verbose_msg("> starting run with", tmp_script_name)
    cmd = f"bash {tmp_script_name}"
    run_command(cmd)
    if remove_tmp_script:
        os.remove(tmp_script_name)
    verbose_msg("< end run with", tmp_script_name)


analyses = {"TrackQA": ["o2-analysis-qa-track-event",
                        "o2-analysis-qa-efficiency --make-eff 1 --eff-pi 1 --eff-el 1 --eff-ka 1 --eff-pr 1 --eta-min -0.8 --eta-max 0.8",
                        "o2-analysis-trackextension",
                        "o2-analysis-alice3-trackselection"],
            "TOF": ["o2-analysis-spectra-tof",
                    "o2-analysis-trackextension",
                    "o2-analysis-alice3-pid-tof --add-qa 1",
                    "o2-analysis-pid-tof-beta --add-qa 1",
                    "o2-analysis-alice3-trackselection",
                    "o2-analysis-alice3-trackextension"],
            "RICH": ["o2-analysis-alice3-pid-rich-qa"],
            "Efficiency": ["o2-analysis-mc-spectra-efficiency",
                           "o2-analysis-trackextension",
                           "o2-analysis-alice3-trackselection"],
            "TPC": ["o2-analysis-pid-tpc --add-qa 1"],
            "TreeD0": ["o2-analysis-hf-tree-creator-d0-tokpi --aod-writer-keep AOD/HFCANDP2Full/0,AOD/HFCANDP2FullE/0,AOD/HFCANDP2FullP/0",
                       "o2-analysis-pid-tpc",
                       "o2-analysis-pid-tof",
                       "o2-analysis-hf-candidate-creator-2prong --doMC",
                       "o2-analysis-hf-track-index-skims-creator",
                       "o2-analysis-hf-d0-candidate-selector"],
            "TreeLC": ["o2-analysis-hf-tree-creator-lc-topkpi --aod-writer-keep AOD/HFCANDP3Full/0,AOD/HFCANDP3FullE/0,AOD/HFCANDP3FullP/0",
                       "o2-analysis-pid-tpc",
                       "o2-analysis-pid-tof",
                       "o2-analysis-hf-candidate-creator-2prong --doMC",
                       "o2-analysis-hf-track-index-skims-creator",
                       "o2-analysis-hf-d0-candidate-selector"]
            }


def main(mode,
         input_file,
         out_path,
         out_tag="",
         batch_size=4,
         n_max_files=100,
         dpl_configuration_file=None,
         njobs=1,
         merge_output=False,
         merge_only=False,
         shm_mem_size=16000000000,
         readers=1,
         extra_arguments=""):
    if len(input_file) == 1:
        input_file = input_file[0]
    else:
        input_file = input_file[0:n_max_files]
    if not merge_only:
        msg("Running", f"'{mode}'", "analysis on",
            f"'{input_file}'", color=bcolors.BOKBLUE)
        msg("Maximum", n_max_files, "files with batch size",
            batch_size, "and", njobs, "jobs" if njobs > 1 else "job", color=bcolors.BOKBLUE)
    else:
        msg("Merging output of", f"'{mode}'",
            "analysis", color=bcolors.BOKBLUE)
    o2_arguments = f"-b --shm-segment-size {shm_mem_size} --readers {readers}"
    o2_arguments += extra_arguments
    if mode not in analyses:
        raise ValueError("Did not find analyses matching mode",
                         mode, ", please choose in", ", ".join(analyses.keys()))
    an = analyses[mode]
    tag = mode + out_tag
    # Build input file list
    input_file_list = []

    def build_list_of_files(file_list):
        if len(file_list) != len(set(file_list)):  # Check that runlist does not have duplicates
            fatal_msg("Runlist has duplicated entries, fix runlist!")
        not_readable = []
        for i in file_list:  # Check that input files can be open
            f = TFile(i.strip(), "READ")
            if not f.IsOpen():
                verbose_msg("Cannot open AOD file:", i, color=bcolors.WARNING)
                not_readable.append(i)
        if len(not_readable) > 0:
            msg(len(not_readable), "files cannot be read and will be skipped",
                color=bcolors.BWARNING)
            for i in not_readable:
                file_list.remove(i)

        files_per_batch = []
        iter_file_list = iter(file_list)
        for i in range(0, len(file_list)):
            sub_set = list(islice(iter_file_list, batch_size))
            if len(sub_set) <= 0:
                continue
            files_per_batch.append(sub_set)
        run_list = []
        if len(files_per_batch) > 0:
            for i, lines in enumerate(files_per_batch):
                p = os.path.join(out_path, f"{i}")
                if not os.path.isdir(p):
                    os.makedirs(p)
                run_list.append(os.path.join(
                    p, f"ListForRun5Analysis.{i}.txt"))
                with open(run_list[-1], "w") as f:
                    for j in lines:
                        f.write(j.strip() + "\n")
        msg("Number or runs:", len(run_list))
        return run_list

    if type(input_file) is list:
        if batch_size == 1:
            input_file_list = os.path.join(os.getcwd(), input_file)
        else:
            input_file_list = build_list_of_files(input_file)
    elif not input_file.endswith(".root"):
        with open(input_file, "r") as f:
            lines = f.readlines()
            msg("Building input list from", len(lines),
                "inputs, limiting to", n_max_files)
            if len(lines) > n_max_files:
                lines = lines[0:n_max_files]
            input_file_list = build_list_of_files(lines)
    else:
        input_file_list = [os.path.join(os.getcwd(), input_file)]

    if dpl_configuration_file is not None:
        dpl_configuration_file = os.path.join(os.getcwd(),
                                              dpl_configuration_file)

    run_list = []
    for i, j in enumerate(input_file_list):
        run_list.append(set_o2_analysis(an,
                                        o2_arguments=o2_arguments,
                                        input_file=j,
                                        tag=tag,
                                        dpl_configuration_file=dpl_configuration_file))
    if not merge_only:
        with multiprocessing.Pool(processes=njobs) as pool:
            msg("Running analysis")
            if "tqdm" not in sys.modules:
                for i in enumerate(pool.imap(run_o2_analysis, run_list)):
                    msg(f"Done: {i[0]+1},", len(run_list)-i[0]-1, "to go")
            else:
                r = list(tqdm.tqdm(pool.imap(run_o2_analysis, run_list),
                                   total=len(run_list),
                                   bar_format='{l_bar}{bar:10}{r_bar}{bar:-10b}'))

    if merge_output or merge_only:
        files_to_merge = []
        for i in input_file_list:
            p = os.path.dirname(os.path.abspath(i))
            for j in os.listdir(p):
                if j.endswith(f"_{tag}.root"):
                    files_to_merge.append(os.path.join(p, j))
        if len(files_to_merge) == 0:
            msg("Warning: Did not find any file to merge for tag",
                tag, color=bcolors.BWARNING)
            return
        msg("Merging", len(files_to_merge), "results", color=bcolors.BOKBLUE)
        files_per_type = {}
        for i in files_to_merge:
            fn = os.path.basename(i)
            files_per_type.setdefault(fn, [])
            files_per_type[fn].append(i)
        merged_files = []
        for i in files_per_type:
            merged_file = os.path.join(out_path, i)
            if os.path.isfile(merged_file):
                msg("Warning: file", merged_file,
                    "is already found, remove it before merging, you can use the --mergeonly flag to avoid running the analysis again",
                    color=bcolors.BWARNING)
                continue
            merged_files.append(merged_file)
            run_command(f"hadd {merged_file} " + " ".join(files_per_type[i]))
        msg("Merging completed, merged:", *merged_files, color=bcolors.BOKGREEN)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Runner for O2 analyses")
    parser.add_argument("modes",
                        type=str,
                        nargs="+",
                        help="Running modes, can be chosen among: " + ", ".join(analyses.keys()))
    parser.add_argument("--input", "-i",
                        type=str,
                        nargs="+",
                        default=["listfiles.txt"],
                        help="Input file, can be in form of a list of AODs or a list of text files with the list of AODs")
    parser.add_argument("--out_path", "-o",
                        type=str,
                        default="AnalysisResults",
                        help="Output path")
    parser.add_argument("--tag", "-t",
                        type=str,
                        default="",
                        help="Tag for output files")
    parser.add_argument("--verbose", "-v",
                        action="store_true", help="Verbose mode")
    parser.add_argument("--batch-size", "-B",
                        type=int,
                        default=10,
                        help="Size of the batch of files to analyze for multiple threads")
    parser.add_argument("--max-files", "-M",
                        type=int,
                        default=10,
                        help="Maximum files to process")
    parser.add_argument("--njobs", "-j",
                        type=int,
                        default=1,
                        help="Number of jobs to use")
    parser.add_argument("--configuration", "--dpl", "-D",
                        type=str,
                        default=None,
                        help="Name of the dpl configuration file e.g. dpl-config_std.json")
    parser.add_argument("--merge_output", "--merge-output", "--merge",
                        action="store_true", help="Flag to merge the output files into one")
    parser.add_argument("--readers", "-r",
                        default=1, type=int,
                        help="Number of parallel readers")
    parser.add_argument("--mem", "-m",
                        default=16000000000, type=int,
                        help="Size of the shared memory to allocate")
    parser.add_argument("--extra_arguments", "-e",
                        default="", type=str,
                        help="Extra arguments to feed to the workflow")
    parser.add_argument("--merge_only", "--merge-only", "--mergeonly",
                        action="store_true", help="Flag avoid running the analysis and to merge the output files into one")
    args = parser.parse_args()
    if args.verbose:
        verbose_mode = False,

    for i in args.modes:
        main(mode=i,
             input_file=args.input,
             dpl_configuration_file=args.configuration,
             batch_size=args.batch_size,
             n_max_files=args.max_files,
             njobs=args.njobs,
             out_tag=args.tag,
             merge_output=args.merge_output,
             out_path=args.out_path,
             merge_only=args.merge_only,
             readers=args.readers,
             extra_arguments=args.extra_arguments,
             shm_mem_size=args.mem)

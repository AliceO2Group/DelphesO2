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


def set_o2_analysis(o2_analyses=["o2-analysis-hf-task-d0 --pipeline qa-tracking-kine:4,qa-tracking-resolution:4"],
                    o2_arguments="--shm-segment-size 16000000000 --readers 4 --configuration json://$PWD/dpl-config_std.json",
                    input_file="listfiles.txt",
                    tag="QA",
                    output_files=["AnalysisResults.root",
                                  "AnalysisResults_trees.root",
                                  "QAResults.root"],
                    dpl_configuration_file=None,
                    output_path=None
                    ):
    """
    Function to prepare everything you need for your O2 analysis.
    From the output folder to the script containing the O2 workflow.
    """
    # Defining log file
    log_file = f"log_{tag.lower()}.log"
    verbose_msg("Configuring the tasks with O2", color=bcolors.BOKBLUE)
    # Creating output directory
    if output_path is not None:
        output_path = os.path.join(os.getcwd(), output_path)
        verbose_msg("Creating new directory", output_path, "for results")
        if not os.path.isdir(output_path):
            os.makedirs(output_path)
    else:
        output_path = os.getcwd()
    # Checking input file
    verbose_msg("Using", input_file, "as input file")
    if not input_file.endswith(".root"):
        input_file = f"@{os.path.join(os.getcwd(), input_file)}"

    # Creating the script to run O2
    tmp_script_name = f"{output_path}/tmpscript{tag}.sh"
    with open(tmp_script_name, "w") as tmp_script:

        verbose_msg("Writing o2 instructions to", f"'{tmp_script_name}'")

        def write_instructions(instructions):
            verbose_msg("--\t", instructions.strip())
            tmp_script.write(f"{instructions}")
        write_instructions(f"#!/bin/bash\n\n")
        write_instructions(f"cd {output_path} \n\n")  # Move to run dir

        for i in output_files:  # Removing old output
            write_instructions(f"rm {i} 2> /dev/null \n")
        write_instructions("\n\n")

        for i in o2_analyses:
            line = f"{i} {o2_arguments}"
            if i == o2_analyses[0]:
                line += f" --aod-file {input_file}"
            if dpl_configuration_file is not None:
                line += f" --configuration json://{dpl_configuration_file}"
            if len(o2_analyses) > 1 and i != o2_analyses[-1]:
                line = f"{line} | \\\n \t"
            else:
                line = f"{line} > {log_file} \n \t"
            write_instructions(line)
        write_instructions("\n\n")

        for i in output_files:  # renaming output with tag
            r = i.replace(".root", f"_{tag}.root")
            write_instructions(f"mv {i} {r} 2> /dev/null\n")

        write_instructions("\n\n")
    return tmp_script_name


def run_o2_analysis(tmp_script_name, remove_tmp_script=False):
    msg("> starting run with", tmp_script_name)
    cmd = f"bash {tmp_script_name}"
    verbose_msg(f"Running '{cmd}'")
    try:
        content = os.popen(cmd).read()
        if content:
            verbose_msg("++", content)
    except:
        raise ValueError("Error!")
    if remove_tmp_script:
        os.remove(tmp_script_name)
    msg("> end run with", tmp_script_name)


analyses = {"TrackQA": ["o2-analysis-qa-simple",
                        "o2-analysis-qa-efficiency --make-eff 1 --eta-min -0.8 --eta-max 0.8",
                        "o2-analysis-trackextension",
                        "o2-analysis-alice3-trackselection"],
            "SpectraTOF": ["o2-analysis-spectra-tof",
                           "o2-analysis-trackextension",
                           "o2-analysis-pid-tof --add-qa 1",
                           "o2-analysis-alice3-trackselection"],
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
            }


def main(mode,
         input_file="listfiles.txt",
         out_tag="",
         batch_size=4,
         n_max_files=100,
         dpl_configuration_file=None,
         njobs=1):
    if len(input_file) == 1:
        input_file = input_file[0]
    else:
        input_file = input_file[0:n_max_files]
    msg("Running", f"'{mode}'", "analysis on",
        f"'{input_file}'", color=bcolors.BOKBLUE)
    msg("Maximum", n_max_files, "files with batch size",
        batch_size, "and", njobs, "jobs", color=bcolors.BOKBLUE)
    args = f"-b --shm-segment-size 16000000000 --readers 4"
    if mode not in analyses:
        raise ValueError("Did not find analyses matching mode",
                         mode, ", please choose in", ", ".join(analyses.keys()))
    an = analyses[mode]
    tag = mode + out_tag
    # Build input file list
    input_file_list = []

    def build_list_of_files(file_list):
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
                if not os.path.isdir(f"AnalysisResults/{i}"):
                    os.makedirs(f"AnalysisResults/{i}")
                run_list.append(
                    f"AnalysisResults/{i}/ListForRun5Analysis.{i}.txt")
                with open(run_list[-1], "w") as f:
                    for j in lines:
                        f.write(j.strip() + "\n")
        return run_list

    if type(input_file) is list:
        if batch_size == 1:
            input_file_list = os.path.join(os.getcwd(), input_file)
        else:
            input_file_list = build_list_of_files(input_file)

    elif not input_file.endswith(".root"):
        with open(input_file, "r") as f:
            lines = f.readlines()
            input_file_list = build_list_of_files(lines)
    else:
        input_file_list = [os.path.join(os.getcwd(), input_file)]

    if dpl_configuration_file is not None:
        dpl_configuration_file = os.path.join(
            os.getcwd(), dpl_configuration_file)

    run_list = []
    for i, j in enumerate(input_file_list):
        run_list.append(set_o2_analysis(an,
                                        o2_arguments=args,
                                        input_file=j,
                                        tag=tag,
                                        dpl_configuration_file=dpl_configuration_file,
                                        output_path=f"AnalysisResults/{i}"))

    with multiprocessing.Pool(processes=njobs) as pool:
        pool.map(run_o2_analysis, run_list)
    msg("Analysis completed", color=bcolors.BOKGREEN)


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
                        help="Input file")
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

    parser.add_argument("-b",
                        action="store_true", help="Background mode")
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
             out_tag=args.tag)

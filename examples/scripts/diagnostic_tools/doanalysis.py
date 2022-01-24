#!/usr/bin/env python3

"""
Script to run o2 analyses on AODs.
This script is used to run the basic QA checks on the productions.
Several analyses are implemented already, you can pick yours and run e.g.:
`./doanalysis.py TrackQA -i ../AODRun5.0.root`
Results will be available for each batch of files in the `AnalysisResults` directory.
You can check the help of the script (i.e. `./doanalysis.py --h`) to have information on the available options and workflows.
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

import configparser
from itertools import islice
import os
from common import bcolors, msg, fatal_msg, verbose_msg, run_in_parallel, set_verbose_mode, get_default_parser, warning_msg, run_cmd, print_all_warnings
from ROOT import TFile


def set_o2_analysis(o2_analyses=["o2-analysis-hf-task-d0 --pipeline qa-tracking-kine:4,qa-tracking-resolution:4"],
                    o2_arguments="--shm-segment-size 16000000000 --readers 4 --configuration json://$PWD/dpl-config_std.json",
                    input_file="listfiles.txt",
                    tag="QA",
                    output_files=["AnalysisResults.root",
                                  "AnalysisResults_trees.root",
                                  "QAResults.root"],
                    dpl_configuration_file=None,
                    resume_previous_analysis=False,
                    write_runner_script=True,
                    analysis_timeout=None):
    """
    Function to prepare everything you need for your O2 analysis.
    From the output folder to the script containing the O2 workflow.
    The output can be found in the same directory as the input data.
    """
    # Creating output directory
    output_path = os.path.dirname(os.path.abspath(input_file))
    # Creating the script to run O2
    tmp_script_name = os.path.join(output_path, f"tmpscript_{tag.lower()}.sh")
    if not write_runner_script:  # Returning in case write_runner_script is False
        return tmp_script_name
    # Defining log file
    log_file = f"log_{tag.lower()}.log"
    verbose_msg("Configuring the tasks with O2", color=bcolors.BOKBLUE)
    # Checking input file
    verbose_msg("Using", input_file, "as input file")
    if not input_file.endswith(".root"):
        input_file = f"@{os.path.join(os.getcwd(), input_file)}"

    # Writing instructions to runner script
    with open(tmp_script_name, "w") as tmp_script:

        verbose_msg("Writing o2 instructions to", f"'{tmp_script_name}'")

        def write_instructions(instructions, n=1, check_status=False):
            verbose_msg("--\t", instructions.strip())
            tmp_script.write(f"{instructions}" + "".join(["\n"]*n))
            if check_status:
                tmp_script.write("\nReturnValue=$?\n")
                tmp_script.write("if [[ $ReturnValue != 0 ]]; then\n")
                tmp_script.write("  echo \"Encountered error with command: '")
                tmp_script.write(instructions.replace("\"", "\\\"").strip())
                tmp_script.write("'\"\n")
                tmp_script.write("  exit $ReturnValue\n")
                tmp_script.write("fi\n\n")

        write_instructions(f"#!/bin/bash", n=2)
        # Move to run dir
        write_instructions(f"cd {output_path} || exit 1", n=2)
        # Print run dir
        write_instructions(f"pwd", n=2)

        def get_tagged_output_file(output_file_name):
            return output_file_name.replace(".root", f"_{tag}.root")

        for i in output_files:  # Removing old output
            write_instructions(f"[ -f {i} ] && rm -v {i} 2>&1")
            i = get_tagged_output_file(i)
            if resume_previous_analysis:
                write_instructions(
                    f"[ -f {i} ] && echo 'file {i} already present, continuing' && exit 0")
            else:
                write_instructions(f"[ -f {i} ] && rm -v {i} 2>&1")
        write_instructions("")

        o2_workflow = ""
        if analysis_timeout is not None:
            o2_workflow = f"timeout {analysis_timeout} "
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

        write_instructions(f"O2Workflow=\"{o2_workflow}\"", n=2)
        write_instructions("if [[ -z \"${1}\" ]]; then", n=2)
        write_instructions("  echo \"Running: \n \t ${O2Workflow}\""
                           f" > {log_file}")
        write_instructions("  eval \"${O2Workflow}\""
                           f" >> {log_file}", check_status=True)
        write_instructions("else")
        write_instructions("  eval \"${O2Workflow}\"")
        write_instructions("fi")

        for i in ["ERROR", "FATAL", "crash"]:
            write_instructions(
                f"if grep -q \"\[{i}\]\" {log_file}; then echo \": got some {i}s in '{log_file}'\" && exit 1; fi")
        write_instructions("")

        for i in output_files:  # renaming output with tag
            write_instructions(
                f"[ -f {i} ] && mv {i} {get_tagged_output_file(i)} 2>&1")

        write_instructions("\nexit 0")
    return tmp_script_name


def run_o2_analysis(tmp_script_name, remove_tmp_script=False, explore_bad_files=True):
    verbose_msg("> starting run with", tmp_script_name)
    cmd = f"bash {tmp_script_name}"
    if explore_bad_files:
        if run_cmd(cmd, check_status=True, throw_fatal=False) == False:
            list_name = os.listdir(os.path.dirname(tmp_script_name))
            for i in list_name:
                if "ListForRun5Analysis" in i:
                    list_name = i
                    break
            if type(list_name) != list:
                with open(os.path.join(os.path.dirname(tmp_script_name), list_name)) as f:
                    list_name = []
                    for i in f:
                        list_name.append(i)
            warning_msg("Issue when running",
                        tmp_script_name, "with", list_name)
    else:
        run_cmd(cmd)
    if remove_tmp_script:
        os.remove(tmp_script_name)
    verbose_msg("< end run with", tmp_script_name)
    return tmp_script_name


analyses = {}  # List of all known analyses, taken from configuration file


def main(mode,
         input_file,
         out_path,
         out_tag="",
         batch_size=4,
         n_max_files=100,
         dpl_configuration_file=None,
         njobs=1,
         merge_output=True,
         merge_only=False,
         shm_mem_size=16000000000,
         rate_lim=1000000000,
         readers=1,
         avoid_overwriting_merge=False,
         clean_localhost_after_running=True,
         extra_arguments="",
         resume_previous_analysis=False,
         check_input_file_integrity=True,
         analysis_timeout=None):
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
    if analysis_timeout is not None:
        msg("Using analysis timeout of", analysis_timeout,
            "seconds", color=bcolors.BOKBLUE)

    o2_arguments = f"-b --shm-segment-size {shm_mem_size} --aod-memory-rate-limit {rate_lim} --readers {readers}"
    o2_arguments += extra_arguments
    if mode not in analyses:
        raise ValueError("Did not find analyses matching mode",
                         mode, ", please choose in", ", ".join(analyses.keys()))
    an = analyses[mode]
    tag = mode + out_tag
    # Build input file list
    input_file_list = []

    def is_root_file_sane(file_name_to_check):
        file_name_to_check = file_name_to_check.strip()
        if not os.path.isfile(file_name_to_check):
            warning_msg("File", file_name_to_check, "does not exist")
            return 3
        file_to_check = TFile(file_name_to_check, "READ")
        if not file_to_check.IsOpen():
            warning_msg("Cannot open AOD file:", file_name_to_check)
            return 1
        elif file_to_check.TestBit(TFile.kRecovered):
            verbose_msg(file_name_to_check, "was a recovered file")
            return 2
        else:
            verbose_msg(file_name_to_check, "is OK")
            return 0

    def build_list_of_files(file_list):
        verbose_msg("Building list of files from", file_list)
        # Check that runlist does not have duplicates
        unique_file_list = set(file_list)
        if len(file_list) != len(unique_file_list):
            # for i in file_list
            fatal_msg("Runlist has duplicated entries, fix runlist!",
                      len(unique_file_list), "unique files, while got", len(file_list), "files")
        not_readable = []
        recovered_files = []
        if check_input_file_integrity:  # Check that input files can be open
            for i in file_list:
                verbose_msg("Checking that TFile",
                            i.strip(), "can be processed")
                file_sane_code = is_root_file_sane(i)
                if file_sane_code == 1:
                    not_readable.append(i)
                elif file_sane_code == 2:
                    recovered_files.append(i)
        if len(recovered_files) > 0:
            msg("Recovered", len(recovered_files),
                "files:\n", )
            not_readable = not_readable + recovered_files
        if len(not_readable) > 0:
            warning_msg(len(not_readable), "over", len(file_list),
                        "files cannot be read and will be skipped")
            for i in not_readable:
                if i not in file_list:
                    warning_msg("did not find file to remove", f"'{i}'")
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
        msg("Number of runs:", len(run_list))
        return run_list

    if type(input_file) is list:
        input_file = [os.path.join(os.getcwd(), i) for i in input_file]
        input_file_list = build_list_of_files(input_file)
    elif not input_file.endswith(".root"):
        with open(input_file, "r") as f:
            lines = f.readlines()
            msg("Building input list from", len(lines),
                "inputs, limiting to", n_max_files)
            if len(lines) > n_max_files:
                lines = lines[0:n_max_files]
            lines = [os.path.join(os.path.dirname(os.path.abspath(input_file)), i)
                     for i in lines]
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
                                        dpl_configuration_file=dpl_configuration_file,
                                        resume_previous_analysis=resume_previous_analysis,
                                        write_runner_script=not merge_only,
                                        analysis_timeout=analysis_timeout))
    if not merge_only:
        run_in_parallel(processes=njobs, job_runner=run_o2_analysis,
                        job_arguments=run_list, job_message="Running analysis")
        if clean_localhost_after_running:
            run_cmd(
                "find /tmp/ -maxdepth 1 -name localhost* -user $(whoami) | xargs rm -v 2>&1",
                check_status=False)

    if (merge_output or merge_only) and len(run_list) > 1:
        files_to_merge = []
        for i in input_file_list:
            p = os.path.dirname(os.path.abspath(i))
            for j in os.listdir(p):
                if j.endswith(f"_{tag}.root"):
                    files_to_merge.append(os.path.join(p, j))
        if len(files_to_merge) == 0:
            warning_msg("Did not find any file to merge for tag", tag)
            return
        files_per_type = {}  # List of files to be merged per type
        # List of files to be merged per type that are not declared sane
        non_sane_files_per_type = {}
        for i in files_to_merge:
            if is_root_file_sane(i) != 0:
                non_sane_files_per_type[fn].setdefault(fn, []).append(i)
                warning_msg("Result file", i, "is not sane")
                continue
            fn = os.path.basename(i)
            files_per_type.setdefault(fn, [])
            files_per_type[fn].append(i)
        for i in non_sane_files_per_type:
            warning_msg("Non sane files for type", i)
            for j in non_sane_files_per_type[i]:
                msg(j)
        merged_files = []
        for i in files_per_type:
            merged_file = os.path.join(out_path, i)
            if avoid_overwriting_merge and os.path.isfile(merged_file):
                warning_msg("file", merged_file,
                            "is already found, remove it before merging, you can use the --mergeonly flag to avoid running the analysis again")
                continue
            merged_files.append(merged_file)
            merge_file_list = os.path.join(os.path.dirname(os.path.abspath(merged_file)),
                                           "tomerge_" + "".join(i.split(".")[:-1])+".txt")
            verbose_msg("List of files to be merged:", merge_file_list)
            with open(merge_file_list, "w") as fmerge:
                for j in files_per_type[i]:
                    fmerge.write(j+"\n")
            if len(files_per_type[i]) > len(run_list):
                fatal_msg("Trying to merge too many files of type", i, "for tag", tag, ":",
                          len(files_per_type[i]), "vs", len(run_list), "runs")
            msg("Merging", len(files_per_type[i]), "files to", merged_file)
            run_cmd(f"hadd -j {njobs} -f {merged_file} `cat {merge_file_list}`",
                    log_file=merge_file_list.replace(".txt", ".log"), time_it=True, comment=f"Merging to {merged_file}")
        if len(merged_files) == 0:
            warning_msg("Merged no files")
        else:
            msg("Merging completed, merged:", *merged_files,
                color=bcolors.BOKGREEN)


if __name__ == "__main__":
    parser = get_default_parser(description="Runner for O2 analyses")
    parser.add_argument("modes",
                        type=str,
                        nargs="+",
                        help="Running modes, as defined in the input configuration file")
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
    parser.add_argument("--timeout", "-T",
                        type=int,
                        default=None,
                        help="Timeout to give to the analyses. If negative no timeout is used")
    parser.add_argument("--batch-size", "-B",
                        type=int,
                        default=10,
                        help="Size of the batch of files to analyze for multiple threads")
    parser.add_argument("--max-files", "-M",
                        type=int,
                        default=10,
                        help="Maximum files to process")
    parser.add_argument("--configuration", "--dpl", "-D",
                        type=str,
                        default=None,
                        help="Name of the dpl configuration file e.g. dpl-config_std.json")
    parser.add_argument("--workflows", "-w",
                        type=str,
                        nargs="+",
                        default=[os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                              "o2_analysis_workflows.ini")],
                        help="Configuration file with all the known workflows")
    parser.add_argument("--readers", "-r",
                        default=1, type=int,
                        help="Number of parallel readers")
    parser.add_argument("--mem", "-m",
                        default=16000000000, type=int,
                        help="Size of the shared memory to allocate")
    parser.add_argument("--extra_arguments", "-e",
                        default="", type=str,
                        help="Extra arguments to feed to the workflow")
    parser.add_argument("--no_merge", "--no_merge_output", "--no_merge-output", "--nomerge",
                        action="store_true", help="Flag to merge the output files into one")
    parser.add_argument("--avoid_overwriting_merge", "--no_overwrite", "-a",
                        action="store_true", help="Flag to check that the old merged files are not overwritten")
    parser.add_argument("--merge_only", "--merge-only", "--mergeonly",
                        action="store_true", help="Flag avoid running the analysis and to merge the output files into one")
    parser.add_argument("--show", "-s",
                        action="store_true", help="Flag to show the workflow of the current tag")
    parser.add_argument("--no_clean", "-nc",
                        action="store_true", help="Flag to avoid cleaning the localhost files after running")
    parser.add_argument("--resume_previous_analysis", "--continue_analysis", "--resume_analysis", "--continue",
                        action="store_true",
                        help="Flag to continue the analysis from the input files that have been already built and not overwriting the output results")
    parser.add_argument("--dont_check_input_integrity", "--no_check_input_integrity", "--NC", "--nocheck",
                        action="store_true",
                        help="Flag to avoid checking the input file integrity so as to gain time")
    args = parser.parse_args()
    set_verbose_mode(args)

    # Load analysis workflows
    workflows = configparser.RawConfigParser()
    msg("Analysis configuration from", args.workflows)
    for i in args.workflows:
        if not os.path.isfile(i):
            fatal_msg(f"Did not fid configuration file '{i}'")
        workflows.read(i)
    for i in workflows.sections():
        full_workflow = workflows.get(i, "w").split("\n")
        analyses[i] = full_workflow
        if "|" in full_workflow:
            fatal_msg("`|` present in workflow", i)
        if len(analyses[i]) == 0:
            fatal_msg("Empty workflow for analysis", i)

    for i in args.modes:
        if i not in analyses.keys():
            fatal_msg("Analysis", i, "not in",
                      " ".join(workflows.sections()), "from configuration files:", args.workflows)
        if args.show:
            msg(i, "workflow:")
            for j in enumerate(analyses[i]):
                msg(" - ", *j)
        main(mode=i,
             input_file=args.input,
             dpl_configuration_file=args.configuration,
             batch_size=args.batch_size,
             n_max_files=args.max_files,
             njobs=args.njobs,
             out_tag=args.tag,
             merge_output=not args.no_merge,
             out_path=args.out_path,
             merge_only=args.merge_only,
             readers=args.readers,
             extra_arguments=args.extra_arguments,
             avoid_overwriting_merge=args.avoid_overwriting_merge,
             shm_mem_size=args.mem,
             clean_localhost_after_running=not args.no_clean,
             resume_previous_analysis=args.resume_previous_analysis,
             check_input_file_integrity=not args.dont_check_input_integrity,
             analysis_timeout=args.timeout)

    print_all_warnings()

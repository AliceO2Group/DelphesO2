#! /usr/bin/env python3

"""
Common header for AOD python scripts
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""

import argparse
import time
import multiprocessing
import sys
import os
import datetime
try:
    import tqdm
except ImportError as e:
    print("Module tqdm is not imported. Progress bar will not be available (you can install tqdm for the progress bar)")


# Global running flags
verbose_mode = False


def set_verbose_mode(parser):
    global verbose_mode
    verbose_mode = parser.verbose


def get_default_parser(description):
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("--verbose", "-v",
                        action="store_true", help="Verbose mode.")
    parser.add_argument("--njobs", "--jobs", "-j", type=int,
                        default=10,
                        help="Number of concurrent jobs, by default 10.")
    return parser


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


def fatal_msg(*args, fatal_message="Fatal Error!"):
    msg("[FATAL]", *args, color=bcolors.BFAIL)
    raise RuntimeError(fatal_message)


list_of_warnings = multiprocessing.Manager().list()


def warning_msg(*args, add=True):
    global list_of_warnings
    if add:
        list_of_warnings.append(args)
    msg("[WARNING]", *args, color=bcolors.BWARNING)


def print_all_warnings():
    if len(list_of_warnings) > 0:
        warning_msg("There were some warnings", add=False)
        for i in list_of_warnings:
            warning_msg(*i, add=False)


def run_in_parallel(processes, job_runner, job_arguments, job_message, linearize_single_core=False, force_no_progress_line=False):
    """
    In parallel processer of functions with a nice progress printing
    If linearize_single_core is True and processes is 1 then the processing is not on multiple cores
    """
    if processes == 1 and linearize_single_core:
        msg(job_message, "using 1 core i.e. no multicores")
        if "tqdm" not in sys.modules or force_no_progress_line:
            for i in enumerate(job_arguments):
                msg(f"Done: {i[0]+1},", len(job_arguments)-i[0]-1, "to go")
                job_runner(i[1])
        else:
            for i in tqdm.tqdm(job_arguments, bar_format='{l_bar}{bar:10}{r_bar}{bar:-10b}'):
                job_runner(i)
        return

    with multiprocessing.Pool(processes=processes) as pool:
        msg(job_message)
        result = []
        if "tqdm" not in sys.modules or force_no_progress_line:
            for i in enumerate(pool.imap(job_runner, job_arguments)):
                msg(f"Done: {i[0]+1},", len(job_arguments)-i[0]-1, "to go")
                result.append(i)
        else:
            result = list(tqdm.tqdm(pool.imap(job_runner, job_arguments),
                                    total=len(job_arguments),
                                    bar_format='{l_bar}{bar:10}{r_bar}{bar:-10b}'))
        return result


def run_cmd(cmd, comment="", check_status=True, log_file=None, print_output=False, time_it=False, throw_fatal=True):
    """
    Function to run a command in bash, allows to check the status of the command and to log the command output.
    If throw_fatal is true and check_status is true then it will throw a fatal message if the command is not OK
    If throw_fatal is true and check_status is true then it will return False if the command is not OK
    """
    verbose_msg("Running", f"'{cmd}'", bcolors.BOKBLUE + comment)
    try:
        if time_it:
            processing_time = time.time()
        to_run = cmd
        if check_status:
            to_run = f"{cmd} && echo OK"
        content = os.popen(to_run).read()
        if content:
            content = content.strip()
            for i in content.strip().split("\n"):
                verbose_msg("++", i)
            if print_output:
                for i in content.strip().split("\n"):
                    msg(i)
            if log_file is not None:
                with open(log_file, "a") as f_log:
                    f_log.write(f" -- {datetime.datetime.now()}\n")
                    f_log.write(f"    '{cmd}'\n")
                    for i in content.strip().split("\n"):
                        f_log.write(i + "\n")
        if "Encountered error" in content:
            warning_msg("Error encountered runtime error in", cmd)
        if check_status:
            if "OK" not in content and "root" not in cmd:
                if throw_fatal:
                    fatal_msg("Command", cmd,
                              "does not have the OK tag", content)
                else:
                    return False
        if time_it:
            processing_time = time.time() - processing_time
            msg(f"-- took {processing_time} seconds --",
                color=bcolors.BOKGREEN)
        return content
    except:
        fatal_msg("Error while running", f"'{cmd}'")

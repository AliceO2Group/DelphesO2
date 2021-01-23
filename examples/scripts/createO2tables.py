#! /usr/bin/env python3

"""
Handler to run the DelphesO2 framework and to create O2 analysis tables
"""

import argparse
import configparser
import subprocess
import os
import shutil
import multiprocessing
import numpy
import time
from datetime import datetime

# Global running flags
verbose_mode = False
metric_mode = False


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


def run_cmd(cmd, comment=""):
    verbose_msg("Running", f"'{cmd}'", comment)
    try:
        content = os.popen(cmd).read()
        if content:
            verbose_msg("++", content)
    except:
        raise ValueError("Error!")


def process_run(run_number):
    processing_time = time.time()
    msg("> starting run", run_number)
    run_cmd(f"bash runner{run_number}.sh")
    if not os.path.isfile(f"AODRun5.{run_number}.root"):
        msg("++ something went wrong for run", run_number, ", no output table found. Please check:",
            f"AODRun5.{run_number}.log", color=bcolors.FAIL)
    msg("< complete run", run_number)
    processing_time = time.time() - processing_time
    verbose_msg(f"-- took {processing_time} seconds --",
                color=bcolors.BOKGREEN)


def main(configuration_file,
         config_entry,
         njobs,
         nruns,
         nevents,
         verbose,
         qa,
         output_path,
         clean_delphes_files):
    global verbose_mode
    verbose_mode = verbose
    parser = configparser.RawConfigParser()
    parser.read(configuration_file)

    run_cmd("./clean.sh &> /dev/null")
    running_options = {}  # Dictionary of fetched options

    def opt(entry, require=True):
        try:
            o = parser.get(config_entry, entry)
            b = ['yes', 'no', 'on', 'off', 'true', 'false']
            for i in b:
                if o.lower() == i:
                    o = parser.getboolean(config_entry, entry)
                    break
            verbose_msg("Got option", entry, "=", f"'{o}'")
            running_options[entry] = o
            return o
        except:
            if require:
                raise ValueError("Missing entry", entry,
                                 "in file", configuration_file)
            return None

    # Config from the config file
    # simulation configuration
    if output_path is None:
        output_path = ""
    output_path = os.path.join(os.getcwd(), output_path)
    msg("Output will be found in", f"'{output_path}'")

    # detector configuration
    bField = opt("bField")
    sigmaT = opt("sigmaT")
    radius = opt("radius")
    length = opt("length")
    etaMax = opt("etaMax")

    # calculate max eta from geometry
    verbose_msg("Computing maximum eta based on detector length and radius")
    th = numpy.arctan2(float(radius), float(length))*0.5
    sth = numpy.sin(th)
    cth = numpy.cos(th)
    etaMax = -numpy.log(sth/cth)
    running_options["etaMax"] = etaMax

    # copy relevant files in the working directory
    def do_copy(in_file, out_file):
        in_file = os.path.expanduser(os.path.expandvars(in_file))
        verbose_msg("Copying", in_file, "to", out_file)
        shutil.copy2(in_file, out_file)

    do_copy(os.path.join(opt("card_path"),
                         opt("propagate_card")),
            "propagate.tcl")

    lut_path = opt("lut_path")
    lut_tag = opt("lut_tag")
    lut_particles = ["el", "mu", "pi", "ka", "pr"]
    for i in lut_particles:
        lut_bg = "{}kG".format(bField).replace(".", "")
        lut_n = f"lutCovm.{i}.{lut_bg}"
        do_copy(os.path.join(lut_path, f"{lut_n}.{lut_tag}.dat"),
                f"{lut_n}.dat")

    custom_gen = opt("custom_gen", require=False)
    if custom_gen is None:
        generators = opt("generators").split(" ")
        for i in generators:
            do_copy(i, ".")
        msg("Using pythia with configuration", generators)
    else:
        def check_duplicate(option_name):
            if f" {option_name}" in custom_gen:
                raise ValueError(f"Remove '{option_name}' from", custom_gen,
                                 "as it will be automatically set")
        for i in ["--output", "-o", "--nevents", "-n"]:
            check_duplicate(i)
        msg("Using custom generator", custom_gen)

    # Printing configuration
    msg(" --- running createO2tables.py", color=bcolors.HEADER)
    msg("  njobs   = ", njobs)
    msg("  nruns   = ", nruns)
    msg("  nevents = ", nevents)
    msg(" --- with detector configuration", color=bcolors.HEADER)
    msg("  bField  = ", bField, " [kG] ")
    msg("  sigmaT  = ", sigmaT, " [ns] ")
    msg("  radius  = ", radius, " [cm] ")
    msg("  length  = ", length, " [cm] ")
    msg("  LUT     = ", lut_tag)
    msg("  etaMax  = ", etaMax)

    aod_path = opt("aod_path")
    do_copy(os.path.join(aod_path, "createO2tables.h"), ".")
    do_copy(os.path.join(aod_path, "createO2tables.C"), ".")

    def set_config(config_file, config, value):
        config = config.strip()
        value = value.strip()
        config_string = f"{config} {value}"
        run_cmd("sed -i -e \"" f"s/{config} .*$/{config_string}" "\" "
                + config_file)
        # Checking that the file has the correct configuration
        with open(config_file) as f:
            has_it = False
            config_string = config_string.replace("\\", "").strip("/")
            for lineno, line in enumerate(f):
                if line.strip() == config_string:
                    verbose_msg("Found config string", config_string,
                                "in line", lineno, line.strip())
                    has_it = True
                    break
            if not has_it:
                raise ValueError(config_file,
                                 "does not have",
                                 config_string)

    # set magnetic field
    set_config("propagate.tcl", "set barrel_Bz", f"{bField}""e\-1/")
    set_config("createO2tables.C", "const double Bz = ", f"{bField}""e\-1\;/")
    set_config("dpl-config_std.json", "\"d_bz\":", "\""f"{bField}""\"\,/")
    # set radius
    set_config("propagate.tcl", "set barrel_Radius", f"{radius}""e\-2/")
    set_config("createO2tables.C",
               "const double tof_radius =", f"{radius}""\;/")
    # set length
    set_config("propagate.tcl", "set barrel_HalfLength", f"{length}""e\-2/")
    set_config("createO2tables.C",
               "const double tof_length =", f"{length}""\;/")
    # # set acceptance
    set_config("propagate.tcl", "set barrel_Acceptance",
               "\{ 0.0 + 1.0 * fabs(eta) < "f"{etaMax}"" \}/")
    # set time resolution
    set_config("propagate.tcl", "set barrel_TimeResolution",
               f"{sigmaT}""e\-9/")
    set_config("createO2tables.C",
               "const double tof_sigmat =", f"{sigmaT}""\;/")

    run_list = range(nruns)

    def configure_run(run_number):
        # Create executable that runs Generation, Delphes and analysis
        runner_file = f"runner{run_number}.sh"
        with open(runner_file, "w") as f_run:

            def write_to_runner(line, log_file=None, check_status=False):
                """
                Writes commands to runner
                """
                if log_file is not None:
                    line += f" &> {log_file}"
                line += "\n"
                f_run.write(line)
                if check_status:
                    f_run.write("\nReturnValue=$?\n")
                    f_run.write("if [[ $ReturnValue != 0 ]]; then\n")
                    f_run.write("  echo \"Encountered error\"\n")
                    f_run.write("  exit $ReturnValue\n")
                    f_run.write("fi\n")

            def copy_and_link(file_name):
                """
                In runner, copies file to output path (if different from current) and links it to current
                """
                if os.path.normpath(output_path) != os.getcwd():
                    write_to_runner(f"mv {file_name} {output_path} \n")
                    write_to_runner(f"ln -s {output_path}/{file_name} . \n")

            write_to_runner("#! /usr/bin/env bash\n")
            delphes_file = f"delphes.{run_number}.root"
            delphes_log_file = delphes_file.replace(".root", ".log")
            if custom_gen:  # Using HEPMC
                gen_log_file = f"gen.{run_number}.log"
                hepmc_file = f"hepmcfile.{run_number}.hepmc"
                custom_gen_option = f" --output {hepmc_file} --nevents {nevents}"
                write_to_runner(custom_gen + custom_gen_option,
                                log_file=gen_log_file)
                write_to_runner(f"DelphesHepMC propagate.tcl {delphes_file} {hepmc_file}",
                                log_file=delphes_log_file)
            else:  # Using DelphesPythia
                # copy generator configuration
                generator_cfg = f"generator.{run_number}.cfg"
                generator_orig = generators[0].split("/")[-1]
                do_copy(generator_orig, generator_cfg)
                # Adjust configuration file
                with open(generator_cfg, "a") as f_cfg:
                    # number of events and random seed
                    f_cfg.write(f"Main:numberOfEvents {nevents}\n")
                    f_cfg.write(f"Random:seed = {run_number}\n")
                    # collision time spread [mm/c]
                    f_cfg.write("Beams:allowVertexSpread on \n")
                    f_cfg.write("Beams:sigmaTime 60.\n")
                    for i in generators[1:]:
                        with open(i.split("/")[-1], "r") as f_append:
                            f_cfg.write(f_append.read())
                write_to_runner(f"DelphesPythia8 propagate.tcl {generator_cfg} {delphes_file}",
                                log_file=delphes_log_file,
                                check_status=True)
            aod_file = f"AODRun5.{run_number}.root"
            aod_log_file = aod_file.replace(".root", ".log")
            write_to_runner(f"root -b -q -l 'createO2tables.C+(\"{delphes_file}\", \"{aod_file}\", 0)'",
                            log_file=aod_log_file,
                            check_status=True)
            if not clean_delphes_files:
                copy_and_link(delphes_file)
            copy_and_link(aod_file)
            if clean_delphes_files:
                write_to_runner(f"rm {delphes_file}")
            write_to_runner("exit 0\n")
    for i in run_list:
        configure_run(i)

    # Compiling the table creator macro once for all
    run_cmd("root -l -b -q 'createO2tables.C+(\"\")' &> /dev/null",
            comment="to compile the table creator only once, before running")
    total_processing_time = time.time()
    msg(" --- start processing the runs ", color=bcolors.HEADER)
    with multiprocessing.Pool(processes=njobs) as pool:
        pool.map(process_run, run_list)

    # merge runs when all done
    msg(" --- all runs are processed, so long", color=bcolors.HEADER)
    total_processing_time = time.time() - total_processing_time
    msg(f"-- took {total_processing_time} seconds in total --",
        color=bcolors.BOKGREEN)

    # Writing the list of produced AODs
    output_list_file = "listfiles.txt"
    with open(output_list_file, "w") as listfiles:
        for i in os.listdir("."):
            if "AODRun5." in i and i.endswith(".root"):
                listfiles.write(f"{os.getcwd()}/{i}\n")

    # Writing summary of production
    summaryfile = "summary.txt"
    with open(summaryfile, "w") as f:
        f.write("\n## Summary of last run ##\n")
        now = datetime.now()
        dt_string = now.strftime("%d/%m/%Y %H:%M:%S")
        f.write(f"Finished at {dt_string}\n")
        f.write(f"Took {total_processing_time} seconds\n")

        def write_config(entry):
            f.write(f"{entry[0]} = {entry[1]}\n")

        f.write("\n## Configuration ##\n")
        write_config(["- configuration_file", configuration_file])
        write_config(["- config_entry", config_entry])
        write_config(["- njobs", njobs])
        write_config(["- nruns", nruns])
        write_config(["- nevents", nevents])

        f.write("\n## Options ##\n")
        for i in running_options:
            write_config([i, running_options[i]])

    run_cmd("echo  >> " + summaryfile)
    run_cmd("echo + DelphesO2 Version + >> " + summaryfile)
    run_cmd("git rev-parse HEAD >> " + summaryfile)
    if os.path.normpath(output_path) != os.getcwd():
        run_cmd(f"mv {summaryfile} {output_path}")

    if qa:
        msg(" --- running test analysis", color=bcolors.HEADER)
        run_cmd(f"./diagnostic_tools/doanalysis.py 2 -i {output_list_file}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("configuration_file", type=str,
                        help="Input configuration file e.g. you can use the provided default_configfile.ini or variations of it.")
    parser.add_argument("--entry", "-e", type=str,
                        default="DEFAULT",
                        help="Entry in the configuration file, e.g. the INEL or CCBAR entries in the configuration file.")
    parser.add_argument("--output-path", "-o", type=str,
                        default=None,
                        help="Output path, by default the current path is used as output.")
    parser.add_argument("--njobs", "-j", type=int,
                        default=10,
                        help="Number of concurrent jobs, by default 10.")
    parser.add_argument("--nevents", "--ev", type=int,
                        default=1000,
                        help="Number of simulated events, by default 1000.")
    parser.add_argument("--nruns", "--runs", "-r", type=int,
                        default=10,
                        help="Number of runs, by default 10.")
    parser.add_argument("--qa", "-qa", action="store_true",
                        help="QA mode: runs basic tasks at the end to assess QA.")
    parser.add_argument("--verbose", "-v",
                        action="store_true", help="Verbose mode.")
    parser.add_argument("--clean-delphes", "-c",
                        action="store_true",
                        help="Option to clean the delphes files in output and keep only the AODs, by default everything is kept.")
    args = parser.parse_args()
    main(configuration_file=args.configuration_file,
         config_entry=args.entry,
         njobs=args.njobs,
         nevents=args.nevents,
         nruns=args.nruns,
         verbose=args.verbose,
         output_path=args.output_path,
         clean_delphes_files=args.clean_delphes,
         qa=args.qa)

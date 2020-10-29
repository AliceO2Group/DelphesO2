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


def verbose_msg(*args):
    if verbose_mode:
        print("** ", bcolors.OKBLUE, *args, bcolors.ENDC)


def msg(*args, color=bcolors.BOKBLUE):
    print(color, *args, bcolors.ENDC)


def run_cmd(cmd):
    verbose_msg("Running", cmd)
    content = os.popen(cmd).read()
    if content:
        verbose_msg(content)


def process_run(run_number):
    if metric_mode:
        start_time = time.time()
    msg("> starting run", run_number)
    run_cmd(f"bash runner{run_number}.sh")
    msg("< complete run", run_number)
    if metric_mode:
        msg(f"-- took {time.time() - start_time} seconds --",
            color=bcolors.BOKGREEN)


def main(configuration_file, config_entry, njobs, nruns, nevents, metric, verbose):
    global verbose_mode
    verbose_mode = verbose
    global metric_mode
    metric_mode = metric
    parser = configparser.RawConfigParser()
    parser.read(configuration_file)

    run_cmd("./clean.sh &> /dev/null")

    def opt(entry, require=True):
        try:
            o = parser.get(config_entry, entry)
            b = ['yes', 'no', 'on', 'off', 'true', 'false']
            for i in b:
                if o.lower() == i:
                    o = parser.getboolean(config_entry, entry)
                    break
            verbose_msg("Got option", entry, "=", f"'{o}'")
            return o
        except:
            if require:
                raise ValueError("Missing entry", entry,
                                 "in file", configuration_file)
            return None

    # Config from the config file
    # detector configuration
    bField = opt("bField")
    sigmaT = opt("sigmaT")
    radius = opt("radius")
    length = opt("length")
    etaMax = opt("etaMax")

    # calculate max eta from geometry
    th = numpy.arctan2(float(radius), float(length))*0.5
    sth = numpy.sin(th)
    cth = numpy.cos(th)
    etaMax = -numpy.log(sth/cth)

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
    msg("  etaMax  = ", etaMax)
    msg(" --- start processing the runs ", color=bcolors.HEADER)

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
        do_copy(os.path.join(
            lut_path, f"{lut_n}.{lut_tag}.dat"), f"{lut_n}.dat")

    custom_gen = opt("custom_gen", require=False)
    if custom_gen is None:
        generators = opt("generators").split(" ")
        for i in generators:
            do_copy(i, ".")
        verbose_msg("Using default pythia with configuration", generators)
    else:
        verbose_msg("Using custom generator", custom_gen)

    aod_path = opt("aod_path")
    do_copy(os.path.join(aod_path, "createO2tables.h"), ".")
    do_copy(os.path.join(aod_path, "createO2tables.C"), ".")

    def set_config(config_file, config, value):
        config = config.strip()
        value = value.strip()
        run_cmd(
            "sed -i -e \"" f"s/{config} .*$/{config} {value}" "\" " + config_file)
        # Checking that the file has the correct configuration
        with open(config_file) as f:
            has_it = False
            config_string = f"{config} {value}".replace("\\", "").strip("/")
            for i in f:
                if i.strip() == config_string:
                    has_it = True
                    break
            if not has_it:
                raise ValueError(config_file,
                                 "does not have",
                                 config_string)

    # set magnetic field
    set_config("propagate.tcl", "set barrel_Bz", f"{bField}""e\-1/")
    set_config("createO2tables.C", "double Bz = ", f"{bField}""e\-1\;/")
    set_config("dpl-config_std.json", "\"d_bz\":", "\""f"{bField}""\"\,/")
    # set radius
    set_config("propagate.tcl", "set barrel_Radius", f"{radius}""e\-2/")
    set_config("createO2tables.C", "double tof_radius =", f"{radius}""\;/")
    # set length
    set_config("propagate.tcl", "set barrel_HalfLength", f"{length}""e\-2/")
    set_config("createO2tables.C", "double tof_length =", f"{length}""\;/")
    # # set acceptance
    set_config("propagate.tcl", "set barrel_Acceptance",
               "\{ 0.0 + 1.0 * fabs(eta) < "f"{etaMax}"" \}/")
    # set time resolution
    set_config("propagate.tcl", "set barrel_TimeResolution",
               f"{sigmaT}""e\-9/")
    set_config("createO2tables.C", "double tof_sigmat =", f"{sigmaT}""\;/")

    run_list = range(nruns)

    def configure_run(run_number):
        # Create executable that runs Geneartio, Delphes and analysis
        runner_file = f"runner{run_number}.sh"
        with open(runner_file, "w") as f_run:
            f_run.write(f"#! /usr/bin/env bash\n")
            delphes_file = f"delphes.{run_number}.root"
            delphes_log_file = delphes_file.replace(".root", ".log")
            if custom_gen:  # Using HEPMC
                gen_log_file = f"gen.{run_number}.root"
                hepmc_file = f"hepmcfile.{run_number}.hepmc"
                f_run.write(
                    custom_gen + f" --output {hepmc_file} &> {gen_log_file}\n")
                f_run.write(
                    f"DelphesHepMC propagate.tcl {delphes_file} {hepmc_file} &> {delphes_log_file}\n")
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
                f_run.write(
                    f"DelphesPythia8 propagate.tcl {generator_cfg} {delphes_file} &> {delphes_log_file}\n")
            aod_file = f"AODRun5.{run_number}.root"
            aod_log_file = aod_file.replace(".root", ".log")
            f_run.write(
                f"root -b -q -l 'createO2tables.C(\"{delphes_file}\", \"{aod_file}\", 0)' &> {aod_log_file}\n")
    for i in run_list:
        configure_run(i)

    if metric_mode:
        total_start_time = time.time()
    with multiprocessing.Pool(processes=njobs) as pool:
        pool.map(process_run, run_list)

    # merge runs when all done
    msg(" --- all runs are processed, so long", color=bcolors.HEADER)
    if metric_mode:
        msg(f"-- took {time.time() - total_start_time} seconds in total --",
            color=bcolors.BOKGREEN)

    with open("listfiles.txt", "w") as listfiles:
        for i in os.listdir("."):
            if "AODRun5." in i and i.endswith(".root"):
                listfiles.write(i)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("configuration_file", type=str,
                        help="Input configuration file")
    parser.add_argument("--entry", type=str,
                        default="DEFAULT",
                        help="Entry in the configuration file")
    parser.add_argument("--njobs", type=int,
                        default=10,
                        help="Number of concurrent jobs")
    parser.add_argument("--nevents", type=int,
                        default=1000,
                        help="Number of simulated events (only in non custom generator mode)")
    parser.add_argument("--nruns", type=int,
                        default=10,
                        help="Number of runs")
    parser.add_argument("-t", action="store_true", help="Metric mode: to compute wall time")
    parser.add_argument("-v", action="store_true", help="Verbose mode")
    args = parser.parse_args()
    main(configuration_file=args.configuration_file,
         config_entry=args.entry,
         njobs=args.njobs,
         nevents=args.nevents,
         nruns=args.nruns,
         metric=args.t,
         verbose=args.v)

#! /usr/bin/env python3

"""
Handler to run the DelphesO2 framework and to create O2 analysis tables.
Author: Nicolo' Jacazio, nicolo.jacazio@cern.ch
"""

import configparser
import os
import shutil
import time
import glob
import random
from datetime import datetime
from common import bcolors, msg, fatal_msg, verbose_msg, run_in_parallel, set_verbose_mode, get_default_parser, run_cmd


def process_run(run_number):
    processing_time = time.time()
    verbose_msg("> starting run", run_number)
    run_cmd(f"bash runner{run_number}.sh")
    aod_name = f"AODRun5.{run_number}.root"
    if not os.path.isfile(aod_name):
        msg(f"++ something went wrong for run {run_number}, no output AOD file {aod_name} found.",
            f"Please check: 'AODRun5.{run_number}.log'",
            color=bcolors.FAIL)
    verbose_msg("< complete run", run_number)
    processing_time = time.time() - processing_time
    verbose_msg(f"-- took {processing_time} seconds --",
                color=bcolors.BOKGREEN)


def main(configuration_file,
         config_entry,
         njobs,
         nruns,
         nevents,
         qa,
         output_path,
         clean_delphes_files,
         create_luts,
         turn_off_vertexing,
         append_production,
         use_nuclei):
    arguments = locals()  # List of arguments to put into the log
    parser = configparser.RawConfigParser()
    parser.read(configuration_file)
    if config_entry not in parser.keys():
        fatal_msg(f"Did not find configuration entry '{config_entry}' in config file",
                  configuration_file + "\n\t Available entries:\n\t\t" + "\n\t\t".join(list(parser.keys())))

    run_cmd("./clean.sh > /dev/null 2>&1", check_status=False)
    # Dictionary of fetched options
    running_options = {}
    for i in arguments:
        running_options["ARG "+i] = arguments[i]

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
                fatal_msg("Missing entry", f"'{entry}'",
                          "in configuration file", f"'{configuration_file}'")
            return None

    # Config from the config file
    # simulation configuration
    if output_path is None:
        output_path = ""
    output_path = os.path.join(os.getcwd(), output_path)
    msg("Output will be found in", f"'{output_path}'")
    if not os.path.isdir(output_path):
        msg("Creating output path")
        os.makedirs(output_path)
        if not os.path.isdir(output_path):
            raise RuntimeError("Cannot find output path", output_path)

    # detector configuration
    bField = opt("bField")
    sigmaT = opt("sigmaT")
    sigmaT0 = opt("sigmaT0")
    barrel_radius = opt("barrel_radius")
    etaMax = opt("etamax")
    barrel_half_length = opt("barrel_half_length")

    # copy relevant files in the working directory
    def do_copy(in_file, out_file=".", in_path=None):
        """Function to copy files"""
        if in_path is not None:
            in_file = os.path.join(in_path, in_file)
        in_file = os.path.expanduser(os.path.expandvars(in_file))
        verbose_msg("Copying", in_file, "to", out_file)
        shutil.copy2(in_file, out_file)

    # Fetching the propagation card
    do_copy(opt("propagate_card"), "propagate.tcl", in_path=opt("card_path"))

    lut_path = opt("lut_path")
    lut_tag = opt("lut_tag")
    lut_particles = ["el", "mu", "pi", "ka", "pr"]
    if use_nuclei:
        lut_particles += ["de", "tr", "he3"]
    if create_luts:
        # Creating LUTs
        minimum_track_radius = opt("minimum_track_radius")
        verbose_msg("Creating LUTs")
        lut_path = os.path.join(lut_path, "create_luts.sh")
        run_cmd(f"{lut_path} {lut_tag} {float(bField)*0.1} {minimum_track_radius} 2>&1",
                f"Creating the lookup tables with tag {lut_tag} from {lut_path} script")
    else:
        # Fetching LUTs
        verbose_msg(f"Fetching LUTs with tag {lut_tag} from path {lut_path}")
        for i in lut_particles:
            lut_bg = "{}kG".format(bField).replace(".", "")
            do_copy(f"lutCovm.{i}.{lut_bg}.{lut_tag}.dat",
                    f"lutCovm.{i}.dat", in_path=lut_path)

    # Checking that we actually have LUTs
    for i in lut_particles:
        i = f"lutCovm.{i}.dat"
        if not os.path.isfile(i):
            fatal_msg("Did not find LUT file", i)

    custom_gen = opt("custom_gen", require=False)
    if custom_gen is None:
        # Checking that the generators are defined
        if opt("generators", require=False) is None:
            fatal_msg("Did not find any generator configuration corresponding to the entry",
                      config_entry, "in your configuration file", configuration_file)
        generators = opt("generators").split(" ")
        for i in generators:
            do_copy(i)
        msg("Using pythia with configuration", generators)
    else:
        def check_duplicate(option_name):
            if f" {option_name}" in custom_gen:
                fatal_msg(f"Remove '{option_name}' from", custom_gen,
                          "as it will be automatically set")
        for i in ["--output", "-o", "--nevents", "-n"]:
            check_duplicate(i)
        if "INPUT_FILES" in custom_gen:
            input_hepmc_files = custom_gen.replace("INPUT_FILES",
                                                   "").strip().split(" ")
            input_hepmc_file_list = []
            for i in input_hepmc_files:
                input_hepmc_file_list += glob.glob(os.path.normpath(i))

            if len(input_hepmc_file_list) >= nruns:
                input_hepmc_file_list = input_hepmc_file_list[0:nruns]
            else:
                nruns = len(input_hepmc_file_list)

            if len(input_hepmc_file_list) <= 0:
                fatal_msg("Did not find any input file matching to the request:",
                          custom_gen)
            custom_gen = f"INPUT_FILES "+" ".join(input_hepmc_file_list)
            msg("Using", len(input_hepmc_file_list),
                "input HepMC file" +
                ("" if len(input_hepmc_file_list) == 1 else "s"),
                input_hepmc_file_list)
        else:
            msg("Using custom generator", custom_gen)

    # Printing configuration
    msg(" --- running createO2tables.py", color=bcolors.HEADER)
    msg("  n. jobs        =", njobs)
    msg("  n. runs        =", nruns)
    msg("  events per run =", nevents)
    msg("  tot. events    =", "{:.0e}".format(nevents*nruns))
    msg("  LUT path       =", f"'{lut_path}'")
    msg(" --- with detector configuration", color=bcolors.HEADER)
    msg("  B field              =", bField, "[kG]")
    msg("  sigmaT               =", sigmaT, "[ns]")
    msg("  sigmaT0              =", sigmaT0, "[ns]")
    msg("  Barrel radius        =", barrel_radius, "[cm]")
    msg("  Barrel half length   =", barrel_half_length, "[cm]")
    if create_luts:
        msg("  Minimum track radius =", minimum_track_radius, "[cm]")
    msg("  LUT                  =", lut_tag)
    msg("  etaMax               =", etaMax)

    aod_path = opt("aod_path")
    do_copy("createO2tables.h", in_path=aod_path)
    do_copy("createO2tables.C", in_path=aod_path)
    do_copy("muonAccEffPID.root", in_path=aod_path)
    if qa:
        do_copy("diagnostic_tools/dpl-config_std.json")

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
                fatal_msg("Configuration file", config_file,
                          f"does not have config string '{config_string}'")

    # set magnetic field
    set_config("propagate.tcl", "set barrel_Bz", f"{bField}""e\-1/")
    set_config("createO2tables.C", "const double Bz = ", f"{bField}""e\-1\;/")
    if turn_off_vertexing:
        set_config("createO2tables.C",
                   "const bool do_vertexing = ", "false\;/")
    if use_nuclei:
        set_config("createO2tables.C",
                   "const bool enable_nuclei = ", "true\;/")
    else:  # Check that the geometry file for the vertexing is there
        if not os.path.isfile("o2sim_grp.root") or not os.path.isfile("o2sim_geometry.root"):
            run_cmd("mkdir tmpo2sim && cd tmpo2sim && o2-sim -m PIPE ITS MFT -g boxgen -n 1 -j 1 --configKeyValues 'BoxGun.number=1' && cp o2sim_grp.root .. && cp o2sim_geometry.root .. && cd .. && rm -r tmpo2sim")
    if qa:
        set_config("dpl-config_std.json", "\\\"d_bz\\\":",
                   "\\\""f"{bField}""\\\"\,/")
    # set barrel_radius
    set_config("propagate.tcl", "set barrel_Radius", f"{barrel_radius}""e\-2/")
    set_config("createO2tables.C",
               "const double tof_radius =", f"{barrel_radius}""\;/")
    # set barrel_half_length
    set_config("propagate.tcl", "set barrel_HalfLength",
               f"{barrel_half_length}""e\-2/")
    set_config("createO2tables.C",
               "const double tof_length =", f"{barrel_half_length}""\;/")
    # # set acceptance
    set_config("propagate.tcl", "set barrel_Acceptance",
               "\{ 0.0 + 1.0 * fabs(eta) < "f"{etaMax}"" \}/")
    # set time resolution
    set_config("propagate.tcl", "set barrel_TimeResolution",
               f"{sigmaT}""e\-9/")
    set_config("createO2tables.C",
               "const double tof_sigmat =", f"{sigmaT}""\;/")
    set_config("createO2tables.C",
               "const double tof_sigmat0 =", f"{sigmaT0}""\;/")
    run_list = range(nruns)
    if append_production:
        if output_path is None:
            fatal_msg("Output path is not defined, cannot append")
        last_preexisting_aod = [each for each in os.listdir(output_path)
                                if each.endswith('.root') and "AODRun5" in each]
        if len(last_preexisting_aod) == 0:
            fatal_msg("Appending to a non existing production")
        last_preexisting_aod = sorted([int(each.replace("AODRun5.", "").replace(".root", ""))
                                       for each in last_preexisting_aod])[-1] + 1
        msg(f" Appending to production with {last_preexisting_aod} AODs",
            color=bcolors.BWARNING)
        run_list = range(last_preexisting_aod,
                         last_preexisting_aod + nruns)

    def configure_run(run_number):
        # Create executable that runs Generation, Delphes and analysis
        runner_file = f"runner{run_number}.sh"
        with open(runner_file, "w") as f_run:

            def write_to_runner(line, log_file=None, check_status=False):
                """
                Writes commands to runner
                """
                log_line = ""
                if log_file is not None:
                    log_line = f" &> {log_file} 2>&1"
                    line += log_line
                line += "\n"
                f_run.write(line)
                if check_status:
                    f_run.write("\nReturnValue=$?\n")
                    f_run.write("if [[ $ReturnValue != 0 ]]; then\n")
                    f_run.write("  echo \"Encountered error with command: '")
                    line = line.replace(log_line, "")
                    f_run.write(line.replace("\"", "\\\"").strip())
                    f_run.write("'\"\n")
                    if log_file is not None:
                        f_run.write("  echo \"Check log: '")
                        f_run.write(log_file.strip() + "'\"\n")
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
            hepmc_file = None
            mc_seed = random.randint(1, 800000000)
            if custom_gen:  # Using HEPMC
                hepmc_file = f"hepmcfile.{run_number}.hepmc"
                if "INPUT_FILES" in custom_gen:
                    input_hepmc_file = custom_gen.replace("INPUT_FILES",
                                                          "").strip().split(" ")
                    input_hepmc_file = input_hepmc_file[run_number]
                    write_to_runner(f"ln -s {input_hepmc_file}"
                                    f" {hepmc_file} \n")
                else:
                    gen_log_file = f"gen.{run_number}.log"
                    custom_gen_option = f" --output {hepmc_file} --nevents {nevents} --seed {mc_seed}"
                    write_to_runner(custom_gen + custom_gen_option,
                                    log_file=gen_log_file, check_status=True)
                write_to_runner(f"DelphesHepMC propagate.tcl {delphes_file} {hepmc_file}",
                                log_file=delphes_log_file, check_status=True)
            else:  # Using DelphesPythia
                # copy generator configuration
                generator_cfg = f"generator.{run_number}.cfg"
                generator_orig = generators[0].split("/")[-1]
                do_copy(generator_orig, generator_cfg)
                # Adjust configuration file
                with open(generator_cfg, "a") as f_cfg:
                    # number of events and random seed
                    f_cfg.write(f"\n\n\n#### Additional part ###\n\n\n\n")
                    f_cfg.write(f"Main:numberOfEvents {nevents}\n")
                    f_cfg.write(f"Random:setSeed = on\n")
                    f_cfg.write(f"Random:seed = {mc_seed}\n")
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
            write_to_runner(f"root -l -b -q 'createO2tables.C+(\"{delphes_file}\", \"tmp_{aod_file}\", 0)'",
                            log_file=aod_log_file,
                            check_status=True)
            # Check that there were no O2 errors
            write_to_runner(
                f"if grep -q \"\[ERROR\]\" {aod_log_file}; then echo \": got some errors in '{aod_log_file}'\" && echo \"Found some ERROR in this log\" >> {aod_log_file}; fi")
            write_to_runner(
                f"if grep -q \"\[FATAL\]\" {aod_log_file}; then echo \": got some fatals in '{aod_log_file}'\" && echo \"Found some FATAL in this log\" >> {aod_log_file} && exit 1; fi")
            # Rename the temporary AODs to standard AODs
            write_to_runner(f"mv tmp_{aod_file} {aod_file}", check_status=True)
            if not clean_delphes_files:
                copy_and_link(delphes_file)
                if hepmc_file is not None:
                    copy_and_link(hepmc_file)
            copy_and_link(aod_file)
            if clean_delphes_files:
                write_to_runner(f"rm {delphes_file}")
                if hepmc_file is not None:
                    write_to_runner(f"rm {hepmc_file}")
            write_to_runner("exit 0\n")

    # Configuring all the runs
    for i in run_list:
        configure_run(i)

    # Compiling the table creator macro once for all
    run_cmd("root -l -b -q 'createO2tables.C+(\"\")' > /dev/null 2>&1",
            comment="to compile the table creator only once, before running")
    if not os.path.isfile("createO2tables_C.so"):
        run_cmd("root -l -b -q 'createO2tables.C+(\"\")'",
                comment="to compile with full log")
        fatal_msg("'createO2tables.C' did not compile!")
    total_processing_time = time.time()
    msg(" --- start processing the runs ", color=bcolors.HEADER)
    run_in_parallel(processes=njobs, job_runner=process_run,
                    job_arguments=run_list, job_message="Running production")

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

        def write_config(entry, prefix=""):
            f.write(prefix + entry.strip("ARG ") +
                    f" = {running_options[entry]}\n")

        f.write("\n## Configuration ##\n")
        for i in running_options:
            if "ARG" in i:
                write_config(i, prefix=" - ")

        f.write("\n## Options ##\n")
        for i in running_options:
            if "ARG" not in i:
                write_config(i, prefix=" * ")

        output_size = sum(os.path.getsize(os.path.join(output_path, f))
                          for f in os.listdir(output_path)
                          if os.path.isfile(os.path.join(output_path, f)))
        f.write("\n##  Size of the ouput ##\n")
        f.write(f" - {output_size} bytes\n")
        f.write(f" - {output_size/1e6} MB\n")
        f.write(f" - {output_size/1e9} GB\n")
    run_cmd("echo  >> " + summaryfile)
    run_cmd("echo + DelphesO2 Version + >> " + summaryfile)
    run_cmd("git rev-parse HEAD >> " + summaryfile, check_status=False)

    if os.path.normpath(output_path) != os.getcwd():
        if append_production:
            s = os.path.join(output_path, summaryfile)
            run_cmd(f"echo '' >> {s}")
            run_cmd(f"echo '  **' >> {s}")
            run_cmd(f"echo 'Appended production' >> {s}")
            run_cmd(f"echo '  **' >> {s}")
            run_cmd(f"echo '' >> {s}")
            run_cmd(f"cat {summaryfile} >> {s}")
        else:
            run_cmd(f"mv {summaryfile} {output_path}")
            run_cmd(f"ln -s {output_path}/{summaryfile} ./")

    if qa:
        msg(" --- running test analysis", color=bcolors.HEADER)
        run_cmd(
            f"./diagnostic_tools/doanalysis.py TrackQA RICH TOF -i {output_list_file} -M 25 -B 25")


if __name__ == "__main__":
    parser = get_default_parser(description=__doc__)
    parser.add_argument("configuration_file", type=str,
                        help="Input configuration file e.g. you can use the provided default_configfile.ini or variations of it.")
    parser.add_argument("--entry", "-e", type=str,
                        default="DEFAULT",
                        help="Entry in the configuration file, e.g. the INEL or CCBAR entries in the configuration file.")
    parser.add_argument("--output-path", "--output_path", "-o", type=str,
                        default=None,
                        help="Output path, by default the current path is used as output.")
    parser.add_argument("--nevents", "--ev", type=int,
                        default=1000,
                        help="Number of simulated events, by default 1000.")
    parser.add_argument("--nruns", "--runs", "-r", type=int,
                        default=10,
                        help="Number of runs, by default 10.")
    parser.add_argument("--qa", "-qa", action="store_true",
                        help="QA mode: runs basic tasks at the end to assess QA.")
    parser.add_argument("--clean-delphes", "-c",
                        action="store_true",
                        help="Option to clean the delphes files in output and keep only the AODs, by default everything is kept.")
    parser.add_argument("--no-vertexing",
                        action="store_true",
                        help="Option turning off the vertexing.")
    parser.add_argument("--append", "-a",
                        action="store_true",
                        help="Option to append the results instead of starting over by shifting the AOD indexing. N.B. the user is responsible of the compatibility between appended AODs. Only works in conjuction by specifying an output path (option '-o')")
    parser.add_argument("--nuclei",
                        action="store_true",
                        help="Option use nuclei LUTs")
    parser.add_argument("--use-preexisting-luts", "-l",
                        action="store_true",
                        help="Option to use preexisting LUTs instead of creating new ones, in this case LUTs with the requested tag are fetched from the LUT path. By default new LUTs are created at each run.")
    args = parser.parse_args()
    set_verbose_mode(args)

    # Check arguments
    if args.append and args.output_path is None:
        fatal_msg(
            "Asked to append production but did not specify output path (option '-o')")
    main(configuration_file=args.configuration_file,
         config_entry=args.entry,
         njobs=args.njobs,
         nevents=args.nevents,
         nruns=args.nruns,
         output_path=args.output_path,
         clean_delphes_files=args.clean_delphes,
         qa=args.qa,
         create_luts=not args.use_preexisting_luts,
         turn_off_vertexing=args.no_vertexing,
         append_production=args.append,
         use_nuclei=args.nuclei)

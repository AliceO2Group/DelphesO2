#!/usr/bin/env python3

"""
Handler script to prepare the working environment for the grid submission of fast simulation + AOD creation with private jobs
"""

import argparse
import os
import configparser


def run_cmd(cmd):
    os.popen(cmd).read()


def main(jdl_file,
         grid_path,
         source_path,
         number_of_events,
         user_mail,
         user,
         njobs,
         configuration_file,
         config_tag,
         delphes_version,
         make_alien_directory):
    """
    Function to create the working environment for GRID job submission
    """
    if make_alien_directory:
        print("Making directory on alien:", grid_path)
        run_cmd(f"alien_mkdir alien://{grid_path}")
        run_cmd(f"alien_cp {configuration_file} alien://{grid_path}" +
                configuration_file.split("/")[-1])
    parser = configparser.RawConfigParser()
    parser.read(configuration_file)
    configuration_file = configuration_file.split("/")[-1]

    print("Writing JDL file to", jdl_file)
    with open(jdl_file, "w") as f:
        def write_line(line):
            f.write(line + "\n")
        write_line("# Simulation + AOD creation JDL")
        write_line("#\n")
        write_line(f"Executable = \"{source_path}/starter.sh\";")
        write_line(f"Validationcommand = \"{source_path}/validation.sh\";")
        write_line(f"Arguments = \"python3 createO2tables.py {configuration_file} -j 1 -r 1 " +
                   f"--ev {number_of_events} -e {config_tag} -l -v --no-vertexing\";\n")
        write_line("Requirements = ( other.Type == \"machine\" );\n")
        write_line("Packages = {")
        write_line(f"\t\"VO_ALICE@{delphes_version}\"")
        write_line("};\n")
        write_line("JDLVariables = {")
        write_line("\t\"Packages\",")
        write_line("\t\"OutputDir\"")
        write_line("};\n")
        write_line("Type = \"Job\";")
        write_line(f"User = \"{user}\";")
        write_line("Jobtag = {")
        write_line("\t\"comment: DelphesO2 Simulation + AOD jdl\"")
        write_line("};")
        if user_mail is None:
            user_mail = f"{user}@cern.ch"
        write_line(f"EMail = \"{user_mail}\";")
        write_line("TTL = \"86400\";")
        write_line("Price = 1;")
        write_line("Workdirectorysize = {")
        write_line("\t\"12000MB\"")
        write_line("};\n")
        write_line(f"Split = \"production:1-{njobs}\";")
        write_line("SplitArguments = \"\";\n")
        write_line("InputFile = {")

        lut_tag = parser.get("DEFAULT", "lut_tag")
        bfield = parser.get("DEFAULT", "bfield").replace(".", "")
        for i in ["el", "mu", "pi", "ka", "pr"]:
            write_line(
                f"\t\"LF:{source_path}/lutCovm.{i}.{int(bfield)}kG.{lut_tag}.dat\"")
        files = ["createO2tables.py",
                 "o2sim_grp.root",
                 "o2sim_geometry.root"]
        for i in files:
            write_line(f"\t\"LF:{source_path}/{i}\",")
        write_line(f"\t\"LF:{grid_path}/{configuration_file}\"")
        write_line("};\n")
        write_line("OutputArchive = {")
        write_line("\t\"log_archive.zip:stdout,stderr,*.log,*.sh@\",")
        write_line("\t\"root_archive.zip:AODRun5*.root@\"")
        write_line("};\n")
        write_line(f"OutputDir = \"{grid_path}/output/#alien_counter_03i#\";")

    if make_alien_directory:
        run_cmd(f"alien_cp {jdl_file} alien://{grid_path}" +
                jdl_file.split("/")[-1])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--jdl_file", "-o", type=str,
                        default="run5aod.jdl",
                        help="Name of the jdl to create")
    parser.add_argument("grid_path", type=str,
                        help="Path on the grid where to store the configurations and output files")
    parser.add_argument("source_path", type=str,
                        help="Path on the grid where to fetch the input files from e.g. LUTs, geometry files for vertexing, ")
    parser.add_argument("--user_mail", "--mail",
                        type=str, default=None,
                        help="Mail address of the user for notification purposes")
    parser.add_argument("--user", "-u", type=str,
                        default=os.getlogin(),
                        help="Name of the user")
    parser.add_argument("--configuration_file",
                        type=str, default="../default_configfile.ini",
                        help="Input configuration file e.g. you can use the provided default_configfile.ini or variations of it.")
    parser.add_argument("--entry", "-e",
                        type=str, default="INEL",
                        help="Entry in the configuration file, e.g. the INEL or CCBAR entries in the configuration file.")
    parser.add_argument("--njobs", "-j",
                        type=int, default=2,
                        help="Number of concurrent jobs, by default 2.")
    parser.add_argument("--nevents", "--ev",
                        type=int, default=1000,
                        help="Number of simulated events, by default 1000.")
    parser.add_argument("--delphes_version", "--delphes",
                        type=str, default="DelphesO2::v20210409-1",
                        help="Version of DelphesO2 to use")
    parser.add_argument("--no-vertexing",
                        action="store_true",
                        help="Option turning off the vertexing.")
    parser.add_argument("--make-alien-dir", "--make-dir",
                        action="store_true",
                        help="Option make the directory and upload files to it.")
    args = parser.parse_args()
    main(jdl_file=args.jdl_file,
         configuration_file=args.configuration_file,
         number_of_events=args.nevents,
         grid_path=args.grid_path,
         source_path=args.source_path,
         user_mail=args.user_mail,
         user=args.user,
         config_tag=args.entry,
         njobs=args.njobs,
         delphes_version=args.delphes_version,
         make_alien_directory=args.make_alien_dir)

#! /usr/bin/env python3

"""
Utility script to download files from grid
Author: Nicolo' Jacazio, nicolo.jacazio@cern.ch
"""


import os
from os import path
from common import bcolors, msg, run_cmd, run_in_parallel, verbose_msg, get_default_parser, set_verbose_mode, warning_msg
import getpass
import datetime
from ROOT import TFile

alienprefix = "alien://"


def print_now():
    now = datetime.datetime.now()
    msg("- Current date and time:", str(now), color=bcolors.OKBLUE)


def listfiles(Path, What, MakeXML=False, MustHave="", SubDirs="", User=None, MainPath=""):
    """
    Lists the content of the path given in input.
    Puts the content to file if required.
    Can also form the output in the xml format so as to run on grid, this is done if the output filename has the xml extension.
    """
    verbose_msg("Listing files", What, "in path", Path)
    if User is None:
        User = getpass.getuser()
        msg("Getting user:", User)
    Path = path.normpath(Path)
    msg("Using path:", Path)
    PathToScan = path.join(MainPath, User[0], User, Path)
    if What == None:
        for i in run_cmd("alien_ls {}".format(PathToScan), check_status=False):
            print(i)
        return

    bashCommand = "alien_find "
    # Printing name of output list
    if MakeXML:
        bashCommand += " -x collection "
    bashCommand += "{} {} ".format(PathToScan, What)
    bashCommand = bashCommand.strip()
    verbose_msg("This is the list of found files:")
    list_of_found_files = run_cmd(
        bashCommand, print_output=False, check_status=False).split("\n")
    FilterList = []
    for i in list_of_found_files:
        if not MakeXML and What not in i:
            continue
        if MustHave and MustHave not in i:
            msg(f"Discarding line '{i}' as it doesn't have '{MustHave}'",
                color=bcolors.OKBLUE)
            continue
        if SubDirs:
            istrip = i.replace(PathToScan, "").strip().strip("/")
            verbose_msg(istrip)
            istrip = istrip.split("/")
            istrip = istrip[:-1]
            verbose_msg("here:", istrip, len(istrip))
            if len(istrip) != int(SubDirs):
                continue
        FilterList.append(i)
    msg(f"Found {len(FilterList)} files responding to all criteria")
    return FilterList


def writefiles(FileList, Outfile, append=False):
    """
    Writes the list of file to the output file given content of the path given in input.
    Can also form the output in the xml format so as to run on grid, this is done if the output filename has the xml extension.
    """
    # Printing name of output list
    msg(f"Output will be into file '{Outfile}'")
    # Check on existing list file of this name
    if path.isfile(Outfile) and not append:
        msg("List file already existing, replace it? (y/[n])")
        if "y" not in input():
            return
    fw = open(Outfile, "a" if append else "w")
    written = 0
    for i in FileList:
        fw.writelines(i.strip() + "\n")
        written += 1
    msg(f"Written {written} files to {Outfile}")
    fw.close()


def check_root_file(file_name):
    if not file_name.endswith(".root"):
        warning_msg("Testing a non root file:", file_name)
        return True
    if not path.isfile(file_name):
        warning_msg("Testing a non existing file:", file_name)
        return True
    try:
        f = TFile(file_name, "READ")
        if f.TestBit(TFile.kRecovered):
            msg("File", file_name, "was recovered", color=bcolors.WARNING)
            return False
        if not f.IsOpen():
            msg("File", file_name, "is not open", color=bcolors.WARNING)
            return False
    except OSError:
        msg("Issue when checking file", file_name, color=bcolors.WARNING)
        return False
    verbose_msg(file_name, "is ok and has size",
                os.path.getsize(file_name)*1e-6, "MB")
    return True


def copyfile(toget, Version=None, replace_preexisting=False, n_retry_root_files=4):
    """Copies a file from grid and puts it in the same path as the grid one.
    The version lets you choose between old and new alien. Versions==None means that it will autoset it"""
    toget = toget.strip()
    if Version == None:
        stream = os.popen("which aliensh")
        stream = stream.read()
        stream = stream.strip()
        print(stream)
        if "aliensh" in stream:
            Version = 0
        else:
            Version = 1

    try:
        if "" == toget:
            raise ValueError("Empty input")
        if "/" not in toget:
            raise ValueError("Input has no path")
        if "." not in toget or toget.rfind("/") > toget.rfind("."):
            raise ValueError("Input has no extension")
        if Version == 0:
            if alienprefix not in toget:
                toget = alienprefix + toget
        elif Version == 1:
            while toget[0] == ".":
                toget = toget[1:]
            while "//" in toget:
                toget = toget.replace("//", "/")
            if toget[0] != "/":
                raise ValueError(toget, "does not start with /")
        else:
            raise ValueError("Version is unknown", Version)
        tofile = path.basename(toget)
        todir = path.normpath("./" + path.dirname(toget.replace(alienprefix,
                                                                "")))
        out_file = path.join(todir,  tofile)
        verbose_msg(
            f"  --copyfile: Output dir. is '{todir}', file is '{tofile}'")

        if not path.isdir(todir):
            msg("Directory '{}' does not exist - creating it".format(todir))
            os.makedirs(todir)
        if path.isfile(out_file):
            if replace_preexisting:
                msg("File '{}' already copied, overwriting".format(out_file))
            else:
                msg("File '{}' already copied".format(out_file))
                return

        def proceed():
            msg(f"Downloading '{toget}'", color=bcolors.OKGREEN)
            print_now()
            if Version == 0:
                cpycmd = "alien_cp -v {} file:{}".format(toget, todir)
            else:
                cpycmd = "alien_cp -v {} file://{}".format(toget, todir)
            verbose_msg("Running command", cpycmd)
            run_cmd(cpycmd)

        for i in range(n_retry_root_files):
            proceed()
            if check_root_file(out_file):
                break

    except ValueError as err:
        msg(err.args, color=bcolors.BWARNING)
        msg("Input: " + toget, color=bcolors.BWARNING)


def copied(fname, extra_msg="", last_time=None, check_root_files=True):
    """Checks if how many files of a text list were correctly copied from grid to the PC"""
    verbose_msg("Checking how many files were copied from from list", fname)
    fname = fname.strip()
    f = open(fname, "r")
    n_to_copy = 0
    n_copied = 0
    not_sane = []
    for line in f:
        if "%" in line:
            break
        if "#" in line:
            continue
        line = path.normpath("./" + line.strip())
        n_to_copy += 1
        if path.isfile(line):
            n_copied += 1
            if check_root_files:
                if not check_root_file(line):
                    msg(f"'{line}' downloaded but with issues",
                        color=bcolors.WARNING)
                    not_sane.append(line)
        else:
            msg(f"'{line}' yet to download", color=bcolors.OKBLUE)
    if last_time is not None:
        n_copied -= last_time[1]
    msg(extra_msg, "downloaded {}/{}, {:.1f}%".format(n_copied,
        n_to_copy, 100 * float(n_copied) / float(n_to_copy)),
        f" -- copied {n_copied} files more, in total copied {last_time[1] + n_copied} files" if last_time is not None else "", f"{len(not_sane)}" if len(not_sane) > 0 else "")

    return n_to_copy, n_copied


def copylist(fname, jobs=1):
    """Takes a text file and downloads the files from grid"""
    verbose_msg("Copying files from list", fname, "with", jobs, "jobs")
    fname = path.normpath(fname)
    if not path.isfile(fname):
        warning_msg("Input file not provided! Aborting")
        return
    sofar = copied(fname, "So far")
    f = open(fname, "r")
    Group = []
    for line in f:
        if "%" in line:
            msg("Character % encountered! Aborting")
            break
        if "#" in line:
            msg("Character # encountered! Skipping")
            continue
        line = "./" + line
        if jobs == 1:
            copyfile(line)
        else:
            Group.append(line)
    if jobs > 1:
        msg("Copying list in parallel with", jobs, "jobs")
        run_in_parallel(processes=jobs, job_runner=copyfile,
                        job_arguments=Group, job_message="Downloading files")
    copied(fname, extra_msg="In recent run", last_time=sofar)


def main(input_files,
         do_list_files=False,
         do_write_files=None,
         do_copy=False,
         do_copied=False,
         do_copylist=False,
         what="AO2D.root",
         append=False,
         jobs=1):
    done_something = False
    if do_list_files:
        for i in input_files:
            list_of_files = listfiles(i, what, False)
            if len(list_of_files) > 0 and do_write_files:
                writefiles(list_of_files, do_write_files,
                           append=(i == list_of_files[0]) or append)
        done_something = True

    if do_copy or do_copylist:
        for i in input_files:
            if do_copylist:
                copylist(i, jobs=jobs)
            else:
                copyfile(i)
        done_something = True
    if do_copied:
        for i in input_files:
            print(copied(i))
        done_something = True

    if not done_something:
        warning_msg("Did not do anything")


if __name__ == "__main__":
    parser = get_default_parser(description=__doc__)
    parser.add_argument("input_files", type=str, nargs="+",
                        help="List of files in .txt file or files to download")
    parser.add_argument("--list", "-l", action="store_true",
                        help=listfiles.__doc__)
    parser.add_argument("--outfile", "-o", type=str, default=None,
                        help=writefiles.__doc__)
    parser.add_argument("--copy", "-c", action="store_true",
                        help=copyfile.__doc__)
    parser.add_argument("--copylist", "-C",
                        action="store_true", help=copylist.__doc__)
    parser.add_argument("--copied", "-K",
                        action="store_true", help=copied.__doc__)
    parser.add_argument("--append", "-a",
                        action="store_true", help="Apppend to file")
    parser.add_argument("--what", "-w",
                        type=str, default=None, help="Object that is looked for on alien")

    args = parser.parse_args()
    set_verbose_mode(args)
    main(args.input_files,
         do_list_files=args.list,
         do_write_files=args.outfile,
         do_copy=args.copy,
         do_copylist=args.copylist,
         what=args.what,
         jobs=args.njobs,
         append=args.append,
         do_copied=args.copied)

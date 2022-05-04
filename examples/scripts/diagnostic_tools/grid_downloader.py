#! /usr/bin/env python3

"""
Utility script to download files from grid
Author: Nicolò Jacazio, nicolo.jacazio@cern.ch
"""


import os
from os import path
from common import bcolors, msg, run_cmd, run_in_parallel, verbose_msg, get_default_parser, set_verbose_mode, warning_msg
import getpass
import datetime
import inspect
from ROOT import TFile

alienprefix = "alien://"


class InputArgument:
    default = ""
    helper = ""
    aliases = []
    thistype = str
    nargs = "?"

    def __init__(self, default, helper="", aliases=[], thistype=str, nargs="?"):
        self.default = default
        self.helper = helper
        self.aliases = aliases
        if type(self.aliases) is not list:
            self.aliases = [self.aliases]
        self.thistype = thistype
        self.nargs = nargs

    def print(self):
        print(f"default = {self.default}",
              f"helper = {self.helper}",
              f"aliases = {self.aliases}",
              f"thistype = {self.thistype}",
              f"nargs = {self.nargs}")


def print_now():
    now = datetime.datetime.now()
    msg("- Current date and time:", str(now), color=bcolors.OKBLUE)


def listfiles(Path=None,
              What=InputArgument("AO2D.root",
                                 "Name of the file to look for", "-w"),
              MakeXML=False,
              MustHave=InputArgument(None,
                                     "String that must be in good files path", [
                                         "-m"],
                                     nargs="+"),
              MustHaveCount=InputArgument(1,
                                          "How many times the MustHave string must be present",
                                          ["-nm"], thistype=int),
              MustNotHave=InputArgument(None,
                                        "String that must not be in good files path", ["-M"]),
              MustNotHaveCount=InputArgument(1,
                                             "How many times the MustHave string must be present",
                                             ["-NM"], thistype=int),
              SubDirs="",
              User=None,
              MainPath=""):
    """
    Lists the content of the path given in input.
    Puts the content to file if required.
    Can also form the output in the xml format so as to run on grid, this is done if the output filename has the xml extension.
    """
    verbose_msg("Listing files", What, "in path", Path)
    if Path is None or Path == "":
        raise ValueError("Passed empty path", Path)
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
        if MustHave is not None:
            hasit = True
            if type(MustHave) is not list:
                raise ValueError("Musthave is not a list!", MustHave)
            for e in MustHave:
                if e not in i:
                    hasit = False
                if i.count(e) < MustHaveCount:
                    hasit = False
            if not hasit:
                msg(f"Discarding line '{i}' as it doesn't have '{MustHave}' {MustHaveCount} times",
                    color=bcolors.OKBLUE)
                continue
        if MustNotHave and MustNotHave in i:
            if i.count(MustNotHave) >= MustNotHaveCount:
                msg(f"Discarding line '{i}' as it has '{MustNotHave}' {MustNotHaveCount} times",
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


def writefiles(FileList="", Outfile=InputArgument("listoffiles.txt",
                                                  "Output file", "-o"),
               append="Append to output file or create a new one"):
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


def copyfile(toget="Full path of the file to get",
             Version=None,
             replace_preexisting=False,
             n_retry_root_files=4):
    """Copies a file from grid and puts it in the same path as the grid one.
    The version lets you choose between old and new alien. Versions==None means that it will autoset it"""
    toget = toget.strip()
    if Version == None:
        stream = os.popen("which aliensh 2>/dev/null")
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
        out_file = path.join(todir, tofile)
        verbose_msg(
            f"  --copyfile: Output dir. is '{todir}', file is '{tofile}'")

        if not path.isdir(todir):
            msg("Directory '{}' does not exist - creating it".format(todir))
            os.makedirs(todir)
        if path.isfile(out_file) and check_root_file(out_file):
            if replace_preexisting:
                msg("File '{}' already copied, overwriting".format(out_file))
            else:
                msg("File '{}' already copied".format(out_file))
                return

        def proceed(handle_exit=True):
            msg(f"Downloading '{toget}'", color=bcolors.OKGREEN)
            print_now()
            if Version == 0:
                cpycmd = "alien_cp -v {} file:{}".format(toget, todir)
            else:
                cpycmd = "alien_cp -v {} file://{}".format(toget, todir)
            verbose_msg("Running command", cpycmd)
            if handle_exit:
                try:
                    run_cmd(cpycmd)
                except KeyboardInterrupt:
                    return False
            else:
                run_cmd(cpycmd)
                return True

        for i in range(n_retry_root_files):
            if not proceed():
                return
            if check_root_file(out_file):
                break

    except ValueError as err:
        msg(err.args, color=bcolors.BWARNING)
        msg("Input: " + toget, color=bcolors.BWARNING)


def copied(fname="", extra_msg="", last_time=None, check_root_files=True):
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
        f" -- copied {n_copied} files more, in total copied {last_time[1] + n_copied} files" if last_time is not None else "", f"{len(not_sane)} are not OK" if len(not_sane) > 0 else "")

    return n_to_copy, n_copied


def copylist(fname="",
             jobs=InputArgument(1, "Number of parallel jobs to use", ["--njobs", "-j"], int)):
    """Takes a text file and downloads the files from grid"""
    if jobs is None:
        jobs = 1
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
                        job_arguments=Group, job_message="Downloading files",
                        linearize_single_core=True)
    copied(fname, extra_msg="In recent run", last_time=sofar)


def merge_aod(in_path="", out_path="./", input_file="AO2D.root", must_have="ctf", bunch_size=50, skip_already_existing=True):
    in_path = os.path.normpath(in_path)
    out_path = os.path.normpath(out_path)
    file_list = []
    for root, dirs, files in os.walk(in_path):
        for file in files:
            if file == input_file:
                to_merge = os.path.abspath(os.path.join(root, file))
                print(to_merge)
                if must_have is not None and must_have in to_merge:
                    file_list.append(to_merge)
    verbose_msg("Found", len(file_list), "files called", input_file)
    # Divide it in bunches
    file_list = [file_list[i:i+bunch_size]
                 for i in range(0, len(file_list), bunch_size)]
    for i in enumerate(file_list):
        bunch_size = 0
        with open("inputfile.txt", "w") as f:
            for j in i[1]:
                f.write(f"{j}\n")
                bunch_size += os.path.getsize(j)
        out_aod = os.path.join(out_path, f"AO2D_{i[0]}.root")
        verbose_msg("Merging bunch of", len(i[1]),
                    "files. I.e.", bunch_size*1e-6, "MB")
        if skip_already_existing and os.path.isfile(out_aod):
            verbose_msg(out_aod, "already existing, skipping")
            continue
        tmp_aod = os.path.join(out_path, "MergedAOD.root")
        run_cmd(
            f"o2-aod-merger --input inputfile.txt --output {tmp_aod} --skip-non-existing-files",
            comment=f"Merging AODs into {out_aod}")
        os.rename(tmp_aod, out_aod)
        merged_size = os.path.getsize(out_aod)
        msg("Produced a merged file of",
            merged_size*1e-6, "MB from", bunch_size*1e-6, "MB, compression:", merged_size/bunch_size)


def main(input_files,
         args=None):
    if type(input_files) is not list:
        input_files = [input_files]
    if len(input_files) <= 0:
        warning_msg("Passed no input, use: --input_files")
        return
    if args.command == "listfiles":
        for i in input_files:
            list_of_files = []
            if os.path.isfile(i):
                paths_to_list = []
                with open(i) as fsecondary:
                    for j in fsecondary:
                        j = j.strip().strip(" ").strip(",")
                        if j == "":
                            continue
                        for k in j.split(","):
                            paths_to_list.append(k)
                for j in paths_to_list:
                    list_of_files += listfiles(Path=j,
                                               What=args.what,
                                               MustHave=args.musthave,
                                               MustHaveCount=args.musthavecount,
                                               MustNotHaveCount=args.mustnothavecount,
                                               MustNotHave=args.mustnothave)
            else:
                list_of_files = listfiles(Path=i,
                                          What=args.what,
                                          MustHave=args.musthave,
                                          MustHaveCount=args.musthavecount,
                                          MustNotHaveCount=args.mustnothavecount,
                                          MustNotHave=args.mustnothave)
            append = args.append
            do_write_files = args.outfile
            if len(list_of_files) > 0 and do_write_files:
                writefiles(list_of_files, do_write_files,
                           append=(i == list_of_files[0]) or append)
    elif args.command == "copyfile":
        for i in input_files:
            copyfile(i)
    elif args.command == "copylist":
        for i in input_files:
            copylist(i, jobs=args.jobs)
    elif args.command == "copied":
        for i in input_files:
            print(copied(i))
    elif args.command == "merge_aod":
        for i in input_files:
            merge_aod(i,
                      input_file=args.what)
    else:
        warning_msg("Did not do anything")


if __name__ == "__main__":
    parser = get_default_parser(description=__doc__, njobs=False)
    parser.add_argument("input_files", type=str,  # nargs="+",
                        help="List of files in .txt file or files to download")
    # parser.add_argument("--input_files", "--input", "-i", type=str,# nargs="+",
    #                     default=[],
    #                     help="List of files in .txt file or files to download")
    subparsers = parser.add_subparsers(dest='command', help='sub-commands')

    def add_subp(fn, g=None):
        if g is None:
            g = subparsers.add_parser(fn.__name__, help=fn.__doc__)
        a = inspect.getfullargspec(fn)
        for i, j in enumerate(a.args):
            d = a.defaults[i]
            # print(fn, i, j, d)
            if type(d) is str:
                if d == "":
                    continue
                # print("Add argument without defaults")
                g.add_argument(f"--{j.lower()}", help=a.defaults[i])
            elif type(d) is InputArgument:
                # print("Add argument", j, "with defaults")
                g.add_argument(f"--{j.lower()}",
                               *d.aliases, help=d.helper,
                               default=d.default,
                               type=d.thistype,
                               nargs=d.nargs)
        return g

    gl = add_subp(listfiles)
    add_subp(writefiles, gl)
    add_subp(copyfile)
    add_subp(copylist)
    add_subp(copied)
    add_subp(merge_aod)

    args = parser.parse_args()

    set_verbose_mode(args)
    main(args.input_files,
         args=args)

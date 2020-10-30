#!/usr/bin/env python3

import argparse
import subprocess
import os


def run_o2_analysis(o2_analyses=["o2-analysis-hf-task-d0 --pipeline qa-tracking-kine:4,qa-tracking-resolution:4"],
                    o2_arguments="--shm-segment-size 16000000000 --readers 4 --configuration json://$PWD/dpl-config_std.json --aod-file @listfiles.txt",
                    tag="QA",
                    verbose=False,
                    remove_tmp_script=False,
                    output_files=["AnalysisResults.root", "QAResults.root"]
                    ):
    log_file = f"log_{tag.lower()}.log"
    print("\nRunning the tasks with O2... (logfile: ", log_file, ")")

    def make_tag_file(fn, ext=".root"):
        return i.replace(ext, "") + tag + ".root"
    for i in output_files:  # removing old output
        for j in [i, make_tag_file(i)]:
            try:
                os.remove(j)
            except:
                if verbose:
                    print("Did not find", j)
    tmp_script_name = f"tmpscript{tag}.sh"
    with open(tmp_script_name, "w") as tmp_script:
        if verbose:
            print("Writing o2 instructions to", tmp_script_name)
        for i in o2_analyses:
            line = f"{i} {o2_arguments}"
            if len(o2_analyses) > 1 and i != o2_analyses[-1]:
                line = f"{line} | \\\n \t"
            tmp_script.write(line)
            if verbose:
                print(line)
        tmp_script.write("\n\n")
    cmd = f"bash {tmp_script_name}"
    if verbose:
        print(f"Running '{cmd}'")
    subprocess.run(cmd.split(),
                   stdout=open(log_file, 'w'),
                   stderr=subprocess.STDOUT)
    if remove_tmp_script:
        os.remove(tmp_script_name)
    output = ""
    for i in output_files:  # renaming output with tag
        try:
            os.rename(i, make_tag_file(i))
        except:
            if verbose:
                print("Did not find", i)
        else:
            output += " " + make_tag_file(i)
    print("Output files:", output.strip())


def main(mode):
    args = "-b --aod-file @listfiles.txt"
    if mode == 0:
        an = ["o2-analysis-trackqa",
              "o2-analysis-trackextension",
              "o2-analysis-alice3-trackselection"]
        tag = "TrackQA"
    elif mode == 1:
        an = ["o2-analysis-spectra-tof",
              "o2-analysis-trackextension",
              "o2-analysis-pid-tof --add-qa 1",
              "o2-analysis-alice3-trackselection"]
        tag = "TOF"
    run_o2_analysis(
        an,
        o2_arguments=args,
        tag=tag)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Runner for O2 analyses")
    parser.add_argument("modes",
                        type=int,
                        nargs="+",
                        help="Running modes")
    args = parser.parse_args()
    for i in args.modes:
        main(i)

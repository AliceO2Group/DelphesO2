#!/usr/bin/env python3

# from ROOT import gROOT
import wget
import os


def get_variable(f, v="TreeName"):
    variable = ""
    f = open(f)
    for i in f:
        if v in i:
            variable += " " + i.strip()
            continue
        if variable != "":
            variable += " " + i.strip()
        if ";" in variable:
            break
    return variable.replace(" ", "\n")


def count_trees(f):
    f = open(f)
    t = []
    for i in f:
        if "->Branch(" in i:
            i = i.strip().split("->")[0]
            if i not in t:
                t.append(i)
    return t


def check_tree(f1, f2, tn="Kinematics", verbose=True):
    print("Checking tree", tn, "in", f1, "and", f2)
    f1 = open(f1)
    f2 = open(f2)

    def get(f):
        t = []
        for i in f:
            i = i.strip()
            i = i.replace(",", ", ").replace("  ", " ")
            if f"{tn}->Branch(" in i:
                i = i.replace("aod_track", "tracks")
                t.append(i)
        return t

    t1 = get(f1)
    t2 = get(f2)
    r = t2
    for i in t1:
        if i in t2:
            r.pop(r.index(i))
    if len(r) > 0:
        print("Remaining", r)


def count_structs(f):
    nw = " "
    if "AliAnalysisTaskAO2Dconverter" in f:
        nw = "   "
    print("Checking", f)
    f = open(f)
    t = {}
    start = False
    dm = []
    for i in f:
        if "struct {" in i:
            start = True
            dm = []
            continue
        if start and "}" in i and not i.startswith(nw):
            if i not in t:
                t[i.replace("}", "").split(";")[0].strip().replace(
                    "aod_track", "tracks")] = dm
            start = False
        elif start:
            i = i.strip()
            while "  " in i:
                i = i.replace("  ", " ")
            if not i.startswith("//"):
                dm.append(i)
    return t


def get_and_check(url, to_check="createO2tables.h"):
    filename = wget.download(url, out="/tmp/", bar=None)
    filename_header = wget.download(
        url.replace(".cxx", ".h"), out="/tmp/", bar=None)
    # print(get_variable(to_check))
    t1 = count_trees(to_check)
    s1 = count_structs(to_check)
    # print(s1)
    # print(get_variable(filename))
    t2 = count_trees(filename)
    s2 = count_structs(filename_header)
    # print(s2)

    for i in s1:
        if i in s2:
            for j in s1[i]:
                if j not in s2[i]:
                    print(j, "Not here")
        else:
            print(i, "not in s2")
    for i in t1:
        if i in ["tRICH", "tMID", "tFTOF"]:
            continue
        check_tree(to_check, filename, tn=i)
        if i not in t2:
            print("Tree", i, "is not in", t2)

    os.remove(filename)
    os.remove(filename_header)


get_and_check(
    "https://raw.githubusercontent.com/alisw/AliPhysics/master/RUN3/AliAnalysisTaskAO2Dconverter.cxx")

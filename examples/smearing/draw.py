#!/usr/bin/env python3

"""
Script to plot the content of the LUT in terms of pointing resolution, efficiency, momentum resolution
"""

from ROOT import gROOT, TLatex, TCanvas, TLegend, TColor, gPad, TGraph
from ROOT import TFile
from os import path
import argparse


def main(reader_name,
         tags,
         lut_path,
         xmin,
         xmax,
         ymin=None,
         ymax=None,
         tags_name=None,
         logx=False,
         logy=False,
         leg_pos=[0.74, 0.2, 0.90, 0.4],
         particles=None,
         ind_var=0,
         dnch_deta=100,
         rmin=None,
         add_eta_label=True,
         add_alice3_label=True,
         save=None,
         background=False,
         use_p_over_z=True,
         aod=None,
         styles=None,
         study_label="ALICE 3 study"):
    gROOT.LoadMacro(reader_name)
    gROOT.LoadMacro("style.C")
    reader_name = reader_name.split(".")[-2]
    reader = getattr(__import__('ROOT', fromlist=[reader_name]),
                     reader_name)
    getattr(__import__('ROOT', fromlist=["style"]),
            "style")()
    p = {"el": "e", "pi": "#pi", "ka": "K", "pr": "p",
         "de": "d", "tr": "t", "he3": "^{3}He"}
    charge = {"el": 1, "pi": 1, "ka": 1, "pr": 1,
              "de": 1, "tr": 1, "he3": 2}
    p_colors = {"el": "#e41a1c", "pi": "#377eb8",
                "ka": "#4daf4a", "pr": "#984ea3",
                "de": "#ff7f00", "tr": "#999999",
                "he3": "#a65628"}

    if particles is not None:
        to_remove = []
        for i in p:
            if i not in particles:
                to_remove.append(i)
        for i in to_remove:
            p.pop(i)

    latex = TLatex()
    latex.SetTextAlign(33)
    canvas = reader_name
    canvas = canvas.replace("lutRead_", "")
    canvas = TCanvas(canvas, canvas, 800, 800)
    canvas.Divide(2, 2)
    drawn = [canvas]
    drawn_graphs = {}
    drawn_frames = {}

    def set_limit(l, v=0):
        if l is None:
            return v
        return l
    if "_dca" in reader_name:
        ymin = set_limit(ymin, 0.1)
        ymax = set_limit(ymax, 1e4)
    elif "_pt" in reader_name:
        ymin = set_limit(ymin, 1)
        ymax = set_limit(ymax, 100)
    elif "_eff" in reader_name:
        ymin = set_limit(ymin, 0)
        ymax = set_limit(ymax, 115)

    def adjust_pad():
        if logx:
            gPad.SetLogx()
        if logy:
            gPad.SetLogy()

    counter = 1
    leg = None
    if tags_name is not None:
        leg = TLegend(*leg_pos)
        if add_eta_label:
            label = f"#eta = {int(ind_var)}"
            if "vs_eta" in reader_name:
                label = "#it{p}_{T} "f"= {int(ind_var)} ""GeV/#it{c}"
            label += " dN_{Ch}/d#eta ="
            label += f" {int(dnch_deta)}"
            if rmin is not None:
                label += "   R_{min} = " + rmin
            else:
                leg.SetHeader()
            leg.SetHeader(label)
        leg.SetLineColor(0)
        drawn.append(leg)

    def draw_study_label(x=0.5, y=0.9):
        latex = TLatex()
        latex.SetTextAlign(13)
        drawn.append(latex.DrawLatexNDC(x, y, " ".join(study_label)))
    for i in p:  # Drawing one canvas per particle species
        c = f"{canvas.GetName()}_{i}"
        c = TCanvas(c, c, 800, 800)
        drawn.append(c)
        adjust_pad()

        frame = c.DrawFrame(xmin, ymin,
                            xmax, ymax, "")
        frame.SetDirectory(0)
        if leg is not None:
            leg.Draw()
        drawn_frames[i] = frame
        g_list = []
        extra = {}
        cols = ['#e41a1c', '#377eb8', '#4daf4a',
                '#984ea3', '#ff7f00', '#ffff33']
        for k, j in enumerate(tags):
            lut = f"{lut_path}/lutCovm.{i}.{j}.dat"
            if j == "":
                lut = f"{lut_path}/lutCovm.{i}.dat"
            if not path.isfile(lut):
                print("LUT file", lut, "does not exist")
                return
            g = reader(lut, ind_var, dnch_deta)
            if g.GetN() <= 0:
                print("Skipping", g.GetName(), "because empty graph")
                continue
            if len(g_list) == 0:
                frame.GetXaxis().SetTitle(g.GetXaxis().GetTitle())
                frame.GetYaxis().SetTitle(g.GetYaxis().GetTitle())
            if use_p_over_z:
                for j in range(g.GetN()):
                    if "_pt" in reader_name:
                        g.SetPoint(j,
                                   g.GetPointX(j)/charge[i],
                                   g.GetPointY(j)/charge[i])
                    else:
                        g.SetPoint(j,
                                   g.GetPointX(j)/charge[i], g.GetPointY(j))
                frame.GetXaxis().SetTitle("#it{p}_{T}/z (GeV/#it{c})")
            col = TColor.GetColor(cols[len(g_list)])
            g.SetLineColor(col)
            g.SetLineStyle(1)
            g.SetLineWidth(3)
            g.Draw("samel")
            if aod is not None:
                f_aod = TFile(aod, "READ")
                if "_eff" in reader_name:
                    extra[g.GetName()] = f_aod.Get(
                        "qa-tracking-efficiency-kaon/pt/num")
                    extra[g.GetName()].Divide(f_aod.Get("qa-tracking-efficiency-kaon/pt/num"),
                                              f_aod.Get("qa-tracking-efficiency-kaon/pt/den"), 1, 1, "B")
                    extra[g.GetName()].Scale(100)
                    extra[g.GetName()].Draw("SAME")
                    extra[g.GetName()].SetDirectory(0)
                elif "_pt" in reader_name:
                    extra[g.GetName()] = f_aod.Get(
                        "qa-tracking-efficiency-kaon/pt/num")
                    extra[g.GetName()].Divide(f_aod.Get("qa-tracking-efficiency-kaon/pt/num"),
                                              f_aod.Get("qa-tracking-efficiency-kaon/pt/den"), 1, 1, "B")
                    extra[g.GetName()].Scale(100)
                    extra[g.GetName()].Draw("SAME")
                    extra[g.GetName()].SetDirectory(0)
                f_aod.Close()

            print("Drawing", g.GetName())
            if tags_name is not None and counter == 1:
                leg.AddEntry(g, tags_name[k], "l")
            g_list.append(g)
        drawn_graphs[i] = g_list
        if len(g_list) <= 0:
            print("Nothing drawn!")
            continue
        drawn.append(latex.DrawLatexNDC(0.9, 0.9, p[i]))
        draw_study_label(.4, .91)
        gPad.Update()
        canvas.cd(counter)
        clone = c.DrawClonePad()
        if counter != 1:
            l = gPad.GetListOfPrimitives()
            for i in l:
                cn = i.ClassName()
                if cn == "TLegend":
                    l.Remove(i)
                elif cn == "TLatex":
                    if "ALICE" in i.GetTitle():
                        l.Remove(i)
        drawn.append(clone)
        c.SaveAs(f"/tmp/{c.GetName()}.png")
        gPad.Update()
        counter += 1
    canvas_all_species = None
    if len(tags) == 1 or styles is not None:
        canvas_all_species = "all_spec_"+canvas.GetName()
        canvas_all_species = TCanvas(canvas_all_species,
                                     canvas_all_species,
                                     800, 800)
        drawn.append(canvas_all_species)
        canvas_all_species.cd()
        drawn_graphs_all_spec = {}
        leg_all_spec = TLegend(*leg_pos)
        leg_all_spec.SetNColumns(2)
        leg_all_spec.SetLineColor(0)
        drawn.append(leg_all_spec)
        for i in drawn_graphs:
            if canvas_all_species.GetListOfPrimitives().GetEntries() == 0:
                drawn_frames[i].Draw()
                leg_all_spec.Draw()
            drawn_graphs_all_spec[i] = []
            for k, g in enumerate(drawn_graphs[i]):
                g = g.Clone()
                drawn_graphs_all_spec[i].append(g)
                g.SetName(g.GetName()+"_color")
                g.SetLineColor(TColor.GetColor(p_colors[i]))
                if styles is not None:
                    g.SetLineStyle(styles[k])
                g.Draw("same")
                if k == 0:
                    leg_all_spec.AddEntry(g, p[i], "L")
        if styles is not None:
            for j, k in enumerate(tags_name):
                g = TGraph()
                g.SetLineWidth(3)
                g.SetLineColor(1)
                g.SetLineStyle(styles[j])
                leg_all_spec.AddEntry(g, k, "L")
                drawn_graphs_all_spec[i].append(g)
        for i in drawn_graphs_all_spec:
            drawn_graphs[i+"_allspec"] = drawn_graphs_all_spec[i]
        if add_alice3_label:
            draw_study_label(.2, .91)
            latex = TLatex()
            latex.SetTextAlign(13)
            latex.SetTextSize(0.04)
            if tags_name is not None and styles is None:
                drawn.append(latex.DrawLatexNDC(0.5, 0.80, tags_name[0]))
            if "vs_eta" in reader_name:
                drawn.append(latex.DrawLatexNDC(0.42, 0.82, "#splitline{" +
                                                "#it{p}_{T}"
                                                + " = {:.1f} GeV/c".format(ind_var)
                                                + " dN_{Ch}/d#eta ="
                                                + f" {int(dnch_deta)}" + "}"
                                                + ("{R_{min} = " + rmin + "}" if rmin is not None else "")))
            else:
                # drawn.append(latex.DrawLatexNDC(0.55, 0.82, "#splitline{" +
                drawn.append(latex.DrawLatexNDC(0.55, 0.45, "#splitline{" +
                                                f"#eta = {int(ind_var)}"
                                                + " dN_{Ch}/d#eta ="
                                                + f" {int(dnch_deta)}" + "}"
                                                + ("{R_{min} = " + rmin + "}" if rmin is not None else "")))

        adjust_pad()
        canvas_all_species.Update()
        canvas_all_species.SaveAs(f"/tmp/{canvas_all_species.GetName()}.png")
        canvas_all_species.SaveAs(f"/tmp/{canvas_all_species.GetName()}.pdf")
        canvas.SaveAs(f"/tmp/lut_{canvas.GetName()}.root")
    if save is not None:
        print("Saving to", save)
        fo = TFile(save, "RECREATE")
        fo.cd()
        for i in drawn_graphs:
            for j in drawn_graphs[i]:
                j.Write(j.GetName().split("/")[-1])
    if not background:
        input("Done, press enter to continue")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reader", type=str,
                        help="Reader macro to access the information e.g. lutRead_eff.C")
    parser.add_argument("--path", "-p",
                        type=str,
                        default="/tmp/myluts/",
                        help="Path of the LUTs")
    parser.add_argument("--particles", "-P",
                        type=str,
                        nargs="+",
                        default=None,
                        help="Particles to show e.g. el pi ka mu pr")
    parser.add_argument("--xmin",
                        type=float,
                        default=1e-2,
                        help="Minimum pT of the plot")
    parser.add_argument("--xmax",
                        type=float,
                        default=100.,
                        help="Maximum pT of the plot")
    parser.add_argument("--tags", "-t", type=str, nargs="+",
                        default=[".5kG.20cm.its3", ".5kG.50cm.its3",
                                 ".5kG.100cm.its3", ".5kG.20cm.scenario3"],
                        help="Tags to collect")
    parser.add_argument("--tags_name", "-T",
                        type=str, nargs="+",
                        default=None,
                        help="Title of the tags that can be used in legend making")
    parser.add_argument("--ind_var", "--ind", "-i",
                        type=float,
                        default=0,
                        help="Value of the indepentend variable, i.e. eta if plotting vs pT or pT if plotting against eta")
    parser.add_argument("--styles",
                        type=int,
                        default=None,
                        nargs="+",
                        help="Plotting style of different analyses")
    parser.add_argument("--nch", "--dndeta",
                        type=float,
                        default=100,
                        help="Value of the charged particle multiplicity")
    parser.add_argument("--ymin",
                        type=float,
                        default=None,
                        help="Minimum y")
    parser.add_argument("--ymax",
                        type=float,
                        default=None,
                        help="Maximum y")
    parser.add_argument("--rmin",
                        type=str,
                        default=None,
                        help="Label for the minimum radius")
    parser.add_argument("--aod",
                        type=str,
                        default=None,
                        help="Results from aod to show")
    parser.add_argument("--save",
                        type=str,
                        default=None,
                        help="Name to save the figure to")
    parser.add_argument("--leg-pos", "-l",
                        type=float, nargs="+",
                        default=[0.74, 0.2, 0.90, 0.4],
                        help="Position of the legend in NDC coordinates")
    parser.add_argument("--study_label", "--label", "-L",
                        type=str, nargs="+",
                        default=["ALICE 3 study"],
                        help="Label to write into the label box")
    parser.add_argument("--logx", action="store_true",
                        help="Log x")
    parser.add_argument("--logy", action="store_true",
                        help="Log y")
    parser.add_argument("--pt_over_z", action="store_true",
                        help="Plot pt over z")
    parser.add_argument("-b", action="store_true",
                        help="Background mode")
    args = parser.parse_args()
    main(args.reader,
         args.tags,
         lut_path=args.path,
         xmin=args.xmin,
         xmax=args.xmax,
         logx=args.logx,
         logy=args.logy,
         tags_name=args.tags_name,
         particles=args.particles,
         leg_pos=args.leg_pos,
         ymin=args.ymin,
         ymax=args.ymax,
         rmin=args.rmin,
         ind_var=args.ind_var,
         save=args.save,
         background=args.b,
         aod=args.aod,
         dnch_deta=args.nch,
         use_p_over_z=args.pt_over_z,
         study_label=args.study_label,
         styles=args.styles)

#!/usr/bin/env python3

"""
Script to plot the content of the LUT in terms of pointing resolution, efficiency, momentum resolution
"""

from ROOT import gROOT, TLatex, TCanvas, TLegend, TColor, gPad
from ROOT import TFile
from os import path
import argparse


def main(reader_name,
         tags,
         lut_path,
         ptmin,
         ptmax,
         ymin=None,
         ymax=None,
         tag_name=None,
         logx=False,
         logy=False,
         leg_pos=[0.74, 0.2, 0.90, 0.4],
         particles=None,
         eta=0,
         dnch_deta=100,
         rmin=None,
         add_eta_label=True,
         add_alice3_label=True,
         save=None,
         background=False,
         use_p_over_z=True,
         aod=None,
         study_label="ALICE 3 study"):
    gROOT.LoadMacro(reader_name)
    gROOT.LoadMacro("style.C")
    reader_name = reader_name.split(".")[-2]
    reader = getattr(__import__('ROOT', fromlist=[reader_name]),
                     reader_name)
    style = getattr(__import__('ROOT', fromlist=["style"]),
                    "style")

    style()

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
    if ymin is None:
        if "_dca" in reader_name:
            ymin = 0.1
        elif "_pt" in reader_name:
            ymin = 1.
        elif "_eff" in reader_name:
            ymin = 0.
    if ymax is None:
        if "_dca" in reader_name:
            ymax = 1e4
        elif "_pt" in reader_name:
            ymax = 100.
        elif "_eff" in reader_name:
            ymax = 115.

    def adjust_pad():
        if logx:
            gPad.SetLogx()
        if logy:
            gPad.SetLogy()

    counter = 1
    leg = None
    if tag_name is not None:
        leg = TLegend(*leg_pos)
        if add_eta_label:
            label = f"#eta = {int(eta)}"
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
    for i in p:
        c = f"{canvas.GetName()}_{i}"
        c = TCanvas(c, c, 800, 800)
        drawn.append(c)
        adjust_pad()

        frame = c.DrawFrame(ptmin, ymin,
                            ptmax, ymax, "")
        frame.SetDirectory(0)
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
            g = reader(lut, eta, dnch_deta)
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
            if tag_name is not None and counter == 1:
                leg.AddEntry(g, tag_name[k], "l")
            g_list.append(g)
        drawn_graphs[i] = g_list
        if len(g_list) <= 0:
            print("Nothing drawn!")
            continue
        drawn.append(latex.DrawLatexNDC(0.9, 0.9, p[i]))
        if leg is not None:
            leg.Draw()
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
    if len(tags) == 1:
        canvas_all_species = TCanvas("all_spec_"+canvas.GetName(),
                                     "all_spec_"+canvas.GetName(), 800, 800)
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
            g_list = []
            for j in drawn_graphs[i]:
                g_list.append(j.Clone())
                g_list[-1].SetName(g_list[-1].GetName()+"_color")
                g_list[-1].SetLineColor(TColor.GetColor(p_colors[i]))
                g_list[-1].Draw("same")
                leg_all_spec.AddEntry(g_list[-1], p[i], "L")
            drawn_graphs_all_spec[i] = g_list
        for i in drawn_graphs_all_spec:
            drawn_graphs[i+"_allspec"] = drawn_graphs_all_spec[i]
        leg_all_spec.Draw()
        if add_alice3_label:
            draw_study_label()
            latex = TLatex()
            latex.SetTextAlign(13)
            latex.SetTextSize(0.04)
            if tag_name is not None:
                drawn.append(latex.DrawLatexNDC(0.5, 0.80, tag_name[0]))
            drawn.append(latex.DrawLatexNDC(0.4, 0.82, f"#eta = {int(eta)}"
                                            + "\n dN_{Ch}/d#eta ="
                                            + f" {int(dnch_deta)}"
                                            + ("\n R_{min} = " + rmin if rmin is not None else "")))

        adjust_pad()
        canvas_all_species.Update()
        canvas_all_species.SaveAs(f"/tmp/{canvas_all_species.GetName()}.png")
    if save is None:
        canvas.SaveAs(f"/tmp/lut_{canvas.GetName()}.root")
    else:
        fo = TFile(save, "RECREATE")
        fo.cd()
        canvas.Write()
        if canvas_all_species is not None:
            canvas_all_species.Write()
        for i in drawn_graphs:
            for j in drawn_graphs[i]:
                j.Write()
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
    parser.add_argument("--ptmin",
                        type=float,
                        default=1e-2,
                        help="Minimum pT of the plot")
    parser.add_argument("--ptmax",
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
    parser.add_argument("--eta",
                        type=float,
                        default=0,
                        help="Eta position")
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
    parser.add_argument("--leg_pos", "-l",
                        type=float, nargs="+",
                        default=[0.74, 0.2, 0.90, 0.4],
                        help="Position of the legend in NDC coordinates")
    parser.add_argument("--label", "-L",
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
         ptmin=args.ptmin,
         ptmax=args.ptmax,
         logx=args.logx,
         logy=args.logy,
         tag_name=args.tags_name,
         particles=args.particles,
         leg_pos=args.leg_pos,
         ymin=args.ymin,
         ymax=args.ymax,
         rmin=args.rmin,
         eta=args.eta,
         save=args.save,
         background=args.b,
         aod=args.aod,
         dnch_deta=args.nch,
         use_p_over_z=args.pt_over_z,
         study_label=args.label)

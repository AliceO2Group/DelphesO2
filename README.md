# DelphesO2
This is a small collection of classes to interface Delphes with the AliceO2.
It is currently meant to provide a way to smear Delphes tracks using the full covariance matrices stored in look-up tables (LUT).
It also provides an interface between Delphes tracks and the o2 secondary vertex reconstruction objects.

The code and examples are work-in-progress and will evolve with time according to needs and ideas.
In the following there is a list of practical examples to get started with analysis.

The workflow is in general the following
* run Delphes
* run your analysis 

## Build and enviroment

The build of DelphesO2 is supported via the `aliBuild` framework.
The receipt is on the other hand not in the official `alidist` repository yet.
You must grap the `delpheso2.sh` file from `https://github.com/preghenella/alidist/blob/delpheso2/delpheso2.sh` and put it in your own `alidist` directory.

Afterwards you should be able to build with
```
# aliBuild build DelphesO2 --defaults o2
```
and load the envoriment with
```
# alienv enter DelphesO2/latest
```
Please adjust the loading command according to your actual case.


## Running Delphes

Delphes is the entry point for the following track smearing, vertexing and data analysis.
The use of Delphes at this stage is to
* provide a track model, namely track objects to be used for analysis
* propagate tracks to outer layers (i.e. TOF)
* propagate tracks to the DCA to primary vertex

In future Delphes will be used also to apply tracking efficiencies and possible other features.

To run Delphes you need a Delphes `card`, which is a file containing the details of what Delphes has to do.
we will simply stick to a simple Delphes `card` that only deals with
* track propagation in magnetic field
* selection of charged hadrons, electron and muons
* writing on tree of all generated particles and selected tracks

One example of that card is `examples/cards/propagate.2kG.tcl`.
This card will 
* propagate tracks in a Bz = 0.2 T field
* merge charged hadrons, electrons and muons in output tracks
* write all generated particles and the above tracks on a tree

Delphes can run in several modes, either taking a HepMC file as input or running Pythia8 and get the input on-the-fly.
We will give as example the case where Pythia8 is used on-the-fly.
This needs a Pythia8 configuration file as well.
An example configuration file is `examples/pythia8/pythia8_inel.cfg`.
With this configuration you will generate 10k events, pp at 14 TeV, inelastic collisions.
Decays in Pythia8 are set to their defaults.
For instance this means that K0s and Lambda0 will decay, whereas pions will not.
It is important to know which particles decay in the input event generator, because this determines which tracks that will appear in the Delphes.

Eventually, to run Delphes with Pythia8 as event generator you do like this 
```
DelphesPythia8 propagate.2kG.tcl pythia8_inel.cfg delphes.root
```
where `delphes.root` is the Delphes output file. Please notice that if this file already exists the command will fail and you will need to remove the file to continue.

Notice that the Delphes output file might become big with a large number of events.
We will try to find a solution for that, if possible.
A way out is to start multiple Delphes-analysis runs and remove the Delphes output when the analysis is completed before starting another run. Care must be taken with the Pythia8 random seed to avoid to have identical runs. An example of doing that is given below.

## Track smearing

Discussing the details of track smearing is a long business.
We will only give the directions to try it out.
This is done in a Delphes analysis.

An example analysis that makes use of smearing is shown in `examples/smearing/dca.C`.
The smearing class makes use of look-up tables (LUT) which can be found in `examples/smearing/luts`.
Please adapt the `dca.C` macro to pick the correct files or put the files in the currect directory.

The analysis loops over the Delphes tracks and smears them.
It selects only pions and fills histograms of their DCAxy (D0) distribution, split according to their origin (primary, secondary, ...).

This is just a ROOT analysis and you can run it with
```
root -b -q -l "dca.C(\"delphes.root\", \"dca.root\")"
```

## Secondary vertices

An example analysis that makes use of both smearing and secondary-vertex reconstruction is shown in `examples/vertexing/K0s.C`. 

The analysis loops over the Delphes tracks and smears them.
It then selectes pions assuming perfect PID and cutting on the recontstructed DCA to reject primary particles. The cut is defined in terms of a 3-sigma rejection cut. The tracks that pass the cuts are then separated by their charge.

For each event, a double loop over positive-negative pion pairs combines them in the vertex fitter. For the cases where the vertex fitter is successfull the invariant mass is reconstructed as well as other topological variables.

## Multiple Delphes-analysis runs

To overcome the limitation that the large Delphes output might give on the statistical sample, one can run multiple times the same Delphes-analysis job, taking care of changing the Pythia8 random seed.

An example of a script sending multiple jobs one after another and properly taking care of the Pythia8 seed can be found in `examples/scripts/dca.sh`. Please, get inspiration from that and adapt it to your needs.

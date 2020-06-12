set ExecutionPath {
    ParticlePropagator
    Merger
    TreeWriter
}

# module Module Name
module ParticlePropagator ParticlePropagator {
    set InputArray Delphes/stableParticles
    set OutputArray stableParticles
    set ChargedHadronOutputArray chargedHadrons
    set ElectronOutputArray electrons
    set MuonOutputArray muons

    set Radius 100.e-2
    set HalfLength 200.e-2
    set Bz 0.2
}

module Merger Merger {
    add InputArray ParticlePropagator/chargedHadrons
    add InputArray ParticlePropagator/electrons
    add InputArray ParticlePropagator/muons
    set OutputArray tracks
}

module TreeWriter TreeWriter {
    # add Branch InputArray BranchName BranchClass
    add Branch Delphes/allParticles Particle GenParticle
    add Branch Merger/tracks Track Track
}


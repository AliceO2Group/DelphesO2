set barrel_Bz 0.2
set barrel_Radius 100.e-2
set barrel_HalfLength 200.e-2
set barrel_TimeResolution 0.020e-9
set barrel_Acceptance { 1.0 + 1.0 * fabs(eta) < 1.443 }

set ExecutionPath {

    ParticlePropagator

    Merger
    Acceptance
    DecayFilter
    TimeSmearing

    Acceptance_neutral

    TreeWriter
}

# module Module Name
module ParticlePropagator ParticlePropagator {
    set InputArray Delphes/stableParticles
    set OutputArray stableParticles
    set ChargedHadronOutputArray chargedHadrons
    set ElectronOutputArray electrons
    set MuonOutputArray muons
    set NeutralOutputArray neutral

    set Bz $barrel_Bz
    set Radius $barrel_Radius
    set HalfLength $barrel_HalfLength
}

module Merger Merger {
    add InputArray ParticlePropagator/chargedHadrons
    add InputArray ParticlePropagator/electrons
    add InputArray ParticlePropagator/muons
    set OutputArray tracks
}

module Efficiency Acceptance {
    add InputArray Merger/tracks
    add OutputArray tracks
    set EfficiencyFormula $barrel_Acceptance
}

module Efficiency Acceptance_neutral {
    add InputArray ParticlePropagator/neutral
    add OutputArray neutral
    set EfficiencyFormula $barrel_Acceptance
}

module DecayFilter DecayFilter {
    set InputArray Acceptance/tracks
    set OutputArray tracks
}

module TimeSmearing TimeSmearing {
    add InputArray DecayFilter/tracks
    add OutputArray tracks
    set TimeResolution $barrel_TimeResolution
}

module TreeWriter TreeWriter {
    # add Branch InputArray BranchName BranchClass
    add Branch Delphes/allParticles Particle GenParticle
    add Branch TimeSmearing/tracks Track Track
    add Branch Acceptance_neutral/neutral Neutral Track
}


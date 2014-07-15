
import FWCore.ParameterSet.Config as cms

from RecoVertex.NuclearInteractionProducer.NuclearInteraction_cff import *
import RecoTracker.NuclearSeedGenerator.NuclearSeed_cfi
firstnuclearSeed = RecoTracker.NuclearSeedGenerator.NuclearSeed_cfi.nuclearSeed.clone()
import RecoTracker.CkfPattern.CkfTrackCandidates_cfi
firstnuclearTrackCandidates = RecoTracker.CkfPattern.CkfTrackCandidates_cfi.ckfTrackCandidates.clone()
import RecoTracker.TrackProducer.CTFFinalFitWithMaterial_cfi
firstnuclearWithMaterialTracks = RecoTracker.TrackProducer.CTFFinalFitWithMaterial_cfi.ctfWithMaterialTracks.clone()
import RecoVertex.NuclearInteractionProducer.NuclearInteraction_cfi
firstnuclearInteractionMaker = RecoVertex.NuclearInteractionProducer.NuclearInteraction_cfi.nuclearInteractionMaker.clone()
nuclear = cms.Sequence(firstnuclearSeed*firstnuclearTrackCandidates*firstnuclearWithMaterialTracks*firstnuclearInteractionMaker)
firstnuclearSeed.producer = 'generalTracks'
firstnuclearTrackCandidates.src = 'firstnuclearSeed'
firstnuclearTrackCandidates.TrajectoryBuilderPSet.refToPSet_ = 'nuclearCkfTrajectoryBuilder'
firstnuclearTrackCandidates.RedundantSeedCleaner = 'none'
firstnuclearWithMaterialTracks.src = 'firstnuclearTrackCandidates'
firstnuclearInteractionMaker.primaryProducer = 'generalTracks'
firstnuclearInteractionMaker.seedsProducer = 'firstnuclearSeed'
firstnuclearInteractionMaker.secondaryProducer = 'firstnuclearWithMaterialTracks'

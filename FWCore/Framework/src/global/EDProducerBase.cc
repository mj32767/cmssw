// -*- C++ -*-
//
// Package:     FWCore/Framework
// Class  :     global::EDProducerBase
// 
// Implementation:
//     [Notes on implementation]
//
// Original Author:  Chris Jones
//         Created:  Thu, 02 May 2013 21:56:04 GMT
//

// system include files

// user include files
#include "FWCore/Framework/interface/global/EDProducerBase.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/LuminosityBlock.h"
#include "FWCore/Framework/interface/Run.h"
#include "FWCore/Framework/src/edmodule_mightGet_config.h"
#include "FWCore/Framework/src/PreallocationConfiguration.h"
#include "FWCore/Framework/src/EventAcquireSignalsSentry.h"
#include "FWCore/Framework/src/EventSignalsSentry.h"

#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"


//
// constants, enums and typedefs
//
namespace edm {

  class WaitingTaskWithArenaHolder;

  namespace global {
    //
    // static data member definitions
    //
    
    //
    // constructors and destructor
    //
    EDProducerBase::EDProducerBase():
    ProducerBase(),
    moduleDescription_(),
    previousParentages_(),
    gotBranchIDsFromAcquire_(),
    previousParentageIds_() { }
    
    EDProducerBase::~EDProducerBase()
    {
    }
    
    bool
    EDProducerBase::doEvent(EventPrincipal const& ep, EventSetup const& c,
                            ActivityRegistry* act,
                            ModuleCallingContext const* mcc) {
      Event e(ep, moduleDescription_, mcc);
      e.setConsumer(this);
      const auto streamIndex = e.streamID().value();
      e.setProducer(this,
                    &previousParentages_[streamIndex],
                    hasAcquire() ? &gotBranchIDsFromAcquire_[streamIndex] : nullptr);
      EventSignalsSentry sentry(act,mcc);
      this->produce(e.streamID(), e, c);
      commit_(e, &previousParentageIds_[streamIndex]);
      return true;
    }

    void
    EDProducerBase::doAcquire(EventPrincipal const& ep, EventSetup const& c,
                              ActivityRegistry* act,
                              ModuleCallingContext const* mcc,
                              WaitingTaskWithArenaHolder& holder) {
      Event e(ep, moduleDescription_, mcc);
      e.setConsumer(this);
      const auto streamIndex = e.streamID().value();
      e.setProducerForAcquire(this,
                              nullptr,
                              gotBranchIDsFromAcquire_[streamIndex]);
      EventAcquireSignalsSentry sentry(act,mcc);
      this->doAcquire_(e.streamID(), e, c, holder);
    }

    void
    EDProducerBase::doPreallocate(PreallocationConfiguration const& iPrealloc) {
      auto const nStreams = iPrealloc.numberOfStreams();
      previousParentages_.reset(new std::vector<BranchID>[nStreams]);
      if (hasAcquire()) {
        gotBranchIDsFromAcquire_.reset(new std::vector<BranchID>[nStreams]);
      }
      previousParentageIds_.reset( new ParentageID[nStreams]);
      preallocStreams(nStreams);
      preallocate(iPrealloc);
    }
    
    void
    EDProducerBase::doBeginJob() {
      this->beginJob();
    }
    
    void
    EDProducerBase::doEndJob() {
      this->endJob();
    }
    
    void
    EDProducerBase::doBeginRun(RunPrincipal const& rp, EventSetup const& c,
                               ModuleCallingContext const* mcc) {
      Run r(rp, moduleDescription_, mcc);
      r.setConsumer(this);
      Run const& cnstR = r;
      this->doBeginRun_(cnstR, c);
      this->doBeginRunSummary_(cnstR, c);
      r.setProducer(this);
      this->doBeginRunProduce_(r,c);
      commit_(r);
    }
    
    void
    EDProducerBase::doEndRun(RunPrincipal const& rp, EventSetup const& c,
                             ModuleCallingContext const* mcc) {
      Run r(rp, moduleDescription_, mcc);
      r.setConsumer(this);
      r.setProducer(this);
      Run const& cnstR = r;
      this->doEndRunProduce_(r, c);
      this->doEndRunSummary_(r,c);
      this->doEndRun_(cnstR, c);
      commit_(r);
    }
    
    void
    EDProducerBase::doBeginLuminosityBlock(LuminosityBlockPrincipal const& lbp, EventSetup const& c,
                                           ModuleCallingContext const* mcc) {
      LuminosityBlock lb(lbp, moduleDescription_, mcc);
      lb.setConsumer(this);
      LuminosityBlock const& cnstLb = lb;
      this->doBeginLuminosityBlock_(cnstLb, c);
      this->doBeginLuminosityBlockSummary_(cnstLb, c);
      lb.setProducer(this);
      this->doBeginLuminosityBlockProduce_(lb, c);
      commit_(lb);
    }
    
    void
    EDProducerBase::doEndLuminosityBlock(LuminosityBlockPrincipal const& lbp, EventSetup const& c,
                                         ModuleCallingContext const* mcc) {
      LuminosityBlock lb(lbp, moduleDescription_, mcc);
      lb.setConsumer(this);
      lb.setProducer(this);
      LuminosityBlock const& cnstLb = lb;
      this->doEndLuminosityBlockProduce_(lb, c);
      this->doEndLuminosityBlockSummary_(cnstLb,c);
      this->doEndLuminosityBlock_(cnstLb, c);
      commit_(lb);
    }
    
    void
    EDProducerBase::doBeginStream(StreamID id) {
      doBeginStream_(id);
    }
    void
    EDProducerBase::doEndStream(StreamID id) {
      doEndStream_(id);
    }
    void
    EDProducerBase::doStreamBeginRun(StreamID id,
                                     RunPrincipal const& rp,
                                     EventSetup const& c,
                                     ModuleCallingContext const* mcc)
    {
      Run r(rp, moduleDescription_, mcc);
      r.setConsumer(this);
      this->doStreamBeginRun_(id, r, c);
    }
    void
    EDProducerBase::doStreamEndRun(StreamID id,
                                   RunPrincipal const& rp,
                                   EventSetup const& c,
                                   ModuleCallingContext const* mcc) {
      Run r(rp, moduleDescription_, mcc);
      r.setConsumer(this);
      this->doStreamEndRun_(id, r, c);
      this->doStreamEndRunSummary_(id, r, c);
    }
    void
    EDProducerBase::doStreamBeginLuminosityBlock(StreamID id,
                                                 LuminosityBlockPrincipal const& lbp,
                                                 EventSetup const& c,
                                                 ModuleCallingContext const* mcc) {
      LuminosityBlock lb(lbp, moduleDescription_, mcc);
      lb.setConsumer(this);
      this->doStreamBeginLuminosityBlock_(id,lb, c);
    }
    
    void
    EDProducerBase::doStreamEndLuminosityBlock(StreamID id,
                                               LuminosityBlockPrincipal const& lbp,
                                               EventSetup const& c,
                                               ModuleCallingContext const* mcc) {
      LuminosityBlock lb(lbp, moduleDescription_, mcc);
      lb.setConsumer(this);
      this->doStreamEndLuminosityBlock_(id,lb, c);
      this->doStreamEndLuminosityBlockSummary_(id,lb, c);
    }
    
    
    
    void
    EDProducerBase::doRespondToOpenInputFile(FileBlock const& fb) {
      //respondToOpenInputFile(fb);
    }
    
    void
    EDProducerBase::doRespondToCloseInputFile(FileBlock const& fb) {
      //respondToCloseInputFile(fb);
    }
    
    void EDProducerBase::preallocStreams(unsigned int) {}
    void EDProducerBase::preallocate(PreallocationConfiguration const&) {}
    void EDProducerBase::doBeginStream_(StreamID id){}
    void EDProducerBase::doEndStream_(StreamID id) {}
    void EDProducerBase::doStreamBeginRun_(StreamID id, Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doStreamEndRun_(StreamID id, Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doStreamEndRunSummary_(StreamID id, Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doStreamBeginLuminosityBlock_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c) {}
    void EDProducerBase::doStreamEndLuminosityBlock_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c) {}
    void EDProducerBase::doStreamEndLuminosityBlockSummary_(StreamID id, LuminosityBlock const& lbp, EventSetup const& c) {}
    
    
    void EDProducerBase::doBeginRun_(Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doEndRun_(Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doBeginRunSummary_(Run const& rp, EventSetup const& c) {}
    void EDProducerBase::doEndRunSummary_(Run const& rp, EventSetup const& c) {}
    
    void EDProducerBase::doBeginLuminosityBlock_(LuminosityBlock const& lbp, EventSetup const& c) {}
    void EDProducerBase::doEndLuminosityBlock_(LuminosityBlock const& lbp, EventSetup const& c) {}
    void EDProducerBase::doBeginLuminosityBlockSummary_(LuminosityBlock const& rp, EventSetup const& c) {}
    void EDProducerBase::doEndLuminosityBlockSummary_(LuminosityBlock const& lb, EventSetup const& c) {}
    
    void EDProducerBase::doBeginRunProduce_(Run& rp, EventSetup const& c) {}
    void EDProducerBase::doEndRunProduce_(Run& rp, EventSetup const& c) {}
    void EDProducerBase::doBeginLuminosityBlockProduce_(LuminosityBlock& lbp, EventSetup const& c) {}
    void EDProducerBase::doEndLuminosityBlockProduce_(LuminosityBlock& lbp, EventSetup const& c) {}

    void EDProducerBase::doAcquire_(StreamID, Event const&, EventSetup const&, WaitingTaskWithArenaHolder&) {}

    void
    EDProducerBase::fillDescriptions(ConfigurationDescriptions& descriptions) {
      ParameterSetDescription desc;
      desc.setUnknown();
      descriptions.addDefault(desc);
    }
    
    void
    EDProducerBase::prevalidate(ConfigurationDescriptions& iConfig) {
      edmodule_mightGet_config(iConfig);
    }
    
    static const std::string kBaseType("EDProducer");
    
    const std::string&
    EDProducerBase::baseType() {
      return kBaseType;
    }

  }
}

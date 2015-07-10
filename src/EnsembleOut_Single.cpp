#ifdef ENABLE_SINGLE_ENSEMBLE
#include "EnsembleOut_Single.h"
#include "CpptrajStdio.h"

// CONSTRUCTOR
EnsembleOut_Single::EnsembleOut_Single() : eio_(0), ensembleSize_(0) {}

// DESTRUCTOR
EnsembleOut_Single::~EnsembleOut_Single() {
  EndEnsemble();
  if (eio_ != 0) delete eio_;
}

// EnsembleOut_Single::InitEnsembleWrite()
int EnsembleOut_Single::InitEnsembleWrite(std::string const& tnameIn,
                                          ArgList const& argIn, int ensembleSizeIn,
                                          TrajectoryFile::TrajFormatType writeFormatIn)
{
  // Require a base filename
  if (tnameIn.empty()) {
    mprinterr("Internal Error: InitTrajWrite: No filename given.\n");
    return 1;
  }
  // Require that ensemble size is set.
  ensembleSize_ = ensembleSizeIn; 
  if (ensembleSize_ < 1) {
    mprinterr("Internal Error: Ensemble size has not been set.\n");
    return 1;
  }
  ArgList trajout_args = argIn;
  // Get onlymembers range
  Range members_to_write = MembersToWrite(trajout_args.GetStringKey("onlymembers"), ensembleSize_);
  if (members_to_write.Empty()) return 1;
  // Process common args
  if (traj_.CommonTrajoutSetup(tnameIn, trajout_args, writeFormatIn))
    return 1;
  if (eio_ != 0) delete eio_;
  // If appending, file must exist and must match the current format.
  // TODO Do not pass in writeformat directly to be changed.
  if (traj_.Append()) {
    if (traj_.CheckAppendFormat( traj_.Filename().Full(), traj_.WriteFormat() ))
      traj_.SetAppend( false );
  }
  // Set up for the specified format.
  eio_ = TrajectoryFile::AllocTrajIO( traj_.WriteFormat() );
  if (eio_ == 0) return 1;
  // Check that the TrajectoryIO object can read/write single ensemble
  if (!eio_->CanProcessEnsemble()) {
    mprinterr("Error: Format '%s' cannot be used for ensemble single file output.\n",
              TrajectoryFile::FormatString(traj_.WriteFormat()));
    return 1;
  }
  mprintf("\tWriting '%s' as %s\n", traj_.Filename().full(),
          TrajectoryFile::FormatString(traj_.WriteFormat()));
  eio_->SetDebug( debug_ );
  // Set specified title - will not set if empty 
  eio_->SetTitle( traj_.Title() );
  // Process any write arguments specific to certain formats not related
  // to parm file. Options related to parm file are handled in SetupTrajWrite 
  if (eio_->processWriteArgs(trajout_args)) {
    mprinterr("Error: trajout %s: Could not process arguments.\n",traj_.Filename().full());
    return 1;
  }
  return 0;
}

// EnsembleOut_Single::EndEnsemble()
void EnsembleOut_Single::EndEnsemble() {
  //if (TrajIsOpen()) {
    eio_->closeTraj();
  //  SetTrajIsOpen(false);
  //}
}

// EnsembleOut_Single::SetupEnsembleWrite()
int EnsembleOut_Single::SetupEnsembleWrite(Topology* tparmIn) {
  // Set up topology and coordinate info.
  if (traj_.SetupCoordInfo(tparmIn, tparmIn->Nframes(), tparmIn->ParmCoordInfo()))
    return 1;
  if (debug_ > 0)
    rprintf("\tSetting up single ensemble %s for WRITE, topology '%s' (%i atoms).\n",
            traj_.Filename().base(), tparmIn->c_str(), tparmIn->Natom());
  // Set up TrajectoryIO
  if (eio_->setupTrajout(traj_.Filename().Full(), traj_.Parm(), traj_.CoordInfo(),
                         traj_.NframesToWrite(), traj_.Append()))
    return 1;
  if (debug_ > 0)
    Frame::PrintCoordInfo(traj_.Filename().base(), traj_.Parm()->c_str(), eio_->CoordInfo());
  // First frame setup
  //if (!TrajIsOpen()) { //}
  return 0;
}

// EnsembleOut_Single::WriteEnsemble()
int EnsembleOut_Single::WriteEnsemble(int set, FramePtrArray const& Farray) {
  // Check that set should be written
  if (traj_.CheckFrameRange(set)) return 0;
  // Write
  //fprintf(stdout,"DEBUG: %20s: Writing %i\n",trajName,set);
  if (eio_->writeArray(set, Farray)) return 1;
  return 0;
}

// EnsembleOut_Single::PrintInfo()
void EnsembleOut_Single::PrintInfo(int showExtended) const {
  mprintf("  '%s' (Single Ensemble, %i members) ",traj_.Filename().base(), ensembleSize_);
  eio_->Info();
  traj_.CommonInfo();
}
#endif

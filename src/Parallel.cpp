#include <cstdio>
#include "Parallel.h"
#ifdef PARALLEL_DEBUG_VERBOSE
# include <stdarg.h>
#endif

/** MPI world communicator. */
Parallel::Comm Parallel::world_ = Parallel::Comm();

#ifdef MPI
// printMPIerr()
/** Wrapper for MPI_Error string.  */
void Parallel::printMPIerr(int err, const char *routineName, int rank) {
  int len, eclass;
  char buffer[1024];

  MPI_Error_string(err, buffer, &len);
  MPI_Error_class(err, &eclass);
  // Remove newlines from MPI error string
  for (int i = 0; i != len; i++)
    if (buffer[i] == '\n') buffer[i] = ':';
  fprintf(stderr, "[%i] MPI ERROR %d: %s: [%s]\n", rank, eclass, routineName, buffer);

  return;
}

// checkMPIerr()
/** \return 1 and print error message if MPI action failed. */
int Parallel::checkMPIerr(int err, const char *routineName, int rank) {
  if (err != MPI_SUCCESS) {
    printMPIerr(err, routineName, rank);
    return 1;
  }
  return 0;
}

#ifdef PARALLEL_DEBUG_VERBOSE
// ----- Debug routines --------------------------------------------------------
FILE* Parallel::mpidebugfile_ = 0;

void Parallel::dbgprintf(const char* format, ...) {
  va_list args;
  va_start(args,format);
  //fprintf(mpidebugfile,"[%i] ",worldrank);
  vfprintf(mpidebugfile_, format, args);
  fflush(mpidebugfile_);
  va_end(args);
  return;
}

/** Open a file named Thread.worldrank for this thread */
int Parallel::debug_init() {
  char outfilename[32];
  sprintf(outfilename, "Thread.%03i", world_.Rank());
  mpidebugfile_ = fopen(outfilename, "w");
  if (mpidebugfile_ == NULL) {
    fprintf(stderr,"[%i]\tCould not open debug file:\n", world_.Rank());
    perror("");
    return 1;
  } /*else {
    dbgprintf("MPI DEBUG: %s %p\n",outfilename,mpidebugfile);
    fprintf(stdout,"MPI DEBUG: %s %p\n",outfilename,mpidebugfile);
  }*/
  return 0;
}

/** Close Thread.worldrank file.  */
int Parallel::debug_end() {
  if (mpidebugfile_ != 0)
    fclose(mpidebugfile_);
  return 0;
}
// -----------------------------------------------------------------------------
#endif

// Parallel::Init()
int Parallel::Init(int argc, char** argv) {
  if ( MPI_Init(&argc, &argv) != MPI_SUCCESS ) {
    fprintf(stderr,"Error: Could not initialize MPI.\n");
    return 1;
  }
  world_ = Comm(MPI_COMM_WORLD);
# ifdef PARALLEL_DEBUG_VERBOSE
  debug_init();
# endif
  return 0;
}

int Parallel::End() {
# ifdef PARALLEL_DEBUG_VERBOSE
  debug_end();
# endif
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  return 0;
}

int Parallel::Abort(int errcode) {
  MPI_Abort( MPI_COMM_WORLD, errcode );
  return 1;
}

#else /* MPI */
// ----- NON-MPI VERSIONS OF ROUTINES ------------------------------------------
int Parallel::Init(int argc, char** argv) { return 0; }
int Parallel::End() { return 0; }
#endif

// ===== Parallel::Comm ========================================================
#ifdef MPI
/** CONSTRUCTOR: Use given MPI communicator */
Parallel::Comm::Comm(MPI_Comm commIn) : comm_(commIn), rank_(0), size_(0) {
  MPI_Comm_size(comm_, &size_);
  MPI_Comm_rank(comm_, &rank_);
}

/** Barrier for this communicator. */
void Parallel::Comm::Barrier() const {
  MPI_Barrier( comm_ );
}

/** Use MPI_REDUCE to OP the values in sendbuffer and place them in
  * recvbuffer on master.
  */
int Parallel::Comm::Reduce(void* recvBuffer, void* sendBuffer, int N,
                           MPI_Datatype datatype, MPI_Op op) const
{
  int err = MPI_Reduce(sendBuffer, recvBuffer, N, datatype, op, 0, comm_);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Reducing data to master.", rank_);
    return Parallel::Abort(err); // TODO handle gracefully?
  }
  //if ( parallel_check_error(err) ) {
  //  checkMPIerr(err,"parallel_sum");
  //  return 1;
  //}
  return 0;
}

/** If master    : receive specified value(s) from sendRank.
  * If not master: send specified value(s) to master.
  */
int Parallel::Comm::SendMaster(void *Buffer, int Count, int sendRank, MPI_Datatype datatype) const
{
  //if (size_ == 1) return 0;
  if (rank_ > 0) {
    // Non-master, send to master.
    int err = MPI_Send(Buffer, Count, datatype, 0, 1234, comm_);
    if (err != MPI_SUCCESS) {
      printMPIerr(err, "Sending data to master.", rank_);
      return Parallel::Abort(err);
    }
  } else {
    // Master, receive from sendRank.
    int err = MPI_Recv(Buffer, Count, datatype, sendRank, 1234, comm_, MPI_STATUS_IGNORE);
    if (err != MPI_SUCCESS) {
      printMPIerr(err, "Receiving data from non-master.", rank_);
      return Parallel::Abort(err);
    }
  }
  //if (parallel_check_error(err)!=0) return 1;
  return 0;
}

/** Perform an mpi allreduce. */
int Parallel::Comm::AllReduce(void *Return, void *input, int count,
                              MPI_Datatype datatype, MPI_Op op) const
{
  int err = MPI_Allreduce(input, Return, count, datatype, op, comm_);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing allreduce.\n", rank_);
    printf("[%i]\tError: allreduce failed for %i elements.\n", rank_, count);
    return Parallel::Abort(err);
  }
  //if (parallel_check_error(err)!=0) return 1;
  return 0;
}

/** Perform an mpi gather to master. Assumes send/recv data type and count are same. */
int Parallel::Comm::GatherMaster(void* sendbuffer, int count, MPI_Datatype datatype,
                                 void* recvbuffer) const
{
  int err = MPI_Gather( sendbuffer, count, datatype, recvbuffer, count, datatype, 0, comm_ );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing gather.\n", rank_);
    return Parallel::Abort(err);
  }
  //if (parallel_check_error(err)!=0) return 1;
  return 0;
}

/** Perform an mpi allgather. Assumes send/recv data type and count are same. */
int Parallel::Comm::AllGather(void* sendbuffer, int count, MPI_Datatype datatype, void* recvbuffer)
const
{
  int err = MPI_Allgather( sendbuffer, count, datatype, recvbuffer, count, datatype, comm_ );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing allgather.\n", rank_);
    return Parallel::Abort(err);
  }
  //if (parallel_check_error(err)!=0) return 1;
  return 0;
}

/** Send data to specified rank. */
int Parallel::Comm::Send(void* sendbuffer, int sendcount, MPI_Datatype sendtype, int dest, int tag)
const
{
  int err = MPI_Send( sendbuffer, sendcount, sendtype, dest, tag, comm_ );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing send.\n", rank_);
    fprintf(stderr,"[%i]\tError: send of %i elements failed to rank %i\n", rank_, sendcount, dest);
    return Parallel::Abort(err);
  }
  return 0;
}

/** Receive data from specified rank. */
int Parallel::Comm::Recv(void* recvbuffer, int recvcount, MPI_Datatype recvtype, int src, int tag)
const
{
  int err = MPI_Recv( recvbuffer, recvcount, recvtype, src, tag, comm_, MPI_STATUS_IGNORE );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing receive.\n", rank_);
    fprintf(stderr,"[%i]\tError: receive of %i elements failed from rank %i\n",
            rank_, recvcount, src);
    return Parallel::Abort(err);
  }
  return 0;
}

/** Broadcast data from master to all ranks. */
int Parallel::Comm::MasterBcast(void* buffer, int count, MPI_Datatype datatype) const
{
  int err = MPI_Bcast( buffer, count, datatype, 0, comm_ );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "Performing broadcast from master to all ranks.\n", rank_);
    return Parallel::Abort(err);
  }
  return 0;
}

/** \param err Rank error value.
  * \return Sum of errors on all ranks.
  */
int Parallel::Comm::CheckError(int err) const {
  int errtotal = 0;
  AllReduce(&err, &errtotal, 1, MPI_INT, MPI_SUM);
  return errtotal;
}

// ===== Parallel::File ========================================================
/** Open file in parallel using specified comm.
  * \param mode File access mode: r, r+, w, a
  */
int Parallel::File::OpenFile(const char* filename, const char* mode, Comm const& commIn)
{
  if (filename == 0 || mode == 0) return 1;
  comm_ = commIn;
//# ifdef PARALLEL_DEBUG_VERBOSE
  fprintf(stdout,"[%i]\tOpening file '%s' in parallel with mode '%s'\n",
          comm_.Rank(), filename, mode);
//# endif
  int amode;
  bool needs_seek = false;
  switch (mode[0]) {
    case 'r':
      if (mode[1] == '+')
        amode = MPI_MODE_RDWR;
      else
        amode = MPI_MODE_RDONLY;
      break;
    case 'w':
      amode = (MPI_MODE_WRONLY | MPI_MODE_CREATE);
      // Remove file if present
      MPI_File_delete((char*)filename, MPI_INFO_NULL);
      break;
    case 'a':
      amode = MPI_MODE_WRONLY;
      needs_seek = true;
      break;
    default:
      fprintf(stderr,"Error: Illegal mode specified for MPI_File_open: %s\n", mode);
      return 1;
  }
  int err = MPI_File_open(comm_.MPIcomm(), (char*)filename, amode, MPI_INFO_NULL, &file_);
  if (err != MPI_SUCCESS)
    printMPIerr(err, "Parallel::File::OpenFile()", comm_.Rank());
  // Check that everyone opened the file.
  if (comm_.CheckError(err) != 0) {
    return 1;
  }
  if (needs_seek) Fseek(0, SEEK_END);
  return 0;
}

/** Flush the file. */
int Parallel::File::Flush() { return MPI_File_sync( file_ ); }

/** \return the position of this rank in the file. */
off_t Parallel::File::Position() {
  MPI_Offset offset;
  int err = MPI_File_get_position( file_, &offset );
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "parallel_position()", comm_.Rank());
    return 0;
  }
  return (off_t)offset;
}

/** Close file. */
int Parallel::File::CloseFile() {
  // TODO: Check that file is open?
  //MPI_Barrier(MPI_COMM_WORLD);
# ifdef PARALLEL_DEBUG_VERBOSE
  dbgprintf("\tparallel_closeFile: Closing file.\n");
# endif
  int err = MPI_File_close( &file_ );
  if (err != MPI_SUCCESS)
    printMPIerr(err, "parallel_closeFile", comm_.Rank());
  return 0;
}

/** Read data in parallel. */
int Parallel::File::Fread(void* buffer, int count, MPI_Datatype datatype) {
  int actualCount;
  MPI_Status status;

  int err = MPI_File_read( file_, buffer, count, datatype, &status);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "parallel_fread", comm_.Rank());
    return -1;
  }
  err = MPI_Get_count(&status, datatype, &actualCount);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "parallel_fread", comm_.Rank());
    return -1;
  }
  if (actualCount == MPI_UNDEFINED) {
    fprintf(stderr,"[%i] Error in parallel read: Number of bytes read was undefined.\n",
            comm_.Rank());
    return -1;
  }
  return actualCount;
}

/** Write data in parallel. */
int Parallel::File::Fwrite(const void* buffer, int count, MPI_Datatype datatype) {
  MPI_Status status;
# ifdef PARALLEL_DEBUG_VERBOSE
  //dbgprintf("Calling MPI write(%i): [%s]\n",count,temp);
  dbgprintf("Calling MPI write(%i):\n",count);
# endif
  // NOTE: Some MPI implementations require the void* cast
  int err = MPI_File_write( file_, (void*)buffer, count, datatype, &status);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "parallel_fwrite", comm_.Rank());
    return 1;
  }
  return 0;
}

/** Seek to offset from specified origin. */
int Parallel::File::Fseek(off_t offsetIn, int origin) {
  int org = MPI_SEEK_SET;
  switch ( origin ) {
    case SEEK_SET : org = MPI_SEEK_SET; break;
    case SEEK_CUR : org = MPI_SEEK_CUR; break;
    case SEEK_END : org = MPI_SEEK_END; break;
    default: return 1;
  }
  int err = MPI_File_seek( file_, (MPI_Offset)offsetIn, org);
  if (err != MPI_SUCCESS) {
    printMPIerr(err, "parallel_fseek", comm_.Rank());
    return 1;
  }
  return 0;
}

/** Like fgets, use mpi file routines to get all chars up to and including
  * null or newline. Returns buffer, or 0 on error.
  */
char* Parallel::File::Fgets(char* buffer, int num) {
  int idx = 0;
  for (; idx < num - 1; idx++) {
    int err = MPI_File_read( file_, buffer+idx, 1, MPI_CHAR, MPI_STATUS_IGNORE);
    if (err != MPI_SUCCESS) {
      printMPIerr(err, "parallel_fgets", comm_.Rank());
      return 0;
    }
    if (buffer[idx]=='\n' || buffer[idx]=='\0') {
      idx++; // Always have idx be 1 more char ahead
      break;
    }
  }
  // NOTE: Uncommenting the next 3 lines replaces newlines with NULL
  //if (i==num && buffer[i-1]=='\n')
  //  buffer[i-1]='\0';
  //else
    buffer[idx] = '\0';
  return buffer;
}

int Parallel::File::SetSize(long int offset) {
  int err = MPI_File_set_size( file_, (MPI_Offset)offset);
  if (err != MPI_SUCCESS)
    printMPIerr(err, "parallel_setSize", comm_.Rank());
  if (comm_.CheckError(err) != 0)
    return 1;
  return 0;
}
#endif

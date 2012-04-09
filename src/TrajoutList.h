#ifndef INC_TRAJOUTLIST_H
#define INC_TRAJOUTLIST_H
#include "FileList.h"
#include "TrajectoryFile.h"
#include "Frame.h"
// Class: TrajoutList
/// Hold trajectories for output
class TrajoutList : public FileList {
  public:
    TrajoutList();
    ~TrajoutList();
    /// Add a traj file to the list with given access and associate with a parm
    int AddTrajout(char*,ArgList *A, Topology *);
    /// Call write for all trajectories
    int Write(int, Topology*, Frame*);
    /// Call end for all trajectories
    void Close();
    void Info();
  private:
    std::vector<TrajectoryFile*> trajout_;
};
#endif


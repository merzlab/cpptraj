#ifndef INC_PTRAJACTIONLIST_H
#define INC_PTRAJACTIONLIST_H
// PtrajActionList
#include "Action.h"

class PtrajActionList {
    Action **ActionList;
    int Naction;
    int debug;

  public:

    PtrajActionList();
    ~PtrajActionList();

    void SetDebug(int);
    int Add(ArgList *);
    int Init(DataSetList *, FrameList *, DataFileList *, ParmFileList*);
    int Setup(AmberParm **);
    void DoActions(Frame **, int);
    void Print();
};

#endif

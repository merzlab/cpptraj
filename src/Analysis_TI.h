#ifndef INC_ANALYSIS_TI_H
#define INC_ANALYSIS_TI_H
#include "Analysis.h"
#include "Array1D.h"
class Analysis_TI : public Analysis {
  public:
    Analysis_TI() : nskip_(0), dAout_(0) {}
    static DispatchObject* Alloc() { return (DispatchObject*)new Analysis_TI(); }
    static void Help();

    Analysis::RetType Setup(ArgList&,DataSetList*,TopologyList*,DataFileList*,int);
    Analysis::RetType Analyze();
  private:
    int SetQuadAndWeights(int);

    Array1D input_dsets_; ///< Input DV/DL data sets
    int nskip_; ///< Number of data points to skip in calculating <DV/DL>
    DataSet* dAout_; ///< Free energy out set
    typedef std::vector<double> Darray;
    Darray quad_; ///< Hold Gaussian quadrature abscissas
    Darray wgt_;  ///< Hold Gaussian quadrature weights
};
#endif

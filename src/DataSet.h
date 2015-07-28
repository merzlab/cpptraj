#ifndef INC_DATASET_H
#define INC_DATASET_H
#include <cstddef> // size_t
#include <vector>
#include "MetaData.h"
#include "Dimension.h"
#include "Range.h"
// Class: DataSet
/// Base class that all DataSet types will inherit.
/** DataSets are given certain attributes to make DataSet selection easier; 
  * these are name, index, and aspect. Name is typically associated with the
  * action that creates the DataSet, e.g. RMSD or distance. Index is used
  * when and action outputs subsets of data, e.g. with RMSD it is possible to 
  * output per-residue RMSD, where the DataSet index corresponds to the residue
  * number. Aspect is used to further subdivide output data type; e.g. with 
  * nucleic acid analysis each base pair (divided by index) has shear,
  * stagger etc calculated.
  */
class DataSet {
  public:
    typedef DataSet* (*AllocatorType)();
    /// DataSet base type. 
    enum DataType {
      UNKNOWN_DATA=0, DOUBLE, FLOAT, INTEGER, STRING, MATRIX_DBL, MATRIX_FLT, 
      COORDS, VECTOR, MODES, GRID_FLT, REMLOG, XYMESH, TRAJ, REF_FRAME,
      MAT3X3
    };

    DataSet();
    /// Set DataSet type, width, precision, and dimension.
    DataSet(DataType,int,int,int);
    DataSet(const DataSet&);
    DataSet& operator=(const DataSet&);
    virtual ~DataSet() {} // Destructor - virtual since this class is inherited

    // ----------===== Inheritable functions =====----------
    /// \return the number of data elements stored in the DataSet.
    virtual size_t Size() const = 0;
    /// Consolidate this DataSet across all threads (MPI only)
    virtual int Sync() = 0;
    /// Print DataSet information
    virtual void Info() const = 0;
    // TODO: Remove this. Should only be in DataSet_1D.h
    virtual void Add( size_t, const void* ) = 0;
    // -----------------------------------------------------
    /// Set output width.
    void SetWidth(int);
    /// Set output width and precision
    void SetPrecision(int,int);
    /// Set DataSet MetaData
    int SetMetaData(MetaData const&);
    /// Set specified DataSet dimension.
    void SetDim(Dimension::DimIdxType i, Dimension const& d) { dim_[(int)i]=d; }
    /// Used to set the data and header format strings 
    int SetDataSetFormat(bool);
    /// Check if name and/or index and aspect wildcard match this DataSet.
    bool Matches_WC(std::string const&, Range const&, std::string const&,
                    Range const&, DataType) const;
    /// \return true if given metadata matches this set metadat exactly.
    bool Matches_Exact(MetaData const& m) const { return meta_.Match_Exact(m); }
    /// True if DataSet is empty. 
    bool Empty()                const { return (Size() == 0);      }
    /// DataSet output label.
    const char* legend()        const { return meta_.Legend().c_str();    }
    /// \return DataSet MetaData
    MetaData const& Meta()      const { return meta_; }
    /// \return Total output width of a data element in characters.
    int ColumnWidth()           const { return colwidth_;          }
    /// \return DataSet type.
    DataType Type()             const { return dType_;             }
    /// \return number of dimensions.
    size_t Ndim()               const { return dim_.size();        }
    /// \return true if set is a coordinate set type.
    bool IsCoordSet()           const { return (dType_ == COORDS || dType_ == TRAJ); }
    /// \return specified DataSet dimension.
    Dimension& Dim(Dimension::DimIdxType i) { return dim_[(int)i]; }
    Dimension&       Dim(int i)             { return dim_[i];      }
    Dimension const& Dim(int i)       const { return dim_[i];      }
    /// Comparison for sorting, name/aspect/idx
    inline bool operator<(const DataSet& rhs) const { return meta_ < rhs.meta_; }
    /// Used to sort DataSet pointers (DataSetList, Array1D).
    struct DS_PtrCmp {
      inline bool operator()(DataSet const* first, DataSet const* second) const {
        return *first < *second;
      }
    };
    const char* DataFormat()    const { return data_format_;       }
  protected:
    /// Width of numbers in output elements.
    int Width()                 const { return width_;             }
    const char* data_format_; ///< Used to avoid constant calls to format_.c_str().
  private:
    /// Type to hold coordinate info for each dimension in DataSet.
    typedef std::vector<Dimension> DimArray;
    DimArray dim_;            ///< Holds info for each dimension in the DataSet.
    DataType dType_;          ///< The DataSet type
    int colwidth_;            ///< The total output width of a data element.
    int width_;               ///< The output width of numbers in a data element.
    int precision_;           ///< The output precision of numbers in a data element.
    bool leftAlign_;          ///< If true output will be left-aligned (no leading space).
    std::string format_;      ///< Output printf format string for data.
    MetaData meta_;           ///< DataSet metadata
};
#endif 

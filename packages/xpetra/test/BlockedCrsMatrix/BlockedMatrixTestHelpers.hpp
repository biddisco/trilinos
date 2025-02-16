
#include <iterator>
#include <Epetra_CrsMatrix.h>

namespace XpetraBlockMatrixTests {

//////////////////////////////////////////////////////////////////////////
// EPETRA helper functions
Teuchos::RCP<Epetra_Map> SplitMap(const Epetra_Map& Amap, const Epetra_Map& Agiven) {
  const Epetra_Comm& Comm = Amap.Comm();
  const Epetra_Map&  Ag = Agiven;

  int count=0;
  std::vector<int> myaugids(Amap.NumMyElements());
  for (int i=0; i<Amap.NumMyElements(); ++i) {
    const int gid = Amap.GID(i);
    if (Ag.MyGID(gid)) continue;
    myaugids[count] = gid;
    ++count;
  }
  myaugids.resize(count);
  int gcount;
  Comm.SumAll(&count,&gcount,1);
  Teuchos::RCP<Epetra_Map> Aunknown = Teuchos::rcp(new Epetra_Map(gcount,count,&myaugids[0],0,Comm));

  return Aunknown;
}

Teuchos::RCP<Epetra_Map> CreateMap(const std::set<int>& gids, const Epetra_Comm& comm) {
  std::vector<int> mapvec;
  mapvec.reserve(gids.size());
  mapvec.assign(gids.begin(), gids.end());
  Teuchos::RCP<Epetra_Map> map =
      Teuchos::rcp(new Epetra_Map(-1,
          mapvec.size(),
          &mapvec[0],
          0,
          comm));
  mapvec.clear();
  return map;
}

Teuchos::RCP<Epetra_Map> MergeMaps(const std::vector<Teuchos::RCP<const Epetra_Map> >& maps) {
  if (maps.size()==0)
    std::cout << "no maps to merge" << std::endl;
  for (unsigned i=0; i<maps.size(); ++i) {
    if (maps[i]==Teuchos::null)
      std::cout << "can not merge extractor with null maps" << std::endl;
    if (maps[i]->UniqueGIDs() == false)
      std::cout << "map " << i <<  " not unique" << std::endl;
  }
  std::set<int> mapentries;
  for (unsigned i=0; i<maps.size(); ++i) {
    const Epetra_Map& map = *maps[i];
    std::copy(map.MyGlobalElements(),
        map.MyGlobalElements()+map.NumMyElements(),
        std::inserter(mapentries,mapentries.begin()));
  }
  return CreateMap(mapentries, maps[0]->Comm());
}


bool SplitMatrix2x2(Teuchos::RCP<const Epetra_CrsMatrix> A,
    const Epetra_Map& A11rowmap,
    const Epetra_Map& A22rowmap,
    Teuchos::RCP<Epetra_CrsMatrix>& A11,
    Teuchos::RCP<Epetra_CrsMatrix>& A12,
    Teuchos::RCP<Epetra_CrsMatrix>& A21,
    Teuchos::RCP<Epetra_CrsMatrix>& A22) {
  if (A==Teuchos::null) {
    std::cout << "ERROR: SplitMatrix2x2: A==null on entry" << std::endl;
    return false;
  }

  const Epetra_Comm& Comm   = A->Comm();
  const Epetra_Map&  A22map = A22rowmap;
  const Epetra_Map&  A11map = A11rowmap;

  //----------------------------- create a parallel redundant map of A22map
  std::map<int,int> a22gmap;
  {
    std::vector<int> a22global(A22map.NumGlobalElements());
    int count=0;
    for (int proc=0; proc<Comm.NumProc(); ++proc) {
      int length = 0;
      if (proc==Comm.MyPID()) {
        for (int i=0; i<A22map.NumMyElements(); ++i) {
          a22global[count+length] = A22map.GID(i);
          ++length;
        }
      }
      Comm.Broadcast(&length,1,proc);
      Comm.Broadcast(&a22global[count],length,proc);
      count += length;
    }
    if (count != A22map.NumGlobalElements()) {
      std::cout << "ERROR SplitMatrix2x2: mismatch in dimensions" << std::endl;
      return false;
    }

    // create the map
    for (int i=0; i<count; ++i)
      a22gmap[a22global[i]] = 1;
    a22global.clear();
  }

  //--------------------------------------------------- create matrix A22
  A22 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A22map,100));
  {
    std::vector<int>    a22gcindices(100);
    std::vector<double> a22values(100);
    for (int i=0; i<A->NumMyRows(); ++i) {
      const int grid = A->GRID(i);
      if (A22map.MyGID(grid)==false)
        continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a22gcindices.size()) {
        a22gcindices.resize(numentries);
        a22values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        // see whether we have gcid in a22gmap
        std::map<int,int>::iterator curr = a22gmap.find(gcid);
        if (curr==a22gmap.end()) continue;
        //cout << gcid << " ";
        a22gcindices[count] = gcid;
        a22values[count]    = values[j];
        ++count;
      }
      //cout << endl; fflush(stdout);
      // add this filtered row to A22
      err = A22->InsertGlobalValues(grid,count,&a22values[0],&a22gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }

    } //for (int i=0; i<A->NumMyRows(); ++i)
    a22gcindices.clear();
    a22values.clear();
  }
  A22->FillComplete();
  A22->OptimizeStorage();

  //----------------------------------------------------- create matrix A11
  A11 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A11map,100));
  {
    std::vector<int>    a11gcindices(100);
    std::vector<double> a11values(100);
    for (int i=0; i<A->NumMyRows(); ++i) {
      const int grid = A->GRID(i);
      if (A11map.MyGID(grid)==false) continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a11gcindices.size()) {
        a11gcindices.resize(numentries);
        a11values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        // see whether we have gcid as part of a22gmap
        std::map<int,int>::iterator curr = a22gmap.find(gcid);
        if (curr!=a22gmap.end()) continue;
        a11gcindices[count] = gcid;
        a11values[count] = values[j];
        ++count;
      }
      err = A11->InsertGlobalValues(grid,count,&a11values[0],&a11gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }

    } // for (int i=0; i<A->NumMyRows(); ++i)
    a11gcindices.clear();
    a11values.clear();
  }
  A11->FillComplete();
  A11->OptimizeStorage();

  //---------------------------------------------------- create matrix A12
  A12 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A11map,100));
  {
    std::vector<int>    a12gcindices(100);
    std::vector<double> a12values(100);
    for (int i=0; i<A->NumMyRows(); ++i) {
      const int grid = A->GRID(i);
      if (A11map.MyGID(grid)==false) continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a12gcindices.size()) {
        a12gcindices.resize(numentries);
        a12values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        // see whether we have gcid as part of a22gmap
        std::map<int,int>::iterator curr = a22gmap.find(gcid);
        if (curr==a22gmap.end()) continue;
        a12gcindices[count] = gcid;
        a12values[count] = values[j];
        ++count;
      }
      err = A12->InsertGlobalValues(grid,count,&a12values[0],&a12gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }

    } // for (int i=0; i<A->NumMyRows(); ++i)
    a12values.clear();
    a12gcindices.clear();
  }
  A12->FillComplete(A22map,A11map);
  A12->OptimizeStorage();

  //----------------------------------------------------------- create A21
  A21 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A22map,100));
  {
    std::vector<int>    a21gcindices(100);
    std::vector<double> a21values(100);
    for (int i=0; i<A->NumMyRows(); ++i) {
      const int grid = A->GRID(i);
      if (A22map.MyGID(grid)==false) continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a21gcindices.size()) {
        a21gcindices.resize(numentries);
        a21values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        // see whether we have gcid as part of a22gmap
        std::map<int,int>::iterator curr = a22gmap.find(gcid);
        if (curr!=a22gmap.end()) continue;
        a21gcindices[count] = gcid;
        a21values[count] = values[j];
        ++count;
      }
      err = A21->InsertGlobalValues(grid,count,&a21values[0],&a21gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }

    } // for (int i=0; i<A->NumMyRows(); ++i)
    a21values.clear();
    a21gcindices.clear();
  }
  A21->FillComplete(A11map,A22map);
  A21->OptimizeStorage();

  //-------------------------------------------------------------- tidy up
  a22gmap.clear();
  return true;
}

bool SplitMatrix2x1(Teuchos::RCP<const Epetra_CrsMatrix> A,
    const Epetra_Map& A1rowmap,
    const Epetra_Map& A2rowmap,
    const Epetra_Map& domainmap,
    Teuchos::RCP<Epetra_CrsMatrix>& A1,
    Teuchos::RCP<Epetra_CrsMatrix>& A2)
{
  if (A==Teuchos::null) {
    std::cout << "ERROR: SplitMatrix2x2: A==null on entry" << std::endl;
    return false;
  }

  const Epetra_Map&  A1map = A1rowmap;
  const Epetra_Map&  A2map = A2rowmap;

  //--------------------------------------------------- create matrix A22
  A2 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A2map,100));
  {
    std::vector<int>    a22gcindices(100);
    std::vector<double> a22values(100);
    for (int i=0; i<A->NumMyRows(); ++i)
    {
      const int grid = A->GRID(i);
      if (A2map.MyGID(grid)==false)
        continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x1: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a22gcindices.size()) {
        a22gcindices.resize(numentries);
        a22values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        a22gcindices[count] = gcid;
        a22values[count]    = values[j];
        ++count;
      }
      //cout << endl; fflush(stdout);
      // add this filtered row to A22
      err = A2->InsertGlobalValues(grid,count,&a22values[0],&a22gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }
    } //for (int i=0; i<A->NumMyRows(); ++i)
    a22gcindices.clear();
    a22values.clear();
  }
  A2->FillComplete(domainmap,A2map);
  A2->OptimizeStorage();

  //----------------------------------------------------- create matrix A11
  A1 = Teuchos::rcp(new Epetra_CrsMatrix(Copy,A1map,100));
  {
    std::vector<int>    a11gcindices(100);
    std::vector<double> a11values(100);
    for (int i=0; i<A->NumMyRows(); ++i) {
      const int grid = A->GRID(i);
      if (A1map.MyGID(grid)==false) continue;
      int     numentries;
      double* values;
      int*    cindices;
      int err = A->ExtractMyRowView(i,numentries,values,cindices);
      if (err) {
        std::cout << "ERROR: SplitMatrix2x2: A->ExtractMyRowView returned " << err << std::endl;
        return false;
      }

      if (numentries>(int)a11gcindices.size()) {
        a11gcindices.resize(numentries);
        a11values.resize(numentries);
      }
      int count=0;
      for (int j=0; j<numentries; ++j) {
        const int gcid = A->ColMap().GID(cindices[j]);
        a11gcindices[count] = gcid;
        a11values[count] = values[j];
        ++count;
      }
      err = A1->InsertGlobalValues(grid,count,&a11values[0],&a11gcindices[0]);
      if (err<0) {
        std::cout << "ERROR: SplitMatrix2x2: A->InsertGlobalValues returned " << err << std::endl;
        return false;
      }

    } // for (int i=0; i<A->NumMyRows(); ++i)
    a11gcindices.clear();
    a11values.clear();
  }
  A1->FillComplete(domainmap,A1map);
  A1->OptimizeStorage();

  return true;
}

} // end namespace XpetraBlockMatrixTests

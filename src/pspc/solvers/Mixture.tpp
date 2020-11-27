#ifndef PSPC_MIXTURE_TPP
#define PSPC_MIXTURE_TPP

/*
* PSCF - Polymer Self-Consistent Field Theory
*
* Copyright 2016 - 2019, The Regents of the University of Minnesota
* Distributed under the terms of the GNU General Public License.
*/

#include "Mixture.h"
#include <pscf/mesh/Mesh.h>

#include <cmath>

namespace Pscf {
namespace Pspc
{

   template <int D>
   Mixture<D>::Mixture()
    : vMonomer_(1.0),
      ds_(-1.0),
      meshPtr_(0),
      unitCellPtr_(0)
   {  setClassName("Mixture"); }

   template <int D>
   Mixture<D>::~Mixture()
   {}

   template <int D>
   void Mixture<D>::readParameters(std::istream& in)
   {
      MixtureTmpl< Polymer<D>, Solvent<D> >::readParameters(in);
      vMonomer_ = 1.0; // Default value
      readOptional(in, "vMonomer", vMonomer_);
      read(in, "ds", ds_);

      UTIL_CHECK(nMonomer() > 0);
      UTIL_CHECK(nPolymer()+ nSolvent() > 0);
      UTIL_CHECK(ds_ > 0);
   }

   template <int D>
   void Mixture<D>::setMesh(Mesh<D> const& mesh)
   {
      UTIL_CHECK(nMonomer() > 0);
      UTIL_CHECK(nPolymer()+ nSolvent() > 0);
      UTIL_CHECK(ds_ > 0);

      meshPtr_ = &mesh;

      int i, j, k;
      int blockId;
      // Decides which blocks are necessary to initialize their workspace
      for (i = 0; i < nPolymer(); ++i) {
         for(j = 0; j < polymer(i).firstMonomerSeenCount(); ++j) {
            if( j == 0) {
               for(k = 0; k < polymer(i).firstMonomerSeen(1); ++k) {
                  polymer(i).block(k).necessary = true;
               }
            } else {
               blockId = polymer(i).firstMonomerSeen(j);
               polymer(i).block(blockId).necessary = true;
            }
         }
      }

      // Set discretization for all blocks
      for (i = 0; i < nPolymer(); ++i) {
         for (j = 0; j < polymer(i).nBlock(); ++j) {
            polymer(i).block(j).setDiscretization(ds_, mesh);
         }
      }
   }

   template <int D>
   void Mixture<D>::setupUnitCell(const UnitCell<D>& unitCell)
   {

      // Set association to unitCell
      unitCellPtr_ = &unitCell;

      for (int i = 0; i < nPolymer(); ++i) {
         polymer(i).setupUnitCell(unitCell);
      }
   }

   /*
   * Compute concentrations (but not total free energy).
   */
   template <int D>
   void Mixture<D>::compute(DArray<Mixture<D>::WField> const & wFields,
                            DArray<Mixture<D>::CField>& cFields)
   {
      now = Timer::now();
      clearTimer.start(now);

      UTIL_CHECK(meshPtr_);
      UTIL_CHECK(mesh().size() > 0);
      UTIL_CHECK(nMonomer() > 0);
      UTIL_CHECK(nPolymer() + nSolvent() > 0);
      UTIL_CHECK(wFields.capacity() == nMonomer());
      UTIL_CHECK(cFields.capacity() == nMonomer());

      int nx = mesh().size();
      int nm = nMonomer();
      int i, j, k;

      // Clear all monomer concentration fields
      for (i = 0; i < nm; ++i) {
         UTIL_CHECK(cFields[i].capacity() == nx);
         UTIL_CHECK(wFields[i].capacity() == nx);
         for (j = 0; j < nx; ++j) {
            cFields[i][j] = 0.0;
         }
      }

      now = Timer::now();
      clearTimer.stop(now);
      now = Timer::now();
      polTimer.start(now);

      // Solve MDE for all polymers
      for (i = 0; i < nPolymer(); ++i) {
         polymer(i).compute(wFields);
      }

      now = Timer::now();
      polTimer.stop(now);
      now = Timer::now();
      cumTimer.start(now);

      // Accumulate monomer concentration fields
      double phi;
      for (i = 0; i < nPolymer(); ++i) {
         phi = polymer(i).phi();
         for(int j = 0; j < polymer(i).firstMonomerSeenCount(); ++j) {
            if(j == 0) {
               for(int k = 0; k < polymer(i).firstMonomerSeen(1); ++k) {
                  int monomerId = polymer(i).block(k).monomerId();
                  UTIL_CHECK(monomerId >= 0);
                  UTIL_CHECK(monomerId < nm);
                  CField& monomerField = cFields[monomerId];
                  CField& blockField = polymer(i).block(k).cField();
                  for (int l = 0; l < nx; ++l) {
                     monomerField[l] += phi * blockField[l];
                  }
                  
               }
            } else {
               int blockId = polymer(i).firstMonomerSeen(j);
               int monomerId = polymer(i).block(blockId).monomerId();
               UTIL_CHECK(monomerId >= 0);
               UTIL_CHECK(monomerId < nm);
               CField& monomerField = cFields[monomerId];
               CField& blockField = polymer(i).block(blockId).cField();
               for (int l = 0; l < nx; ++l) {
                  monomerField[l] += phi * blockField[l];
               }           
            }
         }         
      }
      now = Timer::now();
      cumTimer.stop(now);
      double cumTime = cumTimer.time();
      double clearTime = clearTimer.time();
      double polTime = polTimer.time();
      Log::file() << "clear time  = " << clearTime  << " s  \n";
      Log::file() << "pol time  = " << polTime  << " s  \n";
      Log::file() << "cum time  = " << cumTime  << " s  \n";


      // To do: Add compute functions and accumulation for solvents.
   }

   /*
   * Compute total stress.
   */
   template <int D>
   void Mixture<D>::computeStress()
   {
      int i, j;

      // Initialize stress to zero
      for (i = 0; i < 6; ++i) {
         stress_[i] = 0.0;
      }

      // Compute stress for all polymers, after solving MDE
      for (i = 0; i < nPolymer(); ++i) {
         polymer(i).computeStress();
      }

      // Accumulate stress for all the polymer chains
      for (i = 0; i < unitCellPtr_->nParameter(); ++i) {
         for (j = 0; j < nPolymer(); ++j) {
            stress_[i] += polymer(j).stress(i);
         }
      }
   }

} // namespace Pspc
} // namespace Pscf
#endif

#ifndef PFTS_SPECIES_H
#define PFTS_SPECIES_H

/*
* PFTS - Polymer Field Theory Simulator
*
* Copyright 2013, David Morse (morse012@.umn.edu)
* Distributed under the terms of the GNU General Public License.
*/

namespace Pfts{ 

   using namespace Util;

   class Species
   {
   public:
   
      enum Ensemble {UNKNOWN, CLOSED, OPEN};

      /**
      * Default constructor.
      */    
      Species();
   
      /**
      * Set volume fraction phi, and set ensemble type to closed.
      */
      void setPhi(double phi);
   
      /**
      * Sets chemical potential, and set ensemble type to open.
      */
      void setMu(double mu);
   
      /**
      * Solve modified diffusion equation and set related quantities.
      */
      virtual void compute() = 0;
   
      /**
      * Clear all computed quantities.
      */
      void clear() = 0;
   
      /**
      * Get overall volume fraction for this species.
      */
      double phi() const;
   
      /**
      * Get chemical potential for this species (units kT=1).
      */
      double mu() const;
   
      /**
      * Get molecular partition function for this species.
      */
      double q() const;
   
      /**
      * Get statistical ensemble for this species (open or closed).
      */
      Ensemble ensemble();
   
   protected:
   
      /**
      * Volume fraction, set by either setPhi or compute function.
      */
      double phi_;
   
      /**
      * Chemical potential, set by either setPhi or compute function.
      */
      double mu_;
   
      /**
      * Partition function, set by compute function.
      */
      double q_;
   
      /**
      * Statistical ensemble for this species (open or closed).
      */
      int ensemble_;
   
      /**
      * Set true by upon return by compute() and set false by clear().
      */
      bool isComputed_;
   
   };

} 
#endif 

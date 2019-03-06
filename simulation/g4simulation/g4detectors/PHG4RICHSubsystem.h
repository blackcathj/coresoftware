// $$Id: PHG4RICHSubsystem.h,v 1.2 2013/12/22 19:33:38 nfeege Exp $$

/*!
 * \file ${file_name}
 * \brief
 * \author Jin Huang <jhuang@bnl.gov>
 * \version $$Revision: 1.2 $$
 * \date $$Date: 2013/12/22 19:33:38 $$
 */

#ifndef PHG4RICHSubsystem_h
#define PHG4RICHSubsystem_h

#include "g4main/PHG4Subsystem.h"
#include "ePHENIXRICHConstruction.h"

class PHG4RICHDetector;
class PHG4RICHSteppingAction;

  /**
   * \brief Fun4All module to simulate the RICH detector.
   *
   * The detector is constructed and registered via PHG4RICHDetector,
   * ePHENIXRICH::ePHENIXRICHConstruction and ePHENIXRICH::RICH_Geometry.
   *
   * The PHG4SteppingAction provides the method to detect Cerenkov photons. The x,y,z
   * positions of where photons are detected are stored in a PHG4Hits collection.
   *
   * \see ePHENIXRICH::RICH_Geometry
   * \see ePHENIXRICH::ePHENIXRICHConstruction
   * \see PHG4RICHDetector
   * \see PHG4RICHSteppingAction
   * \see PHG4RICHSubsystem
   *
   */
class PHG4RICHSubsystem: public PHG4Subsystem
{
  
  public:
    
    //! constructor
    PHG4RICHSubsystem( const char* name = "RICH" );
    
    //! destructor
    virtual ~PHG4RICHSubsystem( void )
    {}
    
    //! init
    /*!
    creates the detector_ object and place it on the node tree, under "DETECTORS" node (or whatever)
    reates the stepping action and place it on the node tree, under "ACTIONS" node
    creates relevant hit nodes that will be populated by the stepping action and stored in the output DST
    */
    int Init(PHCompositeNode *);
    
    //! event processing
    /*!
    get all relevant nodes from top nodes (namely hit list)
    and pass that to the stepping action
    */
    int process_event(PHCompositeNode *);
    
    //! accessors (reimplemented)
    virtual PHG4Detector* GetDetector( void ) const;

    ePHENIXRICH::RICH_Geometry & get_RICH_geometry() {return geom;}
    
    void set_RICH_geometry(const ePHENIXRICH::RICH_Geometry &g) {geom = g;}

  private:
    
    //! detector geometry
    /*! defives from PHG4Detector */
    PHG4RICHDetector* detector_;
    
    ePHENIXRICH::RICH_Geometry geom;
};

#endif

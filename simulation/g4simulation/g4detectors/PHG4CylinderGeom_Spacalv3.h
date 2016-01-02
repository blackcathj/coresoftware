// $$Id: PHG4CylinderGeom_Spacalv3.h,v 1.3 2014/08/28 22:18:35 jinhuang Exp $$

/*!
 * \file ${file_name}
 * \brief
 * \author Jin Huang <jhuang@bnl.gov>
 * \version $$Revision: 1.3 $$
 * \date $$Date: 2014/08/28 22:18:35 $$
 */
#ifndef PHG4CylinderGeom_Spacalv3_H__
#define PHG4CylinderGeom_Spacalv3_H__

#include "PHG4CylinderGeom_Spacalv2.h"
#include <string>
#include <map>
#include <utility>      // std::pair, std::make_pair


class PHG4CylinderGeom_Spacalv3 : public PHG4CylinderGeom_Spacalv2
{
public:
  PHG4CylinderGeom_Spacalv3();

  virtual
  ~PHG4CylinderGeom_Spacalv3()
  {
  }

  virtual void
  identify(std::ostream& os = std::cout) const;
  virtual void
  Print(Option_t* option = "") const;
  virtual void
  SetDefault();


  double
  get_sidewall_outer_torr() const
  {
    return sidewall_outer_torr;
  }

  void
  set_sidewall_outer_torr(double sidewallOuterTorr)
  {
    sidewall_outer_torr = sidewallOuterTorr;
  }

  double
  get_sidewall_thickness() const
  {
    return sidewall_thickness;
  }

  void
  set_sidewall_thickness(double sidewallThickness)
  {
    sidewall_thickness = sidewallThickness;
  }

  std::string
  get_sidewall_mat() const
  {
    return sidewall_mat;
  }

  void
  set_sidewall_mat(std::string absorberMat)
  {
    sidewall_mat = absorberMat;
  }

  int get_max_phi_bin_in_sec() const
    {
      return max_phi_bin_in_sec;
    }

  void set_max_phi_bin_in_sec(int maxPhiBinInSec)
    {
      max_phi_bin_in_sec = maxPhiBinInSec;
    }

  

  class geom_tower
  {
  public:
    int id;
    double pDz;

    double pDy1;
    double pDx1;
    double pDx2;
    double pDy2;
    double pDx3;
    double pDx4;

    double pTheta;
    double pPhi;
    double pAlp1;
    double pAlp2;

    double pRotationAngleX;
    double centralX;
    double centralY;
    double centralZ;

    double ModuleSkinThickness;
    int NFiberX;
    int NFiberY;

    geom_tower();
    virtual ~geom_tower(){}

    virtual void
    identify(std::ostream& os = std::cout) const;

  ClassDef(PHG4CylinderGeom_Spacalv3::geom_tower,1)

  };
  typedef std::map<int, geom_tower> tower_map_t;

  void
  load_demo_sector_tower_map1();
  void
  load_demo_sector_tower_map2();
  void
  load_demo_sector_tower_map3();

  const tower_map_t &
  get_sector_tower_map() const
  {
    return sector_tower_map;
  }
//
//  void
//  set_geom_super_tower_map(geom_super_tower_map_t geomSuperTowerMap)
//  {
//    geom_super_tower_map = geomSuperTowerMap;
//  }

//! compact ID of each fiber in 32bit PHG4Hit::set_scint_id(). Buffer the result for repeated use.
  class scint_id_coder
  {

  public:

    scint_id_coder(int scint_id);
    scint_id_coder(int sector_id, int tower_id, int fiber_id);
    virtual
    ~scint_id_coder()
    {
    }

    virtual void
      identify(std::ostream& os = std::cout) const
    {
      os <<"scint_id_coder with "
          <<"scint_ID("<<scint_ID<<") = "
          <<"sector_ID("<<sector_ID<<"), "
          <<"tower_ID("<<tower_ID<<"), "
          <<"fiber_ID("<<fiber_ID<<")"<<std::endl;
    }

    int scint_ID;
    int sector_ID;
    int tower_ID;
    int fiber_ID;

    static const int kfiber_bit = 12;
    static const int ktower_bit = 12;
    static const int ksector_bit = 8;

    ClassDef(PHG4CylinderGeom_Spacalv3::scint_id_coder,1)
  };

  //! convert tower_ID + sector ID to eta and z bins as in other cylindrical calorimeters
  //! @return: a std::pair of zbin and phibin number
  virtual std::pair<int,int> get_tower_z_phi_ID(const int tower_ID, const int sector_ID) const;

protected:
  double sidewall_thickness;
  double sidewall_outer_torr;
  std::string sidewall_mat;
  int max_phi_bin_in_sec;

  tower_map_t sector_tower_map;

ClassDef(PHG4CylinderGeom_Spacalv3,3)

};

#endif

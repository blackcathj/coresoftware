// $Id: $

/*!
 * \file PHGeom_DSTInspection.C
 * \brief Quick inspection of PHGeoTGeo object in RUN/GEOMETRY node inside a DST file
 * \author Jin Huang <jhuang@bnl.gov>
 * \version $Revision:   $
 * \date $Date: $
 */

#include <fun4all/Fun4AllDstInputManager.h>
#include <fun4all/Fun4AllDstOutputManager.h>
#include <fun4all/Fun4AllDummyInputManager.h>
#include <fun4all/Fun4AllInputManager.h>
#include <fun4all/Fun4AllNoSyncDstInputManager.h>
#include <fun4all/Fun4AllOutputManager.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4all/SubsysReco.h>
#include <phgeom/PHGeomUtility.h>
#include <phgeom/PHGeomIOTGeo.h>
#include <phgeom/PHGeomTGeo.h>
#include <phool/recoConsts.h>

#include <TEveGeoNode.h>
#include <TEveManager.h>
#include <TGLClip.h>
#include <TGLUtil.h>
#include <TGLViewer.h>
#include <TGeoManager.h>
#include <TGeoTube.h>
#include <TROOT.h>

#include <cassert>
#include <string>
#include <vector>

using namespace std;

R__LOAD_LIBRARY(libphgeom.so)
R__LOAD_LIBRARY(libg4dst.so)
R__LOAD_LIBRARY(libfun4all.so)

// This function edit and builds TGeoManager from the DST's Geometry IO node.
// It need to be called before any module using TGeo, i.e. before modules that calls PHGeomUtility::GetTGeoManager(se->topNode())
// i.e. after PHG4Reco and before any Kalman fitter calls
void EditTPCGeometry(PHCompositeNode *topNode, const int verbosity = 1)
{
  PHGeomIOTGeo *dst_geom_io = PHGeomUtility::GetGeomIOTGeoNode(topNode, false);

  if (not dst_geom_io)
  {
    cout << __PRETTY_FUNCTION__
         << " - ERROR - failed to update PHGeomTGeo node RUN/GEOMETRY due to missing PHGeomIOTGeo node at RUN/GEOMETRY_IO"
         << endl;
    return ;
  }
  if (not dst_geom_io->isValid())
  {
    cout << __PRETTY_FUNCTION__
         << " - ERROR - failed to update PHGeomTGeo node RUN/GEOMETRY due to invalid PHGeomIOTGeo node at RUN/GEOMETRY_IO"
         << endl;
    return ;
  }

  // build new TGeoManager
  TGeoManager *geoManager = dst_geom_io->ConstructTGeoManager();
//  tgeo->CloseGeometry();

  PHGeomTGeo *dst_geom =  PHGeomUtility::GetGeomTGeoNode(topNode, true);
  assert(dst_geom);
  dst_geom->SetGeometry(geoManager);

//  TGeoManager *geoManager = PHGeomUtility::GetTGeoManager(topnode);

  assert(geoManager);

  TGeoVolume *World_vol = geoManager->GetTopVolume();
  TGeoNode *tpc_envelope_node = nullptr;
  TGeoNode *tpc_gas_north_node = nullptr;

  // find tpc north gas volume at path of World*/tpc_envelope*
  if (verbosity)
  {
    cout << "EditTPCGeometry - searching under volume: ";
    World_vol->Print();
  }
  for (int i = 0; i < World_vol->GetNdaughters(); i++)
  {
    TString node_name = World_vol->GetNode(i)->GetName();

    if (verbosity)
      cout << "EditTPCGeometry - searching node " << node_name << endl;

    if (node_name.BeginsWith("tpc_envelope"))
    {
      if (verbosity)
        cout << "EditTPCGeometry - found! " << endl;

      tpc_envelope_node = World_vol->GetNode(i);
      break;
    }
  }
  assert(tpc_envelope_node);

  // find tpc north gas volume at path of World*/tpc_envelope*/tpc_gas_north*
  TGeoVolume *tpc_envelope_vol = tpc_envelope_node->GetVolume();
  assert(tpc_envelope_vol);
  if (verbosity)
  {
    cout << "EditTPCGeometry - searching under volume: ";
    tpc_envelope_vol->Print();
  }
  for (int i = 0; i < tpc_envelope_vol->GetNdaughters(); i++)
  {
    TString node_name = tpc_envelope_vol->GetNode(i)->GetName();

    if (verbosity)
      cout << "EditTPCGeometry - searching node " << node_name << endl;

    if (node_name.BeginsWith("tpc_gas_north"))
    {
      if (verbosity)
        cout << "EditTPCGeometry - found! " << endl;

      tpc_gas_north_node = tpc_envelope_vol->GetNode(i);
      break;
    }
  }
  assert(tpc_gas_north_node);
  TGeoVolume *tpc_gas_north_vol = tpc_gas_north_node->GetVolume();
  assert(tpc_gas_north_vol);

  if (verbosity)
  {
    cout << "EditTPCGeometry - gas volume: ";
    tpc_gas_north_vol->Print();
  }

  // add two example measurement surfaces
  TGeoMedium *tpc_gas_north_medium = tpc_gas_north_vol->GetMedium();
  assert(tpc_gas_north_medium);
  TGeoTube *tpc_gas_north_shape = dynamic_cast<TGeoTube *>(tpc_gas_north_vol->GetShape());
  assert(tpc_gas_north_shape);

  // make a measurement volume with a box, 1.25x10 cm crosssection, TPC gas vol length
  TGeoVolume *tpc_gas_measurement_vol = geoManager->MakeBox("tpc_gas_measurement", tpc_gas_north_medium, 1.25, 10, tpc_gas_north_shape->GetDz());
  tpc_gas_measurement_vol->SetLineColor(kYellow);
  tpc_gas_measurement_vol->SetFillColor(kYellow);
  tpc_gas_measurement_vol->SetVisibility(kTRUE);
  TGeoCombiTrans *tpc_gas_measurement_location_1 = new TGeoCombiTrans(30,0,0,
                                     new TGeoRotation("tpc_gas_measurement_rotation_1",0,0,0));
  TGeoCombiTrans *tpc_gas_measurement_location_2 = new TGeoCombiTrans(30,0,0,
                                     new TGeoRotation("tpc_gas_measurement_rotation_2",0,0,0));
  tpc_gas_measurement_location_2->RotateZ(45);
  tpc_gas_north_vol->AddNode(tpc_gas_measurement_vol, 1, tpc_gas_measurement_location_1);
  tpc_gas_north_vol->AddNode(tpc_gas_measurement_vol, 2, tpc_gas_measurement_location_2);

  if (verbosity)
  {
    cout << "EditTPCGeometry - tpc_gas_measurement_vol volume: ";
    tpc_gas_measurement_vol->Print();
  }

  // Closing geometry implies checking the geometry validity,
  // fixing shapes with negative parameters (run-time shapes)building the cache manager, voxelizing all volumes,
  // counting the total number of physical nodes and registering the manager class to the browser.
  geoManager->CloseGeometry();
}

//! Quick inspection of PHGeoTGeo object in RUN/GEOMETRY node inside a DST file
//! Based on abhisek's display macro
void PHGeom_DSTInspection(string DST_file_name = "G4sPHENIX.root",
                          bool do_clip = true)
{
  TEveManager::Create();

  Fun4AllServer *se = Fun4AllServer::instance();
  se->Verbosity(1);
  recoConsts *rc = recoConsts::instance();
  rc->set_IntFlag("RUNNUMBER", 12345);

  Fun4AllInputManager *hitsin = new Fun4AllDstInputManager("DSTin");
  hitsin->fileopen(DST_file_name);
  se->registerInputManager(hitsin);

  // run one event as example
  se->run(1);


  EditTPCGeometry(se->topNode());

  PHGeomUtility::GetTGeoManager(se->topNode());
  assert(gGeoManager);

  if (!gROOT->GetListOfGeometries()->FindObject(gGeoManager))
    gROOT->GetListOfGeometries()->Add(gGeoManager);
  if (!gROOT->GetListOfBrowsables()->FindObject(gGeoManager))
    gROOT->GetListOfBrowsables()->Add(gGeoManager);
  //  gGeoManager->UpdateElements();

  TGeoNode *current = gGeoManager->GetCurrentNode();
  //Alternate drawing
  //current->GetVolume()->Draw("ogl");
  //Print the list of daughters
  //current->PrintCandidates();
  for (int igeom = 0; igeom < current->GetNdaughters(); igeom++)
  {
    // hide black holes
    TGeoNode *geo_node = (TGeoNode *) current->GetNodes()->UncheckedAt(igeom);
    if (string(geo_node->GetName()).find("BH_") != string::npos)
      geo_node->GetVolume()->SetVisibility(false);
  }
  TEveGeoTopNode *eve_node = new TEveGeoTopNode(gGeoManager, current);
//  eve_node->SetVisLevel(6);
  gEve->AddGlobalElement(eve_node);
  gEve->FullRedraw3D(kTRUE);

  // EClipType not exported to CINT (see TGLUtil.h):
  // 0 - no clip, 1 - clip plane, 2 - clip box
  TGLViewer *v = gEve->GetDefaultGLViewer();
//  if (do_clip)
//  {
//    v->GetClipSet()->SetClipType(TGLClip::kClipPlane);
//  }z
  //  v->ColorSet().Background().SetColor(kMagenta + 4);
  v->SetGuideState(TGLUtil::kAxesOrigin, kTRUE, kFALSE, 0);
  v->RefreshPadEditor(v);

  v->CurrentCamera().RotateRad(3.14/2, 0.);
  v->DoDraw();
}

#include "PHG4GDMLImportDetector.h"
#include "PHG4BlockRegionSteppingAction.h"

#include <g4main/PHG4RegionInformation.h>
#include <g4main/PHG4Utils.h>


#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>
#include <phool/getClass.h>

#include <Geant4/G4Material.hh>
#include <Geant4/G4Box.hh>
#include <Geant4/G4LogicalVolume.hh>
#include <Geant4/G4PVPlacement.hh>
#include <Geant4/G4AssemblyVolume.hh>

#include <Geant4/G4VisAttributes.hh>
#include <Geant4/G4Colour.hh>

#include <sstream>

#include "Geant4/G4GDMLParser.hh"

using namespace std;

//_______________________________________________________________
//note this inactive thickness is ~1.5% of a radiation length
PHG4GDMLImportDetector::PHG4GDMLImportDetector( PHCompositeNode *Node, const std::string &dnam,const int lyr  ):
  PHG4Detector(Node, dnam),
  center_in_x(0*cm),
  center_in_y(0*cm),
  center_in_z(-200*cm),
  _region(NULL),
  active(0),
  layer(lyr),
  blackhole(0)
{
  //set the default radii
  for (int i = 0; i < 3; i++)
    {
      dimension[i] = 100 * cm;
    }

}

//_______________________________________________________________
bool PHG4GDMLImportDetector::IsInBlock(G4VPhysicalVolume * volume) const
{
  if (active && volume == block_physi)
  {
    return true;
  }
  return false;
}

bool PHG4GDMLImportDetector::IsInBlockActive(G4VPhysicalVolume * volume) const
{
  if (volume == block_physi)
  {
    return true;
  }
  return false;
}


void PHG4GDMLImportDetector::SetDisplayProperty( G4LogicalVolume* lv)
{

  cout <<"SetDisplayProperty - LV "<<lv->GetName()<<endl;


  G4VisAttributes* matVis = new G4VisAttributes();
  matVis->SetColour(.2,.2,.7,.25);
  matVis->SetVisibility(true);
  matVis->SetForceSolid(true);
  lv->SetVisAttributes(matVis);

  int nDaughters = lv->GetNoDaughters();
  for(int i = 0; i < nDaughters; ++i)
  {
      G4VPhysicalVolume* pv = lv->GetDaughter(i);

      cout <<"SetDisplayProperty - PV["<<i<<"] = "<<pv->GetName()<<endl;


      G4LogicalVolume* worldLogical = pv->GetLogicalVolume();
      SetDisplayProperty(worldLogical);
  }
}


void PHG4GDMLImportDetector::SetDisplayProperty( G4AssemblyVolume* av)
{

  cout <<"SetDisplayProperty - G4AssemblyVolume w/ TotalImprintedVolumes "<<av->TotalImprintedVolumes()
      <<"/"<<av->GetImprintsCount()<<endl;

  std::vector<G4VPhysicalVolume*>::iterator it = av->GetVolumesIterator();

  int nDaughters = av->TotalImprintedVolumes();
  for(int i = 0; i < nDaughters; ++i, ++it)
  {
      cout <<"SetDisplayProperty - AV["<<i<<"] = "<<(*it)->GetName()<<endl;
      G4VPhysicalVolume* pv =(*it);

      G4LogicalVolume* worldLogical = pv->GetLogicalVolume();
      SetDisplayProperty(worldLogical);
  }
}

//_______________________________________________________________
void PHG4GDMLImportDetector::Construct( G4LogicalVolume* logicWorld )
{
  G4GDMLReadStructure * reader=
  new G4GDMLReadStructure();
  G4GDMLParser gdmlParser(reader);
  gdmlParser.Read("./ITS.gdml");

//  G4AssemblyVolume* av = reader->GetAssembly("ITSUStave0") ;
  G4AssemblyVolume* av_ITSUStave0 = reader->GetAssembly("ITSUStave0");
  G4AssemblyVolume* av_ITSUStave1 = reader->GetAssembly("ITSUStave1");
  G4AssemblyVolume* av_ITSUStave2 = reader->GetAssembly("ITSUStave2");
  G4AssemblyVolume* av_ITSUStave3 = reader->GetAssembly("ITSUStave3");
  G4AssemblyVolume* av_ITSUStave4 = reader->GetAssembly("ITSUStave4");
  G4AssemblyVolume* av_ITSUStave5 = reader->GetAssembly("ITSUStave5");
  G4AssemblyVolume* av_ITSUStave6 = reader->GetAssembly("ITSUStave6");

//  G4VPhysicalVolume* pv = gdmlParser.GetWorldVolume("default");
//  block_logic = //pv->GetLogicalVolume();
//      gdmlParser.GetVolume("TOPBOX");

//  G4VisAttributes* matVis = new G4VisAttributes();

//  PHG4Utils::SetColour(matVis, "CF4");
//  matVis->SetVisibility(true);
//  matVis->SetForceSolid(true);
//  block_logic->SetVisAttributes(matVis);


//  G4ThreeVector g4vec(center_in_x, center_in_y, center_in_z);

  for (int i = 0; i<20; i++)
    {

      const G4double phi = i/20.*2*M_PI;

      G4RotationMatrix *Rot  = new G4RotationMatrix();
      Rot->rotateZ(phi+.1);

      {
        G4double radius = 5*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave0->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*2*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave1->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*3*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave2->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*4*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave3->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*5*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave4->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*6*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave5->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
      {
        G4double radius = 5*7*cm;
        G4ThreeVector g4vec(radius*cos(phi - M_PI_2),radius*sin(phi - M_PI_2), 0);
        av_ITSUStave6->MakeImprint(logicWorld, g4vec, Rot, i, overlapcheck);
      }
    }

//    G4ThreeVector g4vec(center_in_x, center_in_y, center_in_z);
//  av->MakeImprint(logicWorld, g4vec, 0, 1, overlapcheck);
//  block_physi = new G4PVPlacement(0, G4ThreeVector(center_in_x, center_in_y, center_in_z),
//                                  block_logic,
//                                  G4String(GetName().c_str()),
//                                  logicWorld, 0, false, overlapcheck);

  SetDisplayProperty(av_ITSUStave0);
  SetDisplayProperty(av_ITSUStave1);
  SetDisplayProperty(av_ITSUStave2);
  SetDisplayProperty(av_ITSUStave3);
  SetDisplayProperty(av_ITSUStave4);
  SetDisplayProperty(av_ITSUStave5);
  SetDisplayProperty(av_ITSUStave6);
//    SetDisplayProperty(block_logic);
}




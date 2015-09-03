#include "PHG4ParticleGenerator.h"
#include "PHG4Particlev2.h"

#include "PHG4InEvent.h"


#include <fun4all/getClass.h>

#include <phool/PHCompositeNode.h>
#include <phool/PHIODataNode.h>

#include <cstdlib>
#include <cmath>

using namespace std;

PHG4ParticleGenerator::PHG4ParticleGenerator(const string &name): 
  PHG4ParticleGeneratorBase(name),
  z_min(-10.0),
  z_max(10.0),
  eta_min(-1.0),
  eta_max(1.0),
  phi_min(-M_PI),
  phi_max(M_PI),
  mom_min(0.0),
  mom_max(10.0),mom_spread(0),
  randgen(0)
{
  return;
}

PHG4ParticleGenerator::~PHG4ParticleGenerator()
{
  return;
}


void
PHG4ParticleGenerator::set_z_range(const double min, const double max)
{
  z_min = min;
  z_max = max;
  return;
}

void
PHG4ParticleGenerator::set_eta_range(const double min, const double max)
{
  eta_min = min;
  eta_max = max;
  return;
}

void
PHG4ParticleGenerator::set_phi_range(const double min, const double max)
{
  phi_min = min;
  phi_max = max;
  return;
}

void
PHG4ParticleGenerator::set_vtx(const double x, const double y, const double z)
{
  vtx_x = x;
  vtx_y = y;
  vtx_z = z;
  return;
}

void
PHG4ParticleGenerator::set_mom_range(const double min, const double max, const double gaus_spread)
{
  mom_min = min;
  mom_max = max;
  mom_spread = gaus_spread;
  return;
}

void
PHG4ParticleGenerator::set_seed(const int seed)
{
//  srand(seed);
  randgen.SetSeed(seed);
  return;
}

int
PHG4ParticleGenerator::process_event(PHCompositeNode *topNode)
{
  PHG4InEvent *ineve = findNode::getClass<PHG4InEvent>(topNode,"PHG4INEVENT");

  vtx_z = (z_max-z_min)*randgen.Uniform() + z_min;
  int vtxindex = ineve->AddVtx(vtx_x,vtx_y,vtx_z,t0);

  vector<PHG4Particle *>::const_iterator iter;
  for (iter = particlelist.begin(); iter != particlelist.end(); iter++)
    {
      PHG4Particle *particle = new PHG4Particlev2(*iter);
      SetParticleId(particle,ineve);
      double mom = (mom_max - mom_min)*randgen.Uniform() + mom_min;
      if (mom_spread>0) mom += randgen.Gaus(0,mom_spread);

      double eta = (eta_max - eta_min)*randgen.Uniform() + eta_min;
      double phi = (phi_max - phi_min)*randgen.Uniform() + phi_min;
      double pt = mom/cosh(eta);

      particle->set_e(mom);
      particle->set_px(pt*cos(phi));
      particle->set_py(pt*sin(phi));
      particle->set_pz(pt*sinh(eta));

      ineve->AddParticle(vtxindex, particle);
    }
  if (verbosity > 0)
  {
    ineve->identify();
  }
  return 0;
}

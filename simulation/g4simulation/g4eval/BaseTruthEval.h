
#ifndef __BASETRUTHEVAL_H__
#define __BASETRUTHEVAL_H__

#include <phool/PHCompositeNode.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>
#include <g4main/PHG4VtxPoint.h>

class BaseTruthEval {

public:

  BaseTruthEval(PHCompositeNode *topNode);
  virtual ~BaseTruthEval() {}

  void next_event(PHCompositeNode *topNode);
  void set_strict(bool strict) {_strict = strict;}
  
  PHG4Particle*      get_particle(PHG4Hit* g4hit);  
  int                get_embed(PHG4Particle* particle);
  PHG4VtxPoint*      get_vertex(PHG4Particle* particle);

  bool               is_primary(PHG4Particle* particle);
  PHG4Particle*      get_primary(PHG4Hit* g4hit);
  PHG4Particle*      get_primary(PHG4Particle* particle);  

  bool               is_g4hit_from_particle(PHG4Hit* g4hit, PHG4Particle* particle);
  bool               are_same_particle(PHG4Particle* p1, PHG4Particle* p2);
  bool               are_same_vertex(PHG4VtxPoint* vtx1, PHG4VtxPoint* vtx2);
    
private:

  void get_node_pointers(PHCompositeNode* topNode);
  
  PHG4TruthInfoContainer* _truthinfo;

  bool _strict;
};

#endif // __BASETRUTHEVAL_H__

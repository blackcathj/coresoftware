
#ifndef __SVTXTRUTHEVAL_H__
#define __SVTXTRUTHEVAL_H__

#include <phool/PHCompositeNode.h>
#include <g4main/PHG4HitContainer.h>
#include <g4main/PHG4Hit.h>
#include <g4main/PHG4TruthInfoContainer.h>
#include <g4main/PHG4Particle.h>
#include <g4main/PHG4VtxPoint.h>

#include <set>
#include <map>

class SvtxTruthEval {

public:

  SvtxTruthEval(PHCompositeNode *topNode);
  virtual ~SvtxTruthEval() {}

  void next_event(PHCompositeNode *topNode);
  void do_caching(bool do_cache) {_do_cache = do_cache;}
  void set_strict(bool strict) {_strict = strict;}
  
  std::set<PHG4Hit*> all_truth_hits();
  std::set<PHG4Hit*> all_truth_hits(PHG4Particle* particle);
  PHG4Particle*      get_particle(PHG4Hit* g4hit);  
  int                get_embed(PHG4Particle* particle);
  bool               is_primary(PHG4Particle* particle);
  PHG4VtxPoint*      get_vertex(PHG4Particle* particle);

  PHG4Hit*           get_innermost_truth_hit(PHG4Particle* particle);
  PHG4Hit*           get_outermost_truth_hit(PHG4Particle* particle);
  
private:

  void get_node_pointers(PHCompositeNode* topNode);
  
  PHG4TruthInfoContainer* _truthinfo;
  PHG4HitContainer* _g4hits_svtx;
  PHG4HitContainer* _g4hits_tracker;

  bool _strict;
  
  bool                                        _do_cache;
  std::set<PHG4Hit*>                          _cache_all_truth_hits;
  std::map<PHG4Particle*,std::set<PHG4Hit*> > _cache_all_truth_hits_g4particle;
  std::map<PHG4Particle*,PHG4Hit*>            _cache_get_innermost_truth_hit;
  std::map<PHG4Particle*,PHG4Hit*>            _cache_get_outermost_truth_hit;
};

#endif // __SVTXTRUTHEVAL_H__

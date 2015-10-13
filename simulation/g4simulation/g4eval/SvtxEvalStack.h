
#ifndef __SVTXEVALSTACK_H__
#define __SVTXEVALSTACK_H__

#include "SvtxVertexEval.h"
#include "SvtxTrackEval.h"
#include "SvtxClusterEval.h"
#include "SvtxHitEval.h"
#include "SvtxTruthEval.h"

#include <phool/PHCompositeNode.h>

// This user class provides pointers to the
// full set of tracking evaluators and
// protects the user from future introduction
// of new eval heirachies (new eval objects can
// be introduced without rewrites)

class SvtxEvalStack {

public:

  SvtxEvalStack(PHCompositeNode *topNode);
  virtual ~SvtxEvalStack() {}

  void next_event(PHCompositeNode *topNode);
  void do_caching(bool do_cache) {_vertexeval.do_caching(do_cache);}
  void set_strict(bool strict) {_vertexeval.set_strict(strict);}

  SvtxVertexEval*  get_vertex_eval() {return &_vertexeval;}
  SvtxTrackEval*   get_track_eval() {return _vertexeval.get_track_eval();}
  SvtxClusterEval* get_cluster_eval() {return _vertexeval.get_cluster_eval();}
  SvtxHitEval*     get_hit_eval() {return _vertexeval.get_hit_eval();}
  SvtxTruthEval*   get_truth_eval() {return _vertexeval.get_truth_eval();}
  
private:
  SvtxVertexEval _vertexeval; // right now this is the top-level eval
};

#endif // __SVTXEVALSTACK_H__

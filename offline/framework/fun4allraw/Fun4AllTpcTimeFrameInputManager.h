// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef FUN4ALLRAW_Fun4AllTpcTimeFrameInputManager_H
#define FUN4ALLRAW_Fun4AllTpcTimeFrameInputManager_H

#include "InputManagerType.h"

#include <fun4all/Fun4AllInputManager.h>

#include <map>
#include <set>
#include <string>
#include <utility>

class SingleStreamingInput;
class Gl1Packet;
class PHCompositeNode;
class SyncObject;
class SingleGl1PoolInput;
class TH1;

class Fun4AllTpcTimeFrameInputManager : public Fun4AllInputManager
{
 public:
  Fun4AllTpcTimeFrameInputManager(const std::string &name = "DUMMY", const std::string &dstnodename = "DST", const std::string &topnodename = "TOP");
  ~Fun4AllTpcTimeFrameInputManager() override;
  int fileopen(const std::string & /*filenam*/) override { return 0; }
  // cppcheck-suppress virtualCallInConstructor
  int fileclose() override;
  int run(const int nevents = 0) override;

  void registerStreamingGL1Input(SingleGl1PoolInput *evtin);

  void Print(const std::string &what = "ALL") const override;
  int ResetEvent() override;
  int PushBackEvents(const int i) override;
  int GetSyncObject(SyncObject **mastersync) override;
  int SyncIt(const SyncObject *mastersync) override;
  int HasSyncObject() const override { return 1; }
  std::string GetString(const std::string &what) const override;

  int FillGl1();

  void SetTpcBcoRange(const int min_diff = -256, const int max_diff = 256);

 private:
  void createQAHistos();

  SyncObject *m_SyncObject{nullptr};
  PHCompositeNode *m_topNode{nullptr};

  uint64_t m_RefBCO{0};

  std::pair<uint64_t, Gl1Packet *> m_Gl1RawHitMap{0, nullptr};

  int m_RunNumber{0};

  std::pair<unsigned int, unsigned int> m_tpc_bco_range{-256, 256};

  SingleGl1PoolInput *m_Gl1Input;
};

#endif /* FUN4ALL_Fun4AllTpcTimeFrameInputManager_H */

#include "Fun4AllTpcTimeFrameInputManager.h"

#include "InputManagerType.h"
#include "MvtxRawDefs.h"
#include "SingleGl1PoolInput.h"

#include <ffarawobjects/Gl1Packet.h>
#include <ffarawobjects/TpcRawHit.h>
#include <ffarawobjects/TpcRawHitContainer.h>

#include <fun4all/Fun4AllInputManager.h>  // for Fun4AllInputManager
#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/Fun4AllServer.h>
#include <fun4all/Fun4AllSyncManager.h>

#include <ffaobjects/SyncObject.h>  // for SyncObject
#include <ffaobjects/SyncObjectv1.h>

#include <qautils/QAHistManagerDef.h>
#include <qautils/QAUtil.h>

#include <frog/FROG.h>

#include <phool/PHObject.h>  // for PHObject
#include <phool/getClass.h>
#include <phool/phool.h>  // for PHWHERE
#include <boost/format.hpp>

#include <TH1.h>
#include <TSystem.h>

#include <algorithm>  // for max
#include <cassert>
#include <cstdint>  // for uint64_t, uint16_t
#include <cstdlib>
#include <iostream>  // for operator<<, basic_ostream, endl
#include <utility>   // for pair

Fun4AllTpcTimeFrameInputManager::Fun4AllTpcTimeFrameInputManager(const std::string &name, const std::string &dstnodename, const std::string &topnodename)
  : Fun4AllInputManager(name, dstnodename, topnodename)
  , m_SyncObject(new SyncObjectv1())
{
  Fun4AllServer *se = Fun4AllServer::instance();
  m_topNode = se->topNode(TopNodeName());

  createQAHistos();

  return;
}

Fun4AllTpcTimeFrameInputManager::~Fun4AllTpcTimeFrameInputManager()
{
  if (IsOpen())
  {
    fileclose();
  }
  delete m_SyncObject;
  // clear leftover raw event maps and vectors with poolreaders
  // GL1
  if (m_Gl1Input)
  {
    delete m_Gl1Input;
  }
}

int Fun4AllTpcTimeFrameInputManager::run(const int /*nevents*/)
{
  int iret = 0;
  if (m_Gl1Input)  // Gl1 first to get the reference
  {
    iret += FillGl1();
  }

  iret += FillTpc();

  return iret;
}

int Fun4AllTpcTimeFrameInputManager::fileclose()
{
  return 0;
}

void Fun4AllTpcTimeFrameInputManager::Print(const std::string &what) const
{
  if (what == "ALL" || what == "INPUTFILES")
  {
    std::cout << "-----------------------------" << std::endl;
    if (m_Gl1Input)
    {
      std::cout << "Single Streaming Input Manager " << m_Gl1Input->Name() << " reads run "
                << m_Gl1Input->RunNumber()
                << " from file " << m_Gl1Input->FileName()
                << std::endl;
    }
    Fun4AllInputManager::Print(what);
    return;
  }
}

int Fun4AllTpcTimeFrameInputManager::ResetEvent()
{
  m_RefBCO = 0;
  return 0;
}

int Fun4AllTpcTimeFrameInputManager::PushBackEvents(const int /*i*/)
{
  return 0;
}

int Fun4AllTpcTimeFrameInputManager::GetSyncObject(SyncObject **mastersync)
{
  // here we copy the sync object from the current file to the
  // location pointed to by mastersync. If mastersync is a 0 pointer
  // the syncobject is cloned. If mastersync allready exists the content
  // of syncobject is copied
  if (!(*mastersync))
  {
    if (m_SyncObject)
    {
      *mastersync = dynamic_cast<SyncObject *>(m_SyncObject->CloneMe());
      assert(*mastersync);
    }
  }
  else
  {
    *(*mastersync) = *m_SyncObject;  // copy syncobject content
  }
  return Fun4AllReturnCodes::SYNC_OK;
}

int Fun4AllTpcTimeFrameInputManager::SyncIt(const SyncObject *mastersync)
{
  if (!mastersync)
  {
    std::cout << PHWHERE << Name() << " No MasterSync object, cannot perform synchronization" << std::endl;
    std::cout << "Most likely your first file does not contain a SyncObject and the file" << std::endl;
    std::cout << "opened by the Fun4AllDstInputManager with Name " << Name() << " has one" << std::endl;
    std::cout << "Change your macro and use the file opened by this input manager as first input" << std::endl;
    std::cout << "and you will be okay. Fun4All will not process the current configuration" << std::endl
              << std::endl;
    return Fun4AllReturnCodes::SYNC_FAIL;
  }
  int iret = m_SyncObject->Different(mastersync);
  if (iret)
  {
    std::cout << __PRETTY_FUNCTION__ << ": big problem" << std::endl;
    exit(1);
  }
  return Fun4AllReturnCodes::SYNC_OK;
}

std::string Fun4AllTpcTimeFrameInputManager::GetString(const std::string &what) const
{
  std::cout << PHWHERE << " called with " << what << " , returning empty string" << std::endl;
  return "";
}

void Fun4AllTpcTimeFrameInputManager::registerStreamingGL1Input(
    SingleGl1PoolInput *evtin)
{
  assert(evtin);

  evtin->StreamingInputManager(this);
  if (!m_StreamingFlag)
  {
    evtin->CreateDSTNode(m_topNode);
  }
  m_Gl1Input = evtin;
}

void Fun4AllTpcTimeFrameInputManager::AddGl1RawHit(uint64_t bclk, Gl1Packet *hit)
{
  if (Verbosity() > 1)
  {
    std::cout << "Adding gl1 hit to bclk 0x"
              << std::hex << bclk << std::dec << std::endl;
  }

  if (m_Gl1RawHitMap.second)
  {
    cout << __PRETTY__FUNCTION__ << " Warning: deleting old Gl1RawHitMap ";
    m_Gl1RawHitMap.second->Print();
  }

  m_Gl1RawHitMap = std::make_pair(bclk, hit);
}

int Fun4AllTpcTimeFrameInputManager::FillGl1()
{
  // unsigned int alldone = 0;
  if (m_Gl1Input)
  {
    if (Verbosity() > 0)
    {
      std::cout << "Fun4AllTpcTimeFrameInputManager::FillGl1 - fill pool for " << m_Gl1Input->Name() << std::endl;
    }
    m_Gl1Input->FillPool();
    if (m_RunNumber == 0)
    {
      m_RunNumber = m_Gl1Input->RunNumber();
      SetRunNumber(m_RunNumber);
    }
    else
    {
      if (m_RunNumber != m_Gl1Input->RunNumber())
      {
        std::cout << PHWHERE << " Run Number mismatch, run is "
                  << m_RunNumber << ", " << m_Gl1Input->Name() << " reads "
                  << m_Gl1Input->RunNumber() << std::endl;
        std::cout << "You are likely reading files from different runs, do not do that" << std::endl;
        Print("INPUTFILES");
        gSystem->Exit(1);
        exit(1);
      }
    }
  }
  if (m_Gl1RawHitMap.second == nullptr)
  {
    std::cout << "Gl1RawHitMap is empty - we are done" << std::endl;
    return -1;
  }
  else
  {
    //    std::cout << "stashed gl1 BCOs: " << m_Gl1RawHitMap.size() << std::endl;
    Gl1Packet *gl1packet = m_Gl1RawHitMap.second;
    MySyncManager()->CurrentEvent(gl1packet->getEvtSequence());
    m_RefBCO = gl1hititer->getBCO();
    m_RefBCO = m_RefBCO & 0xFFFFFFFFFFU;  // 40 bits (need to handle rollovers)
                                          //    std::cout << "BCOis " << std::hex << m_RefBCO << std::dec << std::endl;
    m_Gl1Input->CleanupUsedPackets(m_Gl1RawHitMap.first);
    m_Gl1RawHitMap = std::make_pair(0, nullptr);
  }
  return 0;
}

int Fun4AllTpcTimeFrameInputManager::FillTpc()
{ 






  if (tpccont->get_nhits() > 500000)
  {
    std::cout << "Resetting TPC Container with number of entries " << tpccont->get_nhits() << std::endl;
    tpccont->Reset();
  }
  return 0;
}
void SetTpcBcoRange(const int min_diff, const int max_diff)
{
  m_tpc_bco_range = std::make_pair(min_diff, max_diff);
}


void Fun4AllTpcTimeFrameInputManager::createQAHistos()
{
  auto hm = QAHistManagerDef::getHistoManager();
  assert(hm);
}

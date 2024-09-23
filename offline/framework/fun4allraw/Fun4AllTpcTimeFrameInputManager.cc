#include "Fun4AllTpcTimeFrameInputManager.h"

#include "InputManagerType.h"
#include "MvtxRawDefs.h"
#include "SingleStreamingInput.h"

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
  for (auto iter : m_Gl1InputVector)
  {
    delete iter;
  }

  m_Gl1InputVector.clear();
}

int Fun4AllTpcTimeFrameInputManager::run(const int /*nevents*/)
{
  int iret = 0;
  if (m_Gl1Input)  // Gl1 first to get the reference
  {
    iret += FillGl1();
  }
  // std::cout << "size  m_InttRawHitMap: " <<  m_InttRawHitMap.size()
  // 	    << std::endl;
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

  int Fun4AllTpcTimeFrameInputManager::ResetEvent()
  {
    m_RefBCO = 0;
    return 0;
  }

  int Fun4AllTpcTimeFrameInputManager::PushBackEvents(const int /*i*/)
  {
    return 0;
  }

  int Fun4AllTpcTimeFrameInputManager::GetSyncObject(SyncObject * *mastersync)
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
      std::cout << "big problem" << std::endl;
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
      SingleGl1PoolInput * evtin)
  {
    assert(evtin);

    evtin->StreamingInputManager(this);
    if (!m_StreamingFlag)
    {
      evtin->CreateDSTNode(m_topNode);
    }
    m_Gl1Input = evtin;
  }

  void Fun4AllTpcTimeFrameInputManager::AddGl1RawHit(uint64_t bclk, Gl1Packet * hit)
  {
    if (Verbosity() > 1)
    {
      std::cout << "Adding gl1 hit to bclk 0x"
                << std::hex << bclk << std::dec << std::endl;
    }
    m_Gl1RawHitMap[bclk].Gl1RawHitVector.push_back(hit);
  }

  int Fun4AllTpcTimeFrameInputManager::FillGl1()
  {
    // unsigned int alldone = 0;
    for (auto iter : m_Gl1InputVector)
    {
      if (Verbosity() > 0)
      {
        std::cout << "Fun4AllTpcTimeFrameInputManager::FillGl1 - fill pool for " << iter->Name() << std::endl;
      }
      iter->FillPool();
      if (m_RunNumber == 0)
      {
        m_RunNumber = iter->RunNumber();
        SetRunNumber(m_RunNumber);
      }
      else
      {
        if (m_RunNumber != iter->RunNumber())
        {
          std::cout << PHWHERE << " Run Number mismatch, run is "
                    << m_RunNumber << ", " << iter->Name() << " reads "
                    << iter->RunNumber() << std::endl;
          std::cout << "You are likely reading files from different runs, do not do that" << std::endl;
          Print("INPUTFILES");
          gSystem->Exit(1);
          exit(1);
        }
      }
    }
    if (m_Gl1RawHitMap.empty())
    {
      std::cout << "Gl1RawHitMap is empty - we are done" << std::endl;
      return -1;
    }
    //    std::cout << "stashed gl1 BCOs: " << m_Gl1RawHitMap.size() << std::endl;
    Gl1Packet *gl1packet = findNode::getClass<Gl1Packet>(m_topNode, "GL1RAWHIT");
    //  std::cout << "before filling m_Gl1RawHitMap size: " <<  m_Gl1RawHitMap.size() << std::endl;
    for (auto gl1hititer : m_Gl1RawHitMap.begin()->second.Gl1RawHitVector)
    {
      if (Verbosity() > 1)
      {
        gl1hititer->identify();
      } 
        gl1packet->FillFrom(gl1hititer);
        MySyncManager()->CurrentEvent(gl1packet->getEvtSequence());
       m_RefBCO = gl1hititer->getBCO();
      m_RefBCO = m_RefBCO & 0xFFFFFFFFFFU;  // 40 bits (need to handle rollovers)
                                            //    std::cout << "BCOis " << std::hex << m_RefBCO << std::dec << std::endl;
    }
    for (auto iter : m_Gl1InputVector)
    {
      iter->CleanupUsedPackets(m_Gl1RawHitMap.begin()->first);
    }
    m_Gl1RawHitMap.begin()->second.Gl1RawHitVector.clear();
    m_Gl1RawHitMap.erase(m_Gl1RawHitMap.begin());

    return 0;
  }

  int Fun4AllTpcTimeFrameInputManager::FillTpc()
  {
    int iret = FillTpcPool();
    if (iret)
    {
      return iret;
    }
    TpcRawHitContainer *tpccont = findNode::getClass<TpcRawHitContainer>(m_topNode, "TPCRAWHIT");
    if (!tpccont)
    {
      /// if we set the node name and are running over single prdfs, thre is only one prdf in the vector
      tpccont = findNode::getClass<TpcRawHitContainer>(m_topNode, (*(m_TpcInputVector.begin()))->getHitContainerName());
      if (!tpccont)
      {
        std::cout << PHWHERE << "No tpc raw hit container found, exiting." << std::endl;
        gSystem->Exit(1);
        exit(1);
      }
    }
    //  std::cout << "before filling m_TpcRawHitMap size: " <<  m_TpcRawHitMap.size() << std::endl;
    uint64_t select_crossings = m_tpc_bco_range;
    if (m_RefBCO == 0)
    {
      m_RefBCO = m_TpcRawHitMap.begin()->first;
    }
    select_crossings += m_RefBCO;
    if (Verbosity() > 2)
    {
      std::cout << "select TPC crossings"
                << " from 0x" << std::hex << m_RefBCO - m_tpc_negative_bco
                << " to 0x" << select_crossings - m_tpc_negative_bco
                << std::dec << std::endl;
    }
    // m_TpcRawHitMap.empty() does not need to be checked here, FillTpcPool returns non zero
    // if this map is empty which is handled above

    while (m_TpcRawHitMap.begin()->first < m_RefBCO - m_tpc_negative_bco)
    {
      for (auto iter : m_TpcInputVector)
      {
        iter->CleanupUsedPackets(m_TpcRawHitMap.begin()->first);
      }
      m_TpcRawHitMap.begin()->second.TpcRawHitVector.clear();
      m_TpcRawHitMap.erase(m_TpcRawHitMap.begin());
      iret = FillTpcPool();
      if (iret)
      {
        return iret;
      }
    }

    unsigned int refbcobitshift = m_RefBCO & 0x3FU;
    h_refbco_tpc->Fill(refbcobitshift);
    bool allpackets = true;
    for (auto &p : m_TpcInputVector)
    {
      auto bcl_stack = p->BclkStackMap();
      int packetnum = 0;
      int histo_to_fill = (bcl_stack.begin()->first - 4000) / 10;

      for (auto &[packetid, bclset] : bcl_stack)
      {
        bool thispacket = false;
        for (auto &bcl : bclset)
        {
          auto diff = (m_RefBCO > bcl) ? m_RefBCO - bcl : bcl - m_RefBCO;
          if (diff < 256)
          {
            thispacket = true;
            h_gl1tagged_tpc[histo_to_fill][packetnum]->Fill(refbcobitshift);
          }
        }
        if (thispacket == false)
        {
          allpackets = false;
        }
        // we just want to erase anything that is well away from the current GL1
        // so make an arbitrary cut of 40000.
        p->clearPacketBClkStackMap(packetid, m_RefBCO - 40000);

        packetnum++;
      }
    }
    if (allpackets)
    {
      h_taggedAll_tpc->Fill(refbcobitshift);
    }
    // again m_TpcRawHitMap.empty() is handled by return of FillTpcPool()
    while (m_TpcRawHitMap.begin()->first <= select_crossings - m_tpc_negative_bco)
    {
      for (auto tpchititer : m_TpcRawHitMap.begin()->second.TpcRawHitVector)
      {
        if (Verbosity() > 1)
        {
          tpchititer->identify();
        }
        tpccont->AddHit(tpchititer);
      }
      for (auto iter : m_TpcInputVector)
      {
        iter->CleanupUsedPackets(m_TpcRawHitMap.begin()->first);
      }
      m_TpcRawHitMap.begin()->second.TpcRawHitVector.clear();
      m_TpcRawHitMap.erase(m_TpcRawHitMap.begin());
      if (m_TpcRawHitMap.empty())
      {
        break;
      }
    }
    if (Verbosity() > 0)
    {
      std::cout << "tpc container size: " << tpccont->get_nhits();
      std::cout << ", size  m_TpcRawHitMap: " << m_TpcRawHitMap.size()
                << std::endl;
    }
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
  
  int Fun4AllTpcTimeFrameInputManager::FillTpcPool()
  {
    uint64_t ref_bco_minus_range = 0;
    if (m_RefBCO > m_tpc_negative_bco)
    {
      ref_bco_minus_range = m_RefBCO - m_tpc_negative_bco;
    }

    for (auto iter : m_TpcInputVector)
    {
      if (Verbosity() > 0)
      {
        std::cout << "Fun4AllTpcTimeFrameInputManager::FillTpcPool - fill pool for " << iter->Name() << std::endl;
      }
      iter->FillPool(ref_bco_minus_range);
      if (m_RunNumber == 0)
      {
        m_RunNumber = iter->RunNumber();
        SetRunNumber(m_RunNumber);
      }
      else
      {
        if (m_RunNumber != iter->RunNumber())
        {
          std::cout << PHWHERE << " Run Number mismatch, run is "
                    << m_RunNumber << ", " << iter->Name() << " reads "
                    << iter->RunNumber() << std::endl;
          std::cout << "You are likely reading files from different runs, do not do that" << std::endl;
          Print("INPUTFILES");
          gSystem->Exit(1);
          exit(1);
        }
      }
    }
    if (m_TpcRawHitMap.empty())
    {
      std::cout << "TpcRawHitMap is empty - we are done" << std::endl;
      return -1;
    }
    return 0;
  }
 
  void Fun4AllTpcTimeFrameInputManager::createQAHistos()
  {
    auto hm = QAHistManagerDef::getHistoManager();
    assert(hm); 

    
  }

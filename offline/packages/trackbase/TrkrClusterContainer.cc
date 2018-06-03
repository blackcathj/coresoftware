#include "TrkrClusterContainer.h"
#include "TrkrClusterv1.h"

#include <cstdlib>

TrkrClusterContainer::TrkrClusterContainer()
{
}

void TrkrClusterContainer::Reset()
{
  while (m_clusmap.begin() != m_clusmap.end())
  {
    delete m_clusmap.begin()->second;
    m_clusmap.erase(m_clusmap.begin());
  }
  return;
}

void TrkrClusterContainer::identify(std::ostream& os) const
{
  os << "-----TrkrClusterContainer-----" << std::endl;
  ConstIterator iter;
  os << "Number of clusters: " << size() << std::endl;
  for (iter = m_clusmap.begin(); iter != m_clusmap.end(); ++iter)
  {
    os << "clus key 0x" << std::hex << iter->first << std::dec << std::endl;
    (iter->second)->identify();
  }
  os << "------------------------------" << std::endl;
  return;
}

TrkrClusterContainer::ConstIterator
TrkrClusterContainer::addCluster(TrkrCluster* newclus)
{
  TrkrDefs::cluskey key = newclus->getClusKey();
  if (m_clusmap.find(key) != m_clusmap.end())
  {
    TrkrDefUtil util;
    std::cout << "overwriting clus 0x" << std::hex << key << std::dec << std::endl;
    std::cout << "tracker ID: " << util.getTrkrId(key) << std::endl;
  }
  m_clusmap[key] = newclus;
  return m_clusmap.find(key);
}

TrkrClusterContainer::ConstIterator
TrkrClusterContainer::addClusterSpecifyKey(const TrkrDefs::cluskey key, TrkrCluster* newclus)
{
  if (m_clusmap.find(key) != m_clusmap.end())
  {
    std::cout << "TrkrClusterContainer::AddClusterSpecifyKey: duplicate key: " << key << " exiting now" << std::endl;
    exit(1);
  }
  newclus->setClusKey(key);
  m_clusmap[key] = newclus;
  return m_clusmap.find(key);
}

TrkrClusterContainer::ConstRange
TrkrClusterContainer::getClusters(const TrkrDefs::TrkrId trackerid) const
{
  // TrkrDefs::cluskey tmp = trackerid;
  // TrkrDefs::cluskey keylow = tmp << TrackerDefs::bitshift_trackerid;
  // TrkrDefs::cluskey keyup = ((tmp + 1)<< TrackerDefs::bitshift_trackerid) -1 ;
  //   std::cout << "keylow: 0x" << hex << keylow << dec << std::endl;
  //   std::cout << "keyup: 0x" << hex << keyup << dec << std::endl;

  TrkrDefUtil util;
  TrkrDefs::cluskey keylo = util.getClusKeyLo(trackerid);
  TrkrDefs::cluskey keyhi = util.getClusKeyHi(trackerid);

  ConstRange retpair;
  retpair.first = m_clusmap.lower_bound(keylo);
  retpair.second = m_clusmap.upper_bound(keyhi);
  return retpair;
}

TrkrClusterContainer::ConstRange
TrkrClusterContainer::getClusters(const TrkrDefs::TrkrId trackerid, const char layer) const
{
  TrkrDefUtil util;
  TrkrDefs::cluskey keylo = util.getClusKeyLo(trackerid, layer);
  TrkrDefs::cluskey keyhi = util.getClusKeyHi(trackerid, layer);

  ConstRange retpair;
  retpair.first = m_clusmap.lower_bound(keylo);
  retpair.second = m_clusmap.upper_bound(keyhi);
  return retpair;
}

TrkrClusterContainer::ConstRange
TrkrClusterContainer::getClusters(void) const
{
  return std::make_pair(m_clusmap.begin(), m_clusmap.end());
}

TrkrClusterContainer::Iterator
TrkrClusterContainer::findOrAddCluster(TrkrDefs::cluskey key)
{
  TrkrClusterContainer::Iterator it = m_clusmap.find(key);
  if (it == m_clusmap.end())
  {
    m_clusmap[key] = new TrkrClusterv1();
    it = m_clusmap.find(key);
    TrkrCluster* mclus = it->second;
    mclus->setClusKey(key);
  }
  return it;
}

TrkrCluster*
TrkrClusterContainer::findCluster(TrkrDefs::cluskey key)
{
  TrkrClusterContainer::Iterator it = m_clusmap.find(key);

  if (it != m_clusmap.end())
  {
    return it->second;
  }

  return 0;
}

#include "TpcTimeFrameBuilder.h"

#include <Event/packet.h>

#include <fun4all/Fun4AllHistoManager.h>
#include <fun4all/Fun4AllReturnCodes.h>
#include <qautils/QAHistManagerDef.h>

#include <TAxis.h>
#include <TH1.h>
#include <TH2.h>
#include <TNamed.h>
#include <TString.h>
#include <TVector3.h>

#include <stdint.h>
#include <string.h>
#include <cassert>

using namespace std;

TpcTimeFrameBuilder::TpcTimeFrameBuilder(const int packet_id)
  : m_packet_id(packet_id)
  , m_HistoPrefix("TpcTimeFrameBuilder" + to_string(packet_id))
{
  fee_data.resize(MAX_FEECOUNT);

  Fun4AllHistoManager *hm = QAHistManagerDef::getHistoManager();
  assert(hm);

  TH1D *h = new TH1D(TString(m_HistoPrefix.c_str()) + "_Normalization",  //
                     TString(m_HistoPrefix.c_str()) + " Normalization;Items;Count", 10, .5, 10.5);
  int i = 1;
  h->GetXaxis()->SetBinLabel(i++, "Packet");
  h->GetXaxis()->SetBinLabel(i++, "Lv1-Taggers");
  h->GetXaxis()->SetBinLabel(i++, "EnDat-Taggers");
  h->GetXaxis()->SetBinLabel(i++, "ChannelPackets");
  h->GetXaxis()->SetBinLabel(i++, "Waveforms");
  h->GetXaxis()->LabelsOption("v");
  hm->registerHisto(h);

  h = new TH1D(TString(m_HistoPrefix.c_str()) + "_PacketLength",  //
               TString(m_HistoPrefix.c_str()) + " PacketLength;PacketLength [16bit Words];Count", 1000, .5, 5e6);
  hm->registerHisto(h);
}

TpcTimeFrameBuilder::~TpcTimeFrameBuilder()
{
  for (auto itr = gtm_data.begin(); itr != gtm_data.end(); ++itr)
  {
    delete (*itr);
  }
  gtm_data.clear();
}

int TpcTimeFrameBuilder::ProcessPacket(Packet *packet)
{
  if (!packet)
  {
    cout << __PRETTY_FUNCTION__ << " Error : Invalid packet, doing nothing" << endl;
    assert(packet);
    return 0;
  }

  if (m_packet_id != packet->getIdentifier())
  {
    cout << __PRETTY_FUNCTION__ << " Error : mismatched packet with packet ID expectation of " << m_packet_id << ", but received";
    packet->identify();
    assert(m_packet_id == packet->getIdentifier());
    return 0;
  }

  if (m_verbosity)
  {
    cout << __PRETTY_FUNCTION__ << " : received packet ";
    packet->identify();
  }

  Fun4AllHistoManager *hm = QAHistManagerDef::getHistoManager();
  assert(hm);
  TH1D *h_norm = dynamic_cast<TH1D *>(hm->getHisto(
      m_HistoPrefix + "_Normalization"));
  assert(h_norm);
  h_norm->Fill("Event", 1);

  int l = packet->getDataLength() + 16;
  l *= 2;  // int to uint16
  vector<uint16_t> buffer(l);

  int l2 = 0;

  packet->fillIntArray(reinterpret_cast<int *>(buffer.data()), l, &l2, "DATA");
  l2 -= packet->getPadding();
  assert(l2 >= 0);
  unsigned int payload_length = 2 * (unsigned int) l2;  // int to uint16

  TH1D *h_PacketLength = dynamic_cast<TH1D *>(hm->getHisto(
      m_HistoPrefix + "_PacketLength"));
  assert(h_PacketLength);
  h_PacketLength->Fill(payload_length);

  // demultiplexer
  unsigned int index = 0;
  while (index < payload_length)
  {
    if ((buffer[index] & 0xFF00) == FEE_MAGIC_KEY)
    {
      unsigned int fee_id = buffer[index] & 0xff;
      ++index;
      if (fee_id < MAX_FEECOUNT)
      {
        for (unsigned int i = 0; i < DAM_DMA_WORD_BYTE_LENGTH - 1; i++)
        {
          // watch out for any data corruption
          if (index >= payload_length)
          {
            cout << __PRETTY_FUNCTION__ << " : Error : unexpected reach at payload length at position " << index << endl;
            break;
          }
          fee_data[fee_id].push_back(buffer[index++]);
        }
      }
      else
      {
        cout << __PRETTY_FUNCTION__ << " : Error : Invalid FEE ID " << fee_id << " at position " << index << endl;
        index += DAM_DMA_WORD_BYTE_LENGTH - 1;
      }
    }
    else if ((buffer[index] & 0xFF00) == GTM_MAGIC_KEY)
    {
      // watch out for any data corruption
      if (index + DAM_DMA_WORD_BYTE_LENGTH > payload_length)
      {
        cout << __PRETTY_FUNCTION__ << " : Error : unexpected reach at payload length at position " << index << endl;
        break;
      }
      decode_gtm_data(&buffer[index]);
      index += DAM_DMA_WORD_BYTE_LENGTH;
    }
    else
    {
      cout << __PRETTY_FUNCTION__ << " : Error : Unknown data type at position " << index << ": " << hex << buffer[index] << dec << endl;
      // not FEE data, e.g. GTM data or other stream, to be decoded
      index += DAM_DMA_WORD_BYTE_LENGTH ;
    }
  }

  return process_fee_data();
}

int TpcTimeFrameBuilder::process_fee_data()
{
  return Fun4AllReturnCodes::EVENT_OK;
}

int TpcTimeFrameBuilder::decode_gtm_data(unsigned short dat[16])
{
  unsigned char *gtm = reinterpret_cast<unsigned char *>(dat);
  gtm_payload *payload = new gtm_payload;

  payload->pkt_type = gtm[0] | ((unsigned short) gtm[1] << 8);
  if (payload->pkt_type != GTM_LVL1_ACCEPT_MAGIC_KEY && payload->pkt_type != GTM_ENDAT_MAGIC_KEY)
  {
    delete payload;
    return -1;
  }

  payload->is_lvl1 = payload->pkt_type == GTM_LVL1_ACCEPT_MAGIC_KEY;
  payload->is_endat = payload->pkt_type == GTM_ENDAT_MAGIC_KEY;

  payload->bco = ((unsigned long long) gtm[2] << 0) | ((unsigned long long) gtm[3] << 8) | ((unsigned long long) gtm[4] << 16) | ((unsigned long long) gtm[5] << 24) | ((unsigned long long) gtm[6] << 32) | (((unsigned long long) gtm[7]) << 40);
  payload->lvl1_count = ((unsigned int) gtm[8] << 0) | ((unsigned int) gtm[9] << 8) | ((unsigned int) gtm[10] << 16) | ((unsigned int) gtm[11] << 24);
  payload->endat_count = ((unsigned int) gtm[12] << 0) | ((unsigned int) gtm[13] << 8) | ((unsigned int) gtm[14] << 16) | ((unsigned int) gtm[15] << 24);
  payload->last_bco = ((unsigned long long) gtm[16] << 0) | ((unsigned long long) gtm[17] << 8) | ((unsigned long long) gtm[18] << 16) | ((unsigned long long) gtm[19] << 24) | ((unsigned long long) gtm[20] << 32) | (((unsigned long long) gtm[21]) << 40);
  payload->modebits = gtm[22];

  this->gtm_data.push_back(payload);

  return 0;
}


// unsigned short TpcTimeFrameBuilder::reverseBits(const unsigned short x) const
// {
//   unsigned short n = x;
//   n = ((n >> 1) & 0x55555555) | ((n << 1) & 0xaaaaaaaa);
//   n = ((n >> 2) & 0x33333333) | ((n << 2) & 0xcccccccc);
//   n = ((n >> 4) & 0x0f0f0f0f) | ((n << 4) & 0xf0f0f0f0);
//   n = ((n >> 8) & 0x00ff00ff) | ((n << 8) & 0xff00ff00);
//   // n = (n >> 16) & 0x0000ffff | (n << 16) & 0xffff0000;
//   return n;
// }

// unsigned short TpcTimeFrameBuilder::crc16(const unsigned int fee, const unsigned int index, const int l) const
// {
//   unsigned short crc = 0xffff;

//   for (int i = 0; i < l; i++)
//   {
//     unsigned short x = fee_data[fee][index + i];
//     //      cout << "in crc " << hex << x << dec << endl;
//     crc ^= reverseBits(x);
//     for (unsigned short k = 0; k < 16; k++)
//     {
//       crc = crc & 1 ? (crc >> 1) ^ 0xa001 : crc >> 1;
//     }
//   }
//   crc = reverseBits(crc);
//   return crc;
// }

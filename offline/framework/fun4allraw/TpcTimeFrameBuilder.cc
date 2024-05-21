#include "TpcTimeFrameBuilder.h"

#include <Event/oncsSubConstants.h>
#include <Event/packet.h>

#include <ffarawobjects/TpcRawHitv2.h>

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
#include <cassert>
#include <memory>
#include <string>

using namespace std;

TpcTimeFrameBuilder::TpcTimeFrameBuilder(const int packet_id)
  : m_packet_id(packet_id)
  , m_HistoPrefix("TpcTimeFrameBuilder" + to_string(packet_id))
{
  m_feeData.resize(MAX_FEECOUNT);

  Fun4AllHistoManager *hm = QAHistManagerDef::getHistoManager();
  assert(hm);

  TH1 *h = new TH1D(TString(m_HistoPrefix.c_str()) + "_Normalization",  //
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

  h = new TH2I(TString(m_HistoPrefix.c_str()) + "_FEE_DataStream_WordCount",  //
               TString(m_HistoPrefix.c_str()) +
                   " FEE Data Stream Word Count;FEE ID;Type;Count",
               MAX_FEECOUNT, -.5, MAX_FEECOUNT - .5, 2, .5, 10.5);
  i = 1;
  h->GetYaxis()->SetBinLabel(i++, "WordValid");
  h->GetYaxis()->SetBinLabel(i++, "WordSkipped");
  h->GetYaxis()->SetBinLabel(i++, "RawHit");
  h->GetYaxis()->SetBinLabel(i++, "HitFormatError");
  h->GetYaxis()->SetBinLabel(i++, "MissingLastHit");
  h->GetYaxis()->SetBinLabel(i++, "HitCRCError");
  hm->registerHisto(h);
}

TpcTimeFrameBuilder::~TpcTimeFrameBuilder()
{
  for (auto itr = gtm_data.begin(); itr != gtm_data.end(); ++itr)
  {
    delete (*itr);
  }
  gtm_data.clear();

  for (auto it = m_timeFrameMap.begin(); it != m_timeFrameMap.end(); ++it)
  {
    while (!it->second.empty())
    {
      delete it->second.back();
      it->second.pop_back();
    }
  }
}

int TpcTimeFrameBuilder::ProcessPacket(Packet *packet)
{
  if (!packet)
  {
    cout << __PRETTY_FUNCTION__ << " Error : Invalid packet, doing nothing" << endl;
    assert(packet);
    return 0;
  }

  if (packet->getHitFormat() != IDTPCFEEV3)
  {
    cout << __PRETTY_FUNCTION__ << " Error : expect packet format " << IDTPCFEEV3
         << " but received packet format " << packet->getHitFormat() << ":" << endl;
    packet->identify();
    assert(packet->getHitFormat() == IDTPCFEEV3);
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
          m_feeData[fee_id].push_back(buffer[index++]);
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
      index += DAM_DMA_WORD_BYTE_LENGTH;
    }
  }

  return process_fee_data();
}

int TpcTimeFrameBuilder::process_fee_data()
{
  Fun4AllHistoManager *hm = QAHistManagerDef::getHistoManager();
  assert(hm);
  TH2I *h_fee = dynamic_cast<TH2I *>(hm->getHisto(
      m_HistoPrefix + "_FEE_DataStream_WordCount"));
  assert(h_fee);

  for (unsigned int fee = 0; fee < MAX_FEECOUNT; fee++)
  {
    if (m_verbosity)
    {
      cout << __PRETTY_FUNCTION__ << " : processing FEE " << fee << " with " << m_feeData[fee].size() << " words" << endl;
    }

    assert(fee < m_feeData.size());
    auto &data_buffer = m_feeData[fee];

    while (HEADER_LENGTH < data_buffer.size())
    {
      // packet loop
      if (data_buffer[4] != FEE_PACKET_MAGIC_KEY_4)
      {
        if (m_verbosity > 1)
        {
          cout << __PRETTY_FUNCTION__ << " : Error : Invalid FEE magic key at position 4 " << data_buffer[4] << endl;
        }
        h_fee->Fill(fee, "WordSkipped", 1);
        data_buffer.pop_front();
        continue;
      }
      if (data_buffer[4] != FEE_PACKET_MAGIC_KEY_4)
      {
        break;  // insufficient data, break loop to process next FEE
      }

      //valid packet
      const uint16_t &pkt_length = data_buffer[0];                 // this is indeed the number of 10-bit words + 5 in this packet
      const uint16_t adc_length = data_buffer[0] - HEADER_LENGTH;  // this is indeed the number of 10-bit words in this packet
      const uint16_t sampa_address = (data_buffer[1] >> 5) & 0xf;
      const uint16_t sampa_channel = data_buffer[1] & 0x1f;
      const uint16_t channel = data_buffer[1] & 0x1ff;
      const uint16_t bx_timestamp = ((data_buffer[3] & 0x1ff) << 11) | ((data_buffer[2] & 0x3ff) << 1) | (data_buffer[1] >> 9);

      if (m_verbosity > 1)
      {
        cout << __PRETTY_FUNCTION__ << " : received data packet "
             << " pkt_length = " << pkt_length
             << " adc_length = " << adc_length
             << " sampa_address = " << sampa_address
             << " sampa_channel = " << sampa_channel
             << " channel = " << channel
             << " bx_timestamp = " << bx_timestamp

             << endl;
      }

      if (pkt_length < HEADER_LENGTH)
      {
        cout << __PRETTY_FUNCTION__ << " : Error : invalid data packet "
             << " pkt_length = " << pkt_length
             << " adc_length = " << adc_length
             << " sampa_address = " << sampa_address
             << " sampa_channel = " << sampa_channel
             << " channel = " << channel
             << " bx_timestamp = " << bx_timestamp

             << endl;
        data_buffer.pop_front();
        continue;
      }

      if (pkt_length >= data_buffer.size())
      {
        if (m_verbosity > 1)
        {
          cout << __PRETTY_FUNCTION__ << " : packet over boundary, skip to next load "
                                         " pkt_length = "
               << pkt_length
               << " data_buffer.size() = " << data_buffer.size()
               << endl;
        }

        continue;
      }

      //gtm_bco matching
      uint64_t gtm_bco = 0;

      // valid packet in the buffer, create a new hit
      TpcRawHit* hit = new TpcRawHitv2();
      hit->set_bco(bx_timestamp);
      hit->set_gtm_bco(gtm_bco);
      hit->set_packetid(m_packet_id);
      hit->set_fee(fee);
      hit->set_channel(channel);
      hit->set_sampaaddress(sampa_address);
      hit->set_sampachannel(sampa_channel);
      h_fee->Fill(fee, "RawHit", 1);

      const uint16_t data_crc = data_buffer[pkt_length - 1];
      const uint16_t calc_crc = crc16(fee, 0, pkt_length - 1);
      if (data_crc != calc_crc)
      {
        if (m_verbosity > 2)
        {
          cout << __PRETTY_FUNCTION__ << " : CRC error in FEE " << fee << " at position " << pkt_length - 1 << ": data_crc = " << data_crc << " calc_crc = " << calc_crc << endl;
        }
        h_fee->Fill(fee, "CRCError", 1);
        continue;
      }

      size_t pos = HEADER_LENGTH;
      // Format is (N sample) (start time), (1st sample)... (Nth sample)
      while (pos < pkt_length)
      {
        int nsamp = data_buffer[pos++];
        int start_t = data_buffer[pos++];

        if (pos + nsamp >= pkt_length)
        {
          if (m_verbosity > 2)
          {
            cout << __PRETTY_FUNCTION__ << ": WARNING : nsamp: " << nsamp
                 << ", pos: " << pos
                 << " > pkt_length: " << pkt_length << ", format error" << endl;
          }
          h_fee->Fill(fee, "HitFormatError", 1);
          break;
        }

        vector<uint16_t> waveform;
        for (int j = 0; j < nsamp; j++)
        {
          waveform.push_back(data_buffer[pos++]);

          // an exception to deal with the last sample that is missing in the current hit format
          if (pos +1 == pkt_length)
          {
            h_fee->Fill(fee, "MissingLastADC", 1);
            break;
          }
        }
        hit->add_wavelet(start_t, waveform);

        // an exception to deal with the last sample that is missing in the current hit format
        if (pos +1 == pkt_length ) break;
      }

      data_buffer.erase(data_buffer.begin(), data_buffer.begin() + pkt_length);
      h_fee->Fill(fee, "WordValid", pkt_length);

      m_timeFrameMap[gtm_bco].push_back(hit);
    }  //     while (HEADER_LENGTH < data_buffer.size())

  }  //   for (unsigned int fee = 0; fee < MAX_FEECOUNT; fee++)

  return Fun4AllReturnCodes::EVENT_OK;
}

int TpcTimeFrameBuilder::decode_gtm_data(uint16_t dat[16])
{
  unsigned char *gtm = reinterpret_cast<unsigned char *>(dat);
  gtm_payload *payload = new gtm_payload;

  payload->pkt_type = gtm[0] | ((uint16_t) gtm[1] << 8);
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

uint16_t TpcTimeFrameBuilder::reverseBits(const uint16_t x) const
{
  uint16_t n = x;
  n = ((n >> 1) & 0x55555555) | ((n << 1) & 0xaaaaaaaa);
  n = ((n >> 2) & 0x33333333) | ((n << 2) & 0xcccccccc);
  n = ((n >> 4) & 0x0f0f0f0f) | ((n << 4) & 0xf0f0f0f0);
  n = ((n >> 8) & 0x00ff00ff) | ((n << 8) & 0xff00ff00);
  // n = (n >> 16) & 0x0000ffff | (n << 16) & 0xffff0000;
  return n;
}

uint16_t TpcTimeFrameBuilder::crc16(
    const unsigned int fee, const unsigned int index, const int l) const
{
  uint16_t crc = 0xffff;

  for (int i = 0; i < l; i++)
  {
    uint16_t x = m_feeData[fee][index + i];
    crc ^= reverseBits(x);
    for (uint16_t k = 0; k < 16; k++)
    {
      crc = crc & 1 ? (crc >> 1) ^ 0xa001 : crc >> 1;
    }
  }
  crc = reverseBits(crc);
  return crc;
}

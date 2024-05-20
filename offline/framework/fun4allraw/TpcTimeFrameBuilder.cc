#include "TpcTimeFrameBuilder.h"

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
               MAX_FEECOUNT, -.5, MAX_FEECOUNT - .5, 2, .5, 2.5);

  h->GetYaxis()->SetBinLabel(1, "Valid");
  h->GetYaxis()->SetBinLabel(2, "Skipped");
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
      cout << __PRETTY_FUNCTION__ << " : processing FEE " << fee << " with " << fee_data[fee].size() << " words" << endl;
    }

    assert(fee < fee_data.size());
    auto &data_buffer = fee_data[fee];

    while (HEADER_LENGTH < data_buffer.size())
    {
      // packet loop
      if (data_buffer[4] != FEE_PACKET_MAGIC_KEY_4)
      {
        if (m_verbosity > 1)
        {
          cout << __PRETTY_FUNCTION__ << " : Error : Invalid FEE magic key at position 4 " << data_buffer[4] << endl;
        }
        h_fee->Fill(fee, 2);
        data_buffer.pop_front();
        continue;
      }
    }
    if (data_buffer[4] != FEE_PACKET_MAGIC_KEY_4)
    {
      continue;
    }

    //valid packet
    const uint16_t &pkt_length = data_buffer[0];     // this is indeed the number of 10-bit words + 5 in this packet
    const uint16_t adc_length = data_buffer[0] - 5;  // this is indeed the number of 10-bit words in this packet
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

    // valid packet in the buffer, create a new hit
    TpcRawHitv2 *hit = new TpcRawHitv2();
    hit->set_bco(bx_timestamp);
    hit->set_gtm_bco(0);
    hit->set_packetid(m_packet_id);
    hit->set_fee(fee);
    hit->set_channel(channel);
    hit->set_sampaaddress(sampa_address);
    hit->set_sampachannel(sampa_channel);

    // // Format is (N sample) (start time), (1st sample)... (Nth sample)
    // //for (int i = 0 ; i < header[0]-5 ; i++)
    // while (data_size_counter > 0)
    // {
    //   int nsamp = fee_data[ifee][pos++];
    //   int start_t = fee_data[ifee][pos++];

    //   data_size_counter -= 2;

    //   actual_data_size += nsamp;
    //   actual_data_size += 2;

    //   // We decided to suppress errors (May 13, 2024)
    //   //		  if(nsamp>data_size_counter){ cout<<"nsamp: "<<nsamp<<", size: "<<data_size_counter<<", format error"<<endl; break;}

    //   for (int j = 0; j < nsamp; j++)
    //   {
    //     if (start_t + j < 1024)
    //     {
    //       sw->waveform[start_t + j] = fee_data[ifee][pos++];
    //     }
    //     else
    //     {
    //       pos++;
    //     }
    //     //                   cout<<"data: "<< sw->waveform[start_t+j]<<endl;
    //     data_size_counter--;

    //     //
    //     // This line is inserted to accommodate the "wrong format issue", which is the
    //     // last sample from the data is missing. This issue should be fixed and eventually
    //     // the following two lines will be removed. Apr 30. by TS
    //     //
    //     if (data_size_counter == 1) break;
    //   }
    //   //                  cout<<"data_size_counter: "<<data_size_counter<<" "<<endl;
    //   if (data_size_counter == 1) break;
    // }

  }  //   for (unsigned int fee = 0; fee < MAX_FEECOUNT; fee++)

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

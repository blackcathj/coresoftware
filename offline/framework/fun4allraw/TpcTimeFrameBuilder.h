#ifndef __TpcTimeFrameBuilder_H__
#define __TpcTimeFrameBuilder_H__

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <boost/circular_buffer.hpp>

class Packet;
class TpcRawHit;

class TpcTimeFrameBuilder
{
 public:
  explicit TpcTimeFrameBuilder(const int packet_id);
  virtual ~TpcTimeFrameBuilder();

  int ProcessPacket(Packet *);

  void setVerbosity(const int i) { m_verbosity = i; }

 protected:
  // Length for the 256-bit wide Round Robin Multiplexer for the data stream
  static const size_t DAM_DMA_WORD_BYTE_LENGTH = 16;

  static const uint16_t FEE_PACKET_MAGIC_KEY_4 = 0xfe;

  static const uint16_t FEE_MAGIC_KEY = 0xba00;
  static const uint16_t GTM_MAGIC_KEY = 0xbb00;
  static const uint16_t GTM_LVL1_ACCEPT_MAGIC_KEY = 0xbbf0;
  static const uint16_t GTM_ENDAT_MAGIC_KEY = 0xbbf1;

  static const uint16_t MAX_FEECOUNT = 26;      // that many FEEs
  static const uint16_t MAX_CHANNELS = 8 * 32;  // that many channels per FEE
  static const uint16_t HEADER_LENGTH = 5;

  // uint16_t reverseBits(const uint16_t x) const;
  // uint16_t crc16(const uint32_t fee, const uint32_t index, const int l) const;

  int decode_gtm_data(uint16_t gtm[DAM_DMA_WORD_BYTE_LENGTH]);
  int process_fee_data();

  struct gtm_payload
  {
    uint16_t pkt_type;
    bool is_endat;
    bool is_lvl1;
    uint64_t bco;
    uint32_t lvl1_count;
    uint32_t endat_count;
    uint64_t last_bco;
    uint8_t modebits;
  };

  std::vector<gtm_payload *> gtm_data;

  std::vector<boost::circular_buffer_space_optimized<uint16_t>> fee_data;

  int m_verbosity = 0;
  int m_packet_id = 0;

  //! common prefix for QA histograms
  std::string m_HistoPrefix;

  std::map<uint64_t, std::vector<TpcRawHit *> > m_timeFrameMap;
};

#endif

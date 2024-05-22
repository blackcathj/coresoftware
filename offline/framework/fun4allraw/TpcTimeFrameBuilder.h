#ifndef __TpcTimeFrameBuilder_H__
#define __TpcTimeFrameBuilder_H__

#include <algorithm>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class Packet;
class TpcRawHit;

class TpcTimeFrameBuilder
{
 public:
  explicit TpcTimeFrameBuilder(const int packet_id);
  virtual ~TpcTimeFrameBuilder();

  int ProcessPacket(Packet *);
  bool isMoreDataRequired() const { return true; }

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

  uint16_t reverseBits(const uint16_t x) const;
  uint16_t crc16(const uint32_t fee, const uint32_t index, const int l) const;

  int decode_gtm_data(uint16_t gtm[DAM_DMA_WORD_BYTE_LENGTH]);
  int process_fee_data(unsigned int fee_id);

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

  std::vector<std::deque<uint16_t>> m_feeData;

  int m_verbosity = 0;
  int m_packet_id = 0;

  //! common prefix for QA histograms
  std::string m_HistoPrefix;

  //! GTM BCO -> TpcRawHit
  std::map<uint64_t, std::vector<TpcRawHit *>> m_timeFrameMap;

  // -------------------------
  // GTM Matcher
  // -------------------------

  //! GTM Matcher Strategy, order by reliability
  enum enu_gtmMatcherStrategy
  {
    //! if knowing nothing, simply matching FEE waveform to the last levl1 tagger
    kLastLv1Tagger = 0,
    //! tracking FEE BCO to GTM BCO sync
    kFEEWaveformBCOSync = 1,
    //! tracking FEE heartbeat to GTM BCO sync
    kFEEHeartBeatSync = 2
  };
  enu_gtmMatcherStrategy m_gtmMatcherStrategy = kLastLv1Tagger;
  uint64_t matchFEE2GTMBCO(uint16_t fee_bco);
  std::map<uint64_t, gtm_payload> m_gtmData;
};

#endif

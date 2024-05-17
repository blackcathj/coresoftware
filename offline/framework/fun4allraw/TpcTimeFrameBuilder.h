#ifndef __TpcTimeFrameBuilder_H__
#define __TpcTimeFrameBuilder_H__

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <vector>
#include <string>
#include <cstdint>

#include <boost/circular_buffer.hpp>

class Packet;

class TpcTimeFrameBuilder
{
 public:
  explicit TpcTimeFrameBuilder(const int packet_id);
  virtual ~TpcTimeFrameBuilder();

  int ProcessPacket(Packet *);



  void setVerbosity(const int i) { m_verbosity = i; }    

 protected: 
  // Length for the 256-bit wide Round Robin Multiplexer for the data stream
  static const unsigned int DAM_DMA_WORD_BYTE_LENGTH = 16;

  static const unsigned short MAGIC_KEY_0 = 0xfe;
  static const unsigned short MAGIC_KEY_1 = 0x00;

  static const unsigned short FEE_MAGIC_KEY = 0xba00;
  static const unsigned short GTM_MAGIC_KEY = 0xbb00;
  static const unsigned short GTM_LVL1_ACCEPT_MAGIC_KEY = 0xbbf0;
  static const unsigned short GTM_ENDAT_MAGIC_KEY = 0xbbf1;

  static const unsigned short MAX_FEECOUNT = 26;      // that many FEEs
  static const unsigned short MAX_CHANNELS = 8 * 32;  // that many channels per FEE
  static const unsigned short HEADER_LENGTH = 7;

  // unsigned short reverseBits(const unsigned short x) const;
  // unsigned short crc16(const unsigned int fee, const unsigned int index, const int l) const;

  int find_header(const unsigned int xx, const std::vector<unsigned short> &orig);
  int decode_gtm_data(unsigned short gtm[DAM_DMA_WORD_BYTE_LENGTH]);
  int process_fee_data();
 
  struct gtm_payload
  {
    unsigned short pkt_type;
    bool is_endat;
    bool is_lvl1;
    unsigned long long bco;
    unsigned int lvl1_count;
    unsigned int endat_count;
    unsigned long long last_bco;
    unsigned char modebits;
  };

  std::vector<gtm_payload *> gtm_data;

  std::vector<boost::circular_buffer_space_optimized<uint16_t>> fee_data;

  int m_verbosity = 0;
  int m_packet_id = 0;


  //! common prefix for QA histograms
  std::string m_HistoPrefix;

};

#endif

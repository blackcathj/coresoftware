#include "TpcRawHitv2.h"

#include <iostream>

TpcRawHitv2::TpcRawHitv2(TpcRawHit *tpchit)
{
  if (dynamic_cast<TpcRawHitv2 *>(tpchit))
  {
    *this = *dynamic_cast<TpcRawHitv2 *>(tpchit);
    return;
  }
  else
  {
    static bool once = true;
    if (once)
    {
      once = false;
      std::cout << "TpcRawHitv2::TpcRawHitv2(TpcRawHit *tpchit) - WARNING - "
                << "This constructor is casting a different version TpcRawHit to TpcRawHitv2 which can be inefficient. Please use TpcRawHitv2 or write a dedicated converter." << std::endl;
    }

    set_bco(tpchit->get_bco());
    set_gtm_bco(tpchit->get_gtm_bco());
    set_packetid(tpchit->get_packetid());
    set_fee(tpchit->get_fee());
    set_channel(tpchit->get_channel());
    set_sampaaddress(tpchit->get_sampaaddress());
    set_sampachannel(tpchit->get_sampachannel());

    wavelets = std::map<uint16_t, std::vector<uint16_t>>();
    std::vector<uint16_t> wavelet;
    for (size_t i = 0; i < tpchit->get_samples(); ++i)
    {
      wavelet.push_back(tpchit->get_adc(i));
    }

    wavelets.insert(std::make_pair(0, wavelet));
  }
}

void TpcRawHitv2::identify(std::ostream &os) const
{
  os << "TpcRawHitv2 (TPC with zero suppression): ";
  os << "BCO: 0x" << std::hex << bco << std::dec << std::endl;
  os << "packet id: " << packetid << std::endl;
}

void TpcRawHitv2::Clear(Option_t * /*unused*/)
{
  wavelets = std::map<uint16_t, std::vector<uint16_t>>();
}

uint16_t TpcRawHitv2::get_samples() const
{
  uint16_t samples = 0;

  if (wavelets.rbegin() != wavelets.rend())
  {
    samples = wavelets.rbegin()->first + wavelets.rbegin()->second.size();
  }

  return samples;
}

void TpcRawHitv2::set_samples(uint16_t const)
{
  std::cout << __PRETTY_FUNCTION__ << " : Error : deprecated function call" << std::endl;
}

uint16_t TpcRawHitv2::get_adc(size_t sample) const
{
  uint16_t adc = std::numeric_limits<uint16_t>::max();

  const auto wavelet = wavelets.lower_bound(sample);
  if (wavelet != wavelets.end())
  {
    assert(sample >= wavelet->first);
    size_t position_in_wavelet = sample - wavelet->first;

    if (position_in_wavelet < wavelet->second.size())
    {
      adc = wavelet->second[position_in_wavelet];
    }
  }

  return adc;
}

// cppcheck-suppress virtualCallInConstructor
void TpcRawHitv2::set_adc(size_t , uint16_t )
{
  std::cout << __PRETTY_FUNCTION__ << " : Error : deprecated function call" << std::endl;
}
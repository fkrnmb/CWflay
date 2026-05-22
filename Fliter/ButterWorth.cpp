#include "ButterWorth.hpp"




void LP_IIR_1::set_cutoff_frequency( const float sample_freq, const float cutoff_freq )
{
 if( sample_freq < 0.001f || cutoff_freq < 0.001f )
  return;
this->k = 2 * Pi * cutoff_freq / sample_freq;
}

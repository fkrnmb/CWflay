#pragma once
#include <cstdint>

#define Pi 3.1415926

//一阶低通
class LP_IIR_1
{
private:
   double k;
   double out;
public:
//设置截止频率
	void set_cutoff_frequency( const float sample_freq, const float cutoff_freq );

};


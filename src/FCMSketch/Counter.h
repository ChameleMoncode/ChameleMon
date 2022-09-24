#ifndef _FCMSKETCH_COUNTER_H_
#define _FCMSKETCH_COUNTER_H_

#include "../common_func.h"

template <int width_T, typename c_size_T>
class Counter
{
private:
	uint32_t max_range; // 254 for 8-bit
	uint32_t max_reg; // 255 for 8-bit
	vector<c_size_T> carray;

public:
	uint32_t num_empty = width_T;

	Counter(){ 
		carray.resize(width_T, 0);
		this->max_range = std::numeric_limits<c_size_T>::max() - 1;
		this->max_reg = std::numeric_limits<c_size_T>::max();
	}
	~Counter(){}

	uint32_t increment(int w, uint32_t f=1){
		uint32_t old_reg = (uint32_t)carray[w];
		uint32_t new_reg = (uint32_t)old_reg + f;
		new_reg = new_reg < max_reg ? new_reg : max_reg;
		carray[w] = (c_size_T)new_reg; // saturating
		if (old_reg == 0)
			num_empty -= 1;
		return new_reg;
	}

	uint32_t query(int w){
		uint32_t val_reg = (uint32_t)carray[w];
		/* return max_range of counter (e.g., 254 for 8-bit) */
		return val_reg < max_range ? val_reg : max_range;
	}

	uint32_t get_reg(int w){
		/* return register value (e.g., maximally 255 for 8-bit) */
		return (uint32_t)carray[w]; // real register value
	}

	bool check_overflow(int w)
	{
		if (carray[w] == this->max_reg)
			return true;
		return false;
	}

	uint32_t get_memory() {
		return width_T * sizeof(c_size_T);
	}
};

#endif

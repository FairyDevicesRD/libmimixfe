/*
 * @file utils.h
 * \~english
 * @brief 
 * \~japanese
 * @brief 
 * \~
 * @copyright Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 * @copyright Apache License, Version 2.0
 * @author Masato Fujino, created on: 2018/10/12
 *
 * Copyright 2018 Fairy Devices Inc. http://www.fairydevices.jp/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef MIMIXFE_EXAMPLES_UTILS_H_
#define MIMIXFE_EXAMPLES_UTILS_H_

#include <vector>
#include <cmath>

/**
 * @brief 角度の平均を計算する
 * @param [in] degrees 複数の入力角度[度]
 * @return 入力角度の平均
 */
int meanDegree(const std::vector<int>& degrees)
{
	float x=0;
	float y=0;
	int num = 0;
	for(size_t i=0;i<degrees.size();++i){
		if(degrees[i] == -1) continue;
		x += cos(degrees[i]*M_PI/180.0);
		y += sin(degrees[i]*M_PI/180.0);
		num++;
	}
	float mean = 180.0*atan2(y/static_cast<float>(num),x/static_cast<float>(num))/M_PI;
	if(mean < 0){
		mean+=360;
	}
	return mean;
}

/**
 * @brief 角度の差を計算する
 * @param [in] d1 角度[度]
 * @param [in] d2 角度[度]
 * @return |d1-d2|
 */
int diffDegree(int d1, int d2)
{
	int d = d1-d2;
	if(d > 180){
		return std::abs(static_cast<int>(d-360));
	}else if(d < -180){
		return std::abs(static_cast<int>(d+360));
	}else{
		return std::abs(d);
	}
}





#endif /* MIMIXFE_EXAMPLES_UTILS_H_ */

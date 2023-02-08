/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2023 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include <DarkHelp.hpp>


std::ostream & DarkHelp::operator<<(std::ostream & os, const DarkHelp::PredictionResult & pred)
{
	os	<< "\""			<< pred.name << "\""
		<< " #"			<< pred.best_class
		<< " prob="		<< pred.best_probability
		<< " x="		<< pred.rect.x
		<< " y="		<< pred.rect.y
		<< " w="		<< pred.rect.width
		<< " h="		<< pred.rect.height
		<< " tile="		<< pred.tile
		<< " entries="	<< pred.all_probabilities.size()
		;

	if (pred.all_probabilities.size() > 1)
	{
		os << " [";
		for (auto iter : pred.all_probabilities)
		{
			const auto & key = iter.first;
			const auto & val = iter.second;
			os << " " << key << "=" << val;
		}
		os << " ]";
	}

	return os;
}


std::ostream & DarkHelp::operator<<(std::ostream & os, const DarkHelp::PredictionResults & results)
{
	const size_t number_of_results = results.size();
	os << "prediction results: " << number_of_results;

	for (size_t idx = 0; idx < number_of_results; idx ++)
	{
		os << std::endl << "-> " << (idx+1) << "/" << number_of_results << ": ";
		operator<<(os, results.at(idx));
	}

	return os;
}

/* DarkHelp - C++ helper class for Darknet's C API.
 * Copyright 2019-2024 Stephane Charette <stephanecharette@gmail.com>
 * MIT license applies.  See "license.txt" for details.
 */

#include "DarkHelp.hpp"

int main(int argc, char * argv[])
{
	int rc = 0;

	try
	{
		if (argc != 5)
		{
			std::cout
				<< "Usage:" << std::endl
				<< argv[0] << " <filename.cfg> <filename.names> <filename.weights> <filename.jpg>" << std::endl;
			throw std::invalid_argument("wrong number of arguments");
		}

		std::string fn1 = argv[1];
		std::string fn2 = argv[2];
		std::string fn3 = argv[3];
		std::string image = argv[4];

		// Load the neural network.  The order of the 3 files does not matter, DarkHelp should figure out which file is which.
		DarkHelp::Config config(fn1, fn2, fn3);
		// Specifically disable redirection which gives us the full darknet output in case something goes wrong.
		config.redirect_darknet_output = false;
		// You can set many more DarkHelp config options here.  See the DarkHelp C++ API documentation for details.
		DarkHelp::NN nn(config);

		// Call on the neural network to process the given filename
		const auto results = nn.predict(image);

		// Display the results on the console.
		std::cout << image << " " << results << std::endl;
	}
	catch (const std::exception & e)
	{
		std::cout << e.what() << std::endl;
		rc = 1;
	}

	return rc;
}

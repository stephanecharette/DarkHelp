#include "DarkHelp.hpp"


int main(int argc, char * argv[])
{
	int rc = 1;

	try
	{
		if (argc != 5)
		{
			std::cout
				<< std::endl
				<< "Usage:" << std::endl
				<< "\t" << argv[0] << " <phrase> <cfg> <names> <weights>" << std::endl
				<< std::endl
				<< "The key phrase must come first, but the order of the 3 filenames does not matter." << std::endl
				<< "To disable obfuscation, use \"\" as the key phrase." << std::endl;

			throw std::invalid_argument("expected 4 parameters but found " + std::to_string(argc - 1));
		}

		const std::string phrase		= argv[1];
		std::filesystem::path cfg		= std::filesystem::canonical(argv[2]);
		std::filesystem::path names		= std::filesystem::canonical(argv[3]);
		std::filesystem::path weights	= std::filesystem::canonical(argv[4]);

		std::cout << "Combining neural network files into 1 file:" << std::endl;

		const auto output = DarkHelp::combine(phrase, cfg, names, weights);

		std::cout << "Results saved to:  " << output.string() << std::endl;

		rc = 0;
	}
	catch (const std::exception & e)
	{
		std::cout << std::endl << "ERROR:  " << e.what() << std::endl;
	}

	return rc;
}

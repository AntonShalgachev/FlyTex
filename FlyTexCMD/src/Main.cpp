#include "Main.h"

int main()
{
	FlyTexParser parser;
	parser.SetTemplateFile(TEMPLATE_NAME);
	parser.SetResolution(RESOLUTION);
	//parser.SetBackgroundColor("Transparent");
	//parser.SetForegroundColor("Black");

	std::cout << "Enter LaTeX expression >";

	std::string input;
	std::getline(std::cin, input);

	//bool success = parser.ParseToImage(input, "out.png");
	Error error = parser.ParseToClipboard(input);
	
	if(error == ERROR_OK)
	{
		std::cout << "Successful!" << std::endl;
	}
	else
	{
		std::cout << "Failed to parse! Error: " << ErrorString(error) << std::endl;
	}

	//std::system("pause");
	return 0;
}

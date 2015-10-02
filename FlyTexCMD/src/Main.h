#pragma once

#include "../../FlyTexLib/src/FlyTexParser.h"

#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "../Debug/FlyTexLib.lib")
#else
#pragma comment(lib, "../Release/FlyTexLib.lib")
#endif

#define TEMPLATE_NAME "template.tex"
#define RESOLUTION 1200

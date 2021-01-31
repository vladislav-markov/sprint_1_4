#include "read_input_functions.h"

std::string
ReadLine()
    {
        using namespace std;
        std::string s;
        getline( std::cin, s );
        return s;
    }

int
ReadLineWithNumber()
    {
        using namespace std;
        int result;
        std::cin >> result;
        ReadLine();
        return result;
    }



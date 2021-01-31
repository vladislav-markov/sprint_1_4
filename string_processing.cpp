#include "string_processing.h"

#include <algorithm>

std::vector< std::string >
SplitIntoWords( const std::string & text )
    {
        using namespace std;

        const std::vector< std::string > & words = [&text]()
            {
                std::string word;
                std::vector< std::string > v;
                for_each( text.begin(), text.end(), [&word, &v]( char c )
                    {
                        if( c == ' ' )
                            {
                                v.push_back( word );
                                word.clear();
                            }
                        else
                            {
                                word += c;
                            }
                    } );
                v.push_back( word );

                return v;
            }();

        return words;
    }

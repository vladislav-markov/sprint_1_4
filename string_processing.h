#pragma once

#include <algorithm>
#include <string>
#include <vector>

bool
IsValidWord( const std::string & word );

bool
IsValidHyphenWordMultiMinus( const std::string & word );

bool
IsValidHyphenWordSingleMinus( const std::string & word );

void
ValidateRawWord( const std::string & raw_word );

template < typename StopWordsCollection >
void
ValidateRawWordsCollection( const StopWordsCollection & input_stop_words )
    {
        std::for_each(
                input_stop_words.cbegin(),
                input_stop_words.cend(),
                []( const std::string word ) -> void
                    {
                        ValidateRawWord( word );
                    } );
    }


std::vector< std::string >
SplitIntoWords( const std::string & text );


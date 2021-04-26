#pragma once

#include <algorithm>
#include <string>
#include <vector>

bool
IsValidWord( const std::string & raw_word );

bool
IsValidMultiHyphenWord( const std::string & raw_word );

bool
IsValidSingleHyphenWord( const std::string & raw_word );

void
ValidateRawWord( const std::string & raw_word );

template < typename StopWordsCollection >
void
ValidateRawWordsCollection( const StopWordsCollection & raw_stop_words )
    {
        std::for_each(
                raw_stop_words.cbegin(),
                raw_stop_words.cend(),
                []( const std::string raw_word ) -> void
                    {
                        ValidateRawWord( raw_word );
                    } );
    }

std::vector< std::string >
SplitIntoWords( const std::string & raw_text );


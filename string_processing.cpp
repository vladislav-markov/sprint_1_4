#include <algorithm>
#include <stdexcept>

#include "string_processing.h"

bool
IsValidWord( const std::string & raw_word )
    {
        return
                std::none_of(
                        raw_word.cbegin(),
                        raw_word.cend(),
                        []( const char c )
                            {
                                return
                                        ( c >= '\0' )
                                        &&
                                        ( c < ' ' );
                            } );
    }

bool
IsValidMultiHyphenWord( const std::string & raw_word )
    {
        return
                !(
                    ( raw_word.size() > 1 )
                    &&
                    ( raw_word.at( 0 ) == '-' )
                    &&
                    ( raw_word.at( 1 ) == '-' )
                );
    }

bool
IsValidSingleHyphenWord( const std::string & raw_word )
    {
        return
                !(
                    ( raw_word.size() == 1 )
                    &&
                    ( raw_word.at( 0 ) == '-' )
                );
    }

void
ValidateRawWord( const std::string & raw_word )
    {
        if( !IsValidWord( raw_word ) )
            {
                using namespace std::string_literals;

                throw std::invalid_argument(
                        "В словах поискового запроса есть"s
                        +
                        " недопустимые символы с кодами от 0 до 31."s );
            }

        if( !IsValidSingleHyphenWord( raw_word ) )
            {
                using namespace std::string_literals;

                throw std::invalid_argument(
                        "Отсутствие текста после символа 'минус'."s );
            }

        if( !IsValidMultiHyphenWord( raw_word ) )
            {
                using namespace std::string_literals;

                throw std::invalid_argument(
                        "Наличие более чем одного минуса перед словами,"s
                        +
                        " которых не должно быть в искомых документах."s );
            }
    }

std::vector< std::string >
SplitIntoWords( const std::string & raw_text )
    {
        std::vector< std::string > words;

        std::string raw_word;

        for( const char c : raw_text )
            {
                if( c == ' ' )
                    {
                        ValidateRawWord( raw_word );
                        words.push_back( raw_word );
                        raw_word.clear();
                    }
                else
                    {
                        raw_word += c;
                    }
            }

        ValidateRawWord( raw_word );
        words.push_back( raw_word );

        return words;
    }


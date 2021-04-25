#include <algorithm>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "string_processing.h"

bool
IsValidWord( const std::string & word )
    {
        return
                std::none_of(
                        word.cbegin(),
                        word.cend(),
                        []( const char c )
                            {
                                return
                                        ( c >= '\0' )
                                        &&
                                        ( c < ' ' );
                            } );
    }

bool
IsValidHyphenWordMultiMinus( const std::string & word )
    {
        return
                !(
                    ( word.size() > 1 )
                    &&
                    ( word.at( 0 ) == '-' )
                    &&
                    ( word.at( 1 ) == '-' )
                );
    }

bool
IsValidHyphenWordSingleMinus( const std::string & word )
    {
        return
                !(
                    ( word.size() == 1 )
                    &&
                    ( word.at( 0 ) == '-' )
                );
    }

void
ValidateRawWord( const std::string & raw_word )
    {
        using namespace std::string_literals;

        if( !IsValidWord( raw_word ) )
            {
                throw std::invalid_argument(
                        "В словах поискового запроса есть"s
                        +
                        " недопустимые символы с кодами от 0 до 31."s );
            }

        if( !IsValidHyphenWordSingleMinus( raw_word ) )
            {
                throw std::invalid_argument(
                        "Отсутствие текста после символа 'минус'."s );
            }

        if( !IsValidHyphenWordMultiMinus( raw_word ) )
            {
                throw std::invalid_argument(
                        "Наличие более чем одного минуса перед словами,"s
                        +
                        " которых не должно быть в искомых документах."s );
            }
    }

std::vector< std::string >
SplitIntoWords( const std::string & text )
    {
        std::vector< std::string > words;

        std::string word;

        for( const char c : text )
            {
                if( c == ' ' )
                    {
                        ValidateRawWord( word );
                        words.push_back( word );
                        word.clear();
                    }
                else
                    {
                        word += c;
                    }
            }

        ValidateRawWord( word );
        words.push_back( word );

        return words;
    }


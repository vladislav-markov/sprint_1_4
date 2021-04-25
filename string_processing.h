#pragma once

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

std::vector< std::string >
SplitIntoWords( const std::string & text );


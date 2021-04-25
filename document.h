#pragma once

#include <iostream>

struct Document
    {
        Document() = default;

        Document(
                const int id_,
                const double relevance_,
                const int rating_ );

        int id = 0;
        double relevance = 0.0;
        int rating = 0;
    };

enum class DocumentStatus
    {
        ACTUAL,
        IRRELEVANT,
        BANNED,
        REMOVED,
    };

void
PrintDocument( const Document & document );

std::ostream &
operator<<( std::ostream & out, const Document & document );

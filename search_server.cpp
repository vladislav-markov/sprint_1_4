#include <algorithm>
#include <cmath>
#include <exception>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "search_server.h"
#include "document.h"
#include "string_processing.h"

SearchServer::SearchServer( const std::vector< std::string >( ) )
    {

    }

SearchServer::SearchServer( const std::string & stop_words_text )
    :
        SearchServer( SplitIntoWords( stop_words_text ) )
    {

    }

SearchServer::SearchServer( const std::string_view stop_words_text )
    :
        SearchServer(
                SplitIntoWords(
                        static_cast< std::string >( stop_words_text ) ) )
    {

    }

void
SearchServer::AddDocument(
        const int document_id,
        const std::string & document,
        const DocumentStatus status,
        const std::vector< int > & ratings )
    {
        if( document_id < 0 )
            {
                using namespace std::string_literals;

                throw std::invalid_argument(
                        "Попытка добавить документ с отрицательным id."s );
            }

        if( documents_.count( document_id ) )
            {
                using namespace std::string_literals;

                throw std::invalid_argument(
                        "Попытка добавить документ c id ранее добавленного документа."s );
            }

        std::vector< std::string > document_words = SplitIntoWords( document );

        std::vector< std::string > words;

        std::copy_if(
                std::make_move_iterator( document_words.begin() ),
                std::make_move_iterator( document_words.end() ),
                std::back_inserter( words ),
                [this]( const std::string & word )
                    {
                        return !IsStopWord( word );
                    } );

        const double inv_word_count = 1.0 / words.size();

        for( const std::string & word : words )
            {
                word_to_document_freqs_[ word ][ document_id ] += inv_word_count;
            }

        documents_.emplace(
                document_id,
                DocumentData
                    {
                        ComputeAverageRating( ratings ),
                        status,
                    } );
    }

std::vector< Document >
SearchServer::FindTopDocuments(
        const std::string & raw_query,
        const DocumentStatus status ) const
    {
        return
                FindTopDocuments(
                        raw_query,
                        [status](
                                const int document_id,
                                const DocumentStatus document_status,
                                const int rating )
                            {
                                return document_status == status;
                            } );
    }

int
SearchServer::GetDocumentCount() const
    {
        return documents_.size();
    }

int
SearchServer::GetDocumentId( const int index ) const
    {
        if(
                index < 0
                ||
                static_cast< std::size_t >( index ) >= documents_.size() )
            {
                using namespace std::string_literals;

                throw std::out_of_range(
                        "Индекс переданного документа выходит за пределы"s
                        +
                        " допустимого диапазона (0; количество документов)."s );
            }

        return std::next( documents_.cbegin(), index )->first;
    }

std::tuple< std::vector< std::string >, DocumentStatus >
SearchServer::MatchDocument(
        const std::string & raw_query,
        const int document_id ) const
    {
        const Query query = ParseQuery( raw_query );

        std::vector< std::string > matched_words;

        matched_words.reserve( query.plus_words.size() );

        for( const std::string & word : query.plus_words )
            {
                if( word_to_document_freqs_.count( word ) == 0)
                    {
                        continue;
                    }
                if( word_to_document_freqs_.at( word ).count( document_id ) )
                    {
                        matched_words.push_back( word );
                    }
            }
        for( const std::string & word : query.minus_words )
            {
                if( word_to_document_freqs_.count( word ) == 0 )
                    {
                        continue;
                    }
                if( word_to_document_freqs_.at( word ).count( document_id ) )
                    {
                        matched_words.clear();
                        break;
                    }
            }

        return
                {
                    matched_words,
                    documents_.at( document_id ).status
                };
    }

bool
SearchServer::IsStopWord( const std::string & word ) const
    {
        return stop_words_.count( word ) > 0;
    }

std::vector< std::string >
SearchServer::SplitIntoWordsNoStop( const std::string & text ) const
    {
        std::vector< std::string > all_words = SplitIntoWords( text );

        std::vector< std::string > result;

        std::copy_if(
                std::make_move_iterator( all_words.begin() ),
                std::make_move_iterator( all_words.end() ),
                std::back_inserter( result ),
                [this]( const std::string & word )
                    {
                        return !IsStopWord( word );
                    } );

        return result;
    }

int
SearchServer::ComputeAverageRating( const std::vector< int > & ratings )
    {
        if( ratings.empty() )
            {
                return 0;
            }

        const int ratings_sum = std::accumulate(
                ratings.cbegin(),
                ratings.cend(),
                0,
                []( const int lhs, const int rhs )
                    {
                        return lhs + rhs;
                    } );

        const int ratings_size = static_cast< int >( ratings.size() );

        return
                ratings_sum / ratings_size;
    }

SearchServer::QueryWord
SearchServer::ParseQueryWord( std::string text ) const
    {
        bool is_minus = false;

        if( text.at( 0 ) == '-' )
            {
                is_minus = true;
                text = text.substr( 1 );
            }

        return
                { text, is_minus, IsStopWord( text ) };
    }

SearchServer::Query
SearchServer::ParseQuery( const std::string & text ) const
    {
        SearchServer::Query result;

        for( const std::string & word : SplitIntoWords( text ) )
            {
                const SearchServer::QueryWord query_word = ParseQueryWord( word );

                if( !query_word.is_stop )
                    {
                        if( query_word.is_minus )
                            {
                                result.minus_words.insert( query_word.data );
                            }
                        else
                            {
                                result.plus_words.insert( query_word.data );
                            }
                    }
            }

        return result;
    }

double
SearchServer::ComputeWordInverseDocumentFreq(
        const std::string & word ) const
    {
        return
                std::log(
                            1.0
                            *
                            GetDocumentCount()
                            /
                            word_to_document_freqs_.at( word ).size() );
    }


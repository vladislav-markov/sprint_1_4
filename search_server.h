#pragma once

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "document.h"
#include "string_processing.h"

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
    {

        public:

            SearchServer() = delete;

            explicit SearchServer( const std::string & stop_words_text );

            template < typename StopWordsCollection >
            explicit SearchServer( const StopWordsCollection & input_stop_words )
                :
                    stop_words_( ParseStopWords( input_stop_words ) )
                {}

            void
            AddDocument(
                    const int document_id,
                    const std::string & document,
                    const DocumentStatus status,
                    const std::vector< int > & ratings );

            template < typename DocumentPredicate >
            std::vector< Document >
            FindTopDocuments(
                    const std::string & raw_query,
                    const DocumentPredicate document_predicate ) const
                {
                    const Query query = ParseQuery( raw_query );
                    auto matched_documents = FindAllDocuments( query, document_predicate );

                    std::sort(
                            matched_documents.begin(),
                            matched_documents.end(),
                            []( const Document & lhs, const Document & rhs )
                                {
                                    if( std::abs( lhs.relevance - rhs.relevance ) < 1e-6 )
                                        {
                                            return lhs.rating > rhs.rating;
                                        }
                                    else
                                        {
                                            return lhs.relevance > rhs.relevance;
                                        }
                                } );

                    if( matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT )
                        {
                            matched_documents.resize( MAX_RESULT_DOCUMENT_COUNT );
                        }

                    return matched_documents;
                }

            std::vector< Document >
            FindTopDocuments(
                    const std::string & raw_query,
                    const DocumentStatus status = DocumentStatus::ACTUAL ) const;

            int
            GetDocumentCount() const;

            int
            GetDocumentId( const int index ) const;

            std::tuple< std::vector< std::string >, DocumentStatus >
            MatchDocument(
                    const std::string & raw_query,
                    const int document_id ) const;

        private:

            struct DocumentData
                {
                    int rating;
                    DocumentStatus status;
                };

            const std::set< std::string > stop_words_;

            std::map< std::string, std::map< int, double > > word_to_document_freqs_;

            std::map< int, DocumentData > documents_;

            template < typename StopWordsCollection >
            std::set< std::string >
            ParseStopWords( const StopWordsCollection & input_stop_words ) const
                {
                    ValidateRawWordsCollection( input_stop_words );

                    return
                            {
                                input_stop_words.cbegin(),
                                input_stop_words.cend()
                            };
                }

            bool
            IsStopWord( const std::string & word ) const;

            std::vector< std::string >
            SplitIntoWordsNoStop( const std::string & text ) const;

            static int
            ComputeAverageRating( const std::vector< int > & ratings );

            struct QueryWord
                {
                    std::string data;
                    bool is_minus;
                    bool is_stop;
                };

            QueryWord
            ParseQueryWord( std::string text ) const;

            struct Query
                {
                    std::set< std::string > plus_words;
                    std::set< std::string > minus_words;
                };

            Query
            ParseQuery( const std::string & text ) const;

            double
            ComputeWordInverseDocumentFreq( const std::string & word ) const;

            template < typename DocumentPredicate >
            std::vector< Document >
            FindAllDocuments(
                    const Query & query,
                    const DocumentPredicate document_predicate ) const
                {
                    std::map< int, double > document_to_relevance;

                    for( const std::string & word : query.plus_words )
                        {
                            if( word_to_document_freqs_.count( word ) == 0 )
                                {
                                    continue;
                                }

                            const double inverse_document_freq = ComputeWordInverseDocumentFreq( word );

                            for( const auto & [ document_id, term_freq ] : word_to_document_freqs_.at( word ) )
                                {
                                    const auto & document_data = documents_.at( document_id );

                                    if( document_predicate(
                                            document_id,
                                            document_data.status,
                                            document_data.rating ) )
                                        {
                                            document_to_relevance[ document_id ]
                                                    += ( term_freq * inverse_document_freq );
                                        }
                                }
                        }

                    for( const std::string & word : query.minus_words )
                        {
                            if( word_to_document_freqs_.count( word ) == 0 )
                                {
                                    continue;
                                }

                            for( const auto & [ document_id, _ ] : word_to_document_freqs_.at( word ) )
                                {
                                    document_to_relevance.erase( document_id );
                                }
                        }

                    std::vector< Document > result;
                    for( const auto & [ document_id, relevance ] : document_to_relevance )
                        {
                            result.push_back(
                                                Document
                                                    {
                                                        document_id,
                                                        relevance,
                                                        documents_.at( document_id ).rating
                                                    } );
                        }

                    return result;
                }
    };


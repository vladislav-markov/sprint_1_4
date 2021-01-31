#pragma once
//Вставьте сюда своё решение из урока «‎Очередь запросов».‎

#include <algorithm>
#include <iostream>
#include <map>
#include <numeric>
#include <set>

#include "string_processing.h"

#include "document.h"

using namespace std::string_literals;

constexpr int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
    {
        public:

            template < typename StopWordsCollection >
            explicit SearchServer( const StopWordsCollection & input_stop_words )
                : stop_words_( ParseStopWords( input_stop_words ) )
                {
                    using namespace std;
                }

            explicit SearchServer( const std::string & stop_words_text )
                : SearchServer( SplitIntoWords( stop_words_text ) )
                {
                    using namespace std;
                }

            void
            AddDocument(
                    int document_id,
                    const std::string & document,
                    DocumentStatus status,
                    const std::vector< int > & ratings );

            template < typename DocumentPredicate >
            std::vector< Document >
            FindTopDocuments(
                    const std::string & raw_query,
                    DocumentPredicate document_predicate ) const
                {
                    using namespace std;

                    for( const std::string & raw_query_word : SplitIntoWords( raw_query ) )
                        {
                            if( !IsValidWord( raw_query_word ) )
                                {
                                    const std::string & message
                                            = "В словах поискового запроса "s
                                                + "есть недопустимые символы с кодами от 0 до 31"s;

                                    throw std::invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordSingleMinus( raw_query_word ) )
                                {
                                    const std::string & message
                                            = "Отсутствие текста после символа 'минус', например, 'пушистый -'."s;

                                    throw std::invalid_argument( message );
                                }
                            else if( !IsValidHyphenWordMultiMinus( raw_query_word ) )
                                {
                                    const std::string & message
                                            = "Наличие более чем одного минуса перед словами, "s
                                                + "которых не должно быть в искомых документах, например, "s
                                                + "'пушистый --кот'. В середине слов минусы разрешаются, "s
                                                + "например: 'иван-чай.'"s;

                                    throw std::invalid_argument( message );
                                }
                        }

                    const Query & query = ParseQuery( raw_query );
                    auto matched_documents = FindAllDocuments( query, document_predicate );

                    sort(
                            matched_documents.begin(),
                            matched_documents.end(),
                            []( const Document & lhs, const Document & rhs )
                                {
                                    if( abs( lhs.relevance - rhs.relevance ) < 1e-6 )
                                        {
                                            return
                                                    lhs.rating > rhs.rating;
                                        }
                                    else
                                        {
                                            return
                                                    lhs.relevance > rhs.relevance;
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
                    DocumentStatus status = DocumentStatus::ACTUAL ) const;

            int
            GetDocumentCount() const;

            int
            GetDocumentId( const int index ) const;

            std::tuple< std::vector< std::string >, DocumentStatus >
            MatchDocument(
                    const std::string & raw_query,
                    int document_id ) const;

        private:

            struct DocumentData
                {
                    int rating;
                    DocumentStatus status;
                };

            struct Query
                {
                    std::set< std::string > plus_words;
                    std::set< std::string > minus_words;
                };

            struct QueryWord;

            const std::set< std::string > stop_words_;
            std::map< std::string, std::map< int, double > > word_to_document_freqs_;
            std::map< int, DocumentData > documents_;

            static bool
            IsValidHyphenWordMultiMinus( const std::string & word );

            static bool
            IsValidHyphenWordSingleMinus( const std::string & word );

            static bool
            IsValidWord( const std::string & word );

            bool
            IsStopWord( const std::string & word ) const;

            [[nodiscard]]
            static int
            ComputeAverageRating( const std::vector< int > & ratings );

            [[nodiscard]]
            std::vector< std::string >
            SplitIntoWordsNoStop( const std::string & text ) const;

            [[nodiscard]]
            QueryWord
            ParseQueryWord( std::string text ) const;

            [[nodiscard]]
            Query
            ParseQuery( const std::string & text ) const;

            [[nodiscard]]
            double
            ComputeWordInverseDocumentFreq(
                    const std::string & word ) const;

            template < typename StopWordsCollection >
            [[nodiscard]]
            std::set< std::string >
            ParseStopWords( const StopWordsCollection & input_stop_words )
                {
                    using namespace std;

                    if( input_stop_words.empty() )
                        {
                            return {};
                        }

                    if( any_of(
                            input_stop_words.begin(),
                            input_stop_words.end(),
                            []( const std::string & word ){ return !IsValidWord( word ); } ) )
                        {
                            const std::string & message
                                    = "Любое из переданных стоп-слов содержит недопустимые "s
                                        + "символы, то есть символы с кодами от 0 до 31"s;

                            throw std::invalid_argument( message );
                        }

                    const std::string & string_stop_words
                            = accumulate(
                                    input_stop_words.begin(),
                                    input_stop_words.end(),
                                    ""s,
                                    []( std::string & lhs, const std::string & rhs )
                                        {
                                            return
                                                    lhs += ( rhs + ' ' );
                                        } );

                    const std::vector< std::string > & stop_words = SplitIntoWords( string_stop_words );

                    const std::set< std::string > & result = [&stop_words]()
                        {
                            std::set< std::string > r;

                            for_each(
                                    stop_words.begin(),
                                    stop_words.end(),
                                    [&r]( const std::string & word ){ r.insert( word ); } );

                            return r;
                        }();

                    return result;
                }

            template < typename DocumentPredicate >
            std::vector< Document >
            FindAllDocuments(
                    const Query & query,
                    DocumentPredicate document_predicate ) const
                {
                    using namespace std;

                    const std::map< int, double > &
                    document_to_relevance = [this, &query, document_predicate]()
                        {
                            std::map< int, double > result;

                            for( const std::string & word : query.plus_words )
                                {
                                    if( word_to_document_freqs_.count( word ) == 0 )
                                        {
                                            continue;
                                        }

                                    const double inverse_document_freq
                                            = ComputeWordInverseDocumentFreq( word );

                                    for( const auto & [ document_id, term_freq ]
                                            : word_to_document_freqs_.at( word ) )
                                        {
                                            const auto & document_data = documents_.at( document_id );

                                            if( document_predicate(
                                                    document_id,
                                                    document_data.status,
                                                    document_data.rating ) )
                                                {
                                                    result[ document_id ]
                                                                            += term_freq * inverse_document_freq;
                                                }
                                        }
                                }

                            for( const std::string & word : query.minus_words )
                                {
                                    if( word_to_document_freqs_.count( word ) == 0 )
                                        {
                                            continue;
                                        }

                                    for( const auto & [ document_id, _ ]
                                            : word_to_document_freqs_.at( word ) )
                                        {
                                            result.erase( document_id );
                                        }
                                }

                            return result;
                        }();

                    const std::vector< Document > & matched_documents
                            = [&document_to_relevance, this]()
                        {
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
                        }();

                    return matched_documents;
                }
    };


#pragma once

#include <cmath>
#include <deque>

#include "search_server.h"

using namespace std::string_literals;

class RequestQueue
    {
        public:

            explicit RequestQueue( const SearchServer & search_server );

            // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
            template < typename DocumentPredicate >
            std::vector< Document >
            AddFindRequest( const std::string & raw_query, DocumentPredicate document_predicate )
                {
                    using namespace std;

                    if( requests_.size() == sec_in_day_ )
                        {
                            requests_.pop_front();
                        }

                    const std::vector< Document > & found_docs = server_.FindTopDocuments( raw_query, document_predicate );

                    requests_.push_back( { !found_docs.empty() } );
                    return found_docs;
                }

            template < typename DocumentPredicate >
            std::vector< Document >
            AddFindRequest( const std::string & raw_query, DocumentStatus status )
                {
                    using namespace std;

                    return AddFindRequest(
                            raw_query,
                            [status]( int document_id, DocumentStatus document_status, int rating )
                                {
                                    return
                                            document_status == status;
                                } );
                }

            std::vector< Document >
            AddFindRequest( const std::string & raw_query );

            int
            GetNoResultRequests() const;

        private:

            struct QueryResult
                {
                    const bool has_result = false;
                };

            std::deque< QueryResult > requests_;
            const static int sec_in_day_ = 1440;
            const SearchServer & server_;
    };

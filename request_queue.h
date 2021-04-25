#pragma once

#include <deque>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

class RequestQueue
    {
        public:

            explicit RequestQueue( const SearchServer & search_server );

            template < typename DocumentPredicate >
            std::vector< Document >
            AddFindRequest( const std::string & raw_query, const DocumentPredicate document_predicate )
                {
                    if( requests_.size() == sec_in_day_ )
                        {
                            requests_.pop_front();
                        }

                    const std::vector< Document > found_docs = server_.FindTopDocuments( raw_query, document_predicate );

                    requests_.push_back( { !found_docs.empty() } );

                    return found_docs;
                }

            std::vector< Document >
            AddFindRequest( const std::string & raw_query, const DocumentStatus status );

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

            static constexpr const int sec_in_day_ = 1440;

            const SearchServer & server_;
    };



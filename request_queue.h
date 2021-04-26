#pragma once

#include <deque>

#include "search_server.h"

class RequestQueue
    {
        public:

            explicit RequestQueue( const SearchServer & search_server );

            template < typename SearchParameter = DocumentStatus >
            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query,
                    const SearchParameter search_parameter = DocumentStatus::ACTUAL )
                {
                    if( requests_.size() == sec_in_day_ )
                        {
                            requests_.pop_front();
                        }

                    const std::vector< Document > found_docs = server_.FindTopDocuments( raw_query, search_parameter );

                    requests_.push_back( { !found_docs.empty() } );

                    return found_docs;
                }

            int
            GetNoResultRequests() const;


        private:

            struct QueryResult
                {
                    const bool has_result = false;
                };

            std::deque< QueryResult > requests_;

            static constexpr int sec_in_day_ = 1440;

            const SearchServer & server_;
    };



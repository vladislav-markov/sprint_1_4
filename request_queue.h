#pragma once

#include <deque>

#include "search_server.h"

class RequestQueue
    {
        public:

            explicit RequestQueue( const SearchServer & search_server );

            template < typename SearchParameter >
            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query,
                    SearchParameter search_parameter )
                {
                    return AddFindRequest( raw_query, std::optional< SearchParameter >( search_parameter ) );
                }

            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query )
                {
                    return AddFindRequest( raw_query, []() -> std::optional< DocumentStatus > { return std::nullopt; }() );
                }

            int
            GetNoResultRequests() const;

        private:

            template < typename SearchParameter >
            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query,
                    std::optional< SearchParameter > search_parameter = std::nullopt )
                {
                    if( requests_.size() == sec_in_day_ )
                        {
                            requests_.pop_front();
                        }

                    std::vector< Document > found_docs;

                    if( search_parameter )
                        {
                            found_docs = server_.FindTopDocuments( raw_query, *search_parameter );
                        }
                    else
                        {
                            found_docs = server_.FindTopDocuments( raw_query );
                        }

                    requests_.push_back( { !found_docs.empty() } );

                    return found_docs;
                }

            struct QueryResult
                {
                    const bool has_result = false;
                };

            std::deque< QueryResult > requests_;

            static constexpr int sec_in_day_ = 1440;

            const SearchServer & server_;
    };



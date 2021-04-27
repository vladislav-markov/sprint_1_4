#pragma once

#include <deque>

#include "search_server.h"

class RequestQueue
    {
        public:

            explicit RequestQueue( const SearchServer & search_server );

            template < typename DocumentPredicate >
            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query,
                    const DocumentPredicate document_predicate )
                {
                    const std::vector< Document > found_documents
                                    = server_.FindTopDocuments( raw_query, document_predicate );

                    UpdateRequestsResultInfo( found_documents );

                    return found_documents;
                }

            std::vector< Document >
            AddFindRequest(
                    const std::string & raw_query,
                    const DocumentStatus status );

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

            static constexpr int sec_in_day_ = 1440;

            const SearchServer & server_;

            void
            UpdateRequestsResultInfo(
                    const std::vector< Document > & found_documents );
    };



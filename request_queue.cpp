#include "request_queue.h"

using namespace std::string_literals;

RequestQueue::RequestQueue( const SearchServer & search_server )
    : server_( search_server )
    {
using namespace std;
    }

std::vector< Document >
RequestQueue::AddFindRequest( const std::string & raw_query )
    {
using namespace std;
        return AddFindRequest(
                raw_query,
                []( int document_id, DocumentStatus document_status, int rating )
                    {
                        return
                                document_status == DocumentStatus::ACTUAL;
                    } );
    }

int
RequestQueue::GetNoResultRequests() const
    {
using namespace std;
        const int number_of_no_result_requests = std::count_if(
                requests_.begin(),
                requests_.end(),
                []( const QueryResult query ){ return !query.has_result; } );

        return number_of_no_result_requests;
    }

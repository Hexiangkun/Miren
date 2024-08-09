#include <iostream>
#include "test.pb.h"
#include <string>
using namespace std;

int main()
{
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    SearchResponse sr;
    
    SearchResponse_Result* result = sr.add_results();
    result->set_url("url ...");
    
    string str = sr.SerializeAsString();
    std::cout << str << endl;
    return 0;
}

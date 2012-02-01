#ifndef ROXLU_TWITTER_PARSER_JSONH
#define ROXLU_TWITTER_PARSER_JSONH

#include <string>
#include "../../../libs/curl/curl.h"
#include "../../../libs/jansson/jansson.h"
#include "../types/General.h"
#include "../types/Tweet.h"

using std::string;

namespace rtt = roxlu::twitter::type;

namespace roxlu {
namespace twitter {

class Twitter;

namespace parser {



class JSON {
public:
	JSON(roxlu::twitter::Twitter& twitter);
	~JSON();
	
	void parse(const string& line);
	bool parseStatus(json_t* root, rtt::Tweet& tweet);	
	bool parseDestroy(json_t* root, rtt::StatusDestroy& destroy);
	bool parseStreamEvent(json_t* root, rtt::StreamEvent& event);
	bool parseUser(json_t* root, rtt::User& user);
	
private:
	roxlu::twitter::Twitter& twitter;
	
}; // json

}}} // roxlu::twitter::parser
#endif

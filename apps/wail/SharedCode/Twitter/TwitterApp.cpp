#include "TwitterApp.h"
#include "ofxXmlSettings.h"

ofEvent<TwitterAppEvent> twitter_app_dispatcher;

TwitterApp::TwitterApp()
	:stream(twitter)
	,twitter_listener(*this)
{
}

TwitterApp::~TwitterApp() {
}

bool TwitterApp::initDB(){
	

	// TWITTER
	// --------
	printf("initializeing twitter\n");
	ofxXmlSettings userandpass;
	if(!userandpass.loadFile("userandpass.xml")){
		ofSystemAlertDialog("Creat an xml file called userandpass.xml in data/ and add <user>username</user><pass>password</pass> to it");
	}
//	twitter.setTwitterUsername("dewarshub");
//	twitter.setTwitterPassword("HUB2012hub#");
	twitter.setTwitterUsername(userandpass.getValue("user", "dewarshub"));
	twitter.setTwitterUsername(userandpass.getValue("pass", "HUB2012hub"));
	twitter.setConsumerKey("5cL1KRDQzcnGo8ZOaAz0g");
	twitter.setConsumerSecret("e4X9dtxkgmpkRlr9arhOfNe7tTezWad2bmCUNvPtBvQ");
	
	string token_file = ofToDataPath("twitter.txt", true);
	if(!twitter.loadTokens(token_file)) {
        string auth_url;
        twitter.requestToken(auth_url);
        twitter.handlePin(auth_url);
        twitter.accessToken();
        twitter.saveTokens(token_file);
	}
		

	// DATABASE 
	// --------
	if(!db.open("twitter.db")) {
		printf("Error: Cannot open twitter db.\n");
		//return false;
	}
	
	if(!db.createTables()) {
		printf("Error: Cannot create database.\n");
		//return false;
	}
	return true;
}

void TwitterApp::track(string trackingString){
	stream.track(trackingString);
}

bool TwitterApp::connect(){
	if(!stream.connect(URL_STREAM_USER)) {
		printf("Error: cannot connect to user stream.\n");
		return false;
	}
	
	return true;
	
}

void TwitterApp::addDefaultListener(){
	addCustomListener(twitter_listener);
}

void TwitterApp::addCustomListener(rt::IEventListener& listener){
	twitter.addEventListener(listener);
}

void TwitterApp::onNewSearchTerm(rtt::Tweet tweet, const string& term) {
	TwitterAppEvent ev(tweet, term);
	ofNotifyEvent(twitter_app_dispatcher, ev);
}

void TwitterApp::update() {	
	if(stream.isConnected()) {
		stream.update();
	}
}

// get the list of people to follow, separated by comma
bool TwitterApp::getFollowers(vector<string>& result) {
	return db.getFollowers(result);
}

//bool TwitterApp::init() {
	// What do you want to track?
	//	stream.track("ItsFunnyHow");
	//	stream.track("NeverTrustAGuyWho");
	//	stream.track("TolimaDay");
	//	stream.track("love");
	
	
//	// testing queries
//	// -------------------------------------------------------------------------
//	// get tweets with tag...
//	
//	vector<rtt::Tweet> tweets;
//	if(getTweetsWithTag("NeverTrustAGuyWho",10, tweets)) {
//		printf("found: %d!", tweets.size());
//	}
	
	/*
	// get tweets newer then 2400 seconds.
	
	int now = ofGetElapsedTimeMillis();
	vector<rtt::Tweet> tweets;
	if(getTweetsNewerThan(2400,100, tweets)) {
		printf("found: %zu!", tweets.size());
	}
	int done = ofGetElapsedTimeMillis();
	int diff = done-now;
	printf("diff: %d\n", diff);
	// -------------------------------------------------------------------------
	*/

//	if(!stream.connect(URL_STREAM_USER)) {
//		printf("Error: cannot connect to user stream.\n");
//		return false;
//	}
//	
//	return true;
	
//}

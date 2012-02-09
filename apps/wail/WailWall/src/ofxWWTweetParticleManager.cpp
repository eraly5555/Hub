/*
 *  ofxWWTweetManager.cpp
 *  WailWall
 *
 *  Created by James George on 1/30/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "ofxWWTweetParticleManager.h"
#include "ofxSimpleGuiToo.h"

ofxWWTweetParticleManager::ofxWWTweetParticleManager(){
	maxTweets = 100;
}

void ofxWWTweetParticleManager::setup(){
	
	if(!twitter.initDB()){
		printf("Error: cannot initialize twitter db.\n");
	}
	
	//twitter.addDefaultListener();
	twitter.addCustomListener(*this);
	
	// What do you want to track?
	// roxlu, 02/09: use the twitter_hashtags.txt file
	
	//twitter.track("love");
	//twitter.track("usa");
	//twitter.track("monkey");
	
	if(!twitter.connect()) {
		printf("Error: cannot connect to twitter stream.\n");
	}
	
	twitter.addListener(this, &ofxWWTweetParticleManager::onNewSearchTerm);
	canSelectSearchTerms = false;
}

void ofxWWTweetParticleManager::setupGui(){
	
	gui.addPage("Tweet Lifecycle");
	gui.addSlider("Tweet Font Size", fontSize, 5, 24);
	gui.addSlider("Word Wrap Length", wordWrapLength, 100, 300);
	gui.addSlider("Max Tweets", maxTweets, 5, 100);
	gui.addSlider("Start Fade Time", startFadeTime, 2, 10);
	gui.addSlider("Fade Duration", fadeDuration, 2, 10);
	gui.addToggle("Clear Tweets", clearTweets);

	
	gui.addPage("Tweet Appearance");
	gui.addSlider("Two Line Scale", twoLineScaleup, 1.0, 2.0);
	gui.addSlider("User Y Shift", userNameYOffset, -150, 150);
//	gui.addSlider("User X Padding", userNameXPad, -2, 10);
	gui.addSlider("Tweet Y Shift", tweetYOffset, -150, 150);
	gui.addSlider("Two Line Squish", twoLineSquish, .5, 1.0);
	gui.addSlider("Wall Repulsion Dist", wallRepulsionDistance, 0, 300);
	gui.addSlider("Wall Repulsion Atten", wallRepulsionAtten, 0, .5);
	gui.addSlider("Tweet Repulsion Dist", tweetRepulsionDistance, 0, 300);
	gui.addSlider("Tweet Repulsion Atten", tweetRepulsionAtten, 0, .5);
	gui.addSlider("Y Force Bias", yForceBias, 1., 10.);
	gui.addSlider("Fluid Force Scale", fluidForceScale, 1., 100.);
	
	gui.addPage("Search Terms");
//	gui.addToggle("Gen Fake Terms", generateFakeSearchTerms);
	gui.addSlider("Search Min Dist", searchTermMinDistance, 50, 500);
	gui.addSlider("Search Min Hold T", searchTermMinHoldTime, .5, 3.0);

	generateFakeSearchTerms = true;
}


void ofxWWTweetParticleManager::update(){
	
	if(clearTweets){
		tweets.clear();
		clearTweets = false;
	}
	
	//////////////////JUST FOR TESTING SEARCH TERMS BEFORE ITS IMPLEMENTED FULLY
	if(generateFakeSearchTerms){
		vector<string> fakeTerms;
		fakeTerms.push_back("DESIGN");
		fakeTerms.push_back("ECONOMICS");
		fakeTerms.push_back("INTERNET");
		fakeTerms.push_back("GENETICS");
		fakeTerms.push_back("EDUCATION");
		fakeTerms.push_back("SUSTAINABILITY");
		fakeTerms.push_back("OPENFRAMEWORKS");
		fakeTerms.push_back("GREEN");
		fakeTerms.push_back("INNOVATION");
		fakeTerms.push_back("GOOGLE");
		fakeTerms.push_back("INTELLIGENCE");
		
		twitter.populateFakeSearchTerms(fakeTerms);
		generateFakeSearchTerms = false;
	}
	//////////////////END TESTING
	
	if(!sharedFont.isLoaded() || fontSize != sharedFont.getSize() || int(fontSize*twoLineScaleup) != sharedLargeFont.getSize()){
		if(!sharedFont.loadFont("fonts/Tahoma.ttf", fontSize, true, true, false) ||
		   !sharedLargeFont.loadFont("fonts/Tahoma.ttf", fontSize*twoLineScaleup, true, true, false)){
			ofLogError("ofxWWTweetParticleManager::setup() -- couldn't load font!");
		}
	}
	
	twitter.update();
	
	//purge dead tweets
	for(int i = tweets.size()-1; i >= 0; i--){
		if(tweets[i].dead){
			tweets.erase(tweets.begin()+i);
		}
	}
	
	if(tweets.size() > maxTweets){
		tweets.resize(maxTweets);
	}
	
	//////////////
	searchTermSelected = false;
	selectedSearchTerm = -1;
	for(int i = 0; i < searchTerms.size(); i++){
		if(searchTerms[i].selected){
			searchTermSelected = true;
			selectedSearchTerm = i;
			if(!searchTerms[i].populated){
				//TODO: Run Real Query
				vector<rtt::Tweet> result;
				searchTweets.clear();
				twitter.getTweetsNewerThan(1000000, 25, result);
				cout << "query returned " << result.size() << " results " << endl;
				if(result.size() != 0){
					for(int t = 0; t < result.size(); t++){
						ofxWWTweetParticle tweetParticle = createParticleForTweet(result[t]);
						tweetParticle.isSearchTweet = true;
						searchTweets.push_back( tweetParticle );
					}
				}
				//THIS IS A HACK BC QUERY SOMETIMES RETURNS 0
				else {
					for(int t = 0; t < MIN(15, tweets.size()); t++){
						searchTweets.push_back( tweets[i] );
						searchTweets[i].createdTime = ofGetElapsedTimef() + ofRandom(2);
					}
				}
				searchTerms[i].populated = true;
			}
			break;
		}
	}
	
	if(!searchTermSelected && canSelectSearchTerms){	
		//Update Tweets           
		for(int i = 0; i < searchTerms.size(); i++){
			searchTerms[i].touchPresent = false;
			float closestDistance = 99999;
			map<int,KinectTouch>::iterator it;
			for(it = blobsRef->begin(); it != blobsRef->end(); it++){
				ofVec2f point = ofVec2f(it->second.x*simulationWidth, it->second.y*simulationHeight);
				float distance = point.distanceSquared(searchTerms[i].pos);
				if(distance < closestDistance){
					closestDistance = distance;
					searchTerms[i].closestPoint = point;
				}
				searchTerms[i].touchPresent = true;
			}
			searchTerms[i].update();
		}
	}	
	
	if(searchTermSelected){
		updateTweets(searchTweets, 1.0);
	}
	
	if(tweetLayerOpacity > 0){
		updateTweets(tweets, tweetLayerOpacity);
	}
}

float ofxWWTweetParticleManager::weightBetweenPoints(ofVec2f touch, float normalizedSize, ofVec2f tweet){
	float touchMid = normalizedSize*simulationHeight*touchSizeScale;
	return ofMap(touch.distance(tweet), touchMid-touchInfluenceFalloff/2., touchMid+touchInfluenceFalloff/2., 1.0, 0.0, true);
}

void ofxWWTweetParticleManager::updateTweets(vector<ofxWWTweetParticle>& tweetlist, float layerOpacity){
	
	///ANIMATE tweetlist
	//apply wall forces
	for(int i = 0; i < tweetlist.size(); i++){
		
		if (tweetlist[i].pos.x < wallRepulsionDistance) {
			tweetlist[i].force.x += (wallRepulsionDistance - tweetlist[i].pos.x) * wallRepulsionAtten * tweetlist[i].deathAttenuation;
		}
		if ((tweetlist[i].pos.x + tweetlist[i].totalWidth) > (simulationWidth-wallRepulsionDistance)) {
			tweetlist[i].force.x += ( (simulationWidth-wallRepulsionDistance) - (tweetlist[i].pos.x + tweetlist[i].totalWidth) ) * wallRepulsionAtten * tweetlist[i].deathAttenuation;
		}
		
		if (tweetlist[i].pos.y < wallRepulsionDistance) {
			tweetlist[i].force.y += (wallRepulsionDistance - tweetlist[i].pos.y) * wallRepulsionAtten * tweetlist[i].deathAttenuation;
		}
		if ((tweetlist[i].pos.y + tweetlist[i].totalHeight) > (simulationHeight-wallRepulsionDistance)) {
			tweetlist[i].force.y += ( (simulationHeight-wallRepulsionDistance)  - (tweetlist[i].pos.y + tweetlist[i].totalHeight)) * wallRepulsionAtten * tweetlist[i].deathAttenuation;
		}
	}
	
	//apply mutual repulsion
	for(int i = 0; i < tweetlist.size(); i++){
		tweetlist[i].force = ofVec2f(0,0);
		for(int j = 0; j < tweetlist.size(); j++){
			if(i != j){
				ofVec2f awayFromOther = (tweetlist[i].pos - tweetlist[j].pos);
				float distance = awayFromOther.length();
				awayFromOther.normalize();
				if(distance < tweetRepulsionDistance){
					ofVec2f force = (awayFromOther * ((tweetRepulsionDistance - distance) * tweetRepulsionAtten));
					force.y *= yForceBias;
					tweetlist[i].force += force;
				}
			}
		}
	}
		
	for(int i = 0; i < tweetlist.size(); i++){
		/*
		 printf("Fluidforcescale: %f\n", fluidForceScale);
		 printf("TweetLayerOpacity: %f\n", tweetLayerOpacity);
		 printf("DeathAttenuation: %f\n", tweetlist[i].deathAttenuation);
		 printf("tweetlist[i].force.x: %f\n", tweetlist[i].force.x);
		 printf("tweetlist[i].force.y: %f\n", tweetlist[i].force.y);
		 printf("--------------------------------------------------------\n");		
		 */
		fluidRef->applyForce( tweetlist[i].pos/ofVec2f(simulationWidth,simulationHeight), tweetlist[i].force/ofVec2f(simulationWidth,simulationHeight) * fluidForceScale * layerOpacity * tweetlist[i].deathAttenuation );
	}
	
	//hand reveal
	for(int i = 0; i < tweetlist.size(); i++){
		tweetlist[i].selectionWeight = 0;
		map<int,KinectTouch>::iterator it;
		for(it = blobsRef->begin(); it != blobsRef->end(); it++){
			if(tweetlist[i].selectionWeight < 1){
				ofVec2f touchpoint = ofVec2f(it->second.x*simulationWidth, it->second.y*simulationHeight);
				float weightBetween = weightBetweenPoints(ofVec2f(it->second.x*simulationWidth, it->second.y*simulationHeight), it->second.size, tweetlist[i].pos);
				tweetlist[i].selectionWeight = MIN(tweetlist[i].selectionWeight + weightBetween, 1.0);
			}
		}
	}
	
	for(int i = 0; i < tweetlist.size(); i++){
		tweetlist[i].update();
	}
	
}

void ofxWWTweetParticleManager::renderTweets(){
	if(searchTermSelected){
		for(int i = 0; i < searchTweets.size(); i++){
			searchTweets[i].draw();
		}		
	}
	else{
		for(int i = 0; i < tweets.size(); i++){
			tweets[i].draw();
		}
	}
}

void ofxWWTweetParticleManager::renderSearchTerms(){	
	for(int i = 0; i < searchTerms.size(); i++){
		if( !searchTermSelected || (searchTermSelected && i == selectedSearchTerm) ){
			searchTerms[i].draw();
		}
	}	
}

void ofxWWTweetParticleManager::renderCaustics(){
	ofPushStyle();
	for(int i = 0; i < tweets.size(); i++){
		for(int j = 0; j < tweets.size(); j++){
			if(j != i){
				float chanceOfSynapis = tweets[i].selectionWeight * tweets[j].selectionWeight;
				ofSetColor(ofColor::fromHex(0xd5712a, 100));
				if(ofRandomuf() + .5 < chanceOfSynapis){
					ofLine(tweets[i].pos, tweets[j].pos);
				}
			}
		}
	}
	
	ofPopStyle();	
}

void ofxWWTweetParticleManager::onStatusUpdate(const rtt::Tweet& tweet){
	if(twitter.containsBadWord(tweet.getText())) {
		//printf("[ censored ] : %s\n", tweet.getText().c_str());
		return;
	}
	printf(".\n");
	ofxWWTweetParticle tweetParticle = createParticleForTweet(tweet);
	tweets.push_back( tweetParticle );	
	
	//cout << "added tweet " << tweets.size() << " text is " << tweet.getText() << endl;
}

ofxWWTweetParticle ofxWWTweetParticleManager::createParticleForTweet(const rtt::Tweet& tweet){
	ofxWWTweetParticle tweetParticle;
	tweetParticle.manager = this;
	tweetParticle.pos = ofVec2f(ofRandom(simulationWidth-wordWrapLength), ofRandom(simulationHeight));
	tweetParticle.setTweet(tweet);
	return tweetParticle;
}
					
void ofxWWTweetParticleManager::onNewSearchTerm(TwitterAppEvent& event) {
	printf("Yay I got a new search term: %s\n", event.search_term.c_str());
	
	ofxWWSearchTerm searchTerm;
	int tries = 0;
	ofVec2f pos;
	bool validPosFound;
	do {
		validPosFound = true;
		pos = ofVec2f(ofRandom(simulationWidth-wordWrapLength/2.), ofRandom(simulationHeight));
		for(int i = 0; i < searchTerms.size(); i++){
			if (searchTerm.pos.distance(pos) < 400) {
				validPosFound = false;
				break;
			}
		}
	} while (!validPosFound && tries++ < 100);
	searchTerm.manager = this;
	searchTerm.term = event.search_term;
	searchTerm.pos = pos;
	
	searchTerms.push_back( searchTerm );	
	
	// @todo using ofSendMessage to test screenshots
	ofSendMessage("take_screenshot");
}

void ofxWWTweetParticleManager::onStatusDestroy(const rtt::StatusDestroy& destroy){
}

void ofxWWTweetParticleManager::onStreamEvent(const rtt::StreamEvent& event){
}

//JG HACKY WAY OUT, need to fix this
void ofxWWTweetParticleManager::resetTouches(){
	

	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].selected = false;
	}
	
	searchTweets.clear();
}

// roxlu 02/07
TwitterApp& ofxWWTweetParticleManager::getTwitterApp() {
	return twitter;
}

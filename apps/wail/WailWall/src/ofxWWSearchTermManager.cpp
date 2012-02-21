#include "ofxWWSearchTermManager.h"
#include "ofxWWTweetParticleManager.h"

ofxWWSearchTermManager::ofxWWSearchTermManager()
	:screenshot_userdata(NULL)
	,should_take_picture_on(FLT_MAX)
	,selectedSearchTermIndex(-1)
	,twitter(NULL)
{

	
}


void ofxWWSearchTermManager::setup(TwitterApp *twitter, ofxWWTweetParticleManager *parent) {
	this->parent = parent;
	this->twitter = twitter;
}
void ofxWWSearchTermManager::update() {
	
	if(parent->blobsRef->size() > 0){
		handleTouchSearch();
	}
	
	
	
	handleTweetSearch();
	
	
	
	
	doTouchInteraction();
	
	if(ofGetElapsedTimef() > should_take_picture_on) {
		screenshot_callback(screenshot_searchterm.user, screenshot_userdata);
		should_take_picture_on = FLT_MAX;
		
		// do not store this item in the queue.
		if(twitter != NULL) {
			twitter->setSearchTermAsUsed(
						 screenshot_searchterm.user
						,screenshot_searchterm.term
			);
		}
	}
}

void ofxWWSearchTermManager::doTouchInteraction() {
	
	
	
	doSearchTermSelectionTest();
	
	
	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].wallForceApplied = false;
		//LEFT WALL
		if (searchTerms[i].pos.x < parent->wallRepulsionDistance) {
			searchTerms[i].force.x += (parent->wallRepulsionDistance - searchTerms[i].pos.x) * parent->wallRepulsionAtten;
			searchTerms[i].wallForceApplied = true;
		}
		//RIGHT WALL
		if ((searchTerms[i].pos.x) > (parent->simulationWidth-parent->wallRepulsionDistance)) {
			searchTerms[i].force.x += ( (parent->simulationWidth-parent->wallRepulsionDistance) - (searchTerms[i].pos.x) ) * parent->wallRepulsionAtten;
			searchTerms[i].wallForceApplied = true;
		}
		//TOP
		if (searchTerms[i].pos.y < parent->wallRepulsionDistance) {
			searchTerms[i].force.y += (parent->wallRepulsionDistance - searchTerms[i].pos.y) * parent->wallRepulsionAtten;
			searchTerms[i].wallForceApplied = true;
		}		
		//BOTTOM
		if ((searchTerms[i].pos.y) > (parent->simulationHeight-parent->wallRepulsionDistance)) {
			searchTerms[i].force.y += ( (parent->simulationHeight-parent->wallRepulsionDistance)  - searchTerms[i].pos.y) * parent->wallRepulsionAtten;
			searchTerms[i].wallForceApplied = true;
		}
	}
	
	//now calculate repulsion forces
	float squaredMinDistance = searchTermRepulsionDistance*searchTermRepulsionDistance;
	for(int i = 0; i < searchTerms.size(); i++){
		if(searchTerms[i].wallForceApplied){
			//			continue;
		}
		for(int j = 0; j < searchTerms.size(); j++){
			if(i != j){
				float distanceSquared = searchTerms[i].pos.distanceSquared( searchTerms[j].pos );
				if(distanceSquared > squaredMinDistance){
					continue;
				}
				
				float distance = sqrtf(distanceSquared);
				ofVec2f awayFromOther = (searchTerms[i].pos - searchTerms[j].pos)/distance;
				ofVec2f force = (awayFromOther * ((searchTermRepulsionDistance - distance) * searchTermRepulsionAttenuation));
				searchTerms[i].force += force;			
			}
		}
	}
	
	for(int i = searchTerms.size()-1; i >= 0; i--){
		if(searchTerms[i].dead && ofGetElapsedTimef() - searchTerms[i].killedTime > searchTermFadeOutTime){
			searchTerms.erase(searchTerms.begin()+i);
		}
	}
	
	//apply a float and a wobble
	
	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].touchPresent = parent->blobsRef->size() != 0;
		//cout << "accumulated force is " << searchTerms[i].force  << endl;
		searchTerms[i].update();
	}
}

void ofxWWSearchTermManager::render() {
	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].draw();
	}
}
void ofxWWSearchTermManager::addSearchTerm(const string& user, const string& term, bool isUsed) {
	ofxWWSearchTerm search_term;
	search_term.pos = ofVec2f(ofRandom(parent->wallRepulsionDistance, parent->simulationWidth-parent->wallRepulsionDistance), 
							 ofRandom(parent->wallRepulsionDistance, parent->simulationHeight-parent->wallRepulsionDistance));
	search_term.manager = this;
	search_term.term = term;
	search_term.user = user;
	
	if(!isUsed) {
		printf(">>>>>>>>>>>>>>>>>>>>>>>> %s <<<<<<<<<<<<<<<<<<<<<<<<<<\n", search_term.term.c_str());
		incomingSearchTerms.push(search_term);
	}
	else {
		// When the search term is already used we directly add it to the queue.
		// This part is only executed when when we start the applications and 
		// retrieve the mentions from twitter.
		searchTerms.push_back(search_term);
	}

}


void ofxWWSearchTermManager::doSearchTermSelectionTest() {
	int len = searchTerms.size();
	
	// Check if there is a hand in the plinth.
	if(parent->blobsRef->empty()) {
		//printf("(1)\n");
		for(int i = 0; i < len; ++i) {
		
			// TODO add check on timer to keep the current selected term selected.
			searchTerms[i].fade();
		}
		return;
	}
	
	
	int closest_search_term_index = -1;
	float smallest_dist_sq = FLT_MAX;
	float in_range_dist = 0.1 * parent->simulationWidth;
	in_range_dist *= in_range_dist;
	//printf(">> %f\n", in_range_dist);
	
	for(int i = 0; i < len; ++i) {
		ofxWWSearchTerm& search_term = searchTerms.at(i);
		
		map<int, KinectTouch>::iterator kinect_iter = parent->blobsRef->begin();
		while(kinect_iter != parent->blobsRef->end()) {
			KinectTouch& touch = (kinect_iter->second);
			ofVec2f kinect_pos(touch.x * parent->simulationWidth, touch.y * parent->simulationHeight);
			
			// check if current search term is closer then then once handled so far.
			float dist_sq = kinect_pos.distanceSquared(search_term.pos);
			if(dist_sq < smallest_dist_sq && dist_sq <= in_range_dist) {
				smallest_dist_sq = dist_sq;
				closest_search_term_index = i;	
			}
			
			++kinect_iter;
		}
	}
	
	
	//printf("Smallest dist: %f - tweetLayerOpacity: %f\n", smallest_dist_sq, parent->tweetLayerOpacity);
	
	if(parent->tweetLayerOpacity >= 0.5) {
		//printf("(2,2,2,2,2,2,2,2,2,2,2	)\n");
		return;
	}
	else {
		
		if(closest_search_term_index == -1) {
			// not in range of a search term.
			
		}
		else {
			
			// in range of a search term
			ofxWWSearchTerm& selected_term = searchTerms[closest_search_term_index];
			
			// cleanup counters.
			for(int i = 0; i < len; ++i) {
				if(i == closest_search_term_index) {
					continue;
				}
				searchTerms[i].fade();
				searchTerms[i].selection_started_on = 0;
			}
			
			// start counter for selected
			if(selected_term.selection_started_on > 0)  {
				float now = ofGetElapsedTimeMillis();
				float selection_activate_on = selected_term.selection_started_on+1000;
				if(now > selection_activate_on) {
					// TODO add listener/event here
					selected_term.highlight();
				}
			}
			else {
				selected_term.selection_started_on = ofGetElapsedTimeMillis();
				
			}
			
		}
	}
	//printf("Found search term index: %d\n", closest_search_term_index);
}





void ofxWWSearchTermManager::deselectAllSearchTerms() {
	selectedSearchTermIndex = -1;
}

void ofxWWSearchTermManager::handleTouchSearch() {
	
	bool searchDebug = false;
	if(searchDebug) cout << "++++++ SEARCH DEBUG QUERY " << endl;
	
	int oldSelectedSearchTermIndex = selectedSearchTermIndex;
	selectedSearchTermIndex = -1;
	
	//look for a selected search term
	for(int i = 0; i < searchTerms.size(); i++){
		
		if(searchTerms[i].selected){
			
			selectedSearchTermIndex = i;
			//parent->shouldTriggerScreenshot = false;
			break;
		}
	}
	
	if(oldSelectedSearchTermIndex!=selectedSearchTermIndex) {
		if(selectedSearchTermIndex==-1) {
			parent->setCurrentProvider(parent->stream_provider);
		} else {
			parent->db_provider->fillWithTweetsWhichContainTerm(searchTerms[selectedSearchTermIndex].term);
			parent->setCurrentProvider(parent->db_provider);
		}
	}
	
}

void ofxWWSearchTermManager::handleTweetSearch(){
	
	
	// don't allow this to happen too often
	if(ofGetElapsedTimef() - lastSearchTermTime < tweetSearchMinWaitTime){
		return;
	}
	
	lastSearchTermTime = ofGetElapsedTimef();
	
	if(!incomingSearchTerms.empty()){
		ofxWWSearchTerm term = incomingSearchTerms.front();
		incomingSearchTerms.pop();
		
		//parent->shouldTriggerScreenshot = true;
		//selectedSearchTermIndex = searchTerms.size();
		searchTerms.push_back(term);
		
		// as soon as the term gets popped from the queue and added to the array 
		// which gets rendered we tell the system to make a screenshot after
		// X-seconds
		should_take_picture_on = ofGetElapsedTimef()+1.5;
		screenshot_searchterm = term; 
		
		if(searchTerms.size() > maxSearchTerms){
			searchTerms[0].dead = true;
			searchTerms[0].killedTime = ofGetElapsedTimef();
		}
	}
	
}

void ofxWWSearchTermManager::setScreenshotCallback(takeScreenshotCallback func, void* userdata) {
	screenshot_callback = func;
	screenshot_userdata = userdata;
}

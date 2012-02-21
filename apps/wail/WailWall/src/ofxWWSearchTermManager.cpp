//
//  ofxWWSearchTermManager.cpp
//  WailWall
//
//  Created by Joel Lewis on 20/02/2012.
//  Copyright (c) 2012 Hellicar & Lewis. All rights reserved.
//

#include "ofxWWSearchTermManager.h"
#include "ofxWWTweetParticleManager.h"

ofxWWSearchTermManager::ofxWWSearchTermManager() {
	selectedSearchTermIndex = -1;
	tweetSearchMinWaitTime = 1;
	fadeOutTime = 1;
	deselectionDelay = 2;
}


void ofxWWSearchTermManager::setup(TwitterApp *twitter, ofxWWTweetParticleManager *parent) {
	// for testing
	addListener(this);
	
	this->parent = parent;
	this->twitter = twitter;
	// Get previously received search terms.
	// -------------------------------------
	vector<TwitterSearchTerm*> stored_search_terms;
	if(twitter->getUnusedSearchTerms(stored_search_terms)) {
		vector<TwitterSearchTerm*>::iterator it = stored_search_terms.begin();
		while(it != stored_search_terms.end()) {
			addSearchTerm((*it)->user, (*it)->search_term);
			++it;
		}
	}
}

void ofxWWSearchTermManager::update() {
	
	if(parent->blobsRef->size() > 0){
		handleTouchSearch();
	}
	
	
	
	handleTweetSearch();
	
	
	
	
	doTouchInteraction();
	
	if(handRemovedTimer.done()) {
		handRemovedTimer.reset();
		// fade out the selected search term, (make sure you check it's in range)
		if(selectedSearchTermIndex>=0 && selectedSearchTermIndex<searchTerms.size()) {
			searchTerms[selectedSearchTermIndex].fade();
		}
		
		// send an event
		// don't send twice!
		if(lastSearchTermSelectionSentAsEvent!="") {
			lastSearchTermSelectionSentAsEvent = "";
			
			// and fire events
			for(int j = 0; j < listeners.size(); j++) {
				listeners[j]->onAllSearchTermsDeselected();
			}
		}
		selectedSearchTermIndex = -1;
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
	float squaredMinDistance = repulsionDistance*repulsionDistance;
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
				ofVec2f force = (awayFromOther * ((repulsionDistance - distance) * repulsionAttenuation));
				searchTerms[i].force += force;			
			}
		}
	}
	
	for(int i = 0; i < searchTerms.size(); i++){
		if(searchTerms[i].dead && ofGetElapsedTimef() - searchTerms[i].killedTime > fadeOutTime){
			
			// if the selectedSearchTermIndex is later in the list than i, decrement it as we're shuffling along one
			if(i<selectedSearchTermIndex) selectedSearchTermIndex--;
			// otherwise, if we're (god forbid) deleting the selected search term, do an unselect.
			else if(i==selectedSearchTermIndex) {
				selectedSearchTermIndex = -1;
				
				// don't send twice!
				if(lastSearchTermSelectionSentAsEvent!="") {
					lastSearchTermSelectionSentAsEvent = "";
				
					// and fire events
					for(int j = 0; j < listeners.size(); j++) {
						listeners[j]->onAllSearchTermsDeselected();
					}
				}
			}
			
			searchTerms.erase(searchTerms.begin()+i);
			i--;
			
		}
	}
	
	//apply a float and a wobble
	
	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].touchPresent = parent->blobsRef->size() != 0;
		//cout << "accumulated force is " << searchTerms[i].force  << endl;
		searchTerms[i].update();
	}
}


string lastSearchTerm;
float lastTime = 0;
void ofxWWSearchTermManager::onSearchTermSelected(const SearchTermSelectionInfo& term) {
	lastSearchTerm = term.term;
	lastTime = ofGetElapsedTimef();
}
void ofxWWSearchTermManager::onAllSearchTermsDeselected() {
	lastSearchTerm = "";
	lastTime = ofGetElapsedTimef();
}


void ofxWWSearchTermManager::render() {
	for(int i = 0; i < searchTerms.size(); i++){
		searchTerms[i].draw();
	}
	
	// debugging debounce
	float alpha = ofMap(ofGetElapsedTimef(), lastTime, lastTime + 2,  1.5, 0, true );
	
	if(alpha>0) {
		string msg = "";
		if(lastSearchTerm=="") {
			msg = "Deselected";
		} else {
			msg = "Selected " + lastSearchTerm;
		}
		glColor4f(1, 0, 0, alpha);
		parent->sharedSearchFont.drawString(msg, 1000, 1000);
		
	}
	
}
void ofxWWSearchTermManager::addSearchTerm(const string& user, const string& term) {
	ofxWWSearchTerm searchTerm;
	searchTerm.pos = ofVec2f(ofRandom(parent->wallRepulsionDistance, parent->simulationWidth-parent->wallRepulsionDistance), 
							 ofRandom(parent->wallRepulsionDistance, parent->simulationHeight-parent->wallRepulsionDistance));
	searchTerm.manager = this;
	searchTerm.term = term;
	searchTerm.user = user;
	printf(">>>>>>>>>>>>>>>>>>>>>>>> %s <<<<<<<<<<<<<<<<<<<<<<<<<<\n", searchTerm.term.c_str());
	incomingSearchTerms.push(searchTerm);
}


void ofxWWSearchTermManager::doSearchTermSelectionTest() {
	int len = searchTerms.size();
	
	// Check if there is a hand in the plinth.
	if(parent->blobsRef->empty()) {
		//printf("(1)\n");
		for(int i = 0; i < len; ++i) {
			
			// don't necessarily fade out the actual touched one
			// as the hand might be out temporarily.
			if(i!=selectedSearchTermIndex) {
				searchTerms[i].fade();
			}
			
		}
		
		return;
	}
	
	
	int closest_search_term_index = -1;
	float smallest_dist_sq = FLT_MAX;
	float in_range_dist = 0.1 * parent->simulationWidth;
	in_range_dist *= in_range_dist;
	printf(">> %f\n", in_range_dist);
	
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
	
	
	printf("Smallest dist: %f - tweetLayerOpacity: %f\n", smallest_dist_sq, parent->tweetLayerOpacity);
	
	if(parent->tweetLayerOpacity >= 0.5) {
		printf("(2,2,2,2,2,2,2,2,2,2,2	)\n");
		if(selectedSearchTermIndex>=0 && selectedSearchTermIndex<searchTerms.size()) {
			searchTerms[selectedSearchTermIndex].highlight();
		}
		return;
	}
	else {
		
		if(closest_search_term_index == -1) {
			
			// begin the timer for deselection if
			// not in range of a search term.
			handRemovedTimer.start(deselectionDelay);
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
					setSelectedSearchTerm(selected_term);
				}
			}
			else {
				selected_term.selection_started_on = ofGetElapsedTimeMillis();
				
			}
			
			
			
			
			
			
		}
	}
	printf("Found search term index: %d\n", closest_search_term_index);
}



void ofxWWSearchTermManager::setSelectedSearchTerm(ofxWWSearchTerm &searchTerm) {
	
	
	// first of all, check the searchterm exists in our array.
	for(int i = 0; i < searchTerms.size(); i++) {
		if(searchTerms[i].term==searchTerm.term && searchTerms[i].user == searchTerm.user) {
			
			// set the selected search term
			selectedSearchTermIndex = i;
			
			// make it highlighted
			searchTerm.highlight();
			
			// send an event
			if(lastSearchTermSelectionSentAsEvent!=searchTerm.term) {
				
				lastSearchTermSelectionSentAsEvent = searchTerm.term;
				SearchTermSelectionInfo info;
				info.term = searchTerm.term;
				info.position = searchTerm.pos;
				
				for(int j = 0; j < listeners.size(); j++) {
					listeners[j]->onSearchTermSelected(info);
				}
			}
			return;
		}
	}
	printf("Error! Can't find the search term '%s' in the list!\n", searchTerm.term.c_str());
	
}


void ofxWWSearchTermManager::touchUp() {
//	selectedSearchTermIndex = -1;
	handRemovedTimer.start(deselectionDelay);
}

void ofxWWSearchTermManager::touchDown() {
	handRemovedTimer.reset();
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
			parent->shouldTriggerScreenshot = false;
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
		
		parent->shouldTriggerScreenshot = true;
		//selectedSearchTermIndex = searchTerms.size();
		searchTerms.push_back(term);
		
		if(searchTerms.size() > maxSearchTerms){
			searchTerms[0].dead = true;
			searchTerms[0].killedTime = ofGetElapsedTimef();
		}
	}
	
}


void ofxWWSearchTermManager::addListener(SearchLayerListener *listener) {
	listeners.push_back(listener);
}
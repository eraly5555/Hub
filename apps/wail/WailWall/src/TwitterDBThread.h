#ifndef TWITTER_DB_THREADH
#define TWITTER_DB_THREADH

#include "Twitter.h"
#include "TwitterDB.h"
#include "ofMain.h"
#include <deque>

struct TwitterDBThreadTask {
	TwitterDBThreadTask(int task)
		:kind_of_task(task)
	{
	}
	
	~TwitterDBThreadTask() {
		//printf("~TwitterDBThreadTask()\n");
	}
	
	enum {
		 TASK_NONE
		,TASK_SEARCH
		,TASK_INSERT_TWEET
		,TASK_SEND_ITEM
		,TASK_SET_AS_SEND
	};
	int kind_of_task;
	
	
};

struct TwitterDBThreadTask_Search : public TwitterDBThreadTask {
	TwitterDBThreadTask_Search() 
		:TwitterDBThreadTask(TASK_SEARCH)
	{
	}
	
	// values for search task
	int search_older_than;
	int search_howmany;
	string search_term;
	
	void setSearchOlderThan(int d) { search_older_than = d; }
	void setSearchHowMany(int d) { search_howmany = d;}
	void setSearchTerm(const string& s) { search_term = s;}
};

struct TwitterDBThreadTask_InsertTweet : public TwitterDBThreadTask{
	TwitterDBThreadTask_InsertTweet(rtt::Tweet t)
		:tweet(t)
		,TwitterDBThreadTask(TASK_INSERT_TWEET)
	{
	}	
	rtt::Tweet tweet;
};

struct TwitterDBThreadTask_SetAsSend : public TwitterDBThreadTask {
	TwitterDBThreadTask_SetAsSend(int id)
		:id(id)
		,TwitterDBThreadTask(TASK_SET_AS_SEND)
	{
	}
	int id;
};

/*
struct TwitterDBThreadTask_SendItem {
	TwitterDBThreadTask_SendItem()	
		:TwitterDBThreadtask(TASK_SEND_ITEM)
	{
	}
	string username;
	string filename;
	int new_id;
}
*/

struct TweetTimestampSorter {
	bool operator()(const rtt::Tweet& a, const rtt::Tweet& b) {
		return a.created_at_timestamp > b.created_at_timestamp;
	}
};

class TwitterDBThread : public ofThread {
public:
	TwitterDBThread();
	~TwitterDBThread();
	virtual void threadedFunction();
	bool insertTweet(const rtt::Tweet& tweet);
	void addTask(TwitterDBThreadTask* task);
	
	// searching
	//bool getTweetsWithTag(const string& tag, int howMany, vector<rtt::Tweet>& result);
	//bool getTweetsNewerThan(int age, int howMany, vector<rtt::Tweet>& result);
	bool getTweetsWithSearchTerm(const string& q, int howMany);
	void getMoreTweetsMatchingCurrentSearchTerm();
	bool retrieveSearchResultsFromThread(vector<rtt::Tweet>& result);

	// queueing send messages
	bool insertSendQueueItem(const string& username, const string& filename, int& newID);
	bool setSendQueueItemAsSend(int queueID);
	bool getNextSendItemFromSendQueue(string& username, string& filename, int& id);
	
private:
	vector<rtt::Tweet> search_results;
	time_t search_for_older_then;
	string search_term;
	int search_howmany;
	
	std::deque<TwitterDBThreadTask*> tasks;
	TwitterDB db;
};

#endif
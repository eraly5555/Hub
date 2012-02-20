#ifndef TWEET_PROVIDER_STREAMH
#define TWEET_PROVIDER_STREAMH

#include "Twitter.h"
#include "TweetProvider.h"
#include "TwitterApp.h"
#include "IEventListener.h"

class TweetProviderStream : public TweetProvider, public roxlu::twitter::IEventListener {
public:
	TweetProviderStream(TwitterApp& app);
	virtual void update();

	void disableEvents();
	void enableEvents();
	
	virtual void onStatusUpdate(const rtt::Tweet& tweet);
	virtual void onStatusDestroy(const rtt::StatusDestroy& destroy);
	virtual void onStreamEvent(const rtt::StreamEvent& event);

private:

	TwitterApp& app;
	bool dispatch_events;
};
#endif
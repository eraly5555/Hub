/**
 * ofxMPMFluid.cpp
 * The MIT License (MIT)
 * Copyright (c) 2010 respective contributors
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *  
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 *****************************************
 * MPM FLuid Simulation Demo
 * OpenFrameworks version by Golan Levin
 * http://www.flong.com
 *
 * ofxAddon created by James George (@obviousjm)
 * http://www.jamesgeorge.org
 *
 * Original Java version:
 * http://grantkot.com/MPM/Liquid.html
 *
 * Flash version:
 * Copyright iunpin ( http://wonderfl.net/user/iunpin )
 * MIT License ( http://www.opensource.org/licenses/mit-license.php )
 * Downloaded from: http://wonderfl.net/c/6eu4
 *
 * Javascript version:
 * Copyright Stephen Sinclair (radarsat1) ( http://www.music.mcgill.ca/~sinclair )
 * MIT License ( http://www.opensource.org/licenses/mit-license.php )
 * Downloaded from: http://www.music.mcgill.ca/~sinclair/blog 
 */

#pragma once
#include "ofMain.h"
#include "ofxMPMParticle.h"
#include "ofxMPMNode.h"
#include "ofxMPMObstacle.h"
typedef struct {
	ofVec2f pos;
	ofVec2f vel;
} MPMForce;

class ofxMPMFluid : public ofThread {
  public:
	ofxMPMFluid();
	~ofxMPMFluid();
	
	void setup(int gridX, int gridY, int maxParticles);
	void threadedFunction();
	void update();
	
	void draw();
	void draw(float x, float y);
	void draw(float x, float y, float width, float height);
	void draw(ofRectangle rect);
		
	int getGridSizeX();
	int getGridSizeY();

	//normalized 0-1.0;
	void applyForce(ofVec2f position, ofVec2f velocity);
	
	//drawing params
	float scaleFactor;	
	float forceScale;
	float offsetX;
	float offsetY;
	
	int numParticles;
	float densitySetting;
	float stiffness;     
	float bulkViscosity; 
	float elasticity;    
	float viscosity;     
	float yieldRate;     
	bool  bGradient;	
	float gravity;
	bool  bDoObstacles;
	float smoothing;

	bool wrapHorizontal;
	bool wrapVertical;
	
	int historyLevel; //stores where each particle has been for rendering trails
	
	//if not null will be bound as a sample texture
	ofBaseHasTexture* sampleTexture;
	
	vector<ofxMPMParticle*>& getParticles();
	
  protected:
	float gridSizeX;
	float gridSizeY;
	
	float elapsed;

	vector<ofxMPMParticle*> particles;
	int maxNumParticles;
	vector< vector<ofxMPMNode*> > grid;
	vector< ofxMPMNode*> activeNodes;	
	int numActiveNodes;
	
	vector<ofxMPMObstacle*> obstacles;

	vector<MPMForce> forces;
	
	ofVec2f texCoordAtPos(float x, float y);
};
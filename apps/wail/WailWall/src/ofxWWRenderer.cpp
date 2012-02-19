/*
 *  ofxWWRenderer.cpp
 *  WailWell
 *
 *  Created by James George on 1/29/12.
 *  Copyright 2012 __MyCompanyName__. All rights reserved.
 *
 */

#include "ofxWWRenderer.h"
#include "ofxWebSimpleGuiToo.h"

void ofxWWRenderer::setup(int width, int height){
	targetWidth = width;
	targetHeight = height;

	accumbuf = 0;
	//anything that diffuses in liquid gets drawn into here
	accumulator[0].allocate(width, height, GL_RGBA);
	accumulator[1].allocate(width, height, GL_RGBA);
	//glowTarget.allocate(width, height, GL_RGBA);
	
	screenshotTarget.allocate(width/4, height/4, GL_RGB);
	
	//type layer
	//draw everything into here that needs to be warped
	warpMap.allocate(width/2, height/2, GL_RGB);
	
	//final buffer for comping it all together
	renderTarget.allocate(width, height, GL_RGB);
	
	gradientOverlay.allocate(width/8, height/8, GL_RGB);
	
	layer1Target.allocate(width, height, GL_RGBA);
	layer2Target.allocate(width, height, GL_RGBA);
	
	tweets.simulationWidth = width;
	tweets.simulationHeight = height;
	
	accumulator[0].begin();
	ofClear(0);
	accumulator[0].end();
	accumulator[1].begin();
	ofClear(0);
	accumulator[1].end();
	
//	fluid.setup(width/20.0,height/20.0,100000);
//	fluid.scaleFactor = 6.4;
	tweets.fluidRef = &fluid;
	tweets.blobsRef = blobs;
	
	caustics.shaderPath = "shaders/";
	caustics.setup(width/8, height/8);
	
	colorField.loadImage("images/color_palette.png");
	fluid.sampleTexture = &colorField;
	layerOneBackgroundA.loadImage("images/BGGradientA_layer1.png");
	layerTwoBackgroundA.loadImage("images/BGGradientA_layer2.png");
	layerOneBackgroundB.loadImage("images/BGGradientB_layer1.png");
	layerTwoBackgroundB.loadImage("images/BGGradientB_layer2.png");
	
	layer1Opacity = 1.0;
	
	alphaFade.load("shaders/alphafade");
	alphaFade.begin();
	alphaFade.setUniform1i("self", 0);
	alphaFade.end();
	
	permutationImage.loadImage("shaders/permtexture.png");
	noiseShader.load("shaders/noise");
	noiseShader.begin();
	noiseShader.setUniform1i("permTexture", 0);
	noiseShader.end();
	
	blurShader.load("shaders/gaussian_blur");
	blurShader.begin();
	blurShader.setUniform1i("tex0", 0);
	blurShader.end();
	
	warpShader.load("shaders/warp_distort");
	warpShader.begin();
	warpShader.setUniform1i("base",0);
	warpShader.setUniform1i("warp",1);
	warpShader.end();

	enableFluid = false;
	justDrawWarpTexture = false;

//	cout << "setting up tweets" << endl;
	tweets.setup(this);
	
	// roxlu: test screenshots
	ofAddListener(ofEvents.keyPressed, this, &ofxWWRenderer::keyPressed);
	test_screenshot = false;
}

void ofxWWRenderer::setupGui(){
	
	webGui.addPage("Interaction");
	webGui.addSlider("Layer Barrier Z", layerBarrierZ, .25, .75);
	webGui.addSlider("Layer Barrier Width", layerBarrierWidth, 0.05, .25);
	webGui.addSlider("Touch Scale", tweets.touchSizeScale, .5, 2.0);
	webGui.addSlider("Influence Width", tweets.touchInfluenceFalloff, 200, 5000);
	webGui.addToggle("Draw Touch Debug", drawTouchDebug);
	
	webGui.addPage("Caustics");
	webGui.addToggle("Enable Caustics", enableCaustics);
	webGui.addSlider("Delta", caustics.delta, .1, 1.0);
	webGui.addSlider("Drag", caustics.drag, .8, .999);
	webGui.addSlider("Light X", caustics.light.x, -1.0, 1.0);
	webGui.addSlider("Light Y", caustics.light.y, -1.0, 1.0);
	webGui.addSlider("Light Z", caustics.light.z, -1.0, 1.0);
	webGui.addToggle("Draw Debug Texture", drawCausticsDebug);
	
	webGui.addPage("Fluid");
	webGui.addToggle("Enable Fluid",	enableFluid);
	webGui.addSlider("Force Scale",		fluid.forceScale,	1.0, 200); 
	webGui.addSlider("Zoom",			fluid.scaleFactor,	1.0, 40.0); 	
	webGui.addSlider("Offset X",		fluid.offsetX,		-200.0, 0); 	
	webGui.addSlider("Offset Y",		fluid.offsetY,		-200.0, 0); 	
	webGui.addSlider("Particles",		fluid.numParticles,		1000, 100000); 
	webGui.addSlider("Density",			fluid.densitySetting,	0, 30.0);	
	webGui.addSlider("Stiffness",		fluid.stiffness,		0, 2.0);
	webGui.addSlider("Bulk Viscosity",	fluid.bulkViscosity,	0, 10.0);
	webGui.addSlider("Elasticity",		fluid.elasticity,		0, 4.0);
	webGui.addSlider("Viscosity",		fluid.viscosity,		0, 4.0);
	webGui.addSlider("Yield Rate",		fluid.yieldRate,		0, 2.0);
	webGui.addSlider("Gravity",			fluid.gravity,			0, 0.02);
	webGui.addSlider("Smoothing",		fluid.smoothing,		0, 3.0);
	webGui.addToggle("Do Obstacles",	fluid.bDoObstacles); 
	
	webGui.addPage("Shader");
	webGui.addToggle("Use Background A", useBackgroundSetA);
	webGui.addSlider("Blur Diffuse", blurAmount, 0, 10);
	webGui.addSlider("Clear Speed", clearSpeed, 0, 15);
	webGui.addSlider("Warp Amount", warpAmount, 0, 75);
	webGui.addSlider("Noise Scale X", noiseScale.x, 50, 500);
	webGui.addSlider("Noise Scale Y", noiseScale.y, 50, 500);
	webGui.addSlider("Noise Flow", noiseFlow, 0, 200);
	webGui.addSlider("Wobble Speed X", noiseWobbleSpeedX, 0, .2);
	webGui.addSlider("Noise Wobble Speed Y", noiseWobbleSpeedY, 0, .2);
	webGui.addSlider("Noise Wobble Amplitude X", noiseWobbleAmplitudeX, 0, 100);
	webGui.addSlider("Noise Wobble Amplitude Y", noiseWobbleAmplitudeY, 0, 100);
	webGui.addToggle("Just Draw Warp", justDrawWarpTexture);

	tweets.setupGui();
}

void ofxWWRenderer::update(){
	enableFluid = false;

	if(enableFluid){
		fluid.update();
	}
	
	if(enableCaustics){
		//TEMPORARY random force
		if(ofRandomuf() > .5)
			caustics.addDrop(ofRandomuf()*1024, ofRandomuf()*1024, 20, ofGetFrameNum() % 2 == 0 ? 0.02 : -0.02);
		
		caustics.update();
	}
	
	float maxTouchZ = 0;
	map<int,KinectTouch>::iterator it;
	for(it = blobs->begin(); it != blobs->end(); it++){
		if(it->second.z > maxTouchZ){
			maxTouchZ = it->second.z;
		}		
	}

	float targetOpacity = ofMap(maxTouchZ, layerBarrierZ-layerBarrierWidth/2, layerBarrierZ+layerBarrierWidth/2, 1.0, 0.0, true);

	layer1Opacity += (targetOpacity - layer1Opacity) * .1; //dampen
	tweets.tweetLayerOpacity = layer1Opacity;
	tweets.canSelectSearchTerms = maxTouchZ > layerBarrierZ;
		
	tweets.update();
}

void ofxWWRenderer::render(){

	//type
//	renderLayer1();
//	renderLayer2();
	
	//effects
	renderGradientOverlay();
	renderDynamics();
	renderWarpMap();
	
	//blit to main render target
	renderTarget.begin();
	ofClear(0);
	ofEnableAlphaBlending();
//	gradientOverlay.draw(0,0,targetWidth,targetHeight);
						 
	//BLIT DYNAMICS
	warpShader.begin();
	warpShader.setUniform1f("warpScale", warpAmount);
	//our shader uses two textures, the top layer and the alpha
	//we can load two textures into a shader using the multi texture coordinate extensions
	glActiveTexture(GL_TEXTURE0_ARB);
	accumulator[accumbuf].getTextureReference().bind();
	
	glActiveTexture(GL_TEXTURE1_ARB);
	caustics.getTextureReference().bind();
	
	//draw a quad the size of the frame
	glBegin(GL_QUADS);
	
	//move the mask around with the mouse by modifying the texture coordinates
	glMultiTexCoord2d(GL_TEXTURE0_ARB, 0, 0);
	glMultiTexCoord2d(GL_TEXTURE1_ARB, 0, 0);
	glVertex2f( 0, 0 );
	
	glMultiTexCoord2d(GL_TEXTURE0_ARB, accumulator[accumbuf].getWidth(), 0);
	glMultiTexCoord2d(GL_TEXTURE1_ARB, caustics.getTextureReference().getWidth(), 0);
	glVertex2f( targetWidth, 0 );
	
	glMultiTexCoord2d(GL_TEXTURE0_ARB, accumulator[accumbuf].getWidth(), accumulator[accumbuf].getHeight());
	glMultiTexCoord2d(GL_TEXTURE1_ARB, caustics.getTextureReference().getWidth(), caustics.getTextureReference().getHeight());
	glVertex2f( targetWidth, targetHeight );
	
	glMultiTexCoord2d(GL_TEXTURE0_ARB, 0, accumulator[accumbuf].getHeight());
	glMultiTexCoord2d(GL_TEXTURE1_ARB, 0, caustics.getTextureReference().getHeight());
	glVertex2f( 0, targetHeight );
	
	glEnd();
	
	//deactive and clean up
	glActiveTexture(GL_TEXTURE1_ARB);
	caustics.getTextureReference().unbind();
	
	glActiveTexture(GL_TEXTURE0_ARB);
	accumulator[accumbuf].getTextureReference().unbind();
	accumbuf = (accumbuf+1)%2;
	
	warpShader.end();
	
	tweets.renderTweets();	
	tweets.renderSearchTerms();
	
	
	//DEBUG
	if(justDrawWarpTexture){
		warpMap.draw(0,0);	
	}
	
	if(drawTouchDebug){ 
		ofPushStyle();
		ofNoFill();
		map<int,KinectTouch>::iterator it;
		for(it = blobs->begin(); it != blobs->end(); it++){
			ofVec2f touchCenter = ofVec2f( it->second.x*targetWidth, it->second.y*targetHeight );
			float maxTouchRadius = targetHeight*tweets.touchSizeScale;
			ofSetColor(255, 255, 255);
			ofCircle(touchCenter, it->second.size*maxTouchRadius);			
			ofSetColor(0, 255, 0);
			ofCircle(touchCenter, it->second.size*(maxTouchRadius - tweets.touchInfluenceFalloff/2));
			ofSetColor(255, 255, 0);
			ofCircle(touchCenter, it->second.size*(maxTouchRadius + tweets.touchInfluenceFalloff/2));
			
			ofPushMatrix();
			ofTranslate(touchCenter.x, touchCenter.y);
			ofScale(10, 10);
			ofDrawBitmapString("Z:"+ofToString(it->second.z,4), ofVec2f(0,0));
			ofPopMatrix();
//			for(int i = 0; i < tweets.tweets.size(); i++){
//				ofLine(touchCenter, tweets.tweets[i].pos);
//			}
		}
		ofPopStyle();
	}
	
	renderTarget.end();	
	
	// TODO: this is done in testApp now..
	if(test_screenshot) {
		//tweets.addCurrentRenderToScreenshotQueue();
		test_screenshot = false;
	}
}

//roxlu: testing screenshots
void ofxWWRenderer::keyPressed(ofKeyEventArgs& args) {
	if(args.key == '1') {
		test_screenshot = true;
	}
}

void ofxWWRenderer::renderDynamics(){
	//enableFluid = false; // TODO: remove
	
	accumulator[accumbuf].begin();
	
	ofClear(0);
	ofDisableAlphaBlending();
	
	ofPushStyle();
	ofSetColor(255, 255, 255);

	gradientOverlay.draw(0,0,targetWidth,targetHeight);
	
//	alphaFade.begin();
//	alphaFade.setUniform1f("fadeSpeed", tweets.causticFadeSpeed);
//	accumulator[(accumbuf+1)%2].draw(0,0); //this x offset causes the blur to cascade away
//	alphaFade.end();
	
//	blurShader.begin();
//	blurShader.setUniform2f("sampleOffset", 0, blurAmount);
//	accumulator[(accumbuf+1)%2].draw(7,0); //this x offset causes the blur to cascade away
//	blurShader.end();

//	blurShader.begin();
//	blurShader.setUniform2f("sampleOffset", blurAmount, 0);
//	accumulator[(accumbuf+1)%2].draw(3,0); //this x offset causes the blur to cascade away
//	blurShader.end();

	if(enableFluid){
		fluid.draw(0,0,targetWidth,targetHeight);
	}

	tweets.renderCaustics();
	
	ofPopStyle();
	
	accumulator[accumbuf].end();
}

void ofxWWRenderer::renderLayer1(){
	layer1Target.begin();
	ofClear(0,0,0,0);
	tweets.renderTweets();	
	layer1Target.end();
}

void ofxWWRenderer::renderLayer2(){
	layer2Target.begin();
	ofClear(0,0,0,0);
	
	tweets.renderSearchTerms();
	
	layer2Target.end();
}

void ofxWWRenderer::renderWarpMap(){
	
	warpMap.begin();
	ofClear(0);
	
	noiseShader.begin();
	
	noiseShader.setUniform1f("flow", -ofGetElapsedTimef()*noiseFlow);
	noiseShader.setUniform1f("wobbleX", sin(ofGetElapsedTimef()*noiseWobbleSpeedX) * noiseWobbleAmplitudeX);
	noiseShader.setUniform1f("wobbleY", sin(ofGetElapsedTimef()*noiseWobbleSpeedY) * noiseWobbleAmplitudeY);
	noiseShader.setUniform2f("scale", noiseScale.x, noiseScale.y);
	
	permutationImage.getTextureReference().bind();
	
	glBegin(GL_QUADS);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	
	glTexCoord2f(warpMap.getWidth(), 0);
	glVertex2f(warpMap.getWidth(), 0);

	glTexCoord2f(warpMap.getWidth(), warpMap.getHeight());
	glVertex2f(warpMap.getWidth(), warpMap.getHeight());

	glTexCoord2f(0, warpMap.getHeight());
	glVertex2f(0, warpMap.getHeight());

	permutationImage.getTextureReference().unbind();
	glEnd();
	
	noiseShader.end();
	
	warpMap.end();
}

void ofxWWRenderer::renderGradientOverlay(){
	ofPushStyle();
	gradientOverlay.begin();
	ofClear(0, 0, 0);

	if(drawCausticsDebug){
		caustics.getTextureReference().draw(0, 0, gradientOverlay.getWidth(), gradientOverlay.getHeight());	
	}
	else if(useBackgroundSetA){
		layerTwoBackgroundA.draw(0, 0, gradientOverlay.getWidth(), gradientOverlay.getHeight());
		ofSetColor(255, 255, 255, layer1Opacity*255);				
		layerOneBackgroundA.draw(0, 0, gradientOverlay.getWidth(), gradientOverlay.getHeight());
	}
	else{
		layerTwoBackgroundB.draw(0, 0, gradientOverlay.getWidth(), gradientOverlay.getHeight());
		ofSetColor(255, 255, 255, layer1Opacity*255);				
		layerOneBackgroundB.draw(0, 0, gradientOverlay.getWidth(), gradientOverlay.getHeight());
	}
	

	gradientOverlay.end();
	ofPopStyle();
	
}

ofFbo& ofxWWRenderer::getFbo(){
	return renderTarget;
}

void ofxWWRenderer::touchDown(const KinectTouch &touch) {
		
}

void ofxWWRenderer::touchMoved(const KinectTouch &touch) {
	fluid.applyForce(ofVec2f(touch.x, touch.y), ofVec2f(touch.vel.x, touch.vel.y));
}

void ofxWWRenderer::touchUp(const KinectTouch &touch) {
//	tweets.resetTouches();
}

ofxWWTweetParticleManager& ofxWWRenderer::getTweetManager() {
	return tweets;
}

void ofxWWRenderer::stopFluidThread(){
	fluid.stopThread(true);
}
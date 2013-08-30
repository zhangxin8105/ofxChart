/* Copyright (C) 2013 Sergey Yershov*/

/*07/23/2013 ::  Added proper support fox IOS FBO vs GL to allow "clipping" within stencils*/

#include "ofxChartAxisSet.h"

void ofxChartAxisSetBase::init()
{
    _container = ofPtr<ofxChartContainerAxisSet>(new ofxChartContainerAxisSet());
    CameraView.setDistance(0);
    _UseCamera = false;_UseFbo = false;
    ofColor defaultColor = ofColor(255,255,255);
    walls.left = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_LEFT,_container);
    walls.left->setColor(defaultColor/2);
    
    walls.back = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_BACK,_container);
    walls.back ->setColor(defaultColor);
   
    walls.right = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_RIGHT,_container);
    walls.right->setColor(defaultColor);
    
    walls.bottom = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_BOTTOM,_container);
    walls.bottom->setColor(defaultColor/3);
    
    walls.top = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_TOP,_container);
    walls.top->setColor(defaultColor);
    
    walls.front = new ofxChartAxisWall(OFX_CHART_AXIS_WALL_FRONT,_container);
    walls.front->setColor(defaultColor);
    
    
    ofAddListener(ofEvents().setup, this, &ofxChartAxisSetBase::setup);
    _isDynamicRange = true;
    
}




void ofxChartAxisSetBase::setup(ofEventArgs &data)
{
    
    
    ofFbo::Settings s;
    s.width = ofGetWidth();
    s.height = ofGetHeight();
#ifndef TARGET_OPENGLES
    s.useDepth = true;  //works in ver 0.74
    s.useStencil = true;
    s.depthStencilAsTexture = false;
#else
    s.useDepth = false;
    s.useStencil = true;
    s.depthStencilAsTexture = false;
#endif
    fbo.allocate(s);
    
    //ofAddListener(ofEvents().draw, this, &ofxChartAxisSetBase::draw);
    
}


void ofxChartAxisSetBase::setStaticRange(ofxChartVec3d min, ofxChartVec3d max)
{
    _isDynamicRange = false;
    //update range
    //update point size
    getContainer()->dataRange.min = min;
    getContainer()->dataRange.max = max;
    
    //BUG:
    //getContainer()->calculateDataPointSize(max-min) ;
    
    
    this->invalidate();
}


void ofxChartAxisSetBase::update()
{
    ofPtr<ofxChartContainerAxisSet> c = this->getContainer();
    
    //UPDATE RANGE OF ALL SERIES
    
    if(_isDynamicRange){
        ofxChartVec3d svRange;
        
        for (int i=0; i< this->getSeries().size(); i++) {
            ofxChartDataRange r = ofxChartAxisSetBase::getDataSeries(i)->getRange();
            //TODO: Value vs. Index range
            //calculate data point size
            if(i==0)
            {
                c->dataRange = r;
                svRange = ofxChartAxisSetBase::getDataSeries(i)->getShortestValueRange();
            }
            else
            {
                c->dataRange.min = c->getMin(c->dataRange.min, r.min);
                c->dataRange.max = c->getMax(c->dataRange.max, r.max);
                c->dataRange.size = MAX(c->dataRange.size, r.size);
                
                svRange = c->getMin(svRange, ofxChartAxisSetBase::getDataSeries(i)->getShortestValueRange()) ;
            }
            
                this->getDataSeries(i)->update();
            
        }
        //CALCULATE DATA POINT SIZE. TODO: calculate indexed size
        c->calculateDataPointSize(svRange);
    }
    c->isInvalid = false;
}




void ofxChartAxisSetBase::draw()
{
    if(!this->getContainer()->getVisible())
        return;
    
    //ofPushStyle();
    ofEnableAlphaBlending();
    if(this->getContainer()->isInvalid)
    {
        update();
        if(_UseFbo)
        {
            
            drawFBO();
    }
    }
    
    


        if(_UseFbo)
        {
            ofSetColor(255, 255, 255,255); //useful if accidently left out user-color
            glDisable(GL_DEPTH_TEST);
            fbo.draw(0,0);
        }
        else
            drawNonFBO();
        
    ofDisableAlphaBlending();
        //ofPopStyle();
    
//        glEnable(GL_BLEND);
//        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);//GL_ONE_MINUS_SRC_ALPHA);
//        glDisable(GL_BLEND);

    
}
void ofxChartAxisSetBase::drawNonFBO()
{
    
//    GLfloat modl[16];
//    glGetFloatv( GL_MODELVIEW_MATRIX, modl);
    
    ofPushMatrix();
    if(this->getUseCamera())
    {
        
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(OFX_CHART_DEFAULT_DEPTH_FUNC);

        this->CameraView.begin();
    }
    
     ofMultMatrix(this->getContainer()->getModelMatrix());
if(!this->getUseCamera())
{
    glScalef(1, -1, 1);
}
    
    //  ofLoadMatrix(ofMatrix4x4(modl));
    ofPushStyle();
    
    
    //DRAW WALLS
    ofxChartAxisSetBase::walls.back->draw();
    ofxChartAxisSetBase::walls.left->draw();
    ofxChartAxisSetBase::walls.bottom->draw();
    ofxChartAxisSetBase::walls.top->draw();
    ofxChartAxisSetBase::walls.right->draw();
   ofxChartAxisSetBase::walls.front->draw();
    
     glDepthMask(GL_TRUE);
    //DRAW AXES
    for(int axi=0; axi< axes.size(); axi++)
        axes[axi]->draw();

    
    
    
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glClear(GL_STENCIL_BUFFER_BIT);
    
    glEnable(GL_STENCIL_TEST); //Enable using the stencil buffer
    glColorMask(0, 0, 0, 0); //Disable drawing colors to the screen
    glDisable(GL_DEPTH_TEST); //Disable depth testing
    glStencilFunc(GL_ALWAYS, 1, 1); //Make the stencil test always pass
    //Make pixels in the stencil buffer be set to 1 when the stencil test passes
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    
    ofMesh m;
    ofxChartCreate3dRect(&m, ofVec3f().zero(), this->getContainer()->getDimensions(), ofColor(255,255,255));
    m.draw();

    glColorMask(1, 1, 1, 1); //Enable drawing colors to the screen
    //Make the stencil test pass only when the pixel is 1 in the stencil buffer
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP); //Make the stencil buffer not change
    
    
    
    //DRAW SERIES
    int seriesSize = _series.size();
    for(int i=0; i < seriesSize; i++)
    {
        ofxChartAxisSetBase::getDataSeries(i)->draw();
        
    }
    
    
    glDisable(GL_STENCIL_TEST);


    
    if(this->getUseCamera())
    {
               
        this->CameraView.end();
        glDisable(GL_DEPTH_TEST);
    }
    ofPopMatrix();

    
}



void ofxChartAxisSetBase::drawFBO()
{
    //DRAW TO FBO:
    ofPtr<ofxChartContainerAxisSet> c = this->getContainer();
    GLfloat modl[16];
    glGetFloatv( GL_MODELVIEW_MATRIX, modl); // TODO: get the current modelview matrix
    
    this->fbo.begin();
    
    ofPushStyle();
    
    ofLoadMatrix(ofMatrix4x4(modl));
    if(this->getUseCamera())
    {
        glClear( GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_ALWAYS);

        this->CameraView.begin(ofRectangle(0, ofGetHeight()-fbo.getHeight(),  fbo.getWidth(), fbo.getHeight()) );
            //glScalef(1, -1, 1);
      }

    ofMultMatrix(this->getContainer()->getModelMatrix());

    glClearColor(0, 0, 0, 0);
    glClear( GL_COLOR_BUFFER_BIT);
     
     
      
    //DRAW WALLS
    ofxChartAxisSetBase::walls.back->draw();
    ofxChartAxisSetBase::walls.left->draw();
    ofxChartAxisSetBase::walls.bottom->draw();
    ofxChartAxisSetBase::walls.top->draw();
    ofxChartAxisSetBase::walls.right->draw();
    ofxChartAxisSetBase::walls.front->draw();

    glDepthMask(GL_TRUE);
    //DRAW AXES
    for(int axi=0; axi< axes.size(); axi++)
        axes[axi]->draw();

    
    
    glClear(GL_STENCIL_BUFFER_BIT);
    glEnable(GL_STENCIL_TEST);
    glColorMask(0, 0, 0, 0);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
    
    ofSetHexColor(0xffffff);
    //draw rect mesh
    {
        ofMesh m;
        ofxChartCreate3dRect(&m, ofVec3f().zero(), this->getContainer()->getDimensions(), ofColor(255,255,255));
        m.draw();
    }
    

    

    
    glColorMask(1, 1, 1, 1);
    glStencilFunc(GL_EQUAL, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    
    
    ofPushMatrix();
    int seriesSize = _series.size();
    for(int i=0; i < seriesSize; i++)
    {
        this->getDataSeries(i)->draw();
        
    }
    ofPopMatrix();



  
    
    glDisable(GL_STENCIL_TEST);
    
    ofPopStyle();

    if(this->getUseCamera())
    {
        this->CameraView.end();
        glDisable(GL_DEPTH_TEST);
    }

    this->fbo.end();
    
}



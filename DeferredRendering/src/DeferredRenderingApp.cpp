/*
 Copyright (c) 2013, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Light.h"
#include "cinder/gl/Texture.h"
#include "cinder/gl/Vbo.h"
#include "cinder/params/Params.h"
#include "cinder/Camera.h"
#include "cinder/ImageIo.h"
#include "cinder/MayaCamUI.h"
#include "cinder/ObjLoader.h"
#include "cinder/Timer.h"
#include "cinder/TriMesh.h"

#include "Mesh.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class DeferredRenderingApp : public AppNative {
public:
	void	prepareSettings( Settings* settings );

	void	setup();
	void	shutdown();

	void	update();
	void	draw();
	void	resize();

	void	mouseDown( MouseEvent event );	
	void	mouseDrag( MouseEvent event );

	void	keyDown( KeyEvent event );

	bool	isInitialized() const { return (mShader && mLightLantern && mLightAmbient && mMesh); }

private:
	void			delayedSetup();
	TriMesh			createMesh(const fs::path& mshFile, const fs::path& objFile);
	gl::VboMeshRef	createDebugMesh(const TriMesh& mesh);

private:
	CameraPersp			mCamera;
	MayaCamUI			mMayaCamera;

	gl::Light*			mLightLantern;
	gl::Light*			mLightAmbient;

	gl::TextureRef		mLoadingMap;

	gl::GlslProg		mShader;

	MeshRef				mMesh;

	bool				bAutoRotate;
	float				fAutoRotateAngle;

	bool				bEnableDiffuseMap;
	bool				bEnableSpecularMap;
	bool				bEnableNormalMap;

	bool				bShowNormalsAndTangents;
	bool				bShowNormals;

	float				fTime;

	params::InterfaceGlRef	mParams;
};

void DeferredRenderingApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize( 1024, 768 );
	settings->setTitle( "Normal Mapping Demo" );
	settings->setFrameRate( 200.0f );
}

void DeferredRenderingApp::setup()
{	
	// load our "loading" message
	mLoadingMap  = gl::Texture::create( loadImage( loadAsset("loading.png") ) );

	// create a parameter window, so we can toggle stuff
	mParams = params::InterfaceGl::create( getWindow(), "Normal Mapping Demo", Vec2i(300, 200) );
	mParams->addParam( "Auto Rotate Model", &bAutoRotate );
	mParams->addSeparator();
	mParams->addParam( "Show Normals & Tangents", &bShowNormalsAndTangents );
	mParams->addParam( "Show Normals", &bShowNormals );
	mParams->addSeparator();
	mParams->addParam( "Enable Diffuse Map", &bEnableDiffuseMap );
	mParams->addParam( "Enable Specular Map", &bEnableSpecularMap );
	mParams->addParam( "Enable Normal Map", &bEnableNormalMap );

	// setup camera and lights
	mCamera.setEyePoint( Vec3f( 0.2f, 0.4f, 1.8f ) );
	mCamera.setCenterOfInterestPoint( Vec3f(0.0f, 0.5f, 0.0f) );
	mCamera.setNearClip( 0.01f );
	mCamera.setFarClip( 100.0f );

	mLightLantern = new gl::Light(gl::Light::DIRECTIONAL, 0);
	mLightLantern->setAmbient( Color(0.0f, 0.0f, 0.1f) );
	mLightLantern->setDiffuse( Color(0.9f, 0.6f, 0.3f) );
	mLightLantern->setSpecular( Color(0.9f, 0.6f, 0.3f) );

	mLightAmbient = new gl::Light(gl::Light::DIRECTIONAL, 1);
	mLightAmbient->setAmbient( Color(0.0f, 0.0f, 0.0f) );
	mLightAmbient->setDiffuse( Color(0.2f, 0.6f, 1.0f) );
	mLightAmbient->setSpecular( Color(0.2f, 0.2f, 0.2f) );

	// default settings
	bAutoRotate = true;
	fAutoRotateAngle = 0.0f;

	bEnableDiffuseMap = true;
	bEnableSpecularMap = true;
	bEnableNormalMap = true;

	bShowNormalsAndTangents = false;
	bShowNormals = false;

	// keep track of time
	fTime = (float) getElapsedSeconds();
}

void DeferredRenderingApp::delayedSetup()
{
	if( isInitialized() ) return;

	// load mesh file
	try {
		fs::path mshFile = getAssetPath("") / "leprechaun.msh";
		mMesh = Mesh::create(mshFile);
		mMesh->setDiffuseMap( loadImage( loadAsset("leprechaun_diffuse.png") ) );
		mMesh->setSpecularMap( loadImage( loadAsset("leprechaun_specular.png") ) );
		mMesh->setNormalMap( loadImage( loadAsset("leprechaun_normal.png") ) );
	}
	catch( const std::exception& e ) {
		console() << "Error loading mesh: " << e.what() << std::endl;
	}

	// load shaders
	try {
		mShader = gl::GlslProg( loadAsset("normal_mapping_vert.glsl"), loadAsset("normal_mapping_frag.glsl") );
	}
	catch( const std::exception& e ) {
		console() << "Error loading asset: " << e.what() << std::endl;
	}
}

void DeferredRenderingApp::shutdown()
{
	if(mLightAmbient) delete mLightAmbient;
	if(mLightLantern) delete mLightLantern;

	mLightAmbient = mLightLantern = NULL;
}

void DeferredRenderingApp::update()
{
	// keep track of time
	float fElapsed = (float) getElapsedSeconds() - fTime;
	fTime += fElapsed;
	
	// because loading the model and shaders might take a while,
	// we make sure our window is visible and cleared before calling 'delayedSetup()'
	if( !isInitialized() && getElapsedFrames() > 5 ) {
		delayedSetup();

		// reset time after everything is setup
		fTime = (float) getElapsedSeconds();
	}
	
	// rotate the mesh
	if(bAutoRotate && mMesh) {
		fAutoRotateAngle += (fElapsed * 0.2f);

		mMesh->setOrientation( Vec3f::yAxis() * fAutoRotateAngle );
		mMesh->setScale( mMesh->getUnitScale() );
	}
}

void DeferredRenderingApp::draw()
{
	gl::clear( Color(0.01f, 0.03f, 0.05f) ); 
	gl::color( Color::white() );

	if(!isInitialized())
	{
		// render our loading message while loading is in progress
		Area centered = Area::proportionalFit( mLoadingMap->getBounds(), getWindowBounds(), true, false );

		gl::enableAlphaBlending();
		gl::draw( mLoadingMap, mLoadingMap->getBounds(), centered );
		gl::disableAlphaBlending();
	}
	else
	{
		// get ready to draw in 3D
		gl::pushMatrices();
		gl::setMatrices( mCamera );

		gl::enableDepthRead();
		gl::enableDepthWrite();

		// bind our normal mapping shader
		mShader.bind();
		mShader.uniform( "uDiffuseMap", 0 );
		mShader.uniform( "uSpecularMap", 1 );
		mShader.uniform( "uNormalMap", 2 );
		mShader.uniform( "bShowNormals", bShowNormals );
		mShader.uniform( "bUseDiffuseMap", bEnableDiffuseMap );
		mShader.uniform( "bUseSpecularMap", bEnableSpecularMap );
		mShader.uniform( "bUseNormalMap", bEnableNormalMap );

		// enable our lights
		mLightLantern->enable();
		mLightAmbient->enable();

		Vec3f lanternPositionOS = Vec3f(12.5f, 30.0f, 12.5f);
		Vec3f lanternPositionWS = mMesh->getTransform().transformPointAffine( lanternPositionOS );
		mLightLantern->lookAt( lanternPositionWS, Vec3f(0.0f, 0.5f, 0.0f) );
		
		mLightAmbient->lookAt( mCamera.getEyePoint(), mCamera.getCenterOfInterestPoint() );
	
		// render our model
		mMesh->enableDebugging( bShowNormalsAndTangents );
		mMesh->render();

		// disable our lights
		mLightAmbient->disable();
		mLightLantern->disable();

		// disable our shader
		mShader.unbind();

		// get ready to render in 2D again
		gl::disableDepthWrite();
		gl::disableDepthRead();

		gl::popMatrices();

		// render our parameter window
		if(mParams)
			mParams->draw();
	}
}

void DeferredRenderingApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );
}

void DeferredRenderingApp::mouseDown( MouseEvent event )
{
	mMayaCamera.setCurrentCam( mCamera );
	mMayaCamera.mouseDown( event.getPos() );
}

void DeferredRenderingApp::mouseDrag( MouseEvent event )
{
	mMayaCamera.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCamera.getCamera();
}

void DeferredRenderingApp::keyDown( KeyEvent event )
{
	switch( event.getCode() )
	{
	case KeyEvent::KEY_ESCAPE:
		quit();
		break;
	case KeyEvent::KEY_f:
		setFullScreen( !isFullScreen() );
		break;
	case KeyEvent::KEY_v:
		gl::enableVerticalSync( !gl::isVerticalSyncEnabled() );
		break;
	}
}

CINDER_APP_NATIVE( DeferredRenderingApp, RendererGl )

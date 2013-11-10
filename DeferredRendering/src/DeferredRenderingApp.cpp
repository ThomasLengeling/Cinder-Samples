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
#include "cinder/Perlin.h"
#include "cinder/Timer.h"
#include "cinder/TriMesh.h"

#include "Mesh.h"
#include "RenderPass.h"

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

	bool	isInitialized() const { return (mShaderLighting && mLightLantern && mLightAmbient && mMesh); }

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

	gl::GlslProg		mShaderLighting;
	gl::GlslProg		mShaderWireframe;

	MeshRef				mMesh;

	RenderPassRef		mNormalAndDepthPass;
	RenderPassRef		mSSAOPass;

	bool				bAutoRotate;
	float				fAutoRotateAngle;

	bool				bAnimateLantern;
	Perlin				mPerlin;

	bool				bEnableDiffuseMap;
	bool				bEnableSpecularMap;
	bool				bEnableNormalMap;
	bool				bEnableEmmisiveMap;

	bool				bShowNormalsAndTangents;
	bool				bShowNormalMap;
	bool				bShowWireframe;

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
	mParams = params::InterfaceGl::create( getWindow(), "Normal Mapping Demo", Vec2i(320, 240) );
	mParams->addParam( "Auto Rotate Model", &bAutoRotate );
	mParams->addParam( "Animate Light", &bAnimateLantern );
	mParams->addSeparator();
	mParams->addParam( "Show Normal Map", &bShowNormalMap );
	mParams->addParam( "Show Normals & Tangents", &bShowNormalsAndTangents );
	mParams->addParam( "Show Wireframe", &bShowWireframe );
	mParams->addSeparator();
	mParams->addParam( "Enable Diffuse Map", &bEnableDiffuseMap );
	mParams->addParam( "Enable Specular Map", &bEnableSpecularMap );
	mParams->addParam( "Enable Normal Map", &bEnableNormalMap );
	mParams->addParam( "Enable Emmisive Map", &bEnableEmmisiveMap );

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

	mPerlin = Perlin(4, 65535);

	// setup render passes
	gl::Fbo::Format fmt;
	fmt.setSamples(0);

	fmt.setColorInternalFormat( GL_RG16F );
	fmt.enableDepthBuffer(true, true);
	mNormalAndDepthPass = RenderPass::create( fmt );
	
	fmt.setColorInternalFormat( GL_R16F );
	fmt.enableDepthBuffer(false);
	mSSAOPass = RenderPass::create( fmt );
	mSSAOPass->setDownScaleSize( RenderPass::HALF );

	// default settings
	bAutoRotate = true;
	fAutoRotateAngle = 0.0f;

	bEnableDiffuseMap = true;
	bEnableSpecularMap = true;
	bEnableNormalMap = true;
	bEnableEmmisiveMap= true;

	bShowNormalsAndTangents = false;
	bShowNormalMap = false;
	bShowWireframe = false;

	// keep track of time
	fTime = (float) getElapsedSeconds();
}

void DeferredRenderingApp::delayedSetup()
{
	if( isInitialized() ) return;

	// load shaders
	try {
		mShaderLighting = gl::GlslProg( loadAsset("normal_mapping_vert.glsl"), loadAsset("normal_mapping_frag.glsl") );
		mShaderWireframe = gl::GlslProg( loadAsset("wireframe_vert.glsl"), loadAsset("wireframe_frag.glsl"), loadAsset("wireframe_geom.glsl"),
			GL_TRIANGLES, GL_TRIANGLE_STRIP, 3 );
	}
	catch( const std::exception& e ) {
		console() << "Error loading asset: " << e.what() << std::endl;
	}

	// load mesh file
	try {
		fs::path mshFile = getAssetPath("") / "leprechaun.msh";
		mMesh = Mesh::create(mshFile);
		mMesh->setDiffuseMap( loadImage( loadAsset("leprechaun_diffuse.png") ) );
		mMesh->setSpecularMap( loadImage( loadAsset("leprechaun_specular.png") ) );
		mMesh->setNormalMap( loadImage( loadAsset("leprechaun_normal.png") ) );
		mMesh->setEmmisiveMap( loadImage( loadAsset("leprechaun_emmisive.png") ) );
	}
	catch( const std::exception& e ) {
		console() << "Error loading mesh: " << e.what() << std::endl;
	}

	// setup passes
	mNormalAndDepthPass->loadShader( loadAsset("normals_depth_vert.glsl"), loadAsset("normals_depth_frag.glsl") );
	mNormalAndDepthPass->getShader().bind();
	mNormalAndDepthPass->getShader().uniform( "uNormalMap", 2 );
	mNormalAndDepthPass->getShader().uniform( "uHasNormalMap", true );
	mNormalAndDepthPass->getShader().unbind();
	mNormalAndDepthPass->addMesh( mMesh );

	mSSAOPass->loadShader( loadAsset("ssao_vert.glsl"), loadAsset("ssao_frag.glsl") );
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
		mNormalAndDepthPass->render( mCamera );

		// get ready to draw in 3D
		gl::pushMatrices();
		gl::setMatrices( mCamera );

		gl::enableDepthRead();
		gl::enableDepthWrite();

		if(bShowWireframe)
		{
			// bind our single pass wireframe shader
			mShaderWireframe.bind();
			mShaderWireframe.uniform( "uViewportSize", Vec2f( getWindowSize() * 2 ) );

			// render our model
			mMesh->enableDebugging( bShowNormalsAndTangents );
			mMesh->render(false);

			// disable our shader
			mShaderWireframe.unbind();
		}
		else
		{			
			// bind our normal mapping shader
			mShaderLighting.bind();
			mShaderLighting.uniform( "uDiffuseMap", 0 );
			mShaderLighting.uniform( "uSpecularMap", 1 );
			mShaderLighting.uniform( "uNormalMap", 2 );
			mShaderLighting.uniform( "uEmmisiveMap", 3 );
			mShaderLighting.uniform( "bShowNormalMap", bShowNormalMap );
			mShaderLighting.uniform( "bUseDiffuseMap", bEnableDiffuseMap );
			mShaderLighting.uniform( "bUseSpecularMap", bEnableSpecularMap );
			mShaderLighting.uniform( "bUseNormalMap", bEnableNormalMap );
			mShaderLighting.uniform( "bUseEmmisiveMap", bEnableEmmisiveMap );

			// enable our lights
			mLightLantern->enable();
			mLightAmbient->enable();

			Vec3f offset = bAnimateLantern ? mPerlin.dfBm( Vec3f( 0.0f, 0.0f, fTime ) ) * 5.0f : Vec3f::zero();
			Vec3f lanternPositionOS = Vec3f(12.5f, 30.0f, 12.5f) + offset;
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
			mShaderLighting.unbind();
		}

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
	mNormalAndDepthPass->resize( getWindowWidth(), getWindowHeight() );
	mSSAOPass->resize( getWindowWidth(), getWindowHeight() );
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

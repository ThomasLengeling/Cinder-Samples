/*
 Copyright (c) 2013, Paul Houx - All rights reserved.
 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Leprechaun 3D model courtesy of Fabiano Di Liso aka Nazedo
 (c) Fabiano Di Liso - All rights reserved - Used with permission.
 http://www.cgtrader.com/3d-models/character-people/fantasy/the-leprechaun-the-goblin

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
#include "cinder/Timeline.h"
#include "cinder/Timer.h"
#include "cinder/TriMesh.h"

#include "Mesh.h"
#include "RenderPass.h"
#include "Shader.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace ph;

class RenderLikeAProApp : public AppNative {
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

	bool	isInitialized() const { return (mLightLantern && mLightAmbient && mMesh && mCopyrightMap); }

private:
	TriMesh			createMesh(const fs::path& mshFile, const fs::path& objFile);
	gl::VboMeshRef	createDebugMesh(const TriMesh& mesh);

private:
	CameraPersp			mCamera;
	MayaCamUI			mMayaCamera;

	gl::Light*			mLightLantern;
	gl::Light*			mLightAmbient;

	gl::TextureRef		mCopyrightMap;

	bool				bAutoRotate;
	float				fAutoRotateAngle;

	bool				bAnimateLantern;
	Perlin				mPerlin;

	bool				bEnableDiffuseMap;
	bool				bEnableSpecularMap;
	bool				bEnableNormalMap;
	bool				bEnableEmissiveMap;

	bool				bEnableSSAO;

	bool				bShowWireframe;
	bool				bShowNormalsAndTangents;
	bool				bShowNormalMap;
	bool				bShowRenderPasses;

	float				fTime;
	Anim<float>			fOpacity;

	params::InterfaceGlRef		mParams;

	// the following members are custom types defined in namespace ph::render
	render::MeshRef		mMesh;

	render::RenderPassWireframeRef		mPassWireframe;
	render::RenderPassNormalDepthRef	mPassNormalDepth;
	render::RenderPassSSAORef			mPassSSAO;
	render::RenderPassCBFilterRef		mPassCrossBilateralFilter;
	render::RenderPassCompositeRef		mPassComposite;

	render::ShaderRef					mShader;
};

void RenderLikeAProApp::prepareSettings(Settings* settings)
{
	settings->setWindowSize( 1024, 768 );
	settings->setTitle( "Render Effects Demo" );
	settings->setFrameRate( 200.0f );
}

void RenderLikeAProApp::setup()
{
	mShader = render::Shader::create("ssao");
	mShader->load();

	// load mesh file
	try {
		fs::path mshFile = getAssetPath("") / "models/leprechaun.msh";
		mMesh = render::Mesh::create(mshFile);
		mMesh->setDiffuseMap( loadImage( loadAsset("textures/leprechaun_diffuse.png") ) );
		mMesh->setSpecularMap( loadImage( loadAsset("textures/leprechaun_specular.png") ) );
		mMesh->setNormalMap( loadImage( loadAsset("textures/leprechaun_normal.png") ) );
		mMesh->setEmissiveMap( loadImage( loadAsset("textures/leprechaun_emissive.png") ) );
	}
	catch( const std::exception& e ) {
		console() << "Error loading mesh: " << e.what() << std::endl;
	}

	// setup render passes
	mPassWireframe = render::RenderPassWireframe::create();
	mPassWireframe->loadShader();
	mPassWireframe->addMesh( mMesh );

	mPassNormalDepth = render::RenderPassNormalDepth::create();
	mPassNormalDepth->setDownScaleSize( render::RenderPass::HALF );
	mPassNormalDepth->loadShader();
	mPassNormalDepth->addMesh( mMesh );

	mPassSSAO = render::RenderPassSSAO::create();
	mPassSSAO->setDownScaleSize( render::RenderPass::HALF );
	mPassSSAO->loadShader();

	mPassCrossBilateralFilter = render::RenderPassCBFilter::create();
	mPassCrossBilateralFilter->setDownScaleSize( render::RenderPass::HALF );
	mPassCrossBilateralFilter->loadShader();

	mPassComposite = render::RenderPassComposite::create();
	mPassComposite->setClearColor( Color::black() );
	mPassComposite->loadShader();
	mPassComposite->addMesh( mMesh );

	// create a parameter window, so we can toggle stuff
	mParams = params::InterfaceGl::create( getWindow(), "Demo", Vec2i(320, 240) );
	mParams->addParam( "Auto Rotate Model", &bAutoRotate );
	mParams->addParam( "Animate Light", &bAnimateLantern );
	mParams->addSeparator();
	mParams->addParam( "Show Wireframe", &bShowWireframe );
	mParams->addParam( "Show Normal Map", &mPassComposite->bShowNormalMap );
	mParams->addParam( "Show Normals & Tangents", &bShowNormalsAndTangents );
	mParams->addParam( "Show Render Passes", &bShowRenderPasses );
	mParams->addSeparator();
	mParams->addParam( "Enable Ambient Occlusion", &bEnableSSAO );
	mParams->addParam( "Enable Diffuse Map", &mMesh->bUseDiffuseMap );
	mParams->addParam( "Enable Specular Map", &mMesh->bUseSpecularMap );
	mParams->addParam( "Enable Normal Map", &mMesh->bUseNormalMap );
	mParams->addParam( "Enable Emissive Map", &mMesh->bUseEmissiveMap );
	mParams->setOptions( "", "valueswidth=fit" );

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

	// default settings
	bAutoRotate = false;
	fAutoRotateAngle = 0.0f;

	bAnimateLantern = true;

	bShowWireframe = false;
	bEnableSSAO = true;
	bShowNormalsAndTangents = false;
	bShowRenderPasses = false;

	// load texture(s)
	try {		
		mCopyrightMap  = gl::Texture::create( loadImage( loadAsset("copyright.png") ) );
	}
	catch( const std::exception& e ) {
		console() << "Error loading asset: " << e.what() << std::endl;
	}

	// animate copyright message
	timeline().apply( &fOpacity, 0.0f, 0.0f, 2.0f );
	timeline().appendTo( &fOpacity, 1.0f, 2.5f, EaseInOutCubic() );
	timeline().appendTo( &fOpacity, 1.0f, 30.0f );
	timeline().appendTo( &fOpacity, 0.0f, 2.5f, EaseInOutCubic() );

	// keep track of time
	fTime = (float) getElapsedSeconds();
}

void RenderLikeAProApp::shutdown()
{
	if(mLightAmbient) delete mLightAmbient;
	if(mLightLantern) delete mLightLantern;

	mLightAmbient = mLightLantern = NULL;
}

void RenderLikeAProApp::update()
{
	// keep track of time
	float fElapsed = (float) getElapsedSeconds() - fTime;
	fTime += fElapsed;
	
	// rotate the mesh
	if(bAutoRotate) 
		fAutoRotateAngle += (fElapsed * 0.2f);

	if(mMesh) {
		mMesh->setOrientation( Vec3f::yAxis() * fAutoRotateAngle );
		mMesh->setScale( mMesh->getUnitScale() );
		mMesh->enableDebugging(bShowNormalsAndTangents);
	}
}

void RenderLikeAProApp::draw()
{
	gl::clear( Color::black() ); 
	gl::color( Color::white() );

	if(isInitialized())
	{
		if(bShowWireframe)
		{
			mPassWireframe->render(mCamera);
		}
		else
		{		
			// perform pre-render passes
			if(bEnableSSAO)
			{
				mPassNormalDepth->render( mCamera );
				mPassSSAO->render( mCamera );
				mPassCrossBilateralFilter->render();
			}
			else
			{
				mPassSSAO->clear();
				mPassCrossBilateralFilter->clear();
			}

			// enable our lights and set their position
			//  (note: the camera must be enabled before calling "lookAt", otherwise the positions are not transformed correctly)
			mLightLantern->enable();
			mLightAmbient->enable();

			gl::pushModelView(mCamera);

			Vec3f offset = bAnimateLantern ? mPerlin.dfBm( Vec3f( 0.0f, 0.0f, fTime ) ) * 5.0f : Vec3f::zero();
			Vec3f lanternPositionOS = Vec3f(12.5f, 30.0f, 12.5f) + offset;
			Vec3f lanternPositionWS = mMesh->getTransform().transformPointAffine( lanternPositionOS );
			mLightLantern->lookAt( lanternPositionWS, Vec3f(0.0f, 0.5f, 0.0f) );
		
			mLightAmbient->lookAt( mCamera.getEyePoint(), mCamera.getCenterOfInterestPoint() );

			gl::popModelView();
	
			// perform composite pass
			mPassComposite->render(mCamera);

			// disable our lights
			mLightAmbient->disable();
			mLightLantern->disable();
		}
		
		// 
		if(bShowRenderPasses)
		{
			int w = getWindowWidth();
			int h = getWindowHeight();

			gl::color( Color(1, 1, 1) );
			gl::draw( mPassNormalDepth->getTexture(0), Area(w*4/5, 0, w, h*1/5) );
			//gl::color( Color(1, 0, 0) );
			gl::draw( mPassSSAO->getTexture(0), Area(w*4/5, h*1/5, w, h*2/5) );
			gl::draw( mPassCrossBilateralFilter->getTexture(0), Area(w*4/5, h*2/5, w, h*3/5) );
		}

		// render our parameter window
		if(mParams)
			mParams->draw();

		// render the copyright message
		Area centered = Area::proportionalFit( mCopyrightMap->getBounds(), getWindowBounds(), true, false );
		centered.offset( Vec2i(0, (getWindowHeight() - centered.y2) - 20) );

		gl::enableAlphaBlending();
		gl::color( ColorA(1, 1, 1, fOpacity.value()) );
		gl::draw( mCopyrightMap, mCopyrightMap->getBounds(), centered );
		gl::disableAlphaBlending();
	}
}

void RenderLikeAProApp::resize()
{
	mCamera.setAspectRatio( getWindowAspectRatio() );

	// create the frame buffers for several render passes
	int w = getWindowWidth();
	int h = getWindowHeight();

	mPassNormalDepth->resize( w, h );
	mPassSSAO->resize( w, h );
	mPassCrossBilateralFilter->resize( w, h );
	
	// attach output textures to render pass inputs
	mPassSSAO->attachTexture(0, mPassNormalDepth->getTexture(0));
	mPassSSAO->attachTexture(1, mPassNormalDepth->getDepthTexture());

	mPassCrossBilateralFilter->attachTexture(0, mPassSSAO->getTexture(0));
	
	mPassComposite->attachTexture(4, mPassCrossBilateralFilter->getTexture(0));
}

void RenderLikeAProApp::mouseDown( MouseEvent event )
{
	mMayaCamera.setCurrentCam( mCamera );
	mMayaCamera.mouseDown( event.getPos() );
}

void RenderLikeAProApp::mouseDrag( MouseEvent event )
{
	mMayaCamera.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
	mCamera = mMayaCamera.getCamera();
}

void RenderLikeAProApp::keyDown( KeyEvent event )
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

CINDER_APP_NATIVE( RenderLikeAProApp, RendererGl )

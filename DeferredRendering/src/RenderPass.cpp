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

#include "RenderPass.h"
#include "cinder/Rand.h"
#include "cinder/app/AppBasic.h"

using namespace ci;
using namespace ci::app;

RenderPassRef RenderPass::create(const ci::gl::Fbo::Format& format)
{
	return RenderPassRef( new RenderPass(format) );
}

void RenderPass::clear( const ci::ColorA& color )
{
	gl::SaveFramebufferBinding	savedFramebufferBinding;

	glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

	mFrameBuffer.bindFramebuffer();
	gl::setViewport( mFrameBuffer.getBounds() );
	gl::clear( color );

	glPopAttrib();
}

void RenderPass::resize(int width, int height)
{
	// create the frame buffer
	mFrameBuffer = gl::Fbo( width >> mDownScaleSize, height >> mDownScaleSize, mFormat );
}

void RenderPass::render()
{
	gl::SaveColorState			savedColorState;
	gl::SaveFramebufferBinding	savedFramebufferBinding;
	gl::SaveTextureBindState	savedTextureBindState( GL_TEXTURE_2D );

	glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

	mFrameBuffer.bindFramebuffer();

	gl::setViewport( mFrameBuffer.getBounds() );
	gl::pushMatrices();
	gl::setMatricesWindow( mFrameBuffer.getSize(), true );
	{
		// no clear necessary, we're doing a full screen pass

		if(mInputShader)
			mInputShader.bind();

		//
		for(size_t i=0;i<mInputTextures.size();++i)
		{
			if(mInputTextures[i])
			{
				glEnable( mInputTextures[i].getTarget() );
				mInputTextures[i].bind(i);
			}
		}

		gl::color( Color::white() );

		// we have to draw this rectangle upside down because of the flipped nature of Cinder's textures
		gl::drawSolidRect( mFrameBuffer.getBounds() );

		if(mInputShader)
			mInputShader.unbind();
	}
	gl::popMatrices();

	mFrameBuffer.unbindFramebuffer();

	glPopAttrib();
}

void RenderPass::render(const ci::CameraPersp& camera)
{
	gl::SaveColorState			savedColorState;
	gl::SaveFramebufferBinding	savedFramebufferBinding;
	gl::SaveTextureBindState	savedTextureBindState( GL_TEXTURE_2D );

	glPushAttrib( GL_ENABLE_BIT | GL_VIEWPORT_BIT );

	mFrameBuffer.bindFramebuffer();
	gl::setViewport( mFrameBuffer.getBounds() );
	gl::clear();

	if(mFrameBuffer.getFormat().hasDepthBuffer()) 
	{
		gl::enableDepthRead();
		gl::enableDepthWrite();
	}

	gl::pushMatrices();
	gl::setMatrices(camera);
	{
		if(mInputShader)
			mInputShader.bind();

		//
		for(size_t i=0;i<mInputTextures.size();++i)
		{
			if(mInputTextures[i])
			{
				glEnable( mInputTextures[i].getTarget() );
				mInputTextures[i].bind(i);
			}
		}

		// 
		gl::color( Color::white() );

		// render meshes
		for( auto itr=mInputMeshes.begin(); itr != mInputMeshes.end(); ++itr )
			(*itr)->render();

		if(mInputShader)
			mInputShader.unbind();
	}
	gl::popMatrices();

	mFrameBuffer.unbindFramebuffer();

	glPopAttrib();
}

void RenderPass::attachTexture(uint32_t slot, ci::gl::Texture& texture)
{
	if(mInputMeshes.size() <= slot)
		mInputTextures.resize(slot+1);

	mInputTextures[slot] = texture;
}

void RenderPass::detachTexture(uint32_t slot)
{
	if(slot < mInputMeshes.size())
		mInputTextures[slot] = gl::Texture();
}

void RenderPass::addMesh(MeshRef mesh)
{
	mInputMeshes.push_back(mesh);
}

void RenderPass::removeMesh(MeshRef mesh)
{
	auto itr = std::find( mInputMeshes.begin(), mInputMeshes.end(), mesh );
	if(itr != mInputMeshes.end()) 
		mInputMeshes.erase( itr );
}

void RenderPass::loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment)
{
	try {
		mInputShader = gl::GlslProg( vertex, fragment );
	}
	catch (const std::exception& e) {
		app::console() << "Error loading shader: " << e.what() << std::endl;
		mInputShader = gl::GlslProg();
	}
}

void RenderPass::loadShader(const ci::DataSourceRef vertex, const ci::DataSourceRef fragment,
							const ci::DataSourceRef geometry, GLint input, GLint output, GLint vertices)
{
	try {
		mInputShader = gl::GlslProg( vertex, fragment, geometry, input, output, vertices );
	}
	catch (const std::exception& e) {
		app::console() << "Error loading shader: " << e.what() << std::endl;
		mInputShader = gl::GlslProg();
	}
}

ci::gl::Texture& RenderPass::getTexture(uint32_t slot)
{
	//if(mFrameBuffer && slot < mFormat.getNumColorBuffers())
		return mFrameBuffer.getTexture(slot);
}

ci::gl::Texture& RenderPass::getDepthTexture()
{
	//if(mFrameBuffer && mFormat.hasDepthBufferTexture())
		return mFrameBuffer.getDepthTexture();
}

void RenderPass::setFlipped( bool flip )
{
	if(mFormat.hasDepthBuffer())
		mFrameBuffer.getDepthTexture().setFlipped(flip);
	
	for(int i=0;i<mFormat.getNumColorBuffers();++i)
		mFrameBuffer.getTexture(i).setFlipped(flip);
}

/////////////////////////////////////////////////////////////////////////////////////

RenderPassNormalDepthRef RenderPassNormalDepth::create()
{
	gl::Fbo::Format fmt;
	fmt.setSamples(0);
	fmt.setColorInternalFormat( GL_RG16F );
	fmt.enableDepthBuffer(true, true);

	return std::make_shared<RenderPassNormalDepth>(fmt); 
}

void RenderPassNormalDepth::resize(int width, int height)
{
	RenderPass::resize(width, height);
	setFlipped(true);
}

void RenderPassNormalDepth::render(const CameraPersp& camera)
{
	RenderPass::render(camera);
}

void RenderPassNormalDepth::loadShader()
{
	RenderPass::loadShader(loadAsset("normals_depth_vert.glsl"), loadAsset("normals_depth_frag.glsl"));

	getShader().bind();
	getShader().uniform( "uNormalMap", 2 );
	getShader().uniform( "uHasNormalMap", true );
	getShader().unbind();
}

/////////////////////////////////////////////////////////////////////////////////////

RenderPassSSAORef RenderPassSSAO::create()
{	
	gl::Fbo::Format fmt;
	fmt.setSamples(0);
	fmt.setColorInternalFormat( GL_LUMINANCE16F_ARB );
	fmt.enableDepthBuffer(false);

	return std::make_shared<RenderPassSSAO>(fmt); 
}

void RenderPassSSAO::resize(int width, int height)
{
	RenderPass::resize(width, height);
	setFlipped(false);
}

void RenderPassSSAO::render(const CameraPersp& camera)
{	
	const Matrix44f& projection = camera.getProjectionMatrix();

	float zNear = camera.getNearClip();
	float zFar = camera.getFarClip();
	float zRange = zFar - zNear;
	Vec4f projectionParams = Vec4f(projection.at(0,0), projection.at(1,1), zFar / zRange, - (zFar * zNear) / zRange);

	getShader().bind();
	getShader().uniform( "uProjectionParams", projectionParams );
	getShader().uniform( "mProjectionMatrix", projection );
	getShader().uniform( "uGBuffer", 0 );
	getShader().uniform( "uGBufferDepth", 1 );
	getShader().uniform( "uGBufferSize", Vec2f( getFrameBuffer().getSize() ) );
	getShader().unbind();

	RenderPass::render();
}

void RenderPassSSAO::loadShader()
{	
	RenderPass::loadShader( loadAsset("ssao_vert.glsl"), loadAsset("ssao_frag.glsl") );

	getShader().bind();
	getShader().uniform( "uNormalMap", 2 );
	getShader().uniform( "uHasNormalMap", true );
	getShader().unbind();

	resizeSsaoKernel();
	resizeSsaoNoise();
}

bool RenderPassSSAO::resizeSsaoKernel() 
{
	Vec3f*	kernel = new(std::nothrow) Vec3f[kSsaoKernelSize];

	Rand rnd(123456UL);
	for (int i = 0; i < kSsaoKernelSize; ++i) {
		kernel[i].x = rnd.nextFloat(-1.0f, 1.0f);
		kernel[i].y = rnd.nextFloat(-1.0f, 1.0f);
		kernel[i].z = rnd.nextFloat( 0.0f, 1.0f);
		kernel[i].normalize();

		float scale = i / (float) kSsaoKernelSize;
		kernel[i] *= lerp(0.1f, 1.0f, scale * scale);
	}

	getShader().bind();
	getShader().uniform( "uKernelSize", kSsaoKernelSize );
	getShader().uniform( "uKernelOffsets", kernel, kSsaoKernelSize );
	getShader().unbind();

	delete[] kernel;

	return true;
}

bool RenderPassSSAO::resizeSsaoNoise()
{
	const int noiseDataSize = kSsaoNoiseSize * kSsaoNoiseSize;

	//
	Vec3f *noiseData = new(std::nothrow) Vec3f[noiseDataSize];
	assert(noiseData);

	Rand rnd(123456);
	for (int i = 0; i < noiseDataSize; ++i) {
		noiseData[i] = Vec3f(
			rnd.randFloat(-1.0f, 1.0f),
			rnd.randFloat(-1.0f, 1.0f),
			0.0f
		);
		noiseData[i].normalize();
	}

	Surface32f s = Surface32f((float*) noiseData, kSsaoNoiseSize, kSsaoNoiseSize, 3 * kSsaoNoiseSize, SurfaceChannelOrder::RGB);

	gl::Texture::Format fmt;
	fmt.setInternalFormat( GL_RGB32F );
	fmt.setWrap( GL_REPEAT, GL_REPEAT );

	mTextureNoise = gl::Texture::create(s, fmt);	

	getShader().bind();
	getShader().uniform( "uNoise", 2 );
	getShader().uniform( "uNoiseSize", Vec2f( mTextureNoise->getSize() ) );
	getShader().unbind();

	delete[] noiseData;

	return true;
}

